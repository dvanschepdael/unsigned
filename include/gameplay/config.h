/**
 * @file config.h
 * @brief Compile-time gameplay pool and tag limits.
 */

#ifndef UNSIGNED_GAMEPLAY_CONFIG_H
#define UNSIGNED_GAMEPLAY_CONFIG_H

#ifndef UNSIGNED_GAMEPLAY_MAX_TAG
#define UNSIGNED_GAMEPLAY_MAX_TAG 16
#endif

#ifndef UNSIGNED_GAMEPLAY_MAX_ABILITY
#define UNSIGNED_GAMEPLAY_MAX_ABILITY 16
#endif

#ifndef UNSIGNED_GAMEPLAY_MAX_EFFECT
#define UNSIGNED_GAMEPLAY_MAX_EFFECT 16
#endif

#ifndef UNSIGNED_GAMEPLAY_MAX_CUE
#define UNSIGNED_GAMEPLAY_MAX_CUE 16
#endif

/* Authoring contract: tag/ability/effect/cue limits are non-zero and fit their u8 counters. */

#endif
