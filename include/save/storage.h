/**
 * @file storage.h
 * @brief Versioned fixed-size save records over a platform-selected physical backend.
 *
 * Each physical slot stores one 64-byte record containing engine metadata plus an application
 * payload. Reads validate the record before exposing payload bytes to the caller.
 */

#ifndef UNSIGNED_SAVE_STORAGE_H
#define UNSIGNED_SAVE_STORAGE_H

#include "core/types.h"

#define UNSIGNED_SAVE_SLOT_COUNT 16u
#define UNSIGNED_SAVE_RECORD_SIZE 64u
#define UNSIGNED_SAVE_HEADER_SIZE 12u
#define UNSIGNED_SAVE_MAX_DATA_SIZE (UNSIGNED_SAVE_RECORD_SIZE - UNSIGNED_SAVE_HEADER_SIZE)
#define UNSIGNED_SAVE_BRAM_BYTES (UNSIGNED_SAVE_SLOT_COUNT * UNSIGNED_SAVE_RECORD_SIZE)

typedef enum UStorage {
    U_STORAGE_AUTO = 0,
    U_STORAGE_MEMORY_CARD,
    U_STORAGE_BACKUP_RAM,
} UStorage;

typedef enum USaveState {
    U_SAVE_OK = 0,
    U_SAVE_ERROR_INVALID_ARGUMENT,
    U_SAVE_ERROR_SLOT,
    U_SAVE_ERROR_TOO_LARGE,
    U_SAVE_ERROR_BUFFER_TOO_SMALL,
    U_SAVE_ERROR_NO_CARD,
    U_SAVE_ERROR_WRITE_PROTECTED,
    U_SAVE_ERROR_NOT_FORMATTED,
    U_SAVE_ERROR_NO_DATA,
    U_SAVE_ERROR_CORRUPT,
    U_SAVE_ERROR_VERSION_MISMATCH,
    U_SAVE_ERROR_CARD_FULL,
    U_SAVE_ERROR_IO,
    U_SAVE_ERROR_UNSUPPORTED,
} USaveState;

typedef struct UStorageConfig {
    /** Neo Geo game identifier encoded as expected by the backend/record contract. */
    u16 ngh_bcd;
    /** Application schema version; a mismatch is reported instead of decoding incompatible data. */
    u16 data_version;
    /** Requested physical target; AUTO is resolved by the active platform backend. */
    UStorage storage;
} UStorageConfig;

/** Copy a valid configuration into the direct-storage default state. Invalid configs are ignored. */
void unsigned_storage_init(const UStorageConfig *config);

/** Return the resolved physical storage; AUTO becomes card or backup RAM. */
UStorage unsigned_storage_get(void);
/** Return whether the resolved physical backend is currently usable (for example, card present). */
bool unsigned_storage_available(void);

/** Successful absence is reported as U_SAVE_OK with `out_exists == false`. */
USaveState unsigned_storage_exists(u8 slot, bool *out_exists);

/** Records include schema version and CRC; payload size is limited to UNSIGNED_SAVE_MAX_DATA_SIZE. */
USaveState unsigned_storage_write(u8 slot, const void *data, u16 size);
USaveState unsigned_storage_read(u8 slot, void *data, u16 capacity, u16 *out_size);
USaveState unsigned_storage_delete(u8 slot);

#endif
