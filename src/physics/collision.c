/**
 * @file collision.c
 * @brief Implements fixed-capacity collision-layer registry and overlap queries.
 */

#include "physics/collision.h"

u16 unsigned_physics_collision_layer_storage_capacity(const UCollisionManagerConfig *config) {
    u16 capacity = 0u;

    if (config == NULL) {
        return 0u;
    }

    for (UPhysicsCollisionChannel channel = 0u; channel < U_COLLISION_CHANNEL_COUNT; ++channel) {
        capacity = (u16)(capacity + config->layer_capacity[channel]);
    }

    return capacity;
}

bool unsigned_physics_collision_init(UCollisionManager *manager, const UCollisionManagerConfig *config, const UCollisionManagerStorage *storage) {
    u16 offset = 0u;

    if (manager == NULL || config == NULL || storage == NULL) {
        return false;
    }

    const u16 required_layer_boxes = unsigned_physics_collision_layer_storage_capacity(config);

    if ((required_layer_boxes > 0u && storage->layer_boxes == NULL) || required_layer_boxes > storage->layer_box_capacity || (config->query_hit_capacity > 0u && storage->query_hits == NULL) || config->query_hit_capacity > storage->query_hit_capacity) {
        return false;
    }

    *manager = (UCollisionManager){ 0 };
    manager->query_hits = storage->query_hits;
    manager->query_hit_capacity = config->query_hit_capacity;

    for (UPhysicsCollisionChannel channel = 0u; channel < U_COLLISION_CHANNEL_COUNT; ++channel) {
        UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;

        boxes->capacity = config->layer_capacity[channel];
        boxes->instances = boxes->capacity > 0u ? &storage->layer_boxes[offset] : NULL;
        offset = (u16)(offset + boxes->capacity);
    }

    return true;
}

void unsigned_physics_collision_clear(UCollisionManager *manager) {
    if (manager == NULL) {
        return;
    }

    for (UPhysicsCollisionChannel channel = 0u; channel < U_COLLISION_CHANNEL_COUNT; ++channel) {
        UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
        boxes->count = 0u;
        boxes->static_count = 0u;
    }
    manager->overflowed_channels = 0u;
    manager->static_overflowed_channels = 0u;
}

void unsigned_physics_collision_clear_dynamic(UCollisionManager *manager) {
    if (manager == NULL) {
        return;
    }

    for (UPhysicsCollisionChannel channel = 0u; channel < U_COLLISION_CHANNEL_COUNT; ++channel) {
        UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
        boxes->count = boxes->static_count;
    }

    manager->overflowed_channels = manager->static_overflowed_channels;
}

bool unsigned_physics_collision_configure_layer(UCollisionManager *manager, UPhysicsCollisionChannel channel, UPhysicsCollisionMask collision_mask) {
    if (manager == NULL || channel >= U_COLLISION_CHANNEL_COUNT) {
        return false;
    }

    manager->layers[channel].collision_mask = collision_mask;
    return true;
}

bool unsigned_physics_collision_register(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box) {
    if (manager == NULL || channel >= U_COLLISION_CHANNEL_COUNT || box == NULL) {
        return false;
    }

    UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
    if (boxes->count >= boxes->capacity) {
        manager->overflowed_channels |= (UPhysicsCollisionMask)(1u << channel);
        return false;
    }

    boxes->instances[boxes->count++] = box;
    return true;
}

bool unsigned_physics_collision_register_static(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box) {
    if (manager == NULL || channel >= U_COLLISION_CHANNEL_COUNT || box == NULL) {
        return false;
    }

    UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
    if (boxes->count >= boxes->capacity) {
        const UPhysicsCollisionMask bit = (UPhysicsCollisionMask)(1u << channel);
        manager->overflowed_channels |= bit;
        manager->static_overflowed_channels |= bit;
        return false;
    }

    for (u8 i = boxes->count; i > boxes->static_count; --i) {
        boxes->instances[i] = boxes->instances[i - 1u];
    }
    boxes->instances[boxes->static_count] = box;
    ++boxes->static_count;
    ++boxes->count;
    return true;
}

bool unsigned_physics_collision_unregister(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box) {
    if (manager == NULL || channel >= U_COLLISION_CHANNEL_COUNT || box == NULL) {
        return false;
    }

    UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
    for (u8 i = 0; i < boxes->count; ++i) {
        if (boxes->instances[i] != box) {
            continue;
        }

        for (u8 next = (u8)(i + 1); next < boxes->count; ++next) {
            boxes->instances[next - 1] = boxes->instances[next];
        }
        boxes->count--;
        if (i < boxes->static_count) {
            --boxes->static_count;
        }
        boxes->instances[boxes->count] = NULL;
        return true;
    }

    return false;
}

bool unsigned_physics_collision_is_registered(const UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box) {
    if (manager == NULL || channel >= U_COLLISION_CHANNEL_COUNT || box == NULL) {
        return false;
    }

    const UCollisionLayerBoxContainer *boxes = &manager->layers[channel].boxes;
    for (u8 i = 0u; i < boxes->count; ++i) {
        if (boxes->instances[i] == box) {
            return true;
        }
    }

    return false;
}

bool unsigned_physics_collision_any_mask(UCollisionManager *manager, UPhysicsCollisionMask target_mask, const UCollisionBox *box) {
    if (manager == NULL || box == NULL || box->w <= 0 || box->h <= 0) {
        return false;
    }

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

            if (other == box || other == NULL || other->w <= 0 || other->h <= 0) {
                continue;
            }

            if (left < (s32)other->x + other->w && right > other->x && top < (s32)other->y + other->h && bottom > other->y) {
                return true;
            }
        }
    }

    return false;
}

bool unsigned_physics_collision_any(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box) {
    if (manager == NULL || channel >= U_COLLISION_CHANNEL_COUNT) {
        return false;
    }

    return unsigned_physics_collision_any_mask(manager, manager->layers[channel].collision_mask, box);
}

bool unsigned_physics_collision_intersects(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box, UCollisionBoxContainer *hits) {
    if (manager == NULL || channel >= U_COLLISION_CHANNEL_COUNT || box == NULL || hits == NULL) {
        return false;
    }

    *hits = (UCollisionBoxContainer){
        .count = 0,
        .capacity = manager->query_hit_capacity,
        .instances = manager->query_hits,
        .overflowed = false,
    };
    if (box->w <= 0 || box->h <= 0) {
        return false;
    }

    UPhysicsCollisionMask collision_mask = manager->layers[channel].collision_mask;
    if (collision_mask == 0u) {
        return false;
    }

    s32 left = box->x;
    s32 right = left + box->w;
    s32 top = box->y;
    s32 bottom = top + box->h;

    for (UPhysicsCollisionChannel target_channel = 0u; collision_mask != 0u; ++target_channel, collision_mask = (UPhysicsCollisionMask)(collision_mask >> 1)) {
        if ((collision_mask & 1u) == 0u) {
            continue;
        }

        const UCollisionLayerBoxContainer *target_boxes = &manager->layers[target_channel].boxes;
        for (u8 i = 0; i < target_boxes->count; ++i) {
            const UCollisionBox *other = target_boxes->instances[i];

            if (other == box || other == NULL || other->w <= 0 || other->h <= 0) {
                continue;
            }

            if (!(left < (s32)other->x + other->w && right > other->x && top < (s32)other->y + other->h && bottom > other->y)) {
                continue;
            }

            if (hits->count >= hits->capacity) {
                hits->overflowed = true;
                return true;
            }
            hits->instances[hits->count++] = other;
        }
    }

    return hits->count > 0u;
}
