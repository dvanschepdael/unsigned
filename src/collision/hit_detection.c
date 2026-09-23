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

/** Test one attacker only against indexed actors whose hurtbox channels are compatible. */
static void hit_detection_detect_attacker(UCollisionHitContainer *hits, UCollisionManager *collisions, const UCollisionActorIndex *actors, UActor *attacker) {
    UPhysicsCollisionChannel hitbox_channel = attacker->collision.hitbox_channel;

    const UCollisionBox *hitbox = &attacker->collision.hitbox;
    UPhysicsCollisionMask target_mask = (UPhysicsCollisionMask)(collisions->layers[hitbox_channel].collision_mask & actors->hurtbox_channels);
    if (target_mask == 0u) {
        return;
    }

    UCollisionActorRange range = unsigned_collision_actor_index_range_mask(actors, hitbox, target_mask);
    s32 hit_left = hitbox->x;
    s32 hit_right = hit_left + hitbox->w;
    s32 hit_top = hitbox->y;
    s32 hit_bottom = hit_top + hitbox->h;

    for (u8 i = range.first; i < range.end; ++i) {
        UActor *target = actors->actors->instances[i];

        if (target == attacker) {
            continue;
        }

        if (target->collision.hurtbox_channel == U_COLLISION_CHANNEL_NONE) {
            continue;
        }

        UPhysicsCollisionChannel hurtbox_channel = target->collision.hurtbox_channel;
        if ((target_mask & (UPhysicsCollisionMask)(1u << hurtbox_channel)) == 0u) {
            continue;
        }

        const UCollisionBox *hurtbox = &target->collision.hurtbox;
        if ((s32)hurtbox->x + hurtbox->w <= hit_left || hurtbox->x >= hit_right || (s32)hurtbox->y + hurtbox->h <= hit_top || hurtbox->y >= hit_bottom) {
            continue;
        }

        hits->instances[hits->count++] = (UCollisionHit){
            .attacker = attacker,
            .target = target,
        };
    }
}

/** Detect frame-local gameplay hits into the caller-dimensioned fixed frame buffer. */
void unsigned_collision_hit_detect(UCollisionHitContainer *hits, UCollisionManager *collisions, const UCollisionActorIndex *actors, const UTLSS *tlss) {
    hits->count = 0u;

    const UCollisionActorRange attackers = unsigned_collision_actor_index_hitbox_range(actors);
    for (u8 i = attackers.first; i < attackers.end; ++i) {
        UActor *attacker = actors->actors->instances[i];

        if (attacker->collision.hitbox_channel == U_COLLISION_CHANNEL_NONE) {
            continue;
        }

        if (!unsigned_actor_collision_should_resolve(attacker, tlss, i)) {
            continue;
        }

        hit_detection_detect_attacker(hits, collisions, actors, attacker);
    }
}
