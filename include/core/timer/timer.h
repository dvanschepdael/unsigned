/**
 * @file timer.h
 * @brief Timer definition and callback contract.
 */

#ifndef UNSIGNED_CORE_TIMER_H
#define UNSIGNED_CORE_TIMER_H

#include "core/types.h"

typedef struct UTimer {
    u16 duration_frames;
    UCallbackFunc callback;
} UTimer;

#endif
