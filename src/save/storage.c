#include "save/storage.h"

#include "save/save_backend.h"
#include "save/save_record_internal.h"
#include "save/storage_internal.h"

static UStorageConfig storage_config = {
    .ngh_bcd = 0u,
    .data_version = 1u,
    .storage = U_STORAGE_AUTO,
};

static u8 record_buffer[UNSIGNED_SAVE_RECORD_SIZE];

static bool storage_config_is_valid(const UStorageConfig *config) {
    return config != NULL && (config->storage == U_STORAGE_AUTO || config->storage == U_STORAGE_MEMORY_CARD || config->storage == U_STORAGE_BACKUP_RAM);
}

void unsigned_storage_init(const UStorageConfig *config) {
    if (storage_config_is_valid(config)) {
        storage_config = *config;
    }
}

UStorage unsigned_storage_get(void) {
    return unsigned_save_backend_resolve(storage_config.storage);
}

bool unsigned_storage_available_with_config(const UStorageConfig *config) {
    return storage_config_is_valid(config) && unsigned_save_backend_available(config->storage);
}

bool unsigned_storage_available(void) {
    return unsigned_storage_available_with_config(&storage_config);
}

USaveState unsigned_storage_exists_with_config(const UStorageConfig *config, u8 slot, bool *out_exists) {
    if (!storage_config_is_valid(config) || out_exists == NULL) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }
    if (slot >= UNSIGNED_SAVE_SLOT_COUNT) {
        return U_SAVE_ERROR_SLOT;
    }

    *out_exists = false;
    const UStorage storage = unsigned_save_backend_resolve(config->storage);
    if (storage == U_STORAGE_MEMORY_CARD) {
        return unsigned_save_backend_card_exists(config->ngh_bcd, slot, out_exists);
    }
    if (storage != U_STORAGE_BACKUP_RAM) {
        return U_SAVE_ERROR_UNSUPPORTED;
    }

    const USaveState result = unsigned_save_backend_read_record(storage, config->ngh_bcd, slot, record_buffer);
    if (result == U_SAVE_OK) {
        *out_exists = unsigned_save_record_has_magic(record_buffer);
    }
    return result;
}

USaveState unsigned_storage_exists(u8 slot, bool *out_exists) {
    return unsigned_storage_exists_with_config(&storage_config, slot, out_exists);
}

USaveState unsigned_storage_write_with_config(const UStorageConfig *config, u8 slot, const void *data, u16 size) {
    if (!storage_config_is_valid(config)) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }
    if (slot >= UNSIGNED_SAVE_SLOT_COUNT) {
        return U_SAVE_ERROR_SLOT;
    }
    if (size > UNSIGNED_SAVE_MAX_DATA_SIZE) {
        return U_SAVE_ERROR_TOO_LARGE;
    }
    if (size != 0u && data == NULL) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }

    unsigned_save_record_build(record_buffer, data, size, config->data_version);
    return unsigned_save_backend_write_record(config->storage, config->ngh_bcd, slot, record_buffer);
}

USaveState unsigned_storage_write(u8 slot, const void *data, u16 size) {
    return unsigned_storage_write_with_config(&storage_config, slot, data, size);
}

USaveState unsigned_storage_read_with_config(const UStorageConfig *config, u8 slot, void *data, u16 capacity, u16 *out_size) {
    if (!storage_config_is_valid(config)) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }
    if (slot >= UNSIGNED_SAVE_SLOT_COUNT) {
        return U_SAVE_ERROR_SLOT;
    }
    if (capacity != 0u && data == NULL) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }
    if (out_size != NULL) {
        *out_size = 0u;
    }

    const USaveState result = unsigned_save_backend_read_record(config->storage, config->ngh_bcd, slot, record_buffer);
    if (result != U_SAVE_OK) {
        return result;
    }
    return unsigned_save_record_read(record_buffer, data, capacity, config->data_version, out_size);
}

USaveState unsigned_storage_read(u8 slot, void *data, u16 capacity, u16 *out_size) {
    return unsigned_storage_read_with_config(&storage_config, slot, data, capacity, out_size);
}

USaveState unsigned_storage_delete_with_config(const UStorageConfig *config, u8 slot) {
    if (!storage_config_is_valid(config)) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }
    if (slot >= UNSIGNED_SAVE_SLOT_COUNT) {
        return U_SAVE_ERROR_SLOT;
    }
    return unsigned_save_backend_delete_record(config->storage, config->ngh_bcd, slot);
}

USaveState unsigned_storage_delete(u8 slot) {
    return unsigned_storage_delete_with_config(&storage_config, slot);
}
