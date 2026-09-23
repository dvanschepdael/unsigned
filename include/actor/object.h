/**
 * @file object.h
 * @brief World object actor wrapper.
 */

#ifndef UNSIGNED_ACTOR_OBJECT_H
#define UNSIGNED_ACTOR_OBJECT_H

#include "actor/actor.h"

typedef struct UObject {
    UActor actor;
    bool static_collision;
} UObject;

#endif
