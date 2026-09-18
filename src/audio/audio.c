#include "audio/audio.h"

#include "audio/audio_backend.h"
#include "audio/audio_music_internal.h"

void unsigned_audio_manager_init(UAudioManager *audio) {
    if (audio == NULL) {
        return;
    }

    *audio = (UAudioManager){ 0 };
    audio->random_state = 1u;
    audio->music.current_state = U_AUDIO_MUSIC_STATE_NONE;
    audio->music.requested_state = U_AUDIO_MUSIC_STATE_NONE;
}

void unsigned_audio_set_random_seed(UAudioManager *audio, u16 seed) {
    if (audio != NULL) {
        audio->random_state = seed == 0u ? 1u : seed;
    }
}

bool unsigned_audio_play_sound_effect(UAudioManager *audio, USoundCommand command) {
    if (audio == NULL || !unsigned_audio_command_is_game(command) || audio->pending_sound_effect_count >= UNSIGNED_AUDIO_SFX_QUEUE_CAPACITY) {
        return false;
    }

    u16 index = (u16)audio->pending_sound_effect_head + audio->pending_sound_effect_count;
    if (index >= UNSIGNED_AUDIO_SFX_QUEUE_CAPACITY) {
        index -= UNSIGNED_AUDIO_SFX_QUEUE_CAPACITY;
    }

    audio->pending_sound_effects[(u8)index] = command;
    ++audio->pending_sound_effect_count;
    return true;
}

static bool audio_pop_sound_effect(UAudioManager *audio, USoundCommand *command) {
    if (audio->pending_sound_effect_count == 0u) {
        return false;
    }

    *command = audio->pending_sound_effects[audio->pending_sound_effect_head];
    audio->pending_sound_effects[audio->pending_sound_effect_head] = U_AUDIO_COMMAND_NONE;

    ++audio->pending_sound_effect_head;
    if (audio->pending_sound_effect_head >= UNSIGNED_AUDIO_SFX_QUEUE_CAPACITY) {
        audio->pending_sound_effect_head = 0u;
    }

    --audio->pending_sound_effect_count;
    return true;
}

static bool audio_next_command(UAudioManager *audio, USoundCommand *out_command) {
    if (audio == NULL || out_command == NULL) {
        return false;
    }

    *out_command = U_AUDIO_COMMAND_NONE;
    ++audio->tick;

    return unsigned_audio_music_tick(audio, out_command) || audio_pop_sound_effect(audio, out_command);
}

void unsigned_audio_manager_tick(UAudioManager *audio) {
    if (audio == NULL) {
        return;
    }

    /*
     * The platform transport may apply back-pressure. Keep the already-resolved command in the
     * manager until it is accepted so music transitions and SFX queue entries are never lost.
     */
    if (audio->pending_backend_command != U_AUDIO_COMMAND_NONE) {
        if (unsigned_audio_backend_send(audio->pending_backend_command)) {
            audio->pending_backend_command = U_AUDIO_COMMAND_NONE;
        }
        return;
    }

    USoundCommand command;
    if (audio_next_command(audio, &command) && !unsigned_audio_backend_send(command)) {
        audio->pending_backend_command = command;
    }
}
