/**
 * @file actor_renderer.c
 * @brief Performs actor culling, hardware-sprite range allocation and sprite-renderer dispatch.
 */

#include "renderer/actor_renderer.h"

#include "display/sprite/limits.h"
#include "renderer/sprite_renderer.h"

bool unsigned_actor_renderer_is_visible(const UActor *actor, const UViewport *viewport) {
    return actor != NULL && actor->active && actor->sprite.current_frame != NULL && unsigned_sprite_renderer_is_visible(&actor->sprite, viewport, &actor->position);
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
        if (!actor->active) {
            continue;
        }

        const u8 sprite_count = actor->sprite.render.sprite_count;
        if (actor->sprite.definition == NULL || sprite_count != actor->sprite.definition->width_tiles || !unsigned_sprite_range_is_valid(actor->sprite.render.first_sprite, sprite_count)) {
            return false;
        }

        if (actor->sprite.render.first_sprite < minimum) {
            minimum = actor->sprite.render.first_sprite;
        }

        if (!unsigned_actor_renderer_is_visible(actor, viewport)) {
            continue;
        }

        actor->sprite.render.layout_visible = true;
        total += sprite_count;
        if (total > UINT16_MAX) {
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

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];

        if (actor == NULL || !actor->active) {
            continue;
        }

        if (!actor->sprite.render.layout_visible) {
            unsigned_sprite_renderer_release(&actor->sprite);
            continue;
        }

        if (next_sprite > UNSIGNED_SPRITE_LAST || !unsigned_sprite_range_is_valid((u16)next_sprite, actor->sprite.render.sprite_count)) {
            return false;
        }

        if (!unsigned_sprite_renderer_relocate(&actor->sprite, (u16)next_sprite)) {
            return false;
        }
        next_sprite += actor->sprite.render.sprite_count;
    }

    return true;
}

void unsigned_actor_renderer_draw(UActor *actor, const UViewport *viewport) {
    if (actor == NULL || viewport == NULL || !actor->active) {
        return;
    }

    unsigned_sprite_renderer_draw(&actor->sprite, viewport, &actor->position);
}

void unsigned_actor_renderer_draw_prepared(UActor *actor, const UViewport *viewport) {
    if (actor == NULL || viewport == NULL || !actor->active || !actor->sprite.render.layout_visible) {
        return;
    }

    unsigned_sprite_renderer_draw_prepared(&actor->sprite, viewport, &actor->position);
}

void unsigned_actor_renderer_hide(UActor *actor) {
    if (actor == NULL) {
        return;
    }

    unsigned_sprite_renderer_hide(&actor->sprite);
}

void unsigned_actor_renderer_release(UActor *actor) {
    if (actor == NULL) {
        return;
    }

    unsigned_sprite_renderer_release(&actor->sprite);
}
