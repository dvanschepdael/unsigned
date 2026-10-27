/**
 * @file collision.c
 * @brief Implements fixed-capacity collision-layer registry and overlap queries.
 */

#include "physics/collision.h"

void unsigned_physics_collision_init(UCollisionManager *manager, const UCollisionManagerConfig *config, const UCollisionBox **layer_boxes) {
    u16 offset = 0u;

    *manager = (UCollisionManager){0};
    for (UPhysicsCollisionChannel channel = 0u; channel < U_COLLISION_CHANNEL_COUNT; ++channel) {
        UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
        const u8 capacity = config->layer_capacity[channel];
        boxes->instances = capacity > 0u ? &layer_boxes[offset] : NULL;
        offset = (u16)(offset + capacity);
    }
}

void unsigned_physics_collision_clear(UCollisionManager *manager) {
    for (UPhysicsCollisionChannel channel = 0u; channel < U_COLLISION_CHANNEL_COUNT; ++channel) {
        UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
        boxes->count = 0u;
        boxes->static_count = 0u;
    }
}

void unsigned_physics_collision_clear_dynamic(UCollisionManager *manager) {
    for (UPhysicsCollisionChannel channel = 0u; channel < U_COLLISION_CHANNEL_COUNT; ++channel) {
        UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
        boxes->count = boxes->static_count;
    }
}

void unsigned_physics_collision_configure_layer(UCollisionManager *manager, UPhysicsCollisionChannel channel, UPhysicsCollisionMask collision_mask) {
    manager->layers[channel].collision_mask = collision_mask;
}

void unsigned_physics_collision_register(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box) {
    UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
    boxes->instances[boxes->count++] = box;
}

void unsigned_physics_collision_register_static(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box) {
    UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;

    for (u8 i = boxes->count; i > boxes->static_count; --i) {
        boxes->instances[i] = boxes->instances[i - 1u];
    }
    boxes->instances[boxes->static_count] = box;
    ++boxes->static_count;
    ++boxes->count;
}

bool unsigned_physics_collision_any_mask(UCollisionManager *manager, UPhysicsCollisionMask target_mask, const UCollisionBox *box) {
    s32 left = box->x;
    s32 right = left + box->w;
    s32 top = box->y;
    s32 bottom = top + box->h;

    for (UPhysicsCollisionChannel target_channel = 0u; target_mask != 0u; ++target_channel, target_mask = (UPhysicsCollisionMask)(target_mask >> 1)) {
        if ((target_mask & 1u) == 0u) {
            continue;
        }

        const UCollisionLayerBoxContainer *target_boxes = &manager->layers[target_channel].boxes;
        for (u8 i = 0u; i < target_boxes->count; ++i) {
            const UCollisionBox *other = target_boxes->instances[i];

            if (other == box) {
                continue;
            }

            if (left < (s32)other->x + other->w && right > other->x && top < (s32)other->y + other->h && bottom > other->y) {
                return true;
            }
        }
    }

    return false;
}
