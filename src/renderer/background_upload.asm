/*
 * Native Motorola 68000 background ring-buffer upload planner.
 *
 * C ABI contract:
 *   - arguments are passed on the stack by GCC m68k;
 *   - scalar arguments are passed as 32-bit values;
 *   - d0-d1/a0-a1 are call-clobbered;
 *   - d2-d5/a2-a3 are preserved because this routine uses them;
 *   - UBackgroundColumnUploadPlan is the asserted three-byte scalar layout.
 */

#if defined(__m68k__)

    .text

/*
 * u32 unsigned_background_upload_build(
 *     u16 *loaded_source_columns,
 *     UBackgroundColumnUploadPlan *uploads,
 *     u32 columns,
 *     u32 leftmost_slot,
 *     u32 source_column,
 *     u32 source_width);
 */
    .globl unsigned_background_upload_build
unsigned_background_upload_build:
    move.l  %d2,-(%sp)
    move.l  %d3,-(%sp)
    move.l  %d4,-(%sp)
    move.l  %d5,-(%sp)
    move.l  %a2,-(%sp)
    move.l  %a3,-(%sp)

    move.l  28(%sp),%a0
    move.l  32(%sp),%a1
    move.l  36(%sp),%d0
    move.l  40(%sp),%d1
    move.l  44(%sp),%d2
    move.l  48(%sp),%d3

    /* a3 = one-past-end of the active physical ring. */
    move.w  %d0,%d5
    add.w   %d5,%d5
    move.l  %a0,%a3
    adda.w  %d5,%a3

    /* a2 = loaded_source_columns[leftmost_slot]. */
    move.w  %d1,%d5
    add.w   %d5,%d5
    move.l  %a0,%a2
    adda.w  %d5,%a2

    moveq   #0,%d4
    subq.w  #1,%d0

.Lbackground_upload_loop:
    move.w  (%a2),%d5
    cmp.w   %d2,%d5
    beq     .Lbackground_upload_advance

    /* Append { physical_slot, source_column, full }. */
    move.b  %d1,(%a1)+
    move.b  %d2,(%a1)+
    cmpi.w  #-1,%d5
    seq     %d5
    neg.b   %d5
    move.b  %d5,(%a1)+

    move.w  %d2,(%a2)
    addq.w  #1,%d4

.Lbackground_upload_advance:
    addq.w  #1,%d1
    addq.l  #2,%a2
    cmpa.l  %a3,%a2
    bne     .Lbackground_upload_slot_ready
    moveq   #0,%d1
    move.l  %a0,%a2
.Lbackground_upload_slot_ready:

    addq.w  #1,%d2
    cmp.w   %d3,%d2
    bne     .Lbackground_upload_source_ready
    moveq   #0,%d2
.Lbackground_upload_source_ready:

    dbra    %d0,.Lbackground_upload_loop

    moveq   #0,%d0
    move.w  %d4,%d0

    move.l  (%sp)+,%a3
    move.l  (%sp)+,%a2
    move.l  (%sp)+,%d5
    move.l  (%sp)+,%d4
    move.l  (%sp)+,%d3
    move.l  (%sp)+,%d2
    rts

#endif
