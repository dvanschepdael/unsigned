/**
 * @file session.h
 * @brief Public Neo Geo system and player-session state.
 */

#ifndef UNSIGNED_SYSTEM_NEO_GEO_SESSION_H
#define UNSIGNED_SYSTEM_NEO_GEO_SESSION_H

#include "core/types.h"
#include "system/neogeo/config.h"

typedef enum UNeoGeoSystem {
    U_NEO_GEO_SYSTEM_AES = 0x00,
    U_NEO_GEO_SYSTEM_MVS = 0x80,
} UNeoGeoSystem;

typedef enum UNeoGeoPlayerMode {
    U_NEO_GEO_PLAYER_MODE_NEVER_PLAYED = 0,
    U_NEO_GEO_PLAYER_MODE_PLAYING = 1,
    U_NEO_GEO_PLAYER_MODE_CONTINUE = 2,
    U_NEO_GEO_PLAYER_MODE_GAME_OVER = 3,
} UNeoGeoPlayerMode;

typedef enum UNeoGeoPlayer {
    U_NEO_GEO_PLAYER_1 = 0,
    U_NEO_GEO_PLAYER_2 = 1,
    U_NEO_GEO_PLAYER_3 = 2,
    U_NEO_GEO_PLAYER_4 = 3,
} UNeoGeoPlayer;

/** Current hardware mode reported by the BIOS. */
UNeoGeoSystem unsigned_neo_geo_system(void);

/** Current BIOS player mode. Invalid player indices are reported as NEVER_PLAYED. */
UNeoGeoPlayerMode unsigned_neo_geo_player_mode(UNeoGeoPlayer player);

/** True when at least one player slot is currently PLAYING. */
bool unsigned_neo_geo_has_playing_players(void);

/** True while the BIOS is in GAME mode and at least one player is PLAYING. */
bool unsigned_neo_geo_game_started(void);

/** Move one PLAYING player to CONTINUE. Invalid transitions are rejected. */
bool unsigned_neo_geo_player_begin_continue(UNeoGeoPlayer player);

/** Move one PLAYING or CONTINUE player to GAME_OVER. Invalid transitions are rejected. */
bool unsigned_neo_geo_player_game_over(UNeoGeoPlayer player);

/** Move every PLAYING/CONTINUE player to GAME_OVER. */
void unsigned_neo_geo_request_game_over(void);

/** Close the current GAME/GAME_OVER session and return lifecycle control to the BIOS. */
void unsigned_neo_geo_end_session(void);

#endif
