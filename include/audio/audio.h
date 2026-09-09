/**
 * @file audio.h
 * @brief Platform-independent audio request manager.
 *
 * Gameplay queues logical SFX/music work here; the platform backend owns the hardware transport.
 * The manager intentionally applies back-pressure: a command that cannot be sent is retained and
 * retried instead of being silently dropped.
 */

#ifndef UNSIGNED_AUDIO_H
#define UNSIGNED_AUDIO_H

#include "audio/audio_event.h"
#include "audio/audio_music.h"
#include "audio/audio_types.h"
#include "audio/config.h"
#include "core/types.h"

struct UAudioManager {
    /** Caller-owned event catalog; must outlive the manager while installed. */
    const UAudioCatalog *catalog;
    UAudioEventState event_states[UNSIGNED_AUDIO_MAX_EVENTS];
    UAudioMusicPlayer music;
    s16 parameters[UNSIGNED_AUDIO_MAX_PARAMETERS];
    /** FIFO of already resolved gameplay SFX commands waiting for arbitration. */
    USoundCommand pending_sound_effects[UNSIGNED_AUDIO_SFX_QUEUE_CAPACITY];
    /** One command retained when the backend transport was full on the previous tick. */
    USoundCommand pending_backend_command;
    u16 random_state;
    u32 tick;
    u8 pending_sound_effect_head;
    u8 pending_sound_effect_count;
};

/** Reset runtime state; no catalog is installed until `unsigned_audio_manager_set_catalog()`. */
void unsigned_audio_manager_init(UAudioManager *audio);
/** Set deterministic event-selection RNG state; useful for reproducible gameplay/tests. */
void unsigned_audio_set_random_seed(UAudioManager *audio, u16 seed);

/** Queue one gameplay SFX command. Commands below U_AUDIO_COMMAND_FIRST_GAME are reserved. */
bool unsigned_audio_play_sound_effect(UAudioManager *audio, USoundCommand command);

/** Advance audio state and dispatch at most one command through the system audio backend. */
void unsigned_audio_manager_tick(UAudioManager *audio);

#endif
