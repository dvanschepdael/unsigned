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

/** Fixed frame-local hit list. overflowed means valid results were truncated at capacity. */
typedef struct UCollisionHitContainer {
    u16 count;
    bool overflowed;
    UCollisionHit instances[UNSIGNED_COLLISION_HIT_CAPACITY];
} UCollisionHitContainer;

/**
 * Detect all attack hits for actors selected by TLSS this frame.
 *
 * Returns false only when required collision/index arguments are invalid. Filling the fixed
 * result buffer is not an operation failure: already detected hits remain valid, iteration
 * stops, and `hits->overflowed` becomes true so callers can diagnose/tune capacity.
 */
bool unsigned_collision_hit_detect(UCollisionHitContainer *hits, struct UCollisionManager *collisions, const struct UCollisionActorIndex *actors, const struct UTLSS *tlss);

#endif
