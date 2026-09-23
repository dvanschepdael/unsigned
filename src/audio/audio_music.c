#include "audio/audio_music_internal.h"

static void unsigned_audio_music_reset_player(UAudioMusicPlayer *player) {
    *player = (UAudioMusicPlayer){0};
    player->current_state = U_AUDIO_MUSIC_STATE_NONE;
    player->requested_state = U_AUDIO_MUSIC_STATE_NONE;
}

void unsigned_audio_music_play(UAudioManager *audio, const UAudioMusic *music) {
    unsigned_audio_music_reset_player(&audio->music);
    audio->music.definition = music;
    audio->music.requested_state = music->initial_state;
}

void unsigned_audio_music_set_state(UAudioManager *audio, UAudioMusicStateId state) {
    audio->music.requested_state = state;
}

void unsigned_audio_music_stop(UAudioManager *audio) {
    audio->music.requested_state = U_AUDIO_MUSIC_STATE_NONE;
}

USoundCommand unsigned_audio_music_tick(UAudioManager *audio) {
    bool transition_boundary = false;

    UAudioMusicPlayer *player = &audio->music;
    const UAudioMusic *music = player->definition;
    if (music == NULL) {
        return U_AUDIO_COMMAND_NONE;
    }

    if (player->requested_state == U_AUDIO_MUSIC_STATE_NONE) {
        const USoundCommand command = music->stop_command;
        unsigned_audio_music_reset_player(player);
        return command;
    }

    if (player->current_state == U_AUDIO_MUSIC_STATE_NONE) {
        const USoundCommand command = music->states[player->requested_state].command;
        player->current_state = player->requested_state;
        player->transition_tick = 0u;
        return command;
    }

    const UAudioMusicState *current = &music->states[player->current_state];
    if (current->transition_ticks == 0u) {
        transition_boundary = true;
    } else {
        ++player->transition_tick;
        if (player->transition_tick >= current->transition_ticks) {
            player->transition_tick = 0u;
            transition_boundary = true;
        }
    }

    if (!transition_boundary || player->requested_state == player->current_state) {
        return U_AUDIO_COMMAND_NONE;
    }

    const USoundCommand command = music->states[player->requested_state].command;
    player->current_state = player->requested_state;
    player->transition_tick = 0u;
    return command;
}
