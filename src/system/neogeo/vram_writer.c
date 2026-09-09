/**
 * @file vram_writer.c
 * @brief Implements batched Neo Geo renderer transaction and VRAMMOD cache.
 */

#include "system/neogeo/vram_writer_internal.h"
#include "system/renderer/renderer_backend.h"

#include <ngdevkit/registers.h>

typedef struct UNeoGeoVramWriterState {
    u16 mod;
    bool batch_active;
    bool mod_valid;
} UNeoGeoVramWriterState;

static UNeoGeoVramWriterState vram_writer;
void unsigned_renderer_backend_begin(void) {
    vram_writer.batch_active = true;
    vram_writer.mod_valid = false;
}

void unsigned_renderer_backend_end(void) {
    vram_writer.batch_active = false;
    vram_writer.mod_valid = false;
}

void unsigned_neogeo_vram_set_mod(u16 mod) {
    if (!vram_writer.batch_active || !vram_writer.mod_valid || vram_writer.mod != mod) {
        *REG_VRAMMOD = mod;
    }

    if (vram_writer.batch_active) {
        vram_writer.mod = mod;
        vram_writer.mod_valid = true;
    }
}
