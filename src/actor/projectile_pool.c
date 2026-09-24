/**
 * @file projectile_pool.c
 * @brief Implements fixed-capacity projectile pool binding and ticking.
 */

#include "actor/projectile_pool.h"

#include <stdint.h>

UProjectilePoolInstance *unsigned_projectile_pool_reserve(UPoolInstanceContainer *pool, UProjectile *projectile) {
    UProjectilePoolInstance *instance = unsigned_pool_reserve(pool);

    unsigned_physics_trajectory_project(&projectile->trajectory, projectile->projection, projectile->projection_origin, &projectile->ground_position, &projectile->visual_position);
    projectile->actor->position = projectile->visual_position;
    unsigned_actor_collision_state_reset(&projectile->actor->collision);
    projectile->actor->collision.resolve_immediately = true;
    projectile->collision_latched = false;
    instance->args = projectile;
    instance->elapsed = 0u;
    return instance;
}

void unsigned_projectile_pool_release(UPoolInstanceContainer *pool, UProjectilePoolInstance *instance) {
    UProjectile *projectile = instance->args;
    unsigned_actor_destroy(projectile->actor);
    unsigned_pool_release(pool, instance);
}

void unsigned_projectile_pool_tick(UPoolInstanceContainer *pool) {
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool); ++i) {
        UProjectilePoolInstance *instance = &pool->instances[i];
        if (!instance->active) {
            continue;
        }

        UProjectile *projectile = instance->args;
        unsigned_physics_trajectory_tick(&projectile->trajectory, projectile->trajectory_function, projectile->trajectory_context);
        unsigned_physics_trajectory_project(&projectile->trajectory, projectile->projection, projectile->projection_origin, &projectile->ground_position, &projectile->visual_position);
        projectile->actor->position = projectile->visual_position;
        unsigned_actor_tick(projectile->actor);

        ++instance->elapsed;

        if ((projectile->expire_on_trajectory_end && !projectile->trajectory.active) || (projectile->lifetime > 0u && instance->elapsed >= projectile->lifetime)) {
            unsigned_projectile_pool_release(pool, instance);
        }
    }
}
