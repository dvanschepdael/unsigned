/**
 * @file save_backend.h
 * @brief Physical persistence boundary implemented by the active platform.
 *
 * The portable save layer owns record format, versioning and validation. Backends only resolve
 * media and transfer complete fixed-size records to/from physical storage.
 */

#ifndef UNSIGNED_SAVE_BACKEND_H
#define UNSIGNED_SAVE_BACKEND_H

#include "save/storage.h"

/* Physical-storage boundary implemented by the active platform backend. */
UStorage unsigned_save_backend_resolve(UStorage requested);
bool unsigned_save_backend_available(UStorage storage);
USaveState unsigned_save_backend_card_exists(u16 ngh_bcd, u8 slot, bool *out_exists);
USaveState unsigned_save_backend_read_record(UStorage storage, u16 ngh_bcd, u8 slot, u8 record[UNSIGNED_SAVE_RECORD_SIZE]);
USaveState unsigned_save_backend_write_record(UStorage storage, u16 ngh_bcd, u8 slot, const u8 record[UNSIGNED_SAVE_RECORD_SIZE]);
USaveState unsigned_save_backend_delete_record(UStorage storage, u16 ngh_bcd, u8 slot);

#endif
