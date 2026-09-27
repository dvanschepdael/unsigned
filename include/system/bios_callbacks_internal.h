/**
 * @file bios_callbacks_internal.h
 * @brief Internal binding between ngdevkit BIOS callbacks and the engine runtime/session state.
 *
 * PLAYER_START and COIN_SOUND enter through this boundary; game code must not synthesize equivalent
 * lifecycle events from generic controller input.
 */

#ifndef UNSIGNED_SYSTEM_BIOS_CALLBACKS_INTERNAL_H
#define UNSIGNED_SYSTEM_BIOS_CALLBACKS_INTERNAL_H

#include "system/bios_state_internal.h"
#include "system/system_runtime.h"

/** @pre `coin_sound_command` is NONE or a game command in 4..127. */
void unsigned_system_bios_init(USoundCommand coin_sound_command, UNeoGeoBiosRequest request);
void unsigned_system_bios_begin(const UNeoGeoRuntimeDefinition *definition);
void unsigned_system_bios_enable_start(void);
void unsigned_system_bios_end(void);
void unsigned_system_bios_process_start(void);
void unsigned_system_bios_play_coin_sound(void);

/** ngdevkit/Neo Geo BIOS callback entry points. Their external symbol names are part of the platform ABI. */
void player_start(void);
void coin_sound(void);

#endif
