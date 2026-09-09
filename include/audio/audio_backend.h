/**
 * @file audio_backend.h
 * @brief Narrow transport boundary between generic audio policy and the active platform.
 *
 * Generic audio owns retry/back-pressure policy. A backend must therefore report temporary
 * saturation with `false` rather than dropping or reordering a command silently.
 */

#ifndef UNSIGNED_AUDIO_BACKEND_H
#define UNSIGNED_AUDIO_BACKEND_H

#include "audio/audio_types.h"

/**
 * Queue one command for the platform audio transport.
 *
 * Returns false when the platform transport is temporarily full. Callers must retain the command
 * and retry later instead of dropping it.
 */
bool unsigned_audio_backend_send(USoundCommand command);

#endif
