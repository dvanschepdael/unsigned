/**
 * @file set.h
 * @brief Multi-slot save set with interrupted-write detection.
 *
 * Large application data is split over consecutive physical records. Every block carries set
 * metadata including a generation id; reads stage and validate the complete generation before
 * modifying caller-owned data, so a partially written set is never exposed as valid.
 */

#ifndef UNSIGNED_SAVE_SET_H
#define UNSIGNED_SAVE_SET_H

#include "save/block.h"

#define UNSIGNED_SAVE_SET_HEADER_SIZE 5u
#define UNSIGNED_SAVE_SET_BLOCK_DATA_SIZE (UNSIGNED_SAVE_MAX_DATA_SIZE - UNSIGNED_SAVE_SET_HEADER_SIZE)
#define UNSIGNED_SAVE_SET_MAX_DATA_SIZE (UNSIGNED_SAVE_SLOT_COUNT * UNSIGNED_SAVE_SET_BLOCK_DATA_SIZE)

typedef struct UDataSet {
    void *data;
    u32 size;
    u16 version;
    u8 first_slot;
} UDataSet;

/** Return the required consecutive slot count, or 0 when the set definition is invalid. */
u8 unsigned_set_block_count(const UDataSet *set);

/** A set exists only when every block belongs to the same complete generation. */
USaveState unsigned_set_exists(const UDataSet *set, bool *out_exists);

/** Interrupted writes are detected by generation ids and cannot be read as a complete set. */
USaveState unsigned_set_write(const UDataSet *set);

/** Validate every block in staging memory before modifying caller-owned data. */
USaveState unsigned_set_read(const UDataSet *set);
USaveState unsigned_set_delete(const UDataSet *set);

#endif
