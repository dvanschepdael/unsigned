/**
 * @file level_collision.h
 * @brief Public diagnostics for the level collision pipeline.
 */

#ifndef UNSIGNED_LEVEL_COLLISION_H
#define UNSIGNED_LEVEL_COLLISION_H

#include "core/types.h"

struct ULevel;

/** Return true when all static and dynamic collision boxes registered successfully this frame. */
bool unsigned_level_collision_registration_complete(const struct ULevel *level);

/** Return true when a fixed collision-layer registration buffer overflowed. */
bool unsigned_level_collision_registration_overflowed(const struct ULevel *level);

/** Return true when attack-hit results were truncated to UNSIGNED_COLLISION_HIT_CAPACITY. */
bool unsigned_level_collision_hits_overflowed(const struct ULevel *level);

#endif
