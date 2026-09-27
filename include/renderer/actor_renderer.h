/**
 * @file actor_renderer.h
 * @brief Culls actors and assigns contiguous Neo Geo sprite ranges before drawing them.
 */

#ifndef UNSIGNED_RENDERER_ACTOR_RENDERER_H
#define UNSIGNED_RENDERER_ACTOR_RENDERER_H

#include "actor/actor.h"
#include "display/viewport/viewport.h"
#include "renderer/render_plan.h"
#include "renderer/sprite_column_plan.h"

/** Small actor-only work summary built during CPU preparation. */
typedef struct UActorRenderPlan {
    /** Pure SCB3/SCB4 position axes that may be committed through cross-sprite batching. */
    u8 driver_dirty;
    /** True when at least one prepared sprite needs work beyond a pure batched driver update. */
    bool has_non_driver_work;
    /** True when at least one prepared sprite must sever a previous sticky SCB3 chain. */
    bool has_chain_boundary_work;
} UActorRenderPlan;

/**
 * Cache actor visibility and compute the hardware-sprite span required by the batch.
 *
 * @param reserve_hidden When true, active hidden sprites keep their hardware range so depth-only
 *        reorders can reuse stable ownership without viewport-driven relocations.
 * @pre `actors`, its storage, `viewport`, `sprite_count` and `min_first_sprite` are valid.
 * @pre Every entry in the actor view is active and every body/non-NULL underlay owns an initialized sprite/current frame.
 * @pre The authored batch fits the Neo Geo actor sprite range.
 */
void unsigned_renderer_actor_scan(UActorContainer *actors, const UViewport *viewport, u16 *sprite_count, u16 *min_first_sprite, bool reserve_hidden);

/**
 * Allocate the scanned actor batch from `first_sprite`.
 * @pre `actors` was scanned with the same `reserve_hidden` policy.
 * @pre The complete batch fits the Neo Geo hardware-sprite range reserved for actors.
 */
void unsigned_renderer_actor_layout_ready(UActorContainer *actors, u16 first_sprite, bool reserve_hidden);

/** Build CPU-only transform/effect draw data for every sprite in the actor batch. */
void unsigned_renderer_actor_build_draws(UActorContainer *actors, const UViewport *viewport, URenderPlan *plan, USpriteColumnPlanBuffer *column_buffer, UActorRenderPlan *actor_plan);

/** Force every visible staged sprite to rebuild all hardware state on the next commit. */
void unsigned_renderer_actor_force_rebuild(UActorContainer *actors);

/**
 * Break every pending hardware-chain boundary for the actor batch before normal commit work.
 * Call this immediately after layout, before background/FIX work consumes the VBlank window.
 */
void unsigned_renderer_actor_break_chains(UActorContainer *actors);

/**
 * Commit the complete actor batch in hardware ownership order.
 * Driver-only moves are batched first, then all underlays, then all actor bodies.
 */
void unsigned_renderer_actor_commit(UActorContainer *actors, const UActorRenderPlan *actor_plan);

#endif
