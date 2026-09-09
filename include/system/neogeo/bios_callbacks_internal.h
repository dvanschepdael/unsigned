/**
 * @file bios_callbacks_internal.h
 * @brief Internal binding between ngdevkit BIOS callbacks and the engine runtime/session state.
 *
 * PLAYER_START and COIN_SOUND enter through this boundary; game code must not synthesize equivalent
 * lifecycle events from generic controller input.
 */

#ifndef UNSIGNED_SYSTEM_NEO_GEO_BIOS_CALLBACKS_INTERNAL_H
#define UNSIGNED_SYSTEM_NEO_GEO_BIOS_CALLBACKS_INTERNAL_H

#include "system/neogeo/bios_state_internal.h"
#include "system/neogeo/runtime.h"

void neo_geo_bios_callbacks_init(USoundCommand coin_sound_command, UNeoGeoBiosRequest request);
void neo_geo_bios_begin_runtime(const UNeoGeoRuntimeDefinition *definition);
void neo_geo_bios_enable_start_requests(void);
void neo_geo_bios_end_runtime(void);
void neo_geo_bios_process_start(void);
void neo_geo_bios_play_coin_sound(void);

#endif
