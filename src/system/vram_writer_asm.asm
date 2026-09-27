/*
 * Motorola 68000 implementation of the VRAM writer hot paths.
 *
 * Declared through vram_writer.h. VRAM address/modulo policy stays in
 * the C callers; this file owns only the tight data-port loops shared by the
 * Neo Geo renderer backends.
 */

    .text

/*
 * void unsigned_system_vram_fill_asm(
 *     volatile u16 *port, u32 value, u32 count);
 */
    .globl unsigned_system_vram_fill_asm
unsigned_system_vram_fill_asm:
    move.l  4(%sp),%a0
    move.l  8(%sp),%d0
    move.l  12(%sp),%d1

    /* Two writes per DBRA; count is non-zero by contract. */
    lsr.w   #1,%d1
    bcc     .Lfill_nonzero_even
    move.w  %d0,(%a0)
.Lfill_nonzero_even:
    subq.w  #1,%d1
    bmi     .Lfill_nonzero_done

.Lfill_nonzero_loop:
    move.w  %d0,(%a0)
    move.w  %d0,(%a0)
    dbra    %d1,.Lfill_nonzero_loop
.Lfill_nonzero_done:
    rts

/*
 * void unsigned_system_vram_fill_pair_asm(
 *     volatile u16 *port, u32 first, u32 second, u32 count);
 */
    .globl unsigned_system_vram_fill_pair_asm
unsigned_system_vram_fill_pair_asm:
    move.l  %d2,-(%sp)

    move.l  8(%sp),%a0
    move.l  12(%sp),%d0
    move.l  16(%sp),%d1
    move.l  20(%sp),%d2

    /* Two logical pairs per DBRA; count is non-zero by contract. */
    lsr.w   #1,%d2
    bcc     .Lfill_pair_even
    move.w  %d0,(%a0)
    move.w  %d1,(%a0)
.Lfill_pair_even:
    subq.w  #1,%d2
    bmi     .Lfill_pair_done

.Lfill_pair_loop:
    move.w  %d0,(%a0)
    move.w  %d1,(%a0)
    move.w  %d0,(%a0)
    move.w  %d1,(%a0)
    dbra    %d2,.Lfill_pair_loop

.Lfill_pair_done:
    move.l  (%sp)+,%d2
    rts

/*
 * void unsigned_system_vram_stream_arithmetic_asm(
 *     volatile u16 *port, u32 value, u32 count, s32 step);
 */
    .globl unsigned_system_vram_stream_arithmetic_asm
unsigned_system_vram_stream_arithmetic_asm:
    move.l  %d2,-(%sp)

    move.l  8(%sp),%a0
    move.l  12(%sp),%d0
    move.l  16(%sp),%d1
    move.l  20(%sp),%d2

    /* Two sequence elements per DBRA; count is non-zero by contract. */
    lsr.w   #1,%d1
    bcc     .Lstream_arithmetic_even
    move.w  %d0,(%a0)
    add.w   %d2,%d0
.Lstream_arithmetic_even:
    subq.w  #1,%d1
    bmi     .Lstream_arithmetic_done

.Lstream_arithmetic_loop:
    move.w  %d0,(%a0)
    add.w   %d2,%d0
    move.w  %d0,(%a0)
    add.w   %d2,%d0
    dbra    %d1,.Lstream_arithmetic_loop

.Lstream_arithmetic_done:
    move.l  (%sp)+,%d2
    rts

/*
 * void unsigned_system_vram_stream_strided_asm(
 *     volatile u16 *port, const u16 *source, u32 count, u32 stride_words);
 */
    .globl unsigned_system_vram_stream_strided_asm
unsigned_system_vram_stream_strided_asm:
    move.l  4(%sp),%a0
    move.l  8(%sp),%a1
    move.l  12(%sp),%d0
    move.l  16(%sp),%d1

    add.w   %d1,%d1

    /* Two transfers per DBRA. The odd head preserves source order. */
    lsr.w   #1,%d0
    bcc     .Lstream_strided_even
    move.w  (%a1),(%a0)
    adda.w  %d1,%a1
.Lstream_strided_even:
    subq.w  #1,%d0
    bmi     .Lstream_strided_done

.Lstream_strided_loop:
    move.w  (%a1),(%a0)
    adda.w  %d1,%a1
    move.w  (%a1),(%a0)
    adda.w  %d1,%a1
    dbra    %d0,.Lstream_strided_loop
.Lstream_strided_done:
    rts

/*
 * void unsigned_system_vram_stream_tiles_asm(
 *     volatile u16 *port, u32 tile, u32 attributes, u32 count, s32 tile_step);
 */
    .globl unsigned_system_vram_stream_tiles_asm
unsigned_system_vram_stream_tiles_asm:
    move.l  %d2,-(%sp)
    move.l  %d3,-(%sp)

    move.l  12(%sp),%a0
    move.l  16(%sp),%d0
    move.l  20(%sp),%d1
    move.l  24(%sp),%d2
    move.l  28(%sp),%d3

    /* Two tile/attribute pairs per DBRA; count is non-zero by contract. */
    lsr.w   #1,%d2
    bcc     .Lstream_tile_attributes_even
    move.w  %d0,(%a0)
    move.w  %d1,(%a0)
    add.w   %d3,%d0
.Lstream_tile_attributes_even:
    subq.w  #1,%d2
    bmi     .Lstream_tile_attributes_done

.Lstream_tile_attributes_loop:
    move.w  %d0,(%a0)
    move.w  %d1,(%a0)
    add.w   %d3,%d0
    move.w  %d0,(%a0)
    move.w  %d1,(%a0)
    add.w   %d3,%d0
    dbra    %d2,.Lstream_tile_attributes_loop

.Lstream_tile_attributes_done:
    move.l  (%sp)+,%d3
    move.l  (%sp)+,%d2
    rts

