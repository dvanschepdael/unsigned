/**
 * @file gameplay.h
 * @brief Shared gameplay runtime base types.
 *
 * The callbacks operate on caller-owned `args`. They may release the currently executing pool
 * instance, so pool code revalidates generation/object identity after callbacks before continuing.
 */

#ifndef UNSIGNED_GAMEPLAY_H
#define UNSIGNED_GAMEPLAY_H

#include "core/types.h"
#include "gameplay/tag.h"

typedef struct UGameplayObject {
    /** Duration in engine ticks for timed pool users; zero may mean indefinite depending on the wrapper type. */
    u16 duration;
    /** Logical key used for lookup/identity in gameplay pools. */
    UGameplayTag tag;
    /** Called once after the runtime slot is fully bound. */
    UCallbackFunc activate;
    /** Called once per gameplay-pool tick while the instance remains current. */
    UCallbackFunc tick;
    /** Called on normal release after the slot has been detached from the pool. */
    UCallbackFunc end;
} UGameplayObject;

#endif
