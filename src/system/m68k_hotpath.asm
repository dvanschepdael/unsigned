/*
 * Native Motorola 68000 hot paths used by the Neo Geo renderer.
 *
 * C ABI contract:
 *   - arguments are passed on the stack by GCC m68k;
 *   - pointers are 32-bit;
 *   - scalar arguments in m68k_hotpath.h are intentionally widened to
 *     u32/s32, making the stack layout stable with or without -mshort;
 *   - d0-d1/a0-a1 are call-clobbered;
 *   - d2-d7/a2-a6 are preserved when used here.
 *
 * VRAM address and modulo policy stays in the C backends. These routines only
 * perform the tight transfer loops.
 */

#if defined(__m68k__)

    .text

/*
 * void unsigned_m68k_vram_stream_strided_asm(
 *     volatile u16 *port, const u16 *source, u32 count, u32 stride_words);
 */
    .globl unsigned_m68k_vram_stream_strided_asm
unsigned_m68k_vram_stream_strided_asm:
    move.l  4(%sp),%a0
    move.l  8(%sp),%a1
    move.l  12(%sp),%d0
    move.l  16(%sp),%d1

    add.w   %d1,%d1
    subq.w  #1,%d0

.Lstream_strided_loop:
    move.w  (%a1),(%a0)
    adda.w  %d1,%a1
    dbra    %d0,.Lstream_strided_loop
    rts

/*
 * void unsigned_m68k_vram_fill_nonzero_asm(
 *     volatile u16 *port, u32 value, u32 count);
 */
    .globl unsigned_m68k_vram_fill_nonzero_asm
unsigned_m68k_vram_fill_nonzero_asm:
    move.l  4(%sp),%a0
    move.l  8(%sp),%d0
    move.l  12(%sp),%d1

    subq.w  #1,%d1

.Lfill_nonzero_loop:
    move.w  %d0,(%a0)
    dbra    %d1,.Lfill_nonzero_loop
    rts

/*
 * void unsigned_m68k_vram_stream_arithmetic_asm(
 *     volatile u16 *port, u32 value, u32 count, s32 step);
 */
    .globl unsigned_m68k_vram_stream_arithmetic_asm
unsigned_m68k_vram_stream_arithmetic_asm:
    move.l  %d2,-(%sp)

    move.l  8(%sp),%a0
    move.l  12(%sp),%d0
    move.l  16(%sp),%d1
    move.l  20(%sp),%d2

    subq.w  #1,%d1

.Lstream_arithmetic_loop:
    move.w  %d0,(%a0)
    add.w   %d2,%d0
    dbra    %d1,.Lstream_arithmetic_loop

    move.l  (%sp)+,%d2
    rts

/*
 * void unsigned_m68k_vram_stream_tile_attributes_asm(
 *     volatile u16 *port, u32 tile, u32 attributes, u32 count, s32 tile_step);
 */
    .globl unsigned_m68k_vram_stream_tile_attributes_asm
unsigned_m68k_vram_stream_tile_attributes_asm:
    move.l  %d2,-(%sp)
    move.l  %d3,-(%sp)

    move.l  12(%sp),%a0
    move.l  16(%sp),%d0
    move.l  20(%sp),%d1
    move.l  24(%sp),%d2
    move.l  28(%sp),%d3

    subq.w  #1,%d2

.Lstream_tile_attributes_loop:
    move.w  %d0,(%a0)
    move.w  %d1,(%a0)
    add.w   %d3,%d0
    dbra    %d2,.Lstream_tile_attributes_loop

    move.l  (%sp)+,%d3
    move.l  (%sp)+,%d2
    rts

/*
 * void unsigned_m68k_vram_fill_pair_asm(
 *     volatile u16 *port, u32 first, u32 second, u32 count);
 */
    .globl unsigned_m68k_vram_fill_pair_asm
unsigned_m68k_vram_fill_pair_asm:
    move.l  %d2,-(%sp)

    move.l  8(%sp),%a0
    move.l  12(%sp),%d0
    move.l  16(%sp),%d1
    move.l  20(%sp),%d2

    subq.w  #1,%d2

.Lfill_pair_loop:
    move.w  %d0,(%a0)
    move.w  %d1,(%a0)
    dbra    %d2,.Lfill_pair_loop

    move.l  (%sp)+,%d2
    rts

#endif
