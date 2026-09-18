/**
 * @file storage_internal.h
 * @brief Explicit-config operations shared by block.c and the public storage API.
 */

#ifndef UNSIGNED_SAVE_STORAGE_INTERNAL_H
#define UNSIGNED_SAVE_STORAGE_INTERNAL_H

#include "save/storage.h"

/**
 * @brief Returns whether the storage backend/resource is currently available.
 *
 * @param config Storage selection used to resolve and query the concrete save backend.
 * @return true when the requested/resolved backend reports itself usable; false for invalid config or unavailable media.
 */
bool unsigned_storage_available_with_config(const UStorageConfig *config);
/**
 * @brief Checks physical slot presence using an explicit storage configuration instead of the global default.
 *
 * @param config Valid storage/backend, NGH and schema configuration.
 * @param slot Physical save slot in the range supported by `UNSIGNED_SAVE_SLOT_COUNT`.
 * @param out_exists Required output set to true when the slot contains a recognizable record; false otherwise.
 * @return `U_SAVE_OK` when the presence check completed, or the specific validation/backend `USaveState` error.
 */
USaveState unsigned_storage_exists_with_config(const UStorageConfig *config, u8 slot, bool *out_exists);
/**
 * @brief Encodes and persists one versioned save record using an explicit storage configuration.
 *
 * @param config Valid storage/backend, NGH and schema configuration.
 * @param slot Physical save slot in the range supported by `UNSIGNED_SAVE_SLOT_COUNT`.
 * @param data Application payload to persist; may be NULL only when `size` is zero.
 * @param size Payload size in bytes; must not exceed `UNSIGNED_SAVE_MAX_DATA_SIZE`.
 * @return `U_SAVE_OK` on success, or the specific validation/backend `USaveState` error.
 */
USaveState unsigned_storage_write_with_config(const UStorageConfig *config, u8 slot, const void *data, u16 size);
/**
 * @brief Reads, validates and decodes one versioned save record using an explicit storage configuration.
 *
 * @param config Valid storage/backend, NGH and expected schema configuration.
 * @param slot Physical save slot in the range supported by `UNSIGNED_SAVE_SLOT_COUNT`.
 * @param data Destination payload buffer; may be NULL only when `capacity` is zero.
 * @param capacity Writable payload capacity in bytes.
 * @param out_size Optional output receiving the validated stored payload size; initialized to zero before backend I/O.
 * @return `U_SAVE_OK` on success, or the specific validation, I/O, corruption, version or capacity `USaveState` error.
 */
USaveState unsigned_storage_read_with_config(const UStorageConfig *config, u8 slot, void *data, u16 capacity, u16 *out_size);
/**
 * @brief Deletes one physical save record using an explicit storage configuration.
 *
 * @param config Valid storage/backend and NGH configuration.
 * @param slot Physical save slot in the range supported by `UNSIGNED_SAVE_SLOT_COUNT`.
 * @return `U_SAVE_OK` on success, or the specific validation/backend `USaveState` error.
 */
USaveState unsigned_storage_delete_with_config(const UStorageConfig *config, u8 slot);

#endif
