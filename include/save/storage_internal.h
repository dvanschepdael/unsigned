/**
 * @file storage_internal.h
 * @brief Explicit-config persistence operations shared by the block and direct-storage APIs.
 */

#ifndef UNSIGNED_SAVE_STORAGE_INTERNAL_H
#define UNSIGNED_SAVE_STORAGE_INTERNAL_H

#include "save/storage.h"

/** @pre `config` is valid and names a declared storage backend. */
bool unsigned_storage_available_with_config(const UStorageConfig *config);

/**
 * Check physical slot presence with an explicit configuration.
 * @pre `config`, `out_exists` are valid and `slot < UNSIGNED_SAVE_SLOT_COUNT`.
 */
USaveState unsigned_storage_exists_with_config(const UStorageConfig *config, u8 slot, bool *out_exists);

/**
 * Encode and persist one record.
 * @pre `config` is valid, `slot < UNSIGNED_SAVE_SLOT_COUNT`, and `data` addresses `size` readable bytes.
 * @pre `size <= UNSIGNED_SAVE_MAX_DATA_SIZE`.
 */
USaveState unsigned_storage_write_with_config(const UStorageConfig *config, u8 slot, const void *data, u16 size);

/**
 * Read and validate one record.
 * @pre `config` is valid, `slot < UNSIGNED_SAVE_SLOT_COUNT`, and `data` addresses `capacity` writable bytes.
 * @param out_size Receives the validated payload size.
 * @pre `out_size` is valid.
 */
USaveState unsigned_storage_read_with_config(const UStorageConfig *config, u8 slot, void *data, u16 capacity, u16 *out_size);

/** @pre `config` is valid and `slot < UNSIGNED_SAVE_SLOT_COUNT`. */
USaveState unsigned_storage_delete_with_config(const UStorageConfig *config, u8 slot);

#endif
