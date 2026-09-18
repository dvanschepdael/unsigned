/**
 * @file audio_types.h
 * @brief Compact identifiers shared by the audio API and platform backend.
 *
 * `USoundCommand` is deliberately one byte because the Neo Geo transport ultimately uses the
 * 8-bit sound command latch. Generic gameplay must still respect the platform-reserved range.
 */

#ifndef UNSIGNED_AUDIO_TYPES_H
#define UNSIGNED_AUDIO_TYPES_H

#include "core/types.h"

typedef u8 USoundCommand;
typedef u8 UAudioEventId;
typedef u8 UAudioParameterId;
typedef u8 UAudioMusicStateId;

typedef struct UAudioManager UAudioManager;

#define U_AUDIO_COMMAND_NONE ((USoundCommand)0u)
#define U_AUDIO_COMMAND_FIRST_GAME ((USoundCommand)4u)
#define U_AUDIO_NO_PARAMETER ((UAudioParameterId)0xffu)
#define U_AUDIO_MUSIC_STATE_NONE ((UAudioMusicStateId)0xffu)

/** Return true when a command belongs to the game-defined command range. */
static inline bool unsigned_audio_command_is_game(USoundCommand command) {
    return command >= U_AUDIO_COMMAND_FIRST_GAME;
}

#endif
