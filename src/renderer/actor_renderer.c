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
static bool actor_layout_sprite(USprite *sprite, u32 *next_sprite, u16 *relocation_count) {
    if (sprite == NULL || sprite->definition == NULL) {
        return true;
    }

    if (!sprite->render.layout_visible) {
        unsigned_sprite_renderer_unassign(sprite);
        return true;
    }

    if (*next_sprite > UNSIGNED_SPRITE_LAST || !unsigned_sprite_range_is_valid((u16)*next_sprite, sprite->render.sprite_count)) {
        return false;
    }

    if (sprite->render.first_sprite != (u16)*next_sprite && relocation_count != NULL) {
        ++(*relocation_count);
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
            unsigned_sprite_renderer_unassign(&actor->sprite);
            if (underlay != NULL) {
                unsigned_sprite_renderer_unassign(underlay);
            }
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
    return unsigned_actor_renderer_layout_prepared(actors, first_sprite, NULL);
}

bool unsigned_actor_renderer_layout_prepared(UActorContainer *actors, u16 first_sprite, u16 *relocation_count) {
    u32 next_sprite = first_sprite;

    if (actors == NULL || (actors->count > 0u && actors->instances == NULL)) {
        return false;
    }
    if (relocation_count != NULL) {
        *relocation_count = 0u;
    }

    /* Reserve every underlay first so all character shadows remain behind all actor bodies. */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        if (actor != NULL && actor->active && !actor_layout_sprite(actor_underlay(actor), &next_sprite, relocation_count)) {
            return false;
        }
    }

    /* Actor bodies keep the already-sorted actor order after the underlay span. */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        if (actor != NULL && actor->active && !actor_layout_sprite(&actor->sprite, &next_sprite, relocation_count)) {
            return false;
        }
    }

    return true;
}

void unsigned_actor_renderer_force_rebuild_prepared(UActorContainer *actors) {
    if (actors == NULL || (actors->count > 0u && actors->instances == NULL)) {
        return;
    }

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        USprite *underlay;

        if (actor == NULL || !actor->active) {
            continue;
        }
        underlay = actor_underlay(actor);
        if (underlay != NULL && underlay->render.layout_visible) {
            underlay->render.hardware_initialized = false;
            underlay->render.visible = false;
            underlay->render.prepared_valid = false;
            unsigned_sprite_render_mark_dirty(&underlay->render, U_SPRITE_RENDER_DIRTY_ALL);
        }
        if (actor->sprite.render.layout_visible) {
            actor->sprite.render.hardware_initialized = false;
            actor->sprite.render.visible = false;
            actor->sprite.render.prepared_valid = false;
            unsigned_sprite_render_mark_dirty(&actor->sprite.render, U_SPRITE_RENDER_DIRTY_ALL);
        }
    }
}

void unsigned_actor_renderer_prepare_draws(UActorContainer *actors, const UViewport *viewport, URenderPlan *plan) {
    u32 critical_words = 0u;
    u32 high_words = 0u;

    if (actors == NULL || viewport == NULL || (actors->count > 0u && actors->instances == NULL)) {
        return;
    }

    /* Match commit order: all ground underlays first, then actor bodies. */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        USprite *underlay;

        if (actor == NULL || !actor->active) {
            continue;
        }
        underlay = actor_underlay(actor);
        if (underlay != NULL && underlay->render.layout_visible &&
            unsigned_sprite_renderer_prepare_draw(underlay, viewport, &actor->position)) {
            unsigned_sprite_renderer_estimate_prepared_vram_words(underlay, &critical_words, &high_words);
            if (plan != NULL) {
                plan->stats.actor_sprite_columns += underlay->render.sprite_count;
            }
        }
    }

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];

        if (actor != NULL && actor->active && actor->sprite.render.layout_visible &&
            unsigned_sprite_renderer_prepare_draw(&actor->sprite, viewport, &actor->position)) {
            unsigned_sprite_renderer_estimate_prepared_vram_words(&actor->sprite, &critical_words, &high_words);
            if (plan != NULL) {
                plan->stats.actor_sprite_columns += actor->sprite.render.sprite_count;
            }
        }
    }

    unsigned_render_plan_add_words(plan, U_RENDER_PRIORITY_CRITICAL, critical_words);
    unsigned_render_plan_add_words(plan, U_RENDER_PRIORITY_HIGH, high_words);
}

void unsigned_actor_renderer_precommit_chain_boundaries(UActorContainer *actors) {
    if (actors == NULL || (actors->count > 0u && actors->instances == NULL)) {
        return;
    }

    /*
     * Two passes preserve the renderer's underlay/body ownership layout. More
     * importantly, every future chain driver is made non-sticky before any
     * relocated sprite starts rebuilding its interior sticky columns.
     */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        USprite *underlay;

        if (actor == NULL || !actor->active) {
            continue;
        }
        underlay = actor_underlay(actor);
        if (underlay != NULL && underlay->render.layout_visible) {
            unsigned_sprite_renderer_precommit_chain_boundary(underlay);
        }
    }

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];

        if (actor != NULL && actor->active && actor->sprite.render.layout_visible) {
            unsigned_sprite_renderer_precommit_chain_boundary(&actor->sprite);
        }
    }
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
