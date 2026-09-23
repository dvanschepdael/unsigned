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

/** Returns whether the previous actor already belongs before the current actor in the requested stable draw order. */
static bool actor_order_before(const UActor *previous, const UActor *current, UActorSort order) {
    if (previous->position.y < current->position.y) {
        return true;
    }
    if (previous->position.y > current->position.y) {
        return false;
    }
    return order != U_ACTOR_ORDER_Y_X_STABLE || previous->position.x <= current->position.x;
}

void unsigned_actor_container_sort(UActorContainer *container, UActorSort order) {
    if (container->count < 2u) {
        return;
    }

    bool changed = false;
    for (u8 i = 1u; i < container->count; ++i) {
        UActor *current = container->instances[i];
        u8 j = i;

        while (j > 0u && !actor_order_before(container->instances[j - 1u], current, order)) {
            container->instances[j] = container->instances[j - 1u];
            --j;
        }
        container->instances[j] = current;
        changed = changed || j != i;
    }

    if (changed) {
        ++container->layout_revision;
    }
}
