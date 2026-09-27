/**
 * @file actor_renderer.c
 * @brief Performs actor culling, hardware-sprite range allocation and sprite-renderer dispatch.
 */

#include "renderer/actor_renderer.h"

#include "display/sprite/limits.h"
#include "renderer/renderer_config.h"
#include "renderer/sprite_renderer.h"
#include "renderer/sprite_internal.h"
#include "system/sprite_backend.h"

/** Scan one configured sprite, cache visibility and accumulate its hardware span. */
static void actor_scan_sprite(USprite *sprite, const UCamera *camera, const UCameraWorldBounds *bounds, const Vec2 *position, u16 *total, u16 *minimum, bool reserve_hidden) {
    sprite->render.layout.visible = false;
    if (sprite->render.layout.first_sprite < *minimum) {
        *minimum = sprite->render.layout.first_sprite;
    }

    if (unsigned_renderer_sprite_is_visible(sprite, camera, bounds, position)) {
        sprite->render.layout.visible = true;
    }

    if (reserve_hidden || sprite->render.layout.visible) {
        *total += sprite->render.layout.sprite_count;
    }
}

/** Allocate or release one scanned sprite and advance the contiguous actor span. */
static void actor_layout_sprite(USprite *sprite, u16 *next_sprite, bool reserve_hidden) {
    if (!reserve_hidden && !sprite->render.layout.visible) {
        unsigned_renderer_sprite_unassign(sprite);
        return;
    }

    unsigned_renderer_sprite_relocate(sprite, *next_sprite);
    *next_sprite += sprite->render.layout.sprite_count;
}

/** Return whether the scanned stable layout would move at least one existing sprite range. */
static bool actor_renderer_stable_layout_needs_relocation(const UActorContainer *actors, u16 first_sprite) {
    u16 next_sprite = first_sprite;

    /* Stable layouts reserve hidden sprites too, so expected ranges depend only on active actors
     * and sprite widths. Avoid snapshotting every render-state field on the overwhelmingly common
     * frame where this mapping is already correct. */
    for (u8 i = 0u; i < actors->count; ++i) {
        const UActor *actor = actors->instances[i];
        const USprite *underlay = actor->underlay;
        if (underlay == NULL) {
            continue;
        }
        if (underlay->render.layout.first_sprite != next_sprite) {
            return true;
        }
        next_sprite += underlay->render.layout.sprite_count;
    }

    for (u8 i = 0u; i < actors->count; ++i) {
        const UActor *actor = actors->instances[i];
        if (actor->sprite.render.layout.first_sprite != next_sprite) {
            return true;
        }
        next_sprite += actor->sprite.render.layout.sprite_count;
    }
    return false;
}

/** Snapshot hardware ranges/content while each actor is already hot in the relocation path. */
static void actor_renderer_snapshot(UActorContainer *actors) {
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        if (actor->underlay != NULL) {
            unsigned_renderer_sprite_snapshot_layout(actor->underlay);
        }
        unsigned_renderer_sprite_snapshot_layout(&actor->sprite);
    }
}

/** Return a previous owner candidate when it exactly owned `first_sprite`. */
static USprite *actor_renderer_previous_owner_at(UActorContainer *actors, u8 index, u16 first_sprite, bool underlays) {
    UActor *actor = actors->instances[index];
    USprite *candidate = underlays ? actor->underlay : &actor->sprite;
    return candidate != NULL && candidate->render.previous.range_valid && candidate->render.previous.first_sprite == first_sprite ? candidate : NULL;
}

/**
 * Find which sprite owned `first_sprite` before the current stable-layout relocation pass.
 *
 * Depth sorting uses insertion order, so beat'em-up reorders are normally local: an actor crosses
 * one or a few neighbours and the displaced owner stays close to the same array index. Search
 * outwards from the relocated actor instead of rescanning from index 0. Every actor is still
 * examined at most once, preserving the old worst-case bound without extra scratch memory.
 */
static USprite *actor_renderer_previous_owner(UActorContainer *actors, u8 current_index, u16 first_sprite, bool underlays) {
    for (u16 distance = 1u; distance < actors->count; ++distance) {
        const u16 right = (u16)current_index + distance;
        if (right < actors->count) {
            USprite *owner = actor_renderer_previous_owner_at(actors, (u8)right, first_sprite, underlays);
            if (owner != NULL) {
                return owner;
            }
        }

        if (distance <= current_index) {
            USprite *owner = actor_renderer_previous_owner_at(actors, (u8)(current_index - distance), first_sprite, underlays);
            if (owner != NULL) {
                return owner;
            }
        }
    }

    return NULL;
}

/** Reuse compatible hardware state left in one stable slot by its previous owner. */
static void actor_renderer_reuse_relocated_sprite(UActorContainer *actors, u8 index, USprite *sprite, bool underlay) {
    if (sprite == NULL || !sprite->render.previous.range_valid || sprite->render.previous.first_sprite == sprite->render.layout.first_sprite) {
        return;
    }

    USprite *previous_owner = actor_renderer_previous_owner(actors, index, sprite->render.layout.first_sprite, underlay);
    if (unsigned_renderer_sprite_can_reuse_chain(sprite, previous_owner)) {
        unsigned_renderer_sprite_reuse_chain(sprite, previous_owner);
    }
    if (unsigned_renderer_sprite_can_reuse_graphics(sprite, previous_owner)) {
        unsigned_renderer_sprite_reuse_graphics(sprite);
    }
}

/**
 * Reuse stable-slot SCB1 graphics and compatible SCB3 chains after depth-order relocation.
 *
 * The destination range still contains the previous owner's hardware state. When width/height
 * match, keep its valid driver/sticky chain and let the draw build dirty only coordinates/scale that
 * really differ. Identical animation graphics can independently keep SCB1 too.
 */
static void actor_renderer_reuse_relocated_state(UActorContainer *actors) {
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        actor_renderer_reuse_relocated_sprite(actors, i, actor->underlay, true);
        actor_renderer_reuse_relocated_sprite(actors, i, &actor->sprite, false);
    }
}

void unsigned_renderer_actor_scan(UActorContainer *actors, const UCamera *camera, u16 *sprite_count, u16 *min_first_sprite, bool reserve_hidden) {
    u16 total = 0u;
    u16 minimum = UINT16_MAX;
    UCameraWorldBounds bounds;

    unsigned_camera_world_bounds(camera, &bounds);

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        USprite *underlay = actor->underlay;

        actor->sprite.render.layout.visible = false;
        if (underlay != NULL) {
            underlay->render.layout.visible = false;
        }

        /* Underlays are scanned first so their hardware indices remain behind the actor. */
        if (underlay != NULL) {
            actor_scan_sprite(underlay, camera, &bounds, &actor->position, &total, &minimum, reserve_hidden);
        }
        actor_scan_sprite(&actor->sprite, camera, &bounds, &actor->position, &total, &minimum, reserve_hidden);
    }

    *sprite_count = total;
    *min_first_sprite = minimum;
}

void unsigned_renderer_actor_layout_ready(UActorContainer *actors, u16 first_sprite, bool reserve_hidden) {
    u16 next_sprite = first_sprite;
    bool stable_relocation = false;

    /* Only stable layouts can safely reuse prior slot contents: every hidden actor still owns
     * its range, so the previous frame is a complete map of the hardware span. Snapshot that map
     * only when this frame will actually move ownership. */
    if (reserve_hidden) {
        stable_relocation = actor_renderer_stable_layout_needs_relocation(actors, first_sprite);
        if (stable_relocation) {
            actor_renderer_snapshot(actors);
        }
    }

    /* Reserve every underlay first so all character shadows remain behind all actor bodies. */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        if (actor->underlay != NULL) {
            actor_layout_sprite(actor->underlay, &next_sprite, reserve_hidden);
        }
    }

    /* Actor bodies keep the already-sorted actor order after the underlay span. */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        actor_layout_sprite(&actor->sprite, &next_sprite, reserve_hidden);
    }

    if (stable_relocation) {
        actor_renderer_reuse_relocated_state(actors);
    }

}

void unsigned_renderer_actor_force_rebuild(UActorContainer *actors) {
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        USprite *underlay = actor->underlay;
        if (underlay != NULL && underlay->render.layout.assigned) {
            underlay->render.committed.initialized = false;
            underlay->render.committed.chained = false;
            underlay->render.committed.visible = false;
            underlay->render.prepared.valid = false;
            unsigned_sprite_render_mark_dirty(&underlay->render, U_SPRITE_RENDER_DIRTY_ALL);
        }
        if (actor->sprite.render.layout.assigned) {
            actor->sprite.render.committed.initialized = false;
            actor->sprite.render.committed.chained = false;
            actor->sprite.render.committed.visible = false;
            actor->sprite.render.prepared.valid = false;
            unsigned_sprite_render_mark_dirty(&actor->sprite.render, U_SPRITE_RENDER_DIRTY_ALL);
        }
    }
}

static inline void actor_renderer_plan_sprite(const USprite *sprite, UActorRenderPlan *actor_plan) {
    if (!sprite->render.prepared.valid) {
        return;
    }

    const u8 driver_dirty = unsigned_renderer_sprite_driver_dirty(sprite);
    actor_plan->driver_dirty = (u8)(actor_plan->driver_dirty | driver_dirty);
    if (driver_dirty == 0u) {
        actor_plan->has_non_driver_work = true;
    }
    if ((sprite->render.dirty & U_SPRITE_RENDER_DIRTY_CHAIN_BOUNDARY) != 0u) {
        actor_plan->has_chain_boundary_work = true;
    }
}

void unsigned_renderer_actor_build_draws(UActorContainer *actors, const UCamera *camera, URenderPlan *plan, USpriteColumnPlanBuffer *column_buffer, UActorRenderPlan *actor_plan) {
    *actor_plan = (UActorRenderPlan){0};

    /* CPU-side preparation can process underlay + body while the actor pointer is already hot.
     * Hardware ownership/commit order remains unchanged. */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        USprite *underlay = actor->underlay;

        if (underlay != NULL) {
            unsigned_renderer_sprite_build_draw(underlay, camera, &actor->position, column_buffer);
            if (underlay->render.prepared.valid) {
                unsigned_render_plan_mark_work(plan);
                actor_renderer_plan_sprite(underlay, actor_plan);
            }
        }

        unsigned_renderer_sprite_build_draw(&actor->sprite, camera, &actor->position, column_buffer);
        if (actor->sprite.render.prepared.valid) {
            unsigned_render_plan_mark_work(plan);
            actor_renderer_plan_sprite(&actor->sprite, actor_plan);
        }
    }
}

/** Return one active sprite from the hardware-layout pass selected by `underlays`. */
static USprite *actor_renderer_pass_sprite(UActorContainer *actors, u8 index, bool underlays) {
    UActor *actor = actors->instances[index];
    return underlays ? actor->underlay : &actor->sprite;
}

/** Flush a candidate driver run and report whether every sprite in it was committed here. */
static bool actor_renderer_flush_driver_run(USprite **run, u8 count, u8 axis, bool all_axis_only) {
    if (count == 0u) {
        return true;
    }

    /* One two-axis sprite is cheaper through the existing SCB3->SCB4 0x200 stride. Two sprites
     * with both axes dirty are address-neutral. Single-axis pairs and all runs of 3+ save at least
     * one VRAMADDR write, so only those are streamed here. */
    if (count < 3u && !(count >= 2u && all_axis_only)) {
        return false;
    }

    unsigned_system_sprite_write_driver_batch(run, count, axis);
    for (u8 i = 0u; i < count; ++i) {
        unsigned_renderer_sprite_commit_driver_axis(run[i], axis);
    }
    return true;
}

/** Batch one SCB driver axis over contiguous same-width ranges in one underlay/body pass. */
static bool actor_renderer_commit_driver_axis_pass(UActorContainer *actors, bool underlays, u8 axis) {
    USprite *run[UNSIGNED_RENDERER_DRIVER_BATCH_CAPACITY];
    u8 run_count = 0u;
    u8 stride = 0u;
    u16 expected_first = 0u;
    bool all_axis_only = true;
    bool all_batched = true;

    for (u8 cursor = 0u; cursor < actors->count; ++cursor) {
        USprite *sprite = actor_renderer_pass_sprite(actors, cursor, underlays);
        const u8 dirty = sprite != NULL ? unsigned_renderer_sprite_driver_dirty(sprite) : 0u;
        const bool axis_dirty = (dirty & axis) != 0u;
        const bool contiguous = run_count == 0u || (sprite != NULL && sprite->render.layout.sprite_count == stride && sprite->render.layout.first_sprite == expected_first);
        const bool capacity_available = run_count < UNSIGNED_RENDERER_DRIVER_BATCH_CAPACITY;

        if (!axis_dirty || !contiguous || !capacity_available) {
            all_batched = actor_renderer_flush_driver_run(run, run_count, axis, all_axis_only) && all_batched;
            run_count = 0u;
            stride = 0u;
            expected_first = 0u;
            all_axis_only = true;
        }

        if (!axis_dirty) {
            continue;
        }

        if (run_count == 0u) {
            stride = sprite->render.layout.sprite_count;
            expected_first = sprite->render.layout.first_sprite;
        }

        if (sprite->render.layout.sprite_count != stride || sprite->render.layout.first_sprite != expected_first) {
            stride = sprite->render.layout.sprite_count;
        }

        run[run_count++] = sprite;
        all_axis_only = all_axis_only && dirty == axis;
        expected_first = (u16)(sprite->render.layout.first_sprite + sprite->render.layout.sprite_count);
    }

    all_batched = actor_renderer_flush_driver_run(run, run_count, axis, all_axis_only) && all_batched;
    return all_batched;
}

void unsigned_renderer_actor_break_chains(UActorContainer *actors) {
    /* Preserve hardware ownership order for the rare relocation/rebuild path. */
    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        USprite *underlay = actor->underlay;
        if (underlay != NULL && underlay->render.prepared.valid) {
            unsigned_renderer_sprite_break_chain(underlay);
        }
    }

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        if (actor->sprite.render.prepared.valid) {
            unsigned_renderer_sprite_break_chain(&actor->sprite);
        }
    }
}

void unsigned_renderer_actor_commit(UActorContainer *actors, const UActorRenderPlan *actor_plan) {
    bool all_driver_work_batched = true;

    /* Do not even traverse an axis that CPU preparation proved clean. Camera scrolling is normally
     * X-only, so the two complete Y passes disappear from that hot path. */
    if ((actor_plan->driver_dirty & U_SPRITE_RENDER_DIRTY_Y) != 0u) {
        all_driver_work_batched = actor_renderer_commit_driver_axis_pass(actors, true, U_SPRITE_RENDER_DIRTY_Y) && all_driver_work_batched;
        all_driver_work_batched = actor_renderer_commit_driver_axis_pass(actors, false, U_SPRITE_RENDER_DIRTY_Y) && all_driver_work_batched;
    }
    if ((actor_plan->driver_dirty & U_SPRITE_RENDER_DIRTY_X) != 0u) {
        all_driver_work_batched = actor_renderer_commit_driver_axis_pass(actors, true, U_SPRITE_RENDER_DIRTY_X) && all_driver_work_batched;
        all_driver_work_batched = actor_renderer_commit_driver_axis_pass(actors, false, U_SPRITE_RENDER_DIRTY_X) && all_driver_work_batched;
    }

    /* A large homogeneous crowd moving only because the camera scrolled is fully consumed by the
     * driver batches above. Skip two otherwise-empty underlay/body draw scans in that case. */
    if (!actor_plan->has_non_driver_work && all_driver_work_batched) {
        return;
    }

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        USprite *underlay = actor->underlay;
        if (underlay != NULL && underlay->render.prepared.valid) {
            unsigned_renderer_sprite_commit_draw(underlay);
        }
    }

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];
        if (actor->sprite.render.prepared.valid) {
            unsigned_renderer_sprite_commit_draw(&actor->sprite);
        }
    }
}
