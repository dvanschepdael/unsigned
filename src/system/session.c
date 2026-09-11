#include "system/session.h"

#include "system/bios_state_internal.h"
#include "system/session_internal.h"

#include <ngdevkit/bios-ram.h>

static bool neo_geo_session_ended = true;

static u8 *neo_geo_player_mode_byte(UNeoGeoPlayer player) {
    switch (player) {
        case U_NEO_GEO_PLAYER_1:
            return &bios_player_mod1;
        case U_NEO_GEO_PLAYER_2:
            return &bios_player_mod2;
        case U_NEO_GEO_PLAYER_3:
            return &bios_player_mod3;
        case U_NEO_GEO_PLAYER_4:
            return &bios_player_mod4;
        default:
            return NULL;
    }
}

static bool neo_geo_player_mode_is_participating(u8 mode) {
    return mode == U_NEO_GEO_PLAYER_MODE_PLAYING || mode == U_NEO_GEO_PLAYER_MODE_CONTINUE;
}

u8 neo_geo_session_playing_player_mask(void) {
    u8 mask = 0u;

    for (u8 player = 0u; player < UNSIGNED_NEO_GEO_PLAYER_CAPACITY; ++player) {
        const u8 *mode = neo_geo_player_mode_byte((UNeoGeoPlayer)player);
        if (mode != NULL && *mode == U_NEO_GEO_PLAYER_MODE_PLAYING) {
            mask |= (u8)(1u << player);
        }
    }

    return mask;
}

bool neo_geo_session_player_is_playing(UNeoGeoPlayer player) {
    const u8 *mode = neo_geo_player_mode_byte(player);
    return mode != NULL && *mode == U_NEO_GEO_PLAYER_MODE_PLAYING;
}

bool neo_geo_session_has_participating_players(void) {
    for (u8 player = 0u; player < UNSIGNED_NEO_GEO_PLAYER_CAPACITY; ++player) {
        const u8 *mode = neo_geo_player_mode_byte((UNeoGeoPlayer)player);
        if (mode != NULL && neo_geo_player_mode_is_participating(*mode)) {
            return true;
        }
    }
    return false;
}

void neo_geo_session_activate_players(u8 player_mask) {
    for (u8 player = 0u; player < UNSIGNED_NEO_GEO_PLAYER_CAPACITY; ++player) {
        if ((player_mask & (u8)(1u << player)) == 0u) {
            continue;
        }

        u8 *mode = neo_geo_player_mode_byte((UNeoGeoPlayer)player);
        if (mode != NULL) {
            *mode = U_NEO_GEO_PLAYER_MODE_PLAYING;
        }
    }
}

void neo_geo_session_begin(void) {
    neo_geo_session_ended = false;
}

bool neo_geo_session_has_ended(void) {
    return neo_geo_session_ended;
}

void neo_geo_session_reset(void) {
    neo_geo_session_ended = true;
}

UNeoGeoSystem unsigned_neo_geo_system(void) {
    return bios_mvs_flag == U_NEO_GEO_SYSTEM_MVS ? U_NEO_GEO_SYSTEM_MVS : U_NEO_GEO_SYSTEM_AES;
}

UNeoGeoPlayerMode unsigned_neo_geo_player_mode(UNeoGeoPlayer player) {
    const u8 *mode = neo_geo_player_mode_byte(player);
    if (mode == NULL || *mode > U_NEO_GEO_PLAYER_MODE_GAME_OVER) {
        return U_NEO_GEO_PLAYER_MODE_NEVER_PLAYED;
    }
    return (UNeoGeoPlayerMode)*mode;
}

bool unsigned_neo_geo_has_playing_players(void) {
    return neo_geo_session_playing_player_mask() != 0u;
}

bool unsigned_neo_geo_game_started(void) {
    return bios_user_mode == U_NEO_GEO_MODE_GAME && unsigned_neo_geo_has_playing_players();
}

bool unsigned_neo_geo_player_begin_continue(UNeoGeoPlayer player) {
    u8 *mode = neo_geo_player_mode_byte(player);
    if (mode == NULL || *mode != U_NEO_GEO_PLAYER_MODE_PLAYING) {
        return false;
    }

    *mode = U_NEO_GEO_PLAYER_MODE_CONTINUE;
    return true;
}

bool unsigned_neo_geo_player_game_over(UNeoGeoPlayer player) {
    u8 *mode = neo_geo_player_mode_byte(player);
    if (mode == NULL || !neo_geo_player_mode_is_participating(*mode)) {
        return false;
    }

    *mode = U_NEO_GEO_PLAYER_MODE_GAME_OVER;
    return true;
}

void unsigned_neo_geo_request_game_over(void) {
    for (u8 player = 0u; player < UNSIGNED_NEO_GEO_PLAYER_CAPACITY; ++player) {
        (void)unsigned_neo_geo_player_game_over((UNeoGeoPlayer)player);
    }
}

void unsigned_neo_geo_end_session(void) {
    unsigned_neo_geo_request_game_over();
    neo_geo_session_ended = true;
}
