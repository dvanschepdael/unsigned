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

/**
 * Cache actor visibility and compute the hardware-sprite span required by the batch.
 *
 * @param reserve_hidden When true, active hidden sprites keep their hardware range so depth-only
 *        reorders can reuse stable ownership without viewport-driven relocations.
 * @pre `actors`, its storage, `viewport`, `sprite_count` and `min_first_sprite` are valid.
 * @pre Every entry in the actor view is active and every body/non-NULL underlay owns an initialized sprite/current frame.
 * @pre The authored batch fits the Neo Geo actor sprite range.
 */
void unsigned_actor_renderer_prepare(UActorContainer *actors, const UViewport *viewport, u16 *sprite_count, u16 *min_first_sprite, bool reserve_hidden);

/**
 * Allocate the prepared actor batch from `first_sprite`.
 * @pre `actors` was prepared with the same `reserve_hidden` policy.
 * @pre The complete batch fits the Neo Geo hardware-sprite range reserved for actors.
 */
void unsigned_actor_renderer_layout_prepared(UActorContainer *actors, u16 first_sprite, bool reserve_hidden);

/** CPU-only transform/effect preparation for every sprite in the prepared actor batch. */
void unsigned_actor_renderer_prepare_draws(UActorContainer *actors, const UViewport *viewport, URenderPlan *plan, USpriteColumnPlanBuffer *column_buffer);

/** Force every visible prepared sprite to rebuild all hardware state on the next commit. */
void unsigned_actor_renderer_force_rebuild_prepared(UActorContainer *actors);

/**
 * Precommit every pending hardware-chain boundary for the prepared actor batch.
 * Call this immediately after layout, before background/FIX work consumes the VBlank window.
 */
void unsigned_actor_renderer_precommit_chain_boundaries(UActorContainer *actors);

/**
 * Commit the complete prepared actor batch in hardware ownership order.
 * Driver-only moves are batched first, then all underlays, then all actor bodies.
 */
void unsigned_actor_renderer_commit_prepared(UActorContainer *actors);

#endif
