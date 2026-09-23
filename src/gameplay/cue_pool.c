/**
 * @file cue_pool.c
 * @brief Implements runtime pool for transient gameplay cues.
 */

#include "gameplay/cue_pool.h"

void unsigned_gameplay_cue_pool_init(UCuePool *cues) {
    *cues = (UCuePool){0};
    cues->pool.capacity = UNSIGNED_GAMEPLAY_MAX_CUE;
    cues->pool.instances = cues->instances;
    unsigned_pool_init(&cues->pool);
}

void unsigned_gameplay_cue_pool_clear(UCuePool *cues) {
    unsigned_gameplay_pool_clear(&cues->pool, NULL, NULL);
}

UCuePoolInstance *unsigned_gameplay_cue_pool_reserve(UCuePool *cues, const UGameplayCue *cue, void *args) {
    UCuePoolInstance *instance = unsigned_gameplay_pool_reserve(&cues->pool, &cue->base, args);
    unsigned_gameplay_pool_activate(instance);
    return instance;
}

void unsigned_gameplay_cue_pool_release(UCuePool *cues, UCuePoolInstance *instance) {
    unsigned_gameplay_pool_release(&cues->pool, instance);
}

void unsigned_gameplay_cue_pool_tick(UCuePool *cues) {
    unsigned_gameplay_pool_tick(&cues->pool, NULL, NULL);
}
