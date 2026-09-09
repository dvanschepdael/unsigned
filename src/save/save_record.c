/**
 * @file save_record.c
 * @brief Encodes and validates the portable fixed-size Unsigned save-record format.
 */

#include "save/save_record_internal.h"

#define UNSIGNED_SAVE_MAGIC_0 'U'
#define UNSIGNED_SAVE_MAGIC_1 'S'
#define UNSIGNED_SAVE_MAGIC_2 'A'
#define UNSIGNED_SAVE_MAGIC_3 'V'

#define UNSIGNED_SAVE_FORMAT_VERSION 1u
#define UNSIGNED_SAVE_FORMAT_OFFSET 4u
#define UNSIGNED_SAVE_DATA_VERSION_OFFSET 6u
#define UNSIGNED_SAVE_SIZE_OFFSET 8u
#define UNSIGNED_SAVE_CRC_OFFSET 10u
#define UNSIGNED_SAVE_PAYLOAD_OFFSET UNSIGNED_SAVE_HEADER_SIZE

/** Encodes one 16-bit save-record field in stable big-endian byte order. */
static void save_record_write_u16_be(u8 *dst, u16 value) {
    dst[0] = (u8)(value >> 8u);
    dst[1] = (u8)value;
}

/** Decodes one 16-bit save-record field from stable big-endian byte order. */
static u16 save_record_read_u16_be(const u8 *src) {
    return (u16)(((u16)src[0] << 8u) | (u16)src[1]);
}

/** Computes the record CRC over every physical byte except the CRC field itself. */
static u16 save_record_crc(const u8 *record) {
    u16 crc = 0xffffu;

    for (u16 i = 0u; i < UNSIGNED_SAVE_RECORD_SIZE; ++i) {
        if (i == UNSIGNED_SAVE_CRC_OFFSET || i == UNSIGNED_SAVE_CRC_OFFSET + 1u) {
            continue;
        }

        crc ^= (u16)record[i] << 8u;
        for (u8 bit = 0u; bit < 8u; ++bit) {
            const u16 shifted = (u16)(crc << 1u);
            crc = (crc & 0x8000u) != 0u ? (u16)(shifted ^ (u16)0x1021u) : shifted;
        }
    }

    return crc;
}

bool unsigned_save_record_has_magic(const u8 *record) {
    return record != NULL && record[0] == (u8)UNSIGNED_SAVE_MAGIC_0 && record[1] == (u8)UNSIGNED_SAVE_MAGIC_1 && record[2] == (u8)UNSIGNED_SAVE_MAGIC_2 && record[3] == (u8)UNSIGNED_SAVE_MAGIC_3;
}

void unsigned_save_record_build(u8 record[UNSIGNED_SAVE_RECORD_SIZE], const void *data, u16 size, u16 data_version) {
    const u8 *src = data;

    if (record == NULL || size > UNSIGNED_SAVE_MAX_DATA_SIZE || (size != 0u && data == NULL)) {
        return;
    }

    for (u16 i = 0u; i < UNSIGNED_SAVE_RECORD_SIZE; ++i) {
        record[i] = 0u;
    }

    record[0] = (u8)UNSIGNED_SAVE_MAGIC_0;
    record[1] = (u8)UNSIGNED_SAVE_MAGIC_1;
    record[2] = (u8)UNSIGNED_SAVE_MAGIC_2;
    record[3] = (u8)UNSIGNED_SAVE_MAGIC_3;

    save_record_write_u16_be(&record[UNSIGNED_SAVE_FORMAT_OFFSET], UNSIGNED_SAVE_FORMAT_VERSION);
    save_record_write_u16_be(&record[UNSIGNED_SAVE_DATA_VERSION_OFFSET], data_version);
    save_record_write_u16_be(&record[UNSIGNED_SAVE_SIZE_OFFSET], size);

    for (u16 i = 0u; i < size; ++i) {
        record[UNSIGNED_SAVE_PAYLOAD_OFFSET + i] = src[i];
    }

    save_record_write_u16_be(&record[UNSIGNED_SAVE_CRC_OFFSET], save_record_crc(record));
}

USaveState unsigned_save_record_read(const u8 record[UNSIGNED_SAVE_RECORD_SIZE], void *data, u16 capacity, u16 expected_data_version, u16 *out_size) {
    if (record == NULL) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }
    if (!unsigned_save_record_has_magic(record)) {
        return U_SAVE_ERROR_NO_DATA;
    }

    u16 size = save_record_read_u16_be(&record[UNSIGNED_SAVE_SIZE_OFFSET]);
    if (size > UNSIGNED_SAVE_MAX_DATA_SIZE) {
        return U_SAVE_ERROR_CORRUPT;
    }
    if (save_record_read_u16_be(&record[UNSIGNED_SAVE_CRC_OFFSET]) != save_record_crc(record)) {
        return U_SAVE_ERROR_CORRUPT;
    }
    if (save_record_read_u16_be(&record[UNSIGNED_SAVE_FORMAT_OFFSET]) != UNSIGNED_SAVE_FORMAT_VERSION) {
        return U_SAVE_ERROR_CORRUPT;
    }
    if (save_record_read_u16_be(&record[UNSIGNED_SAVE_DATA_VERSION_OFFSET]) != expected_data_version) {
        return U_SAVE_ERROR_VERSION_MISMATCH;
    }

    if (out_size != NULL) {
        *out_size = size;
    }
    if (size > capacity) {
        return U_SAVE_ERROR_BUFFER_TOO_SMALL;
    }
    if (size != 0u && data == NULL) {
        return U_SAVE_ERROR_INVALID_ARGUMENT;
    }

    for (u16 i = 0u; i < size; ++i) {
        ((u8 *)data)[i] = record[UNSIGNED_SAVE_PAYLOAD_OFFSET + i];
    }

    return U_SAVE_OK;
}
