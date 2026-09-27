/**
 * @file vram_writer.c
 * @brief Implements batched Neo Geo renderer transaction and VRAMMOD cache.
 */

#include "system/renderer_backend.h"
#include "system/vram_writer.h"

#include <ngdevkit/registers.h>

typedef struct UNeoGeoVramWriterState {
    u16 mod;
    bool mod_valid;
} UNeoGeoVramWriterState;

static UNeoGeoVramWriterState vram_writer;
void unsigned_system_renderer_begin(void) {
    vram_writer.mod_valid = false;
}

void unsigned_system_renderer_end(void) {
    vram_writer.mod_valid = false;
}

void unsigned_system_vram_set_mod(u16 mod) {
    if (!vram_writer.mod_valid || vram_writer.mod != mod) {
        *REG_VRAMMOD = mod;
        vram_writer.mod = mod;
        vram_writer.mod_valid = true;
    }
}
