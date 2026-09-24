/**
 * @file save_backend.c
 * @brief Maps logical fixed-size save records to Neo Geo memory card or MVS backup RAM.
 *
 * @details
 * The generic save layer validates logical records/version/CRC; this file owns physical Neo Geo I/O.
 * Memory-card operations populate ngdevkit's BIOS_CARD parameters and call bios_card(), while backup
 * RAM uses a fixed `_backup_ram` record array. `bios_card_sub` is a 16-slot bitfield, hence the
 * `(1u << slot)` mapping. The backend never exposes BIOS variables to callers.
 */

#include "save/save_backend.h"
#include "save/save_internal.h"

#include <ngdevkit/backup-ram.h>
#include <ngdevkit/bios-ram.h>
#include <ngdevkit/memory-card.h>

#define UNSIGNED_SYSTEM_MVS 0x80u

static u8 _backup_ram bram_records[UNSIGNED_SAVE_SLOT_COUNT][UNSIGNED_SAVE_RECORD_SIZE];

/** Translate ngdevkit memory-card BIOS result codes into the engine save error vocabulary. */
static USaveState save_backend_map_card_error(u8 answer) {
    switch (answer) {
    case MC_ERR_OK:
        return U_SAVE_OK;
    case MC_ERR_NO_CARD:
        return U_SAVE_ERROR_NO_CARD;
    case MC_ERR_NOT_FORMATTED:
        return U_SAVE_ERROR_NOT_FORMATTED;
    case MC_ERR_DATA_DOES_NOT_EXIST:
        return U_SAVE_ERROR_NO_DATA;
    case MC_ERR_CARD_FULL:
        return U_SAVE_ERROR_CARD_FULL;
    case MC_ERR_WRITE_DISABLED:
        return U_SAVE_ERROR_WRITE_PROTECTED;
    case MC_ERR_FAT_ERROR:
    default:
        return U_SAVE_ERROR_IO;
    }
}

UStorage unsigned_save_backend_resolve(UStorage requested) {
    if (requested != U_STORAGE_AUTO) {
        return requested;
    }
    return bios_mvs_flag == UNSIGNED_SYSTEM_MVS ? U_STORAGE_BACKUP_RAM : U_STORAGE_MEMORY_CARD;
}

bool unsigned_save_backend_available(UStorage storage) {
    switch (unsigned_save_backend_resolve(storage)) {
    case U_STORAGE_BACKUP_RAM:
        return bios_mvs_flag == UNSIGNED_SYSTEM_MVS;
    case U_STORAGE_MEMORY_CARD:
        return ng_memory_card_inserted();
    default:
        U_UNREACHABLE();
    }
}

/** Populate the BIOS memory-card fields shared by fixed-size record reads and writes. */
static void save_backend_prepare_card_record(u16 ngh_bcd, u8 slot, const u8 record[UNSIGNED_SAVE_RECORD_SIZE]) {
    bios_card_fcb = ngh_bcd;
    bios_card_sub = (u16)(1u << slot);
    bios_card_start = (u32)(uintptr_t)record;
    bios_card_size = UNSIGNED_SAVE_RECORD_SIZE;
}

/** Validate the mutable memory-card state shared by write and delete operations. */
static USaveState save_backend_card_write_state(void) {
    if (!ng_memory_card_inserted()) {
        return U_SAVE_ERROR_NO_CARD;
    }
    if (ng_memory_card_write_protected()) {
        return U_SAVE_ERROR_WRITE_PROTECTED;
    }
    return U_SAVE_OK;
}

/** Execute a mutating memory-card command after its BIOS fields have been prepared. */
static USaveState save_backend_execute_card_write(void) {
    ng_memory_card_unlock();
    bios_card();
    return save_backend_map_card_error(bios_card_answer);
}

USaveState unsigned_save_backend_card_exists(u16 ngh_bcd, u8 slot, bool *out_exists) {
    *out_exists = false;
    bios_card_fcb = ngh_bcd;
    bios_card_command = MC_CMD_DATA_SEARCH;
    bios_card();

    const USaveState result = save_backend_map_card_error(bios_card_answer);
    if (result == U_SAVE_OK) {
        *out_exists = (bios_card_sub & (u16)(1u << slot)) != 0u;
    }
    return result;
}

USaveState unsigned_save_backend_read_record(UStorage storage, u16 ngh_bcd, u8 slot, u8 record[UNSIGNED_SAVE_RECORD_SIZE]) {
    switch (unsigned_save_backend_resolve(storage)) {
    case U_STORAGE_MEMORY_CARD:
        save_backend_prepare_card_record(ngh_bcd, slot, record);
        bios_card_command = MC_CMD_LOAD_DATA;
        bios_card();
        return save_backend_map_card_error(bios_card_answer);

    case U_STORAGE_BACKUP_RAM:
        if (bios_mvs_flag != UNSIGNED_SYSTEM_MVS) {
            return U_SAVE_ERROR_UNSUPPORTED;
        }
        unsigned_save_copy_bytes(record, bram_records[slot], UNSIGNED_SAVE_RECORD_SIZE);
        return U_SAVE_OK;

    default:
        U_UNREACHABLE();
    }
}

USaveState unsigned_save_backend_write_record(UStorage storage, u16 ngh_bcd, u8 slot, const u8 record[UNSIGNED_SAVE_RECORD_SIZE]) {
    switch (unsigned_save_backend_resolve(storage)) {
    case U_STORAGE_MEMORY_CARD: {
        const USaveState state = save_backend_card_write_state();
        if (state != U_SAVE_OK) {
            return state;
        }

        save_backend_prepare_card_record(ngh_bcd, slot, record);
        bios_card_command = MC_CMD_SAVE_DATA;
        return save_backend_execute_card_write();
    }

    case U_STORAGE_BACKUP_RAM:
        if (bios_mvs_flag != UNSIGNED_SYSTEM_MVS) {
            return U_SAVE_ERROR_UNSUPPORTED;
        }
        unsigned_save_copy_bytes(bram_records[slot], record, UNSIGNED_SAVE_RECORD_SIZE);
        return U_SAVE_OK;

    default:
        U_UNREACHABLE();
    }
}

USaveState unsigned_save_backend_delete_record(UStorage storage, u16 ngh_bcd, u8 slot) {
    switch (unsigned_save_backend_resolve(storage)) {
    case U_STORAGE_MEMORY_CARD: {
        const USaveState state = save_backend_card_write_state();
        if (state != U_SAVE_OK) {
            return state;
        }

        bios_card_fcb = ngh_bcd;
        bios_card_sub = (u16)(1u << slot);
        bios_card_command = MC_CMD_DELETE_DATA;
        return save_backend_execute_card_write();
    }

    case U_STORAGE_BACKUP_RAM:
        if (bios_mvs_flag != UNSIGNED_SYSTEM_MVS) {
            return U_SAVE_ERROR_UNSUPPORTED;
        }
        for (u16 i = 0u; i < UNSIGNED_SAVE_RECORD_SIZE; ++i) {
            bram_records[slot][i] = 0u;
        }
        return U_SAVE_OK;

    default:
        U_UNREACHABLE();
    }
}
