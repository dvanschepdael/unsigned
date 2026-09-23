/**
 * @file actor_index.c
 * @brief Implements lookup index from registered collision boxes back to actors.
 */

#include "collision/actor_index.h"

#include "actor/actor.h"

/** Expands one actor-index Y interval to include another collision box. */
static void actor_index_expand_y(s32 *min_top_offset, s32 *max_bottom_offset, const UCollisionBox *box, bool first_box) {
    const s32 top = box->offset_y;
    const s32 bottom = top + box->h;

    if (first_box) {
        *min_top_offset = top;
        *max_bottom_offset = bottom;
        return;
    }

    if (top < *min_top_offset) {
        *min_top_offset = top;
    }
    if (bottom > *max_bottom_offset) {
        *max_bottom_offset = bottom;
    }
}

void unsigned_collision_actor_index_build(UCollisionActorIndex *index, const UActorContainer *actors, const UCollisionManager *collisions) {
    u16 actor_box_count[U_COLLISION_CHANNEL_COUNT] = {0u};
    bool has_hitbox_bounds = false;
    bool has_hurtbox_bounds = false;

    *index = (UCollisionActorIndex){
        .actors = actors,
        .hitbox_first = actors->count,
    };

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];

        const UPhysicsCollisionChannel hitbox_channel = actor->collision.hitbox_channel;
        const UPhysicsCollisionChannel hurtbox_channel = actor->collision.hurtbox_channel;

        if (actor->collision.hitbox_channel != U_COLLISION_CHANNEL_NONE) {
            const UPhysicsCollisionMask mask = (UPhysicsCollisionMask)(1u << hitbox_channel);
            index->hitbox_channels |= mask;
            if (i < index->hitbox_first) {
                index->hitbox_first = i;
            }
            index->hitbox_end = (u8)(i + 1u);
            actor_box_count[hitbox_channel]++;
            actor_index_expand_y(&index->hitbox_min_top_offset, &index->hitbox_max_bottom_offset, &actor->collision.hitbox, !has_hitbox_bounds);
            has_hitbox_bounds = true;
        }

        if (actor->collision.hurtbox_channel != U_COLLISION_CHANNEL_NONE) {
            const UPhysicsCollisionMask mask = (UPhysicsCollisionMask)(1u << hurtbox_channel);
            index->hurtbox_channels |= mask;
            actor_box_count[hurtbox_channel]++;
            actor_index_expand_y(&index->hurtbox_min_top_offset, &index->hurtbox_max_bottom_offset, &actor->collision.hurtbox, !has_hurtbox_bounds);
            has_hurtbox_bounds = true;
        }
    }

    for (UPhysicsCollisionChannel channel = 0u; channel < U_COLLISION_CHANNEL_COUNT; ++channel) {
        const u8 manager_count = collisions->layers[channel].boxes.count;

        if (manager_count > 0u && actor_box_count[channel] == (u16)manager_count) {
            index->covered_channels |= (UPhysicsCollisionMask)(1u << channel);
        }
    }
}

/** Finds the first indexed actor range whose maximum Y can overlap the query box. */
static u8 actor_index_lower_bound_y(const UCollisionActorIndex *index, s32 y) {
    u8 first = 0u;
    u8 end = index->actors->count;

    while (first < end) {
        const u8 middle = (u8)(first + ((end - first) >> 1));
        const UActor *actor = index->actors->instances[middle];

        if (actor->position.y < y) {
            first = (u8)(middle + 1u);
        } else {
            end = middle;
        }
    }

    return first;
}

UCollisionActorRange unsigned_collision_actor_index_range_mask(const UCollisionActorIndex *index, const UCollisionBox *box, UPhysicsCollisionMask target_mask) {
    UCollisionActorRange range = {0u, 0u};
    s32 min_top_offset = 0;
    s32 max_bottom_offset = 0;
    bool has_bounds = false;

    if (index->actors->count == 0u) {
        return range;
    }

    if ((target_mask & index->hitbox_channels) != 0u) {
        min_top_offset = index->hitbox_min_top_offset;
        max_bottom_offset = index->hitbox_max_bottom_offset;
        has_bounds = true;
    }
    if ((target_mask & index->hurtbox_channels) != 0u) {
        if (!has_bounds) {
            min_top_offset = index->hurtbox_min_top_offset;
            max_bottom_offset = index->hurtbox_max_bottom_offset;
        } else {
            if (index->hurtbox_min_top_offset < min_top_offset) {
                min_top_offset = index->hurtbox_min_top_offset;
            }
            if (index->hurtbox_max_bottom_offset > max_bottom_offset) {
                max_bottom_offset = index->hurtbox_max_bottom_offset;
            }
        }
        has_bounds = true;
    }
    if (!has_bounds) {
        return range;
    }

    const s32 top = box->y;
    const s32 bottom = top + box->h;
    const s32 first_y = top - max_bottom_offset + 1;
    const s32 end_y = bottom - min_top_offset;

    range.first = actor_index_lower_bound_y(index, first_y);
    range.end = actor_index_lower_bound_y(index, end_y);
    return range;
}

UCollisionActorRange unsigned_collision_actor_index_hitbox_range(const UCollisionActorIndex *index) {
    if (index->actors->count == 0u || index->hitbox_first >= index->hitbox_end) {
        return (UCollisionActorRange){0u, 0u};
    }
    return (UCollisionActorRange){index->hitbox_first, index->hitbox_end};
}
