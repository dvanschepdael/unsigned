/**
 * @file hit_detection.h
 * @brief Produces gameplay attack hits from registered actor hitboxes and hurtboxes.
 */

#ifndef UNSIGNED_COLLISION_HIT_DETECTION_H
#define UNSIGNED_COLLISION_HIT_DETECTION_H

#include "collision/config.h"
#include "core/types.h"

struct UActor;
struct UCollisionActorIndex;
struct UCollisionManager;
struct UTLSS;

typedef struct UCollisionHit {
    struct UActor *attacker;
    struct UActor *target;
} UCollisionHit;

/** Fixed frame-local hit list sized by the game composition. */
typedef struct UCollisionHitContainer {
    u16 count;
    UCollisionHit instances[UNSIGNED_COLLISION_HIT_CAPACITY];
} UCollisionHitContainer;

/**
 * Detect all attack hits selected by TLSS for this frame.
 *
 * @pre All arguments are valid and `actors` was built for the current collision frame.
 * @pre The maximum number of simultaneous attacker/target pairs for the game fits
 *      `UNSIGNED_COLLISION_HIT_CAPACITY`.
 */
void unsigned_collision_hit_detect(UCollisionHitContainer *hits, struct UCollisionManager *collisions, const struct UCollisionActorIndex *actors, const struct UTLSS *tlss);

#endif
