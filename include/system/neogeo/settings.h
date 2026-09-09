/**
 * @file settings.h
 * @brief Public access to the MVS cabinet settings modeled by the engine.
 */

#ifndef UNSIGNED_SYSTEM_NEO_GEO_SETTINGS_H
#define UNSIGNED_SYSTEM_NEO_GEO_SETTINGS_H

#include "core/types.h"

/** True when the MVS cabinet GAME START COMPULSION setting is enabled. AES returns false. */
bool unsigned_neo_geo_game_start_compulsion_enabled(void);

/**
 * Enable or disable GAME START COMPULSION in MVS BIOS backup RAM.
 * AES is left unchanged.
 */
void unsigned_neo_geo_set_game_start_compulsion(bool enabled);

/** Return the configured MVS compulsion delay in normal seconds (0..99). AES returns zero. */
u8 unsigned_neo_geo_game_start_compulsion_seconds(void);

/** Set the persistent MVS compulsion delay. Values above 99 are clamped to 99. */
void unsigned_neo_geo_set_game_start_compulsion_seconds(u8 seconds);

/** True when attract/demo audio is allowed by the MVS cabinet setting. AES always returns true. */
bool unsigned_neo_geo_demo_sound_enabled(void);

#endif
