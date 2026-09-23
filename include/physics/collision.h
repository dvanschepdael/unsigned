/**
 * @file collision.h
 * @brief Fixed-capacity collision-layer registry and overlap-query service.
 */

#ifndef UNSIGNED_PHYSICS_COLLISION_H
#define UNSIGNED_PHYSICS_COLLISION_H

#include "core/types.h"
#include "physics/collision_box.h"

typedef u8 UPhysicsCollisionChannel;
typedef u16 UPhysicsCollisionMask;

#define U_COLLISION_CHANNEL_NONE ((UPhysicsCollisionChannel)U_COLLISION_CHANNEL_COUNT)

/** Semantic channels available to gameplay and level collision code. */
typedef enum UPhysicsCollisionChannelType {
    U_COLLISION_CHANNEL_WORLD_STATIC = 0,
    U_COLLISION_CHANNEL_WORLD_DYNAMIC,
    U_COLLISION_CHANNEL_PLAYER_BODY,
    U_COLLISION_CHANNEL_ENEMY_BODY,
    U_COLLISION_CHANNEL_PLAYER_HURTBOX,
    U_COLLISION_CHANNEL_ENEMY_HURTBOX,
    U_COLLISION_CHANNEL_PLAYER_ATTACK,
    U_COLLISION_CHANNEL_ENEMY_ATTACK,
    U_COLLISION_CHANNEL_PLAYER_PROJECTILE,
    U_COLLISION_CHANNEL_ENEMY_PROJECTILE,
    U_COLLISION_CHANNEL_PICKUP,
    U_COLLISION_CHANNEL_TRIGGER,
    U_COLLISION_CHANNEL_INTERACTABLE,
    U_COLLISION_CHANNEL_DESTRUCTIBLE,
    U_COLLISION_CHANNEL_GAME_0,
    U_COLLISION_CHANNEL_GAME_1,
    U_COLLISION_CHANNEL_COUNT
} UPhysicsCollisionChannelType;

/** One channel's fixed-capacity pointer view. Static entries are always kept first. */
typedef struct UCollisionLayerBoxContainer {
    u8 count;
    u8 static_count;
    const UCollisionBox **instances;
} UCollisionLayerBoxContainer;

/** Registered boxes plus the mask of channels this layer is allowed to query against. */
typedef struct UCollisionLayer {
    UCollisionLayerBoxContainer boxes;
    UPhysicsCollisionMask collision_mask;
} UCollisionLayer;

/** Capacity contract used to partition caller-owned collision pointer storage during initialization. */
typedef struct UCollisionManagerConfig {
    u8 layer_capacity[U_COLLISION_CHANNEL_COUNT];
} UCollisionManagerConfig;

typedef struct UCollisionManager {
    UCollisionLayer layers[U_COLLISION_CHANNEL_COUNT];
} UCollisionManager;

/**
 * @brief Partitions caller-owned collision storage into semantic channels.
 *
 * Layer capacities partition caller-owned storage once at initialization. Registration then writes
 * directly into those authored ranges; content composition guarantees that each channel fits.
 *
 * @param layer_boxes Caller-owned pointer storage partitioned between configured channels.
 * @pre `manager` and `config` are valid.
 * @pre `layer_boxes` provides the sum of all configured `layer_capacity` entries when that sum is non-zero.
 */
void unsigned_physics_collision_init(UCollisionManager *manager, const UCollisionManagerConfig *config, const UCollisionBox **layer_boxes);

/** Remove every static and dynamic registration from all collision layers. */
void unsigned_physics_collision_clear(UCollisionManager *manager);

/**
 * Remove only frame-owned registrations while preserving the static prefix of each layer.
 * Level code calls this before rebuilding actors and other dynamic collision geometry.
 */
void unsigned_physics_collision_clear_dynamic(UCollisionManager *manager);

/** Configure which target channels `channel` is allowed to collide/query against.
 * @pre `channel < U_COLLISION_CHANNEL_COUNT`. */
void unsigned_physics_collision_configure_layer(UCollisionManager *manager, UPhysicsCollisionChannel channel, UPhysicsCollisionMask collision_mask);

/**
 * Append a frame-owned box to `channel`.
 * @pre `channel < U_COLLISION_CHANNEL_COUNT`.
 * @pre The channel has room for this registration; capacities are authored from the maximum
 *      number of actors/objects that can publish into each channel.
 */
void unsigned_physics_collision_register(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box);

/**
 * Insert a persistent box in the static prefix of `channel`. Static registrations survive
 * `unsigned_physics_collision_clear_dynamic()` and should normally be created at level load.
 * @pre `channel < U_COLLISION_CHANNEL_COUNT`.
 * @pre The channel has room for this registration.
 */
void unsigned_physics_collision_register_static(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box);

/** Return whether `box` overlaps any registered box in the explicitly supplied target mask. */
bool unsigned_physics_collision_any_mask(UCollisionManager *manager, UPhysicsCollisionMask target_mask, const UCollisionBox *box);

#endif
