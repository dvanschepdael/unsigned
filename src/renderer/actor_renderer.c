/**
 * @file actor_renderer.c
 * @brief Performs actor culling, hardware-sprite range allocation and sprite-renderer dispatch.
 */

#include "renderer/actor_renderer.h"

#include "display/sprite/limits.h"
#include "renderer/sprite_renderer.h"

/** Return the optional configured underlay, ignoring an uninitialized pointer target. */
static USprite *actor_underlay(UActor *actor) {
    return actor != NULL && actor->underlay != NULL && actor->underlay->definition != NULL ? actor->underlay : NULL;
}

/** Const counterpart used by visibility queries. */
static const USprite *actor_underlay_const(const UActor *actor) {
    return actor != NULL && actor->underlay != NULL && actor->underlay->definition != NULL ? actor->underlay : NULL;
}

/** Validate one sprite's logical width against its currently owned hardware range. */
static bool actor_sprite_allocation_is_valid(const USprite *sprite) {
    return sprite != NULL && sprite->definition != NULL && sprite->render.sprite_count == sprite->definition->width_tiles &&
           unsigned_sprite_range_is_valid(sprite->render.first_sprite, sprite->render.sprite_count);
}

/** Accumulate a configured sprite into prepare() and cache its independent visibility. */
static bool actor_prepare_sprite(USprite *sprite, const UViewport *viewport, const Vec2 *position, u32 *total, u16 *minimum) {
    if (!actor_sprite_allocation_is_valid(sprite)) {
        return false;
    }

    sprite->render.layout_visible = false;
    if (sprite->render.first_sprite < *minimum) {
        *minimum = sprite->render.first_sprite;
    }

    if (sprite->current_frame == NULL || !unsigned_sprite_renderer_is_visible(sprite, viewport, position)) {
        return true;
    }

    sprite->render.layout_visible = true;
    *total += sprite->render.sprite_count;
    return *total <= UINT16_MAX;
}

/** Allocate or release one prepared sprite and advance the contiguous actor span. */
static bool actor_layout_sprite(USprite *sprite, u32 *next_sprite) {
    if (sprite == NULL || sprite->definition == NULL) {
        return true;
    }

    if (!sprite->render.layout_visible) {
        unsigned_sprite_renderer_release(sprite);
        return true;
    }

    if (*next_sprite > UNSIGNED_SPRITE_LAST || !unsigned_sprite_range_is_valid((u16)*next_sprite, sprite->render.sprite_count)) {
        return false;
    }

    if (!unsigned_sprite_renderer_relocate(sprite, (u16)*next_sprite)) {
        return false;
    }
    *next_sprite += sprite->render.sprite_count;
    return true;
}

bool unsigned_actor_renderer_is_visible(const UActor *actor, const UViewport *viewport) {
    if (actor == NULL || !actor->active || viewport == NULL) {
        return false;
    }

    const USprite *underlay = actor_underlay_const(actor);
    const bool underlay_visible = underlay != NULL && underlay->current_frame != NULL && unsigned_sprite_renderer_is_visible(underlay, viewport, &actor->position);
    const bool actor_visible = actor->sprite.current_frame != NULL && unsigned_sprite_renderer_is_visible(&actor->sprite, viewport, &actor->position);
    return underlay_visible || actor_visible;
}

bool unsigned_actor_renderer_prepare(UActorContainer *actors, const UViewport *viewport, u16 *visible_sprite_count, u16 *min_first_sprite) {
    u32 total = 0u;
    u16 minimum = UINT16_MAX;

    if (actors == NULL || viewport == NULL || visible_sprite_count == NULL || (actors->count > 0u && actors->instances == NULL)) {
        return false;
    }

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];

        if (actor == NULL) {
            continue;
        }

        actor->sprite.render.layout_visible = false;
        USprite *underlay = actor_underlay(actor);
        if (underlay != NULL) {
            underlay->render.layout_visible = false;
        }
        if (!actor->active) {
            continue;
        }

        /* Underlays are prepared first so their hardware indices remain behind the actor. */
        if (underlay != NULL && !actor_prepare_sprite(underlay, viewport, &actor->position, &total, &minimum)) {
            return false;
        }
        if (!actor_prepare_sprite(&actor->sprite, viewport, &actor->position, &total, &minimum)) {
            return false;
        }
    }

    *visible_sprite_count = (u16)total;
    if (min_first_sprite != NULL) {
        *min_first_sprite = minimum;
    }
    return true;
}

bool unsigned_actor_renderer_layout(UActorContainer *actors, const UViewport *viewport, u16 first_sprite) {
    u16 visible_sprite_count;

    if (!unsigned_actor_renderer_prepare(actors, viewport, &visible_sprite_count, NULL)) {
        return false;
    }
    (void)visible_sprite_count;
    return unsigned_actor_renderer_layout_prepared(actors, first_sprite);
}

bool unsigned_actor_renderer_layout_prepared(UActorContainer *actors, u16 first_sprite) {
    u32 next_sprite = first_sprite;

    if (actors == NULL || (actors->count > 0u && actors->instances == NULL)) {
        return false;
    }

    /* Reserve every underlay first so all character shadows remain behind all actor bodies. */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        if (actor != NULL && actor->active && !actor_layout_sprite(actor_underlay(actor), &next_sprite)) {
            return false;
        }
    }

    /* Actor bodies keep the already-sorted actor order after the underlay span. */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        if (actor != NULL && actor->active && !actor_layout_sprite(&actor->sprite, &next_sprite)) {
            return false;
        }
    }

    return true;
}

void unsigned_actor_renderer_draw(UActor *actor, const UViewport *viewport) {
    if (actor == NULL || viewport == NULL || !actor->active) {
        return;
    }

    USprite *underlay = actor_underlay(actor);
    if (underlay != NULL) {
        unsigned_sprite_renderer_draw(underlay, viewport, &actor->position);
    }
    unsigned_sprite_renderer_draw(&actor->sprite, viewport, &actor->position);
}

void unsigned_actor_renderer_draw_underlay_prepared(UActor *actor, const UViewport *viewport) {
    if (actor == NULL || viewport == NULL || !actor->active) {
        return;
    }

    USprite *underlay = actor_underlay(actor);
    if (underlay != NULL && underlay->render.layout_visible) {
        unsigned_sprite_renderer_draw_prepared(underlay, viewport, &actor->position);
    }
}

void unsigned_actor_renderer_draw_body_prepared(UActor *actor, const UViewport *viewport) {
    if (actor == NULL || viewport == NULL || !actor->active || !actor->sprite.render.layout_visible) {
        return;
    }

    unsigned_sprite_renderer_draw_prepared(&actor->sprite, viewport, &actor->position);
}

void unsigned_actor_renderer_draw_prepared(UActor *actor, const UViewport *viewport) {
    unsigned_actor_renderer_draw_underlay_prepared(actor, viewport);
    unsigned_actor_renderer_draw_body_prepared(actor, viewport);
}

void unsigned_actor_renderer_hide(UActor *actor) {
    if (actor == NULL) {
        return;
    }

    USprite *underlay = actor_underlay(actor);
    if (underlay != NULL) {
        unsigned_sprite_renderer_hide(underlay);
    }
    unsigned_sprite_renderer_hide(&actor->sprite);
}

void unsigned_actor_renderer_release(UActor *actor) {
    if (actor == NULL) {
        return;
    }

    USprite *underlay = actor_underlay(actor);
    if (underlay != NULL) {
        unsigned_sprite_renderer_release(underlay);
    }
    unsigned_sprite_renderer_release(&actor->sprite);
}
