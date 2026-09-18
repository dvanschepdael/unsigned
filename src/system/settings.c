/**
 * @file settings.c
 * @brief Reads and writes the MVS cabinet settings modeled by the engine.
 */

#include "system/settings.h"

#include "system/session.h"

#include <ngdevkit/bios-backup-ram.h>
#include <ngdevkit/registers.h>

static void neo_geo_write_bios_bram_setting(u8 *setting, u8 value) {
    *REG_SRAMUNLOCK = 0u;
    __asm__ volatile("" ::: "memory");
    *(volatile u8 *)setting = value;
    __asm__ volatile("" ::: "memory");
    *REG_SRAMLOCK = 0u;
}

static u8 neo_geo_bcd_to_u8(u8 value) {
    const u8 tens = (u8)((value >> 4) & 0x0fu);
    const u8 ones = (u8)(value & 0x0fu);

    if (tens > 9u || ones > 9u) {
        return 0u;
    }
    return (u8)(tens * 10u + ones);
}

static u8 neo_geo_u8_to_bcd(u8 value) {
    if (value > 99u) {
        value = 99u;
    }
    return (u8)(((value / 10u) << 4) | (value % 10u));
}

bool unsigned_neo_geo_game_start_compulsion_enabled(void) {
    return unsigned_neo_geo_system() == U_NEO_GEO_SYSTEM_MVS && bram_settings_game_start_compulsion == 0u;
}

void unsigned_neo_geo_set_game_start_compulsion(bool enabled) {
    if (unsigned_neo_geo_system() != U_NEO_GEO_SYSTEM_MVS) {
        return;
    }
    neo_geo_write_bios_bram_setting(&bram_settings_game_start_compulsion, enabled ? 0u : 1u);
}

u8 unsigned_neo_geo_game_start_compulsion_seconds(void) {
    if (unsigned_neo_geo_system() != U_NEO_GEO_SYSTEM_MVS) {
        return 0u;
    }
    return neo_geo_bcd_to_u8(bram_settings_compulsion_secs_bcd);
}

void unsigned_neo_geo_set_game_start_compulsion_seconds(u8 seconds) {
    if (unsigned_neo_geo_system() != U_NEO_GEO_SYSTEM_MVS) {
        return;
    }
    neo_geo_write_bios_bram_setting(&bram_settings_compulsion_secs_bcd, neo_geo_u8_to_bcd(seconds));
}

bool unsigned_neo_geo_demo_sound_enabled(void) {
    if (unsigned_neo_geo_system() != U_NEO_GEO_SYSTEM_MVS) {
        return true;
    }
    return bram_settings_demo_sound == 0u;
}
