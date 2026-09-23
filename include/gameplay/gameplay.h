/**
 * @file gameplay.h
 * @brief Shared gameplay runtime base types.
 *
 * Callbacks operate on caller-owned `args`. Slot ownership remains with the pool: activation,
 * tick and completion callbacks must not release or reuse the slot currently executing them.
 */

#ifndef UNSIGNED_GAMEPLAY_H
#define UNSIGNED_GAMEPLAY_H

#include "core/types.h"
#include "gameplay/tag.h"

typedef bool (*UGameplayCompleteFunction)(void *context);

typedef struct UGameplayObject {
    /** Duration in engine ticks for timed pool users; zero may mean indefinite depending on the wrapper type. */
    u16 duration;
    /** Logical key used for lookup/identity in gameplay pools. */
    UGameplayTag tag;
    /** Called once after the runtime slot is fully bound. */
    UCallbackFunc activate;
    /** Called once per gameplay-pool tick while the instance remains active. */
    UCallbackFunc tick;
    /** Optional completion predicate evaluated after tick; true requests normal pool release. */
    UGameplayCompleteFunction complete;
    /** Called on normal release after the slot has been detached from the pool. */
    UCallbackFunc end;
} UGameplayObject;

#endif
