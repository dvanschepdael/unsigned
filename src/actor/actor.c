/**
 * @file actor.c
 * @brief Implements shared actor state, lifetime and draw-order container.
 */

#include "actor/actor.h"

void unsigned_actor_container_init(UActorContainer *container, UActor **instances) {
    *container = (UActorContainer){
        .instances = instances,
        .layout_revision = 1u,
    };
}

void unsigned_actor_tick(UActor *actor) {
    unsigned_sprite_tick(&actor->sprite, actor);
}

void unsigned_actor_tick_batch(UActor *actor, u16 ticks) {
    unsigned_sprite_tick_batch(&actor->sprite, ticks);
}

void unsigned_actor_destroy(UActor *actor) {
    unsigned_actor_collision_state_reset(&actor->collision);
    unsigned_sprite_invalidate_render_state(&actor->sprite);
}

void unsigned_actor_container_sort(UActorContainer *container, UActorSort order) {
    if (container->count < 2u) {
        return;
    }

    const bool changed = unsigned_actor_sort(container->instances, (u32)container->count, (u32)order) != 0u;

    if (changed) {
        ++container->layout_revision;
    }
}
