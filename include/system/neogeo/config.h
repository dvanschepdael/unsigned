/**
 * @file config.h
 * @brief Compile-time limits and capability boundaries for the Neo Geo platform adapter.
 */

#ifndef UNSIGNED_SYSTEM_NEO_GEO_CONFIG_H
#define UNSIGNED_SYSTEM_NEO_GEO_CONFIG_H

#include "core/types.h"

/** BIOS/ngdevkit expose controller and PLAYER_MOD state for up to four players. */
#define UNSIGNED_NEO_GEO_PLAYER_CAPACITY 4u

/** Standard MVS backup-RAM credit counters directly modeled by this engine (P1 and P2). */
#define UNSIGNED_NEO_GEO_STANDARD_CREDIT_PLAYER_CAPACITY 2u

/** 68k-side commands waiting for the next safe frame-boundary dispatch slot. */
#ifndef UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY
#define UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY 8u
#endif

#if UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY < 1 || UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY > 255
#error "UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY must fit the non-zero u8 queue count"
#endif

typedef enum UNeoGeoPlayerCount {
    U_NEO_GEO_PLAYER_COUNT_1 = 1,
    U_NEO_GEO_PLAYER_COUNT_2 = 2,
    U_NEO_GEO_PLAYER_COUNT_4 = 4,
} UNeoGeoPlayerCount;

#endif
