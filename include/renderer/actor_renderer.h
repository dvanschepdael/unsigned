/**
 * @file actor_renderer.h
 * @brief Culls actors and assigns contiguous Neo Geo sprite ranges before drawing them.
 */

#ifndef UNSIGNED_RENDERER_ACTOR_RENDERER_H
#define UNSIGNED_RENDERER_ACTOR_RENDERER_H

#include "actor/actor.h"
#include "display/viewport/viewport.h"

/** Return true when an active actor with a frame is visible in the viewport. */
bool unsigned_actor_renderer_is_visible(const UActor *actor, const UViewport *viewport);

/**
 * Cache visibility for every actor and compute total hardware columns required.
 * `min_first_sprite` receives the smallest existing allocation that can be reused, when any.
 */
bool unsigned_actor_renderer_prepare(UActorContainer *actors, const UViewport *viewport, u16 *visible_sprite_count, u16 *min_first_sprite);

/** Cull and allocate visible actors starting at `first_sprite`. */
bool unsigned_actor_renderer_layout(UActorContainer *actors, const UViewport *viewport, u16 first_sprite);

/** Allocate actors using visibility results already produced by `prepare`. */
bool unsigned_actor_renderer_layout_prepared(UActorContainer *actors, u16 first_sprite);

/** Cull and draw one actor. */
void unsigned_actor_renderer_draw(UActor *actor, const UViewport *viewport);

/** Draw one actor using its already-prepared layout visibility. */
void unsigned_actor_renderer_draw_prepared(UActor *actor, const UViewport *viewport);

/** Hide one actor without releasing its allocation. */
void unsigned_actor_renderer_hide(UActor *actor);

/** Release one actor's renderer ownership and invalidate its hardware state. */
void unsigned_actor_renderer_release(UActor *actor);

#endif
