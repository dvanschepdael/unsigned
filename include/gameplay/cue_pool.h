/**
 * @file cue_pool.h
 * @brief Runtime pool for transient gameplay cues.
 */

#ifndef UNSIGNED_GAMEPLAY_CUE_POOL_H
#define UNSIGNED_GAMEPLAY_CUE_POOL_H

#include "gameplay/config.h"
#include "gameplay/cue.h"
#include "gameplay/gameplay_pool.h"

typedef UPoolInstance UCuePoolInstance;

typedef struct UCuePool {
    UPoolInstanceContainer pool;
    UCuePoolInstance instances[UNSIGNED_GAMEPLAY_MAX_CUE];
} UCuePool;

/**
 * @brief Initializes the gameplay cue pool to a valid empty runtime state.
 *
 * @param cues Gameplay cue runtime pool to initialize, reserve, release or tick.
 */
void unsigned_gameplay_cue_pool_init(UCuePool *cues);

/**
 * @brief Clears the gameplay cue pool state without freeing caller-owned storage.
 *
 * @param cues Gameplay cue runtime pool to initialize, reserve, release or tick.
 */
void unsigned_gameplay_cue_pool_clear(UCuePool *cues);

/**
 * @brief Reserves an inactive slot from the gameplay cue pool and binds it to the supplied runtime data.
 *
 * @param cues Gameplay cue runtime pool to initialize, reserve, release or tick.
 * @param cue Cue definition copied into a runtime cue slot.
 * @param args Opaque caller arguments associated with the runtime instance.
 * @return Active cue slot, or NULL when inputs are invalid or the fixed cue pool is full.
 */
UCuePoolInstance *unsigned_gameplay_cue_pool_reserve(UCuePool *cues, const UGameplayCue *cue, void *args);

/**
 * @brief Releases the selected gameplay cue pool runtime slot for reuse.
 *
 * @param cues Gameplay cue runtime pool to initialize, reserve, release or tick.
 * @param instance Runtime pool slot to inspect or release.
 */
void unsigned_gameplay_cue_pool_release(UCuePool *cues, UCuePoolInstance *instance);

/**
 * @brief Advances the gameplay cue pool by one scheduled engine frame.
 *
 * @param cues Gameplay cue runtime pool to initialize, reserve, release or tick.
 */
void unsigned_gameplay_cue_pool_tick(UCuePool *cues);

#endif
