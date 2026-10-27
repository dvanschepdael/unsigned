/**
 * @file hit_detection.c
 * @brief Implements gameplay attack hit detection.
 */

#include "collision/hit_detection.h"

#include "actor/actor.h"
#include "collision/hit_scan.h"
#include "collision/actor_collision.h"
#include "collision/actor_index.h"
#include "core/tlss/tlss.h"
#include "physics/collision.h"

/** Test one attacker only against indexed actors whose hurtbox channels are compatible. */
static void hit_detection_scan_attacker(UCollisionHitContainer *hits, UCollisionManager *collisions, const UCollisionActorIndex *actors, UActor *attacker) {
    UPhysicsCollisionChannel hitbox_channel = attacker->collision.hitbox_channel;

    const UCollisionBox *hitbox = &attacker->collision.hitbox;
    UPhysicsCollisionMask target_mask = (UPhysicsCollisionMask)(collisions->layers[hitbox_channel].collision_mask & actors->hurtbox_channels);
    if (target_mask == 0u) {
        return;
    }

    const UCollisionActorRange range = unsigned_collision_actor_index_range_mask(actors, hitbox, target_mask);
    const u32 candidate_count = (u32)(range.end - range.first);
    if (candidate_count == 0u) {
        return;
    }

    const u32 added = unsigned_collision_hit_scan(&hits->instances[hits->count], &actors->actors->instances[range.first], candidate_count, attacker, (u32)target_mask, hitbox,
                                                        (u32)__builtin_offsetof(UActor, collision.hurtbox), (u32)__builtin_offsetof(UActor, collision.hurtbox_channel));
    hits->count = (u16)(hits->count + added);
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

        if (!unsigned_collision_actor_should_resolve(attacker, tlss, i)) {
            continue;
        }

        hit_detection_scan_attacker(hits, collisions, actors, attacker);
    }
}
