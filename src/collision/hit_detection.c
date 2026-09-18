/**
 * @file hit_detection.c
 * @brief Implements gameplay attack hit detection.
 */

#include "collision/hit_detection.h"

#include "actor/actor.h"
#include "collision/actor_collision.h"
#include "collision/actor_index.h"
#include "core/tlss/tlss.h"
#include "physics/collision.h"

/** Append one valid attacker/target pair; false means the fixed hit buffer is now full. */
static bool hit_detection_try_add_hit(UCollisionHitContainer *hits, UActor *attacker, UActor *target) {
    if (attacker == NULL || target == NULL || attacker == target) {
        return true;
    }

    if ((size_t)hits->count >= ARRAY_COUNT(hits->instances)) {
        hits->overflowed = true;
        return false;
    }

    hits->instances[hits->count++] = (UCollisionHit){
        .attacker = attacker,
        .target = target,
    };
    return true;
}

/** Test one attacker only against indexed actors whose hurtbox channels are compatible. */
static bool hit_detection_detect_attacker(UCollisionHitContainer *hits, UCollisionManager *collisions, const UCollisionActorIndex *actors, UActor *attacker) {
    if (attacker == NULL) {
        return true;
    }

    if (!attacker->collision.hitbox_active) {
        return true;
    }

    UPhysicsCollisionChannel hitbox_channel = attacker->collision.hitbox_channel;
    if (hitbox_channel >= U_COLLISION_CHANNEL_COUNT) {
        return true;
    }

    const UCollisionBox *hitbox = &attacker->collision.hitbox;
    UPhysicsCollisionMask target_mask = (UPhysicsCollisionMask)(collisions->layers[hitbox_channel].collision_mask & actors->hurtbox_channels);
    if (target_mask == 0u) {
        return true;
    }

    UCollisionActorRange range = unsigned_collision_actor_index_range(actors, hitbox);
    s32 hit_left = hitbox->x;
    s32 hit_right = hit_left + hitbox->w;
    s32 hit_top = hitbox->y;
    s32 hit_bottom = hit_top + hitbox->h;

    for (u8 i = range.first; i < range.end; ++i) {
        UActor *target = actors->actors->instances[i];

        if (target == NULL || target == attacker) {
            continue;
        }

        if (!target->collision.hurtbox_active) {
            continue;
        }

        UPhysicsCollisionChannel hurtbox_channel = target->collision.hurtbox_channel;
        if (hurtbox_channel >= U_COLLISION_CHANNEL_COUNT || (target_mask & (UPhysicsCollisionMask)(1u << hurtbox_channel)) == 0u) {
            continue;
        }

        const UCollisionBox *hurtbox = &target->collision.hurtbox;
        if ((s32)hurtbox->x + hurtbox->w <= hit_left || hurtbox->x >= hit_right || (s32)hurtbox->y + hurtbox->h <= hit_top || hurtbox->y >= hit_bottom) {
            continue;
        }

        if (!hit_detection_try_add_hit(hits, attacker, target)) {
            return false;
        }
    }

    return true;
}

/** Detect frame-local gameplay hits; buffer overflow truncates results but still returns true. */
bool unsigned_collision_hit_detect(UCollisionHitContainer *hits, UCollisionManager *collisions, const UCollisionActorIndex *actors, const UTLSS *tlss) {
    if (hits == NULL || collisions == NULL || actors == NULL || actors->actors == NULL) {
        return false;
    }

    hits->count = 0u;
    hits->overflowed = false;

    for (u8 i = 0u; i < actors->count; ++i) {
        UActor *attacker = actors->actors->instances[i];

        if (attacker == NULL || !attacker->collision.hitbox_active || attacker->collision.hitbox_channel >= U_COLLISION_CHANNEL_COUNT) {
            continue;
        }

        if (!unsigned_actor_collision_should_resolve(attacker, tlss, i)) {
            continue;
        }

        if (!hit_detection_detect_attacker(hits, collisions, actors, attacker)) {
            break;
        }
    }

    return true;
}
