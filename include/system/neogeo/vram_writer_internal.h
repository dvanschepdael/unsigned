/**
 * @file vram_writer_internal.h
 * @brief Neo Geo-only helper for batching VRAMMOD register writes.
 */

#ifndef UNSIGNED_SYSTEM_NEO_GEO_VRAM_WRITER_INTERNAL_H
#define UNSIGNED_SYSTEM_NEO_GEO_VRAM_WRITER_INTERNAL_H

#include "core/types.h"

/**
 * @brief Sets mod on the neogeo VRAM.
 *
 * @param mod Neo Geo VRAMMOD value cached/written for subsequent VRAM accesses.
 */
void unsigned_neogeo_vram_set_mod(u16 mod);

#endif
