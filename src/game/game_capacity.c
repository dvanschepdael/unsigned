/**
 * @file game_capacity.c
 * @brief Derives game-wide actor and collision capacities from fixed pool sizes.
 */

#include "game/game_internal.h"

u16 unsigned_game_actor_capacity(u8 players, u8 npcs, u8 objects, u8 projectiles) {
    return (u16)players + (u16)npcs + (u16)objects + (u16)projectiles;
}

u8 unsigned_game_collision_query_storage_capacity(u8 players, u8 npcs, u8 objects, u8 projectiles) {
    const u16 actors = unsigned_game_actor_capacity(players, npcs, objects, projectiles);
    const u16 capacity = (u16)(actors * 2u);
    return capacity > UINT8_MAX ? UINT8_MAX : (u8)capacity;
}

UCollisionManagerConfig game_collision_config_build(u8 players, u8 npcs, u8 objects, u8 projectiles) {
    UCollisionManagerConfig collision = { 0 };
    const u16 actor_total = unsigned_game_actor_capacity(players, npcs, objects, projectiles);
    const u8 actor_capacity = actor_total > UINT8_MAX ? UINT8_MAX : (u8)actor_total;

    collision.layer_capacity[U_COLLISION_CHANNEL_WORLD_STATIC] = objects;
    collision.layer_capacity[U_COLLISION_CHANNEL_WORLD_DYNAMIC] = objects;
    collision.layer_capacity[U_COLLISION_CHANNEL_PICKUP] = objects;
    collision.layer_capacity[U_COLLISION_CHANNEL_TRIGGER] = objects;
    collision.layer_capacity[U_COLLISION_CHANNEL_INTERACTABLE] = objects;
    collision.layer_capacity[U_COLLISION_CHANNEL_DESTRUCTIBLE] = objects;

    collision.layer_capacity[U_COLLISION_CHANNEL_PLAYER_BODY] = players;
    collision.layer_capacity[U_COLLISION_CHANNEL_PLAYER_HURTBOX] = players;
    collision.layer_capacity[U_COLLISION_CHANNEL_PLAYER_ATTACK] = players;

    collision.layer_capacity[U_COLLISION_CHANNEL_ENEMY_BODY] = npcs;
    collision.layer_capacity[U_COLLISION_CHANNEL_ENEMY_HURTBOX] = npcs;
    collision.layer_capacity[U_COLLISION_CHANNEL_ENEMY_ATTACK] = npcs;

    collision.layer_capacity[U_COLLISION_CHANNEL_PLAYER_PROJECTILE] = projectiles;
    collision.layer_capacity[U_COLLISION_CHANNEL_ENEMY_PROJECTILE] = projectiles;

    collision.layer_capacity[U_COLLISION_CHANNEL_GAME_0] = actor_capacity;
    collision.layer_capacity[U_COLLISION_CHANNEL_GAME_1] = actor_capacity;

    collision.query_hit_capacity = unsigned_game_collision_query_storage_capacity(players, npcs, objects, projectiles);
    return collision;
}

u16 unsigned_game_collision_layer_storage_capacity(u8 players, u8 npcs, u8 objects, u8 projectiles) {
    const UCollisionManagerConfig collision = game_collision_config_build(players, npcs, objects, projectiles);
    return unsigned_physics_collision_layer_storage_capacity(&collision);
}
