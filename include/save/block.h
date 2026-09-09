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

/** Configure the physical storage and NGH used by subsequent block operations. */
void unsigned_block_init(const UBlockConfig *config);
bool unsigned_block_available(void);

/** Presence is physical only; version and payload validation happen on read. */
USaveState unsigned_block_exists(const UDataBlock *block_data, bool *out_exists);

/** Read/write require an exact 1..UNSIGNED_SAVE_MAX_DATA_SIZE payload contract. */
USaveState unsigned_block_write(const UDataBlock *block_data);
USaveState unsigned_block_read(const UDataBlock *block_data);
USaveState unsigned_block_delete(const UDataBlock *block_data);

#endif
