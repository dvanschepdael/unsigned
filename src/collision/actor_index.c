/**
 * @file actor_index.c
 * @brief Implements lookup index from registered collision boxes back to actors.
 */

#include "collision/actor_index.h"

#include "actor/actor.h"

/** Expands an actor-index Y interval to include another collision box. */
static void actor_index_expand_y(UCollisionActorIndex *index, const UCollisionBox *box, bool first_box) {
    const s32 top = box->offset_y;
    const s32 bottom = top + box->h;

    if (first_box) {
        index->min_top_offset = top;
        index->max_bottom_offset = bottom;
        return;
    }

    if (top < index->min_top_offset) {
        index->min_top_offset = top;
    }
    if (bottom > index->max_bottom_offset) {
        index->max_bottom_offset = bottom;
    }
}

bool unsigned_collision_actor_index_build(UCollisionActorIndex *index, const UActorContainer *actors, const UCollisionManager *collisions) {
    const UActor *previous = NULL;
    u16 actor_box_count[U_COLLISION_CHANNEL_COUNT] = { 0u };
    bool has_bounds = false;

    if (index == NULL || actors == NULL || collisions == NULL || actors->count > actors->capacity || (actors->count > 0u && actors->instances == NULL)) {
        return false;
    }

    *index = (UCollisionActorIndex){
        .actors = actors,
        .count = actors->count,
        .sorted = true,
    };

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *actor = actors->instances[i];

        if (actor == NULL || !actor->active) {
            continue;
        }

        if (previous != NULL && actor->position.y < previous->position.y) {
            index->sorted = false;
        }
        previous = actor;

        UPhysicsCollisionChannel hitbox_channel = actor->collision.hitbox_channel;
        UPhysicsCollisionChannel hurtbox_channel = actor->collision.hurtbox_channel;

        if (hitbox_channel >= U_COLLISION_CHANNEL_COUNT) {
            hitbox_channel = U_COLLISION_CHANNEL_NONE;
        }
        if (hurtbox_channel >= U_COLLISION_CHANNEL_COUNT) {
            hurtbox_channel = U_COLLISION_CHANNEL_NONE;
        }

        if (actor->collision.hitbox_active && hitbox_channel != U_COLLISION_CHANNEL_NONE) {
            index->channels |= (UPhysicsCollisionMask)(1u << hitbox_channel);
            actor_box_count[hitbox_channel]++;
            actor_index_expand_y(index, &actor->collision.hitbox, !has_bounds);
            has_bounds = true;
        }

        if (actor->collision.hurtbox_active && hurtbox_channel != U_COLLISION_CHANNEL_NONE) {
            const UPhysicsCollisionMask mask = (UPhysicsCollisionMask)(1u << hurtbox_channel);
            index->channels |= mask;
            index->hurtbox_channels |= mask;
            actor_box_count[hurtbox_channel]++;
            actor_index_expand_y(index, &actor->collision.hurtbox, !has_bounds);
            has_bounds = true;
        }
    }

    for (UPhysicsCollisionChannel channel = 0u; channel < U_COLLISION_CHANNEL_COUNT; ++channel) {
        const u8 manager_count = collisions->layers[channel].boxes.count;

        if (manager_count > 0u && actor_box_count[channel] == (u16)manager_count) {
            index->covered_channels |= (UPhysicsCollisionMask)(1u << channel);
        }
    }

    return true;
}

/** Finds the first indexed actor range whose maximum Y can overlap the query box. */
static u8 actor_index_lower_bound_y(const UCollisionActorIndex *index, s32 y) {
    u8 first = 0u;
    u8 end = index->count;

    while (first < end) {
        const u8 middle = (u8)(first + ((end - first) >> 1));
        const UActor *actor = index->actors->instances[middle];

        if (actor != NULL && actor->position.y < y) {
            first = (u8)(middle + 1u);
        } else {
            end = middle;
        }
    }

    return first;
}

UCollisionActorRange unsigned_collision_actor_index_range(const UCollisionActorIndex *index, const UCollisionBox *box) {
    UCollisionActorRange range = { 0u, 0u };

    if (index == NULL || index->actors == NULL || box == NULL || index->count == 0u || box->w <= 0 || box->h <= 0) {
        return range;
    }

    if (!index->sorted) {
        range.end = index->count;
        return range;
    }

    const s32 top = box->y;
    const s32 bottom = top + box->h;
    const s32 first_y = top - index->max_bottom_offset + 1;
    const s32 end_y = bottom - index->min_top_offset;

    range.first = actor_index_lower_bound_y(index, first_y);
    range.end = actor_index_lower_bound_y(index, end_y);
    return range;
}
