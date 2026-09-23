#include "save/block.h"

#include "save/storage_internal.h"

static UBlockConfig game_save_config = {
    .storage = U_STORAGE_AUTO,
    .ngh_bcd = 0u,
};

static u8 block_buffer[UNSIGNED_SAVE_MAX_DATA_SIZE];

static void block_copy(u8 *dst, const u8 *src, u16 size) {
    while (size-- != 0u) {
        *dst++ = *src++;
    }
}

static UStorageConfig block_storage_config(const UDataBlock *block_data) {
    return (UStorageConfig){
        .ngh_bcd = game_save_config.ngh_bcd,
        .data_version = block_data->version,
        .storage = game_save_config.storage,
    };
}

bool unsigned_block_available(void) {
    const UStorageConfig config = {
        .ngh_bcd = game_save_config.ngh_bcd,
        .data_version = 1u,
        .storage = game_save_config.storage,
    };
    return unsigned_storage_available_with_config(&config);
}

void unsigned_block_init(const UBlockConfig *config) {
    game_save_config = *config;
}

USaveState unsigned_block_exists(const UDataBlock *block_data, bool *out_exists) {
    const UStorageConfig config = block_storage_config(block_data);
    return unsigned_storage_exists_with_config(&config, block_data->slot, out_exists);
}

USaveState unsigned_block_write(const UDataBlock *block_data) {
    const UStorageConfig config = block_storage_config(block_data);
    return unsigned_storage_write_with_config(&config, block_data->slot, block_data->data, block_data->size);
}

USaveState unsigned_block_read(const UDataBlock *block_data) {
    const UStorageConfig config = block_storage_config(block_data);
    u16 loaded_size = 0u;
    const USaveState result = unsigned_storage_read_with_config(&config, block_data->slot, block_buffer, UNSIGNED_SAVE_MAX_DATA_SIZE, &loaded_size);
    if (result != U_SAVE_OK) {
        return result;
    }
    if (loaded_size != block_data->size) {
        return U_SAVE_ERROR_CORRUPT;
    }

    block_copy((u8 *)block_data->data, block_buffer, loaded_size);
    return U_SAVE_OK;
}

USaveState unsigned_block_delete(const UDataBlock *block_data) {
    const UStorageConfig config = block_storage_config(block_data);
    return unsigned_storage_delete_with_config(&config, block_data->slot);
}
