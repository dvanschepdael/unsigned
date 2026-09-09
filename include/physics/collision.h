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
    u8 capacity;
    const UCollisionBox **instances;
} UCollisionLayerBoxContainer;

/** Registered boxes plus the mask of channels this layer is allowed to query against. */
typedef struct UCollisionLayer {
    UCollisionLayerBoxContainer boxes;
    UPhysicsCollisionMask collision_mask;
} UCollisionLayer;

/** Capacity contract used to partition `UCollisionManagerStorage` during initialization. */
typedef struct UCollisionManagerConfig {
    u8 layer_capacity[U_COLLISION_CHANNEL_COUNT];
    u8 query_hit_capacity;
} UCollisionManagerConfig;

/** Caller-owned memory; `layer_boxes` contains the sum of every configured layer capacity. */
typedef struct UCollisionManagerStorage {
    const UCollisionBox **layer_boxes;
    u16 layer_box_capacity;
    const UCollisionBox **query_hits;
    u8 query_hit_capacity;
} UCollisionManagerStorage;

typedef struct UCollisionManager {
    UCollisionLayer layers[U_COLLISION_CHANNEL_COUNT];
    const UCollisionBox **query_hits;
    u8 query_hit_capacity;
    /** Channels whose dynamic/static registration could not fit this frame. */
    UPhysicsCollisionMask overflowed_channels;
    /** Subset of overflowed_channels caused while registering persistent boxes. */
    UPhysicsCollisionMask static_overflowed_channels;
} UCollisionManager;

/** Sum every per-channel layer capacity in a manager configuration. */
u16 unsigned_physics_collision_layer_storage_capacity(const UCollisionManagerConfig *config);

/**
 * Partition caller-provided storage among the configured layers and query scratch buffer.
 * Returns false when pointers/capacities are inconsistent; no partial manager should be used.
 */
bool unsigned_physics_collision_init(UCollisionManager *manager, const UCollisionManagerConfig *config, const UCollisionManagerStorage *storage);

/** Remove every static and dynamic registration and clear overflow diagnostics. */
void unsigned_physics_collision_clear(UCollisionManager *manager);

/**
 * Remove only frame-owned registrations while preserving the static prefix of each layer.
 * Level code calls this before rebuilding actors and other dynamic collision geometry.
 */
void unsigned_physics_collision_clear_dynamic(UCollisionManager *manager);

/** Configure which target channels `channel` is allowed to collide/query against. */
bool unsigned_physics_collision_configure_layer(UCollisionManager *manager, UPhysicsCollisionChannel channel, UPhysicsCollisionMask collision_mask);

/**
 * Append a frame-owned box to `channel`. On capacity exhaustion the channel overflow bit is
 * set and false is returned; registrations that already succeeded remain valid.
 */
bool unsigned_physics_collision_register(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box);

/**
 * Insert a persistent box in the static prefix of `channel`. Static registrations survive
 * `unsigned_physics_collision_clear_dynamic()` and should normally be created at level load.
 */
bool unsigned_physics_collision_register_static(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box);

/** Remove one exact box pointer from a channel while preserving static/dynamic ordering. */
bool unsigned_physics_collision_unregister(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box);

/** Return whether the exact box pointer is currently registered in `channel`. */
bool unsigned_physics_collision_is_registered(const UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box);

/** Return whether `box` overlaps any box in channels allowed by `channel`'s collision mask. */
bool unsigned_physics_collision_any(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box);

/** Return whether `box` overlaps any registered box in the explicitly supplied target mask. */
bool unsigned_physics_collision_any_mask(UCollisionManager *manager, UPhysicsCollisionMask target_mask, const UCollisionBox *box);

/**
 * Collect all overlaps allowed by `channel` into `out_hits` using the manager query scratch
 * buffer. The result container is caller-visible but its entries refer to registered boxes.
 */
bool unsigned_physics_collision_intersects(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box, UCollisionBoxContainer *out_hits);

/** True when at least one registration was truncated because a layer reached capacity. */
static inline bool unsigned_physics_collision_has_registration_overflow(const UCollisionManager *manager) {
    return manager != NULL && manager->overflowed_channels != 0u;
}

/** True when the selected channel specifically reached its configured registration capacity. */
static inline bool unsigned_physics_collision_channel_overflowed(const UCollisionManager *manager, UPhysicsCollisionChannel channel) {
    return manager != NULL && channel < U_COLLISION_CHANNEL_COUNT && (manager->overflowed_channels & (UPhysicsCollisionMask)(1u << channel)) != 0u;
}

#endif
