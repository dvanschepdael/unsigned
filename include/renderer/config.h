/**
 * @file config.h
 * @brief Build-time renderer scratch policy.
 */

#ifndef UNSIGNED_RENDERER_CONFIG_H
#define UNSIGNED_RENDERER_CONFIG_H


/*
 * CPU-side scratch used to freeze sprite effects before VBlank.
 *
 * 96 columns matches the engine's current UNSIGNED_SPRITE_MAX_PER_SCANLINE configuration and
 * costs 576 bytes with the current three-word prepared-column layout. Composition must size this
 * capacity for the maximum number of per-column actor sprite columns prepared in one frame.
 */
#ifndef UNSIGNED_RENDERER_SPRITE_EFFECT_COLUMN_CAPACITY
#define UNSIGNED_RENDERER_SPRITE_EFFECT_COLUMN_CAPACITY 96u
#endif

/*
 * Maximum number of same-width actor sprites grouped into one SCB3/SCB4 driver stream.
 * The actor renderer uses a tiny stack array of pointers and transparently splits longer runs.
 */
#ifndef UNSIGNED_RENDERER_DRIVER_BATCH_CAPACITY
#define UNSIGNED_RENDERER_DRIVER_BATCH_CAPACITY 16u
#endif

#endif
