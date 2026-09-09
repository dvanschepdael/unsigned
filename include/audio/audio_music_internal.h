/**
 * @file audio_music_internal.h
 * @brief Internal music-state resolver used by the audio manager tick.
 *
 * Kept internal so game code requests music state changes but does not bypass command arbitration.
 */

#ifndef UNSIGNED_AUDIO_MUSIC_INTERNAL_H
#define UNSIGNED_AUDIO_MUSIC_INTERNAL_H

#include "audio/audio.h"

bool unsigned_audio_music_tick(UAudioManager *audio, USoundCommand *command);

#endif
