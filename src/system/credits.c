#include "system/credits.h"

#include "system/credits_internal.h"
#include "system/session_internal.h"

#include <ngdevkit/bios-backup-ram.h>
#include <ngdevkit/bios-ram.h>

typedef enum UNeoGeoCreditRouting {
    U_NEO_GEO_CREDITS_SHARED = 0,
    U_NEO_GEO_CREDITS_SPLIT,
} UNeoGeoCreditRouting;

static UNeoGeoCreditRouting neo_geo_credit_routing(void) {
    switch (bios_country_code) {
    case BIOS_COUNTRY_US:
    case BIOS_COUNTRY_EU:
        return U_NEO_GEO_CREDITS_SPLIT;
    case BIOS_COUNTRY_JP:
    default:
        return U_NEO_GEO_CREDITS_SHARED;
    }
}

void unsigned_system_credits_init(void) {
    bios_credit_dec1 = 1u;
    bios_credit_dec2 = 1u;
    bios_credit_dec3 = 1u;
    bios_credit_dec4 = 1u;
}

void unsigned_system_credits_mark_start(u8 accepted_players) {
    if ((accepted_players & (1u << U_NEO_GEO_PLAYER_1)) != 0u) {
        bios_credit_dec1 = 1u;
    }
    if ((accepted_players & (1u << U_NEO_GEO_PLAYER_2)) != 0u) {
        bios_credit_dec2 = 1u;
    }
    if ((accepted_players & (1u << U_NEO_GEO_PLAYER_3)) != 0u) {
        bios_credit_dec3 = 1u;
    }
    if ((accepted_players & (1u << U_NEO_GEO_PLAYER_4)) != 0u) {
        bios_credit_dec4 = 1u;
    }
}

bool unsigned_system_credits_shared(void) {
    return unsigned_system_session_type() == U_NEO_GEO_SYSTEM_MVS && neo_geo_credit_routing() == U_NEO_GEO_CREDITS_SHARED;
}

u8 unsigned_system_credits_player(UNeoGeoPlayer player) {
    if (unsigned_system_session_type() != U_NEO_GEO_SYSTEM_MVS || player >= UNSIGNED_NEO_GEO_STANDARD_CREDIT_PLAYER_CAPACITY) {
        return 0u;
    }

    if (neo_geo_credit_routing() == U_NEO_GEO_CREDITS_SHARED || player == U_NEO_GEO_PLAYER_1) {
        return bram_p1_credits_bcd;
    }
    return bram_p2_credits_bcd;
}

UNeoGeoPrompt unsigned_system_credits_attract_prompt(void) {
    if (unsigned_system_session_type() == U_NEO_GEO_SYSTEM_AES || unsigned_system_credits_player(U_NEO_GEO_PLAYER_1) != 0u || unsigned_system_credits_player(U_NEO_GEO_PLAYER_2) != 0u) {
        return U_NEO_GEO_PROMPT_PRESS_START;
    }
    return U_NEO_GEO_PROMPT_INSERT_COIN;
}

UNeoGeoPrompt unsigned_system_credits_player_prompt(UNeoGeoPlayer player) {
    if (unsigned_system_session_player_is_playing(player)) {
        return U_NEO_GEO_PROMPT_NONE;
    }
    if (unsigned_system_session_type() == U_NEO_GEO_SYSTEM_AES) {
        return U_NEO_GEO_PROMPT_PRESS_START;
    }
    if (player >= UNSIGNED_NEO_GEO_STANDARD_CREDIT_PLAYER_CAPACITY) {
        return U_NEO_GEO_PROMPT_NONE;
    }
    return unsigned_system_credits_player(player) != 0u ? U_NEO_GEO_PROMPT_PRESS_START : U_NEO_GEO_PROMPT_INSERT_COIN;
}
