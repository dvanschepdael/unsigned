/**
 * @file config.h
 * @brief Compile-time background layer/rendering limits.
 */

#ifndef UNSIGNED_LEVEL_BACKGROUND_CONFIG_H
#define UNSIGNED_LEVEL_BACKGROUND_CONFIG_H

#ifndef UNSIGNED_BACKGROUND_MAX_LAYERS
#define UNSIGNED_BACKGROUND_MAX_LAYERS 3
#endif

#ifndef UNSIGNED_BACKGROUND_RENDER_MAX_COLUMNS
#define UNSIGNED_BACKGROUND_RENDER_MAX_COLUMNS 21
#endif

/* Authoring contract: both limits are non-zero and fit the u8 counters used by the renderer. */

#endif
