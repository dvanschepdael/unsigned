/**
 * @file block.h
 * @brief Application-facing fixed-slot save block contract.
 *
 * `UDataBlock` keeps payload pointer/size/version/slot together so call sites cannot accidentally
 * read a record with a different schema or destination buffer than the one they declared.
 */

#ifndef UNSIGNED_SAVE_BLOCK_H
#define UNSIGNED_SAVE_BLOCK_H

#include "save/storage.h"

typedef struct UBlockConfig {
    UStorage storage;
    u16 ngh_bcd;
} UBlockConfig;

typedef struct UDataBlock {
    void *data;
    u16 size;
    u16 version;
    u8 slot;
} UDataBlock;

/**
 * Configure the physical storage and NGH used by subsequent block operations.
 * @pre `config` is valid and names a declared storage backend.
 */
void unsigned_block_init(const UBlockConfig *config);
/** Return whether the configured physical medium is currently usable. */
bool unsigned_block_available(void);

/**
 * Presence is physical only; version and payload validation happen on read.
 * @pre `block_data` defines a valid slot/payload contract and `out_exists` is valid.
 */
USaveState unsigned_block_exists(const UDataBlock *block_data, bool *out_exists);

/**
 * Read/write use the exact payload size declared by the block.
 * @pre `block_data->slot < UNSIGNED_SAVE_SLOT_COUNT`, `block_data->size` is in 1..UNSIGNED_SAVE_MAX_DATA_SIZE,
 *      and `block_data->data` addresses that many bytes.
 */
USaveState unsigned_block_write(const UDataBlock *block_data);
USaveState unsigned_block_read(const UDataBlock *block_data);
USaveState unsigned_block_delete(const UDataBlock *block_data);

#endif
