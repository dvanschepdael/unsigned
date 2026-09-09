/**
 * @file actor.c
 * @brief Implements shared actor state, lifetime and draw-order container.
 */

#include "actor/actor.h"

void unsigned_actor_container_init(UActorContainer *container, UActor **instances, u8 capacity) {
    if (container == NULL) {
        return;
    }

    *container = (UActorContainer){
        .capacity = capacity,
        .instances = instances,
    };
}

void unsigned_actor_tick(UActor *actor) {
    if (actor == NULL || !actor->active) {
        return;
    }

    unsigned_sprite_tick(&actor->sprite, actor);
}

void unsigned_actor_destroy(UActor *actor) {
    if (actor == NULL) {
        return;
    }

    unsigned_actor_collision_state_reset(&actor->collision);
    unsigned_sprite_invalidate_render_state(&actor->sprite);
    actor->active = false;
}

/** Returns whether the previous actor already belongs before the current actor in the requested stable draw order. */
static bool actor_order_before(const UActor *previous, const UActor *current, UActorSort order) {
    if (previous->position.y < current->position.y) {
        return true;
    }
    if (previous->position.y > current->position.y) {
        return false;
    }

    if (order == U_ACTOR_ORDER_Y_X_STABLE) {
        return previous->position.x <= current->position.x;
    }

    return true;
}

void unsigned_actor_container_sort(UActorContainer *container, UActorSort order) {
    if (container == NULL || container->instances == NULL || container->count < 2u) {
        return;
    }
    if (order >= U_ACTOR_ORDER_COUNT) {
        order = U_ACTOR_ORDER_Y_STABLE;
    }

    for (u8 i = 1u; i < container->count; ++i) {
        UActor *current = container->instances[i];
        u8 j = i;

        if (current == NULL) {
            continue;
        }

        while (j > 0u) {
            UActor *previous = container->instances[j - 1u];

            if (previous == NULL || actor_order_before(previous, current, order)) {
                break;
            }

            container->instances[j] = previous;
            --j;
        }
        container->instances[j] = current;
    }
}
