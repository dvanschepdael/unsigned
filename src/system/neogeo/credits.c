#include "system/neogeo/credits.h"

#include "system/neogeo/credits_internal.h"
#include "system/neogeo/session_internal.h"

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

void neo_geo_credits_init(void) {
    bios_credit_dec1 = 1u;
    bios_credit_dec2 = 1u;
    bios_credit_dec3 = 1u;
    bios_credit_dec4 = 1u;
}

void neo_geo_credits_prepare_start(u8 accepted_players) {
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

bool unsigned_neo_geo_credits_are_shared(void) {
    return unsigned_neo_geo_system() == U_NEO_GEO_SYSTEM_MVS && neo_geo_credit_routing() == U_NEO_GEO_CREDITS_SHARED;
}

u8 unsigned_neo_geo_player_credits(UNeoGeoPlayer player) {
    if (unsigned_neo_geo_system() != U_NEO_GEO_SYSTEM_MVS || player >= UNSIGNED_NEO_GEO_STANDARD_CREDIT_PLAYER_CAPACITY) {
        return 0u;
    }

    if (neo_geo_credit_routing() == U_NEO_GEO_CREDITS_SHARED || player == U_NEO_GEO_PLAYER_1) {
        return bram_p1_credits_bcd;
    }
    return bram_p2_credits_bcd;
}

UNeoGeoPrompt unsigned_neo_geo_attract_prompt(void) {
    if (unsigned_neo_geo_system() == U_NEO_GEO_SYSTEM_AES || unsigned_neo_geo_player_credits(U_NEO_GEO_PLAYER_1) != 0u || unsigned_neo_geo_player_credits(U_NEO_GEO_PLAYER_2) != 0u) {
        return U_NEO_GEO_PROMPT_PRESS_START;
    }
    return U_NEO_GEO_PROMPT_INSERT_COIN;
}

UNeoGeoPrompt unsigned_neo_geo_player_prompt(UNeoGeoPlayer player) {
    if (player >= UNSIGNED_NEO_GEO_PLAYER_CAPACITY || neo_geo_session_player_is_playing(player)) {
        return U_NEO_GEO_PROMPT_NONE;
    }
    if (unsigned_neo_geo_system() == U_NEO_GEO_SYSTEM_AES) {
        return U_NEO_GEO_PROMPT_PRESS_START;
    }
    if (player >= UNSIGNED_NEO_GEO_STANDARD_CREDIT_PLAYER_CAPACITY) {
        return U_NEO_GEO_PROMPT_NONE;
    }
    return unsigned_neo_geo_player_credits(player) != 0u ? U_NEO_GEO_PROMPT_PRESS_START : U_NEO_GEO_PROMPT_INSERT_COIN;
}
