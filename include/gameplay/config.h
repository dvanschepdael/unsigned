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

#if UNSIGNED_GAMEPLAY_MAX_TAG < 1 || UNSIGNED_GAMEPLAY_MAX_TAG > 255
#error "UNSIGNED_GAMEPLAY_MAX_TAG must fit the non-zero u8 tag count"
#endif

#if UNSIGNED_GAMEPLAY_MAX_ABILITY < 1 || UNSIGNED_GAMEPLAY_MAX_ABILITY > 255
#error "UNSIGNED_GAMEPLAY_MAX_ABILITY must fit the non-zero u8 pool capacity"
#endif

#if UNSIGNED_GAMEPLAY_MAX_EFFECT < 1 || UNSIGNED_GAMEPLAY_MAX_EFFECT > 255
#error "UNSIGNED_GAMEPLAY_MAX_EFFECT must fit the non-zero u8 pool capacity"
#endif

#if UNSIGNED_GAMEPLAY_MAX_CUE < 1 || UNSIGNED_GAMEPLAY_MAX_CUE > 255
#error "UNSIGNED_GAMEPLAY_MAX_CUE must fit the non-zero u8 pool capacity"
#endif

#endif
