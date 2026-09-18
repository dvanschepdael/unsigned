/**
 * @file save_record_internal.h
 * @brief Internal portable 64-byte Unsigned save-record codec.
 */

#ifndef UNSIGNED_SAVE_RECORD_INTERNAL_H
#define UNSIGNED_SAVE_RECORD_INTERNAL_H

#include "save/storage.h"

/** Return true when the four-byte Unsigned record magic is present. No CRC/version check is performed. */
bool unsigned_save_record_has_magic(const u8 *record);

/** Build one complete record from a payload and application data version. Invalid arguments are ignored. */
void unsigned_save_record_build(u8 record[UNSIGNED_SAVE_RECORD_SIZE], const void *data, u16 size, u16 data_version);

/** Validate magic, CRC, format and expected data version, then copy the payload to the caller buffer. */
USaveState unsigned_save_record_read(const u8 record[UNSIGNED_SAVE_RECORD_SIZE], void *data, u16 capacity, u16 expected_data_version, u16 *out_size);

#endif
