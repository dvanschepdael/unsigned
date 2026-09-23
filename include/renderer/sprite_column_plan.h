/**
 * @file sprite_column_plan.h
 * @brief Bounded CPU-side scratch for sprite transforms that must be independent per hardware column.
 */

#ifndef UNSIGNED_RENDERER_SPRITE_COLUMN_PLAN_H
#define UNSIGNED_RENDERER_SPRITE_COLUMN_PLAN_H

#include "renderer/config.h"
#include "renderer/prepared_column.h"

/**
 * Shared frame scratch for actor sprites using per-column effects.
 *
 * @invariant `used <= UNSIGNED_RENDERER_SPRITE_EFFECT_COLUMN_CAPACITY`.
 * @invariant The configured capacity covers the maximum per-column actor workload of one frame.
 */
typedef struct USpriteColumnPlanBuffer {
    UPreparedColumn columns[UNSIGNED_RENDERER_SPRITE_EFFECT_COLUMN_CAPACITY];
    u16 used;
} USpriteColumnPlanBuffer;

static inline void unsigned_sprite_column_plan_reset(USpriteColumnPlanBuffer *buffer) {
    buffer->used = 0u;
}

#endif
