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
    u8 state_count;
} UAudioMusic;

typedef struct UAudioMusicPlayer {
    const UAudioMusic *definition;
    u16 transition_tick;
    UAudioMusicStateId current_state;
    UAudioMusicStateId requested_state;
    bool stop_requested;
} UAudioMusicPlayer;

/** Music definitions are caller-owned and must remain valid while playing. */
bool unsigned_audio_music_play(UAudioManager *audio, const UAudioMusic *music);
bool unsigned_audio_music_set_state(UAudioManager *audio, UAudioMusicStateId state);
bool unsigned_audio_music_stop(UAudioManager *audio);

#endif
