#include "audio/audio_music_internal.h"

static bool unsigned_audio_music_is_valid(const UAudioMusic *music) {
    return music != NULL && music->states != NULL && music->state_count != 0u && music->initial_state < music->state_count && unsigned_audio_command_is_game(music->states[music->initial_state].command);
}

static void unsigned_audio_music_reset_player(UAudioMusicPlayer *player) {
    *player = (UAudioMusicPlayer){ 0 };
    player->current_state = U_AUDIO_MUSIC_STATE_NONE;
    player->requested_state = U_AUDIO_MUSIC_STATE_NONE;
}

bool unsigned_audio_music_play(UAudioManager *audio, const UAudioMusic *music) {
    if (audio == NULL || !unsigned_audio_music_is_valid(music)) {
        return false;
    }

    unsigned_audio_music_reset_player(&audio->music);
    audio->music.definition = music;
    audio->music.requested_state = music->initial_state;
    return true;
}

bool unsigned_audio_music_set_state(UAudioManager *audio, UAudioMusicStateId state) {
    if (audio == NULL || audio->music.definition == NULL) {
        return false;
    }

    const UAudioMusic *music = audio->music.definition;
    if (state >= music->state_count || !unsigned_audio_command_is_game(music->states[state].command)) {
        return false;
    }

    audio->music.requested_state = state;
    audio->music.stop_requested = false;
    return true;
}

bool unsigned_audio_music_stop(UAudioManager *audio) {
    if (audio == NULL || audio->music.definition == NULL || !unsigned_audio_command_is_game(audio->music.definition->stop_command)) {
        return false;
    }

    audio->music.stop_requested = true;
    return true;
}

bool unsigned_audio_music_tick(UAudioManager *audio, USoundCommand *command) {
    bool transition_boundary = false;

    if (audio == NULL || command == NULL) {
        return false;
    }

    UAudioMusicPlayer *player = &audio->music;
    const UAudioMusic *music = player->definition;
    if (music == NULL) {
        return false;
    }

    if (player->stop_requested) {
        *command = music->stop_command;
        unsigned_audio_music_reset_player(player);
        return true;
    }

    if (player->current_state == U_AUDIO_MUSIC_STATE_NONE) {
        if (player->requested_state >= music->state_count) {
            return false;
        }
        *command = music->states[player->requested_state].command;
        player->current_state = player->requested_state;
        player->transition_tick = 0u;
        return true;
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
        return false;
    }

    if (player->requested_state >= music->state_count || !unsigned_audio_command_is_game(music->states[player->requested_state].command)) {
        return false;
    }

    *command = music->states[player->requested_state].command;
    player->current_state = player->requested_state;
    player->transition_tick = 0u;
    return true;
}
