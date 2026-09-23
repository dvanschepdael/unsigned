/**
 * @file audio_music.h
 * @brief Small state-machine description for music command transitions.
 *
 * A music definition does not contain audio data; it maps application states to sound-driver
 * commands and optional transition delays. Definitions remain caller-owned while playing.
 */

#ifndef UNSIGNED_AUDIO_MUSIC_H
#define UNSIGNED_AUDIO_MUSIC_H

#include "audio/audio_types.h"

typedef struct UAudioMusicState {
    USoundCommand command;
    u16 transition_ticks;
} UAudioMusicState;

typedef struct UAudioMusic {
    const UAudioMusicState *states;
    USoundCommand stop_command;
    UAudioMusicStateId initial_state;
} UAudioMusic;

typedef struct UAudioMusicPlayer {
    const UAudioMusic *definition;
    u16 transition_tick;
    UAudioMusicStateId current_state;
    UAudioMusicStateId requested_state;
} UAudioMusicPlayer;

/**
 * @brief Start a caller-owned music state machine at its authored initial state.
 * @pre `audio` and `music` are valid; `music->states` is non-empty.
 * @pre `initial_state` addresses an authored state, every state command is a game command, and `stop_command` is a game command.
 */
void unsigned_audio_music_play(UAudioManager *audio, const UAudioMusic *music);

/**
 * @brief Request a music state; transition timing remains controlled by the current state's boundary.
 * @pre Music is currently installed and `state` addresses an authored state in `audio->music.definition->states`.
 */
void unsigned_audio_music_set_state(UAudioManager *audio, UAudioMusicStateId state);

/** Request the authored stop command at the next audio manager tick.
 * @pre Music is currently installed.
 */
void unsigned_audio_music_stop(UAudioManager *audio);

#endif
