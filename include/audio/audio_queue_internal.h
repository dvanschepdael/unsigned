/**
 * @file audio_queue_internal.h
 * @brief Shared fixed-ring operations used by logical and Neo Geo audio queues.
 */

#ifndef UNSIGNED_AUDIO_AUDIO_QUEUE_INTERNAL_H
#define UNSIGNED_AUDIO_AUDIO_QUEUE_INTERNAL_H

#include "audio/audio_types.h"
#include "core/types.h"

/**
 * Resolve one relative offset in a fixed audio ring without division/modulo.
 * @pre `capacity > 0`, `head < capacity`, and `offset < capacity`.
 */
static inline u8 unsigned_audio_queue_index(u8 head, u8 offset, u8 capacity) {
    u16 index = (u16)head + offset;
    if (index >= capacity) {
        index = (u16)(index - capacity);
    }
    return (u8)index;
}

/**
 * Queue one command in a caller-owned fixed ring.
 * @pre `capacity > 0`, `*head < capacity`, and `*count <= capacity`.
 */
static inline bool unsigned_audio_queue_push(USoundCommand *commands, u8 *head, u8 *count, u8 capacity, USoundCommand command) {
    if (*count >= capacity) {
        return false;
    }

    commands[unsigned_audio_queue_index(*head, *count, capacity)] = command;
    ++(*count);
    return true;
}

/**
 * Pop one command from a caller-owned fixed ring, or NONE when empty.
 * @pre `capacity > 0`, `*head < capacity`, and `*count <= capacity`.
 */
static inline USoundCommand unsigned_audio_queue_pop(USoundCommand *commands, u8 *head, u8 *count, u8 capacity) {
    if (*count == 0u) {
        return U_AUDIO_COMMAND_NONE;
    }

    const USoundCommand command = commands[*head];
    *head = unsigned_audio_queue_index(*head, 1u, capacity);
    --(*count);
    return command;
}

/** Reset ring ownership metadata; stale array bytes are outside the active queue by contract. */
static inline void unsigned_audio_queue_reset(u8 *head, u8 *count) {
    *head = 0u;
    *count = 0u;
}

#endif
