#include "system/session.h"

#include "system/bios_state_internal.h"
#include "system/session_internal.h"

#include <ngdevkit/bios-ram.h>

static bool neo_geo_session_ended = true;

static u8 *neo_geo_player_mode_byte(UNeoGeoPlayer player) {
    u8 *const modes[UNSIGNED_NEO_GEO_PLAYER_CAPACITY] = {
        &bios_player_mod1,
        &bios_player_mod2,
        &bios_player_mod3,
        &bios_player_mod4,
    };
    return modes[player];
}

static bool neo_geo_player_mode_is_participating(u8 mode) {
    return mode == U_NEO_GEO_PLAYER_MODE_PLAYING || mode == U_NEO_GEO_PLAYER_MODE_CONTINUE;
}

u8 unsigned_system_session_player_mask(void) {
    u8 mask = 0u;

    for (u8 player = 0u; player < UNSIGNED_NEO_GEO_PLAYER_CAPACITY; ++player) {
        const u8 *mode = neo_geo_player_mode_byte((UNeoGeoPlayer)player);
        if (*mode == U_NEO_GEO_PLAYER_MODE_PLAYING) {
            mask |= (u8)(1u << player);
        }
    }

    return mask;
}

bool unsigned_system_session_player_is_playing(UNeoGeoPlayer player) {
    const u8 *mode = neo_geo_player_mode_byte(player);
    return *mode == U_NEO_GEO_PLAYER_MODE_PLAYING;
}

bool unsigned_system_session_has_players(void) {
    for (u8 player = 0u; player < UNSIGNED_NEO_GEO_PLAYER_CAPACITY; ++player) {
        const u8 *mode = neo_geo_player_mode_byte((UNeoGeoPlayer)player);
        if (neo_geo_player_mode_is_participating(*mode)) {
            return true;
        }
    }
    return false;
}

void unsigned_system_session_activate_players(u8 player_mask) {
    for (u8 player = 0u; player < UNSIGNED_NEO_GEO_PLAYER_CAPACITY; ++player) {
        if ((player_mask & (u8)(1u << player)) == 0u) {
            continue;
        }

        *neo_geo_player_mode_byte((UNeoGeoPlayer)player) = U_NEO_GEO_PLAYER_MODE_PLAYING;
    }
}

void unsigned_system_session_begin(void) {
    neo_geo_session_ended = false;
}

bool unsigned_system_session_has_ended(void) {
    return neo_geo_session_ended;
}

void unsigned_system_session_reset(void) {
    neo_geo_session_ended = true;
}

UNeoGeoSystem unsigned_system_session_type(void) {
    return bios_mvs_flag == U_NEO_GEO_SYSTEM_MVS ? U_NEO_GEO_SYSTEM_MVS : U_NEO_GEO_SYSTEM_AES;
}

UNeoGeoPlayerMode unsigned_system_session_player_mode(UNeoGeoPlayer player) {
    const u8 *mode = neo_geo_player_mode_byte(player);
    if (*mode > U_NEO_GEO_PLAYER_MODE_GAME_OVER) {
        return U_NEO_GEO_PLAYER_MODE_NEVER_PLAYED;
    }
    return (UNeoGeoPlayerMode)*mode;
}

bool unsigned_system_session_has_playing_players(void) {
    return unsigned_system_session_player_mask() != 0u;
}

bool unsigned_system_session_game_started(void) {
    return bios_user_mode == U_NEO_GEO_MODE_GAME && unsigned_system_session_has_playing_players();
}

bool unsigned_system_session_continue(UNeoGeoPlayer player) {
    u8 *mode = neo_geo_player_mode_byte(player);
    if (*mode != U_NEO_GEO_PLAYER_MODE_PLAYING) {
        return false;
    }

    *mode = U_NEO_GEO_PLAYER_MODE_CONTINUE;
    return true;
}

bool unsigned_system_session_player_game_over(UNeoGeoPlayer player) {
    u8 *mode = neo_geo_player_mode_byte(player);
    if (!neo_geo_player_mode_is_participating(*mode)) {
        return false;
    }

    *mode = U_NEO_GEO_PLAYER_MODE_GAME_OVER;
    return true;
}

void unsigned_system_session_request_game_over(void) {
    for (u8 player = 0u; player < UNSIGNED_NEO_GEO_PLAYER_CAPACITY; ++player) {
        (void)unsigned_system_session_player_game_over((UNeoGeoPlayer)player);
    }
}

void unsigned_system_session_end(void) {
    unsigned_system_session_request_game_over();
    neo_geo_session_ended = true;
}
