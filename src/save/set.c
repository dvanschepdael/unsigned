#include "save/set.h"

#include "core/math/math.h"

#define UNSIGNED_SAVE_SET_MARKER 0xa5u
#define UNSIGNED_SAVE_SET_MARKER_OFFSET 0u
#define UNSIGNED_SAVE_SET_GENERATION_OFFSET 1u
#define UNSIGNED_SAVE_SET_BLOCK_INDEX_OFFSET 3u
#define UNSIGNED_SAVE_SET_BLOCK_COUNT_OFFSET 4u
#define UNSIGNED_SAVE_SET_PAYLOAD_OFFSET UNSIGNED_SAVE_SET_HEADER_SIZE

/* Reads are staged here so corrupt/mixed generations never partially overwrite user data. */
static u8 set_staging[UNSIGNED_SAVE_SET_MAX_DATA_SIZE];
static u8 set_record[UNSIGNED_SAVE_MAX_DATA_SIZE];

static void set_write_u16_be(u8 *dst, u16 value) {
    dst[0] = (u8)(value >> 8);
    dst[1] = (u8)value;
}

static u16 set_read_u16_be(const u8 *src) {
    return (u16)(((u16)src[0] << 8) | (u16)src[1]);
}

static void set_copy(u8 *dst, const u8 *src, u32 size) {
    while (size-- != 0u) {
        *dst++ = *src++;
    }
}

static USaveState set_validate(const UDataSet *set, u8 *out_block_count) {
    if (out_block_count == NULL) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }

    *out_block_count = 0u;

    if (set == NULL || set->data == NULL || set->size == 0u) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }

    if (set->first_slot >= UNSIGNED_SAVE_SLOT_COUNT) {
        return U_SAVE_ERROR_SLOT;
    }

    if (set->size > (u32)UNSIGNED_SAVE_SET_MAX_DATA_SIZE) {
        return U_SAVE_ERROR_TOO_LARGE;
    }

    u32 block_count = ((set->size - 1u) / (u32)UNSIGNED_SAVE_SET_BLOCK_DATA_SIZE) + 1u;

    if (block_count > (u32)UNSIGNED_SAVE_SLOT_COUNT) {
        return U_SAVE_ERROR_TOO_LARGE;
    }

    if ((u32)set->first_slot + block_count > (u32)UNSIGNED_SAVE_SLOT_COUNT) {
        return U_SAVE_ERROR_SLOT;
    }

    *out_block_count = (u8)block_count;
    return U_SAVE_OK;
}

static u16 set_payload_size(const UDataSet *set, u8 block_index) {
    const u32 offset = (u32)block_index * (u32)UNSIGNED_SAVE_SET_BLOCK_DATA_SIZE;
    const u32 remaining = set->size - offset;

    return (u16)unsigned_math_min_u32(remaining, (u32)UNSIGNED_SAVE_SET_BLOCK_DATA_SIZE);
}

static u16 set_record_size(const UDataSet *set, u8 block_index) {
    return (u16)(UNSIGNED_SAVE_SET_HEADER_SIZE + set_payload_size(set, block_index));
}

static void set_make_block(const UDataSet *set, u8 block_index, UDataBlock *out_block) {
    out_block->data = set_record;
    out_block->size = set_record_size(set, block_index);
    out_block->version = set->version;
    out_block->slot = (u8)(set->first_slot + block_index);
}

/** Serializes set marker, generation, block index/count and the corresponding user-data slice. */
static void set_build_record(const UDataSet *set, u8 block_index, u8 block_count, u16 generation) {
    const u16 payload_size = set_payload_size(set, block_index);
    const u32 offset = (u32)block_index * (u32)UNSIGNED_SAVE_SET_BLOCK_DATA_SIZE;

    set_record[UNSIGNED_SAVE_SET_MARKER_OFFSET] = UNSIGNED_SAVE_SET_MARKER;
    set_write_u16_be(&set_record[UNSIGNED_SAVE_SET_GENERATION_OFFSET], generation);
    set_record[UNSIGNED_SAVE_SET_BLOCK_INDEX_OFFSET] = block_index;
    set_record[UNSIGNED_SAVE_SET_BLOCK_COUNT_OFFSET] = block_count;
    set_copy(&set_record[UNSIGNED_SAVE_SET_PAYLOAD_OFFSET], ((const u8 *)set->data) + offset, payload_size);
}

static bool set_record_header_is_valid(u8 block_index, u8 block_count) {
    return set_record[UNSIGNED_SAVE_SET_MARKER_OFFSET] == UNSIGNED_SAVE_SET_MARKER && set_record[UNSIGNED_SAVE_SET_BLOCK_INDEX_OFFSET] == block_index && set_record[UNSIGNED_SAVE_SET_BLOCK_COUNT_OFFSET] == block_count;
}

static USaveState set_read_record(const UDataSet *set, u8 block_index, u8 block_count, u16 *inout_generation) {
    UDataBlock block;

    set_make_block(set, block_index, &block);
    USaveState result = unsigned_block_read(&block);
    if (result != U_SAVE_OK) {
        return result;
    }

    if (!set_record_header_is_valid(block_index, block_count)) {
        return U_SAVE_ERROR_CORRUPT;
    }

    u16 generation = set_read_u16_be(&set_record[UNSIGNED_SAVE_SET_GENERATION_OFFSET]);
    if (block_index == 0u) {
        *inout_generation = generation;
    } else if (generation != *inout_generation) {
        return U_SAVE_ERROR_CORRUPT;
    }

    return U_SAVE_OK;
}

static bool set_generation_is_used(const u16 *generations, u8 count, u16 generation) {
    for (u8 i = 0u; i < count; ++i) {
        if (generations[i] == generation) {
            return true;
        }
    }

    return false;
}

static USaveState set_next_generation(const UDataSet *set, u8 block_count, u16 *out_generation) {
    u16 generations[UNSIGNED_SAVE_SLOT_COUNT];
    u8 generation_count = 0u;
    u16 candidate = 1u;

    if (out_generation == NULL) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }

    for (u8 i = 0u; i < block_count; ++i) {
        UDataBlock block;

        set_make_block(set, i, &block);
        USaveState result = unsigned_block_read(&block);

        if (result == U_SAVE_OK) {
            if (set_record_header_is_valid(i, block_count)) {
                generations[generation_count++] = set_read_u16_be(&set_record[UNSIGNED_SAVE_SET_GENERATION_OFFSET]);
            }
            continue;
        }

        if (result == U_SAVE_ERROR_NO_DATA || result == U_SAVE_ERROR_CORRUPT || result == U_SAVE_ERROR_VERSION_MISMATCH) {
            continue;
        }

        return result;
    }

    if (generation_count != 0u) {
        candidate = (u16)(generations[0] + 1u);
    }

    while (set_generation_is_used(generations, generation_count, candidate)) {
        ++candidate;
    }

    *out_generation = candidate;
    return U_SAVE_OK;
}

u8 unsigned_set_block_count(const UDataSet *set) {
    u8 block_count;

    if (set_validate(set, &block_count) != U_SAVE_OK) {
        return 0u;
    }

    return block_count;
}

USaveState unsigned_set_exists(const UDataSet *set, bool *out_exists) {
    u16 generation = 0u;
    u8 block_count;

    if (out_exists == NULL) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }

    *out_exists = false;
    USaveState result = set_validate(set, &block_count);
    if (result != U_SAVE_OK) {
        return result;
    }

    for (u8 i = 0u; i < block_count; ++i) {
        result = set_read_record(set, i, block_count, &generation);
        if (result == U_SAVE_ERROR_NO_DATA) {
            return U_SAVE_OK;
        }
        if (result != U_SAVE_OK) {
            return result;
        }
    }

    *out_exists = true;
    return U_SAVE_OK;
}

USaveState unsigned_set_write(const UDataSet *set) {
    u16 generation;
    u8 block_count;

    USaveState result = set_validate(set, &block_count);
    if (result != U_SAVE_OK) {
        return result;
    }

    result = set_next_generation(set, block_count, &generation);
    if (result != U_SAVE_OK) {
        return result;
    }

    for (u8 i = 0u; i < block_count; ++i) {
        UDataBlock block;

        set_build_record(set, i, block_count, generation);
        set_make_block(set, i, &block);

        result = unsigned_block_write(&block);
        if (result != U_SAVE_OK) {
            return result;
        }
    }

    return U_SAVE_OK;
}

USaveState unsigned_set_read(const UDataSet *set) {
    u16 generation = 0u;
    u8 block_count;

    USaveState result = set_validate(set, &block_count);
    if (result != U_SAVE_OK) {
        return result;
    }

    for (u8 i = 0u; i < block_count; ++i) {
        const u16 payload_size = set_payload_size(set, i);
        const u32 offset = (u32)i * (u32)UNSIGNED_SAVE_SET_BLOCK_DATA_SIZE;

        result = set_read_record(set, i, block_count, &generation);
        if (result != U_SAVE_OK) {
            return result;
        }

        set_copy(&set_staging[offset], &set_record[UNSIGNED_SAVE_SET_PAYLOAD_OFFSET], payload_size);
    }

    set_copy((u8 *)set->data, set_staging, set->size);
    return U_SAVE_OK;
}

USaveState unsigned_set_delete(const UDataSet *set) {
    u8 block_count;

    USaveState result = set_validate(set, &block_count);
    if (result != U_SAVE_OK) {
        return result;
    }

    for (u8 i = 0u; i < block_count; ++i) {
        UDataBlock block;

        set_make_block(set, i, &block);
        result = unsigned_block_delete(&block);
        if (result != U_SAVE_OK) {
            return result;
        }
    }

    return U_SAVE_OK;
}
