/**
 * @file actor_renderer.h
 * @brief Culls actors and assigns contiguous Neo Geo sprite ranges before drawing them.
 */

#ifndef UNSIGNED_RENDERER_ACTOR_RENDERER_H
#define UNSIGNED_RENDERER_ACTOR_RENDERER_H

#include "actor/actor.h"
#include "display/viewport/viewport.h"
#include "renderer/render_plan.h"

/** Return true when an active actor sprite or its optional underlay is visible in the viewport. */
bool unsigned_actor_renderer_is_visible(const UActor *actor, const UViewport *viewport);

/**
 * Cache visibility for every actor and compute total hardware columns required.
 * `min_first_sprite` receives the smallest existing allocation that can be reused, when any.
 */
bool unsigned_actor_renderer_prepare(UActorContainer *actors, const UViewport *viewport, u16 *visible_sprite_count, u16 *min_first_sprite);

/** Cull and allocate visible actors starting at `first_sprite`. */
bool unsigned_actor_renderer_layout(UActorContainer *actors, const UViewport *viewport, u16 first_sprite);

/** Allocate actors using visibility results already produced by `prepare`. */
bool unsigned_actor_renderer_layout_prepared(UActorContainer *actors, u16 first_sprite, u16 *relocation_count);

/** CPU-only transform/effect preparation for every visible sprite in the prepared actor batch. */
void unsigned_actor_renderer_prepare_draws(UActorContainer *actors, const UViewport *viewport, URenderPlan *plan);

/** Force every visible prepared sprite to rebuild all hardware state on the next commit. */
void unsigned_actor_renderer_force_rebuild_prepared(UActorContainer *actors);

/**
 * Precommit every pending hardware-chain boundary for the prepared actor batch.
 * Call this immediately after layout, before background/FIX work consumes the VBlank window.
 */
void unsigned_actor_renderer_precommit_chain_boundaries(UActorContainer *actors);

/** Cull and draw one actor. */
void unsigned_actor_renderer_draw(UActor *actor, const UViewport *viewport);

/** Draw only an actor's prepared underlay; used by level rendering to emit all shadows first. */
void unsigned_actor_renderer_draw_underlay_prepared(UActor *actor, const UViewport *viewport);

/** Draw only an actor's prepared body sprite. */
void unsigned_actor_renderer_draw_body_prepared(UActor *actor, const UViewport *viewport);

/** Draw one actor using its already-prepared layout visibility; the optional underlay is emitted first. */
void unsigned_actor_renderer_draw_prepared(UActor *actor, const UViewport *viewport);

/** Hide one actor without releasing its allocation. */
void unsigned_actor_renderer_hide(UActor *actor);

/** Release one actor's renderer ownership and invalidate its hardware state. */
void unsigned_actor_renderer_release(UActor *actor);

#endif
