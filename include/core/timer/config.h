/**
 * @file config.h
 * @brief Compile-time timer-pool limits.
 */

#ifndef UNSIGNED_CORE_TIMER_CONFIG_H
#define UNSIGNED_CORE_TIMER_CONFIG_H

#ifndef UNSIGNED_GAME_MAX_TIMER
#define UNSIGNED_GAME_MAX_TIMER 16
#endif

#if UNSIGNED_GAME_MAX_TIMER < 1 || UNSIGNED_GAME_MAX_TIMER > 255
#error "UNSIGNED_GAME_MAX_TIMER must fit the non-zero u8 pool capacity"
#endif

#endif
