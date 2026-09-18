/**
 * @file cue_pool.c
 * @brief Implements runtime pool for transient gameplay cues.
 */

#include "gameplay/cue_pool.h"

void unsigned_gameplay_cue_pool_init(UCuePool *cues) {
    if (cues == NULL) {
        return;
    }

    *cues = (UCuePool){ 0 };
    cues->pool.capacity = UNSIGNED_GAMEPLAY_MAX_CUE;
    cues->pool.instances = cues->instances;
    unsigned_gameplay_pool_init(&cues->pool);
}

void unsigned_gameplay_cue_pool_clear(UCuePool *cues) {
    if (cues == NULL) {
        return;
    }

    for (u8 i = 0u; i < cues->pool.capacity; ++i) {
        if (cues->pool.instances[i].active) {
            unsigned_gameplay_cue_pool_release(cues, &cues->pool.instances[i]);
        }
    }
}

UCuePoolInstance *unsigned_gameplay_cue_pool_reserve(UCuePool *cues, const UGameplayCue *cue, void *args) {
    if (cues == NULL || cue == NULL) {
        return NULL;
    }

    UCuePoolInstance *instance = unsigned_gameplay_pool_reserve(&cues->pool, &cue->base, args);
    if (instance == NULL) {
        return NULL;
    }

    if (!unsigned_gameplay_pool_activate(&cues->pool, instance)) {
        return NULL;
    }

    return instance;
}

void unsigned_gameplay_cue_pool_release(UCuePool *cues, UCuePoolInstance *instance) {
    if (cues != NULL) {
        unsigned_gameplay_pool_release(&cues->pool, instance);
    }
}

void unsigned_gameplay_cue_pool_tick(UCuePool *cues) {
    if (cues != NULL) {
        unsigned_gameplay_pool_tick(&cues->pool, NULL, NULL);
    }
}
