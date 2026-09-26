/**
 * @file game_capacity.c
 * @brief Derives game-wide actor and collision capacities from fixed pool sizes.
 */

#include "game/game_internal.h"

static const UPhysicsCollisionChannel PLAYER_CHANNELS[] = {
    U_COLLISION_CHANNEL_PLAYER_BODY,
    U_COLLISION_CHANNEL_PLAYER_HURTBOX,
    U_COLLISION_CHANNEL_PLAYER_ATTACK,
};

static const UPhysicsCollisionChannel NPC_CHANNELS[] = {
    U_COLLISION_CHANNEL_ENEMY_BODY,
    U_COLLISION_CHANNEL_ENEMY_HURTBOX,
    U_COLLISION_CHANNEL_ENEMY_ATTACK,
};

static const UPhysicsCollisionChannel OBJECT_CHANNELS[] = {
    U_COLLISION_CHANNEL_WORLD_STATIC,
    U_COLLISION_CHANNEL_WORLD_DYNAMIC,
    U_COLLISION_CHANNEL_PICKUP,
    U_COLLISION_CHANNEL_TRIGGER,
    U_COLLISION_CHANNEL_INTERACTABLE,
    U_COLLISION_CHANNEL_DESTRUCTIBLE,
};

static const UPhysicsCollisionChannel PROJECTILE_CHANNELS[] = {
    U_COLLISION_CHANNEL_PLAYER_PROJECTILE,
    U_COLLISION_CHANNEL_ENEMY_PROJECTILE,
};

static const UPhysicsCollisionChannel GAME_CHANNELS[] = {
    U_COLLISION_CHANNEL_GAME_0,
    U_COLLISION_CHANNEL_GAME_1,
};

/** Apply one actor-class capacity to every collision channel published by that class. */
static void game_collision_config_set(UCollisionManagerConfig *collision, const UPhysicsCollisionChannel *channels, u8 channel_count, u8 capacity) {
    for (u8 i = 0u; i < channel_count; ++i) {
        collision->layer_capacity[channels[i]] = capacity;
    }
}

UCollisionManagerConfig game_collision_config_build(u8 players, u8 npcs, u8 objects, u8 projectiles) {
    UCollisionManagerConfig collision = {0};
    const u8 actor_capacity = (u8)((u16)players + (u16)npcs + (u16)objects + (u16)projectiles);

    game_collision_config_set(&collision, PLAYER_CHANNELS, ARRAY_COUNT_U8(PLAYER_CHANNELS), players);
    game_collision_config_set(&collision, NPC_CHANNELS, ARRAY_COUNT_U8(NPC_CHANNELS), npcs);
    game_collision_config_set(&collision, OBJECT_CHANNELS, ARRAY_COUNT_U8(OBJECT_CHANNELS), objects);
    game_collision_config_set(&collision, PROJECTILE_CHANNELS, ARRAY_COUNT_U8(PROJECTILE_CHANNELS), projectiles);
    game_collision_config_set(&collision, GAME_CHANNELS, ARRAY_COUNT_U8(GAME_CHANNELS), actor_capacity);

    return collision;
}
