/**
 * @file projectile_pool.c
 * @brief Implements fixed-capacity projectile pool binding and ticking.
 */

#include "actor/projectile_pool.h"

#include <stdint.h>
UProjectilePoolInstance *unsigned_projectile_pool_reserve(UPoolInstanceContainer *pool, UProjectile *projectile) {
    if (pool == NULL || projectile == NULL || projectile->actor == NULL || projectile->trajectory_function == NULL || projectile->projection == NULL ||
        !unsigned_physics_trajectory_project(&projectile->trajectory, projectile->projection, projectile->projection_origin, &projectile->ground_position, &projectile->visual_position)) {
        return NULL;
    }

    UProjectilePoolInstance *instance = unsigned_pool_reserve(pool);

    if (instance == NULL) {
        return NULL;
    }

    projectile->actor->position = projectile->visual_position;
    projectile->actor->active = true;
    unsigned_actor_collision_state_reset(&projectile->actor->collision);
    projectile->actor->collision.resolve_immediately = true;
    projectile->collision_latched = false;
    instance->object = projectile;
    instance->args = projectile;
    instance->elapsed = 0;
    return instance;
}

void unsigned_projectile_pool_release(UPoolInstanceContainer *pool, UProjectilePoolInstance *instance) {
    if (!unsigned_pool_owns(pool, instance) || !instance->active) {
        return;
    }

    UProjectile *projectile = instance->args;

    if (projectile != NULL) {
        unsigned_actor_destroy(projectile->actor);
    }

    unsigned_pool_release(pool, instance);
}

void unsigned_projectile_pool_tick(UPoolInstanceContainer *pool) {
    if (pool == NULL || pool->count == 0u) {
        return;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UProjectilePoolInstance *instance = &pool->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UProjectile *projectile = instance->args;
        bool trajectory_active = unsigned_physics_trajectory_tick(&projectile->trajectory, projectile->trajectory_function, projectile->trajectory_context);
        (void)unsigned_physics_trajectory_project(&projectile->trajectory, projectile->projection, projectile->projection_origin, &projectile->ground_position, &projectile->visual_position);
        projectile->actor->position = projectile->visual_position;
        unsigned_actor_tick(projectile->actor);

        if (instance->elapsed < UINT16_MAX) {
            instance->elapsed++;
        }

        if ((projectile->expire_on_trajectory_end && !trajectory_active) || (projectile->lifetime > 0u && instance->elapsed >= projectile->lifetime)) {
            unsigned_projectile_pool_release(pool, instance);
        }
    }
}
