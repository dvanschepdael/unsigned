/*
 * Native Motorola 68000 sampled-effect composition hot path.
 *
 * C ABI contract:
 *   - arguments are passed on the stack by GCC m68k;
 *   - a0-a1/d0-d1 are call-clobbered;
 *   - a2 is preserved because this routine uses it for the third pointer;
 *   - UEffectSample field offsets are asserted by effect_compose.h.
 */

#if defined(__m68k__)

    .text

/*
 * void unsigned_effect_compose(
 *     UEffectSample *composed,
 *     const UEffectSample *first,
 *     const UEffectSample *second);
 */
    .globl unsigned_effect_compose
unsigned_effect_compose:
    move.l  %a2,-(%sp)

    move.l  8(%sp),%a0
    move.l  12(%sp),%a1
    move.l  16(%sp),%a2

    /* Saturating signed offset_x addition. */
    move.w  0(%a1),%d0
    add.w   0(%a2),%d0
    bvc     .Leffect_offset_x_store
    bmi     .Leffect_offset_x_positive_overflow
    move.w  #0x8000,%d0
    bra     .Leffect_offset_x_store
.Leffect_offset_x_positive_overflow:
    move.w  #0x7fff,%d0
.Leffect_offset_x_store:
    move.w  %d0,0(%a0)

    /* Saturating signed offset_y addition. */
    move.w  2(%a1),%d0
    add.w   2(%a2),%d0
    bvc     .Leffect_offset_y_store
    bmi     .Leffect_offset_y_positive_overflow
    move.w  #0x8000,%d0
    bra     .Leffect_offset_y_store
.Leffect_offset_y_positive_overflow:
    move.w  #0x7fff,%d0
.Leffect_offset_y_store:
    move.w  %d0,2(%a0)

    /* Saturating signed zoom_offset addition. */
    move.w  4(%a1),%d0
    add.w   4(%a2),%d0
    bvc     .Leffect_zoom_store
    bmi     .Leffect_zoom_positive_overflow
    move.w  #0x8000,%d0
    bra     .Leffect_zoom_store
.Leffect_zoom_positive_overflow:
    move.w  #0x7fff,%d0
.Leffect_zoom_store:
    move.w  %d0,4(%a0)

    /* Q8 scale_x composition; 256 is the identity and avoids MULU. */
    move.w  6(%a1),%d0
    cmpi.w  #256,%d0
    beq     .Leffect_scale_x_first_identity
    move.w  6(%a2),%d1
    cmpi.w  #256,%d1
    beq     .Leffect_scale_x_store_first
    mulu.w  %d1,%d0
    addi.l  #128,%d0
    lsr.l   #8,%d0
.Leffect_scale_x_store_first:
    move.w  %d0,6(%a0)
    bra     .Leffect_scale_y
.Leffect_scale_x_first_identity:
    move.w  6(%a2),6(%a0)

.Leffect_scale_y:
    /* Q8 scale_y composition; 256 is the identity and avoids MULU. */
    move.w  8(%a1),%d0
    cmpi.w  #256,%d0
    beq     .Leffect_scale_y_first_identity
    move.w  8(%a2),%d1
    cmpi.w  #256,%d1
    beq     .Leffect_scale_y_store_first
    mulu.w  %d1,%d0
    addi.l  #128,%d0
    lsr.l   #8,%d0
.Leffect_scale_y_store_first:
    move.w  %d0,8(%a0)
    bra     .Leffect_pivots
.Leffect_scale_y_first_identity:
    move.w  8(%a2),8(%a0)

.Leffect_pivots:
    /* The second sample owns the pivot iff it changes either scale axis. */
    cmpi.w  #256,6(%a2)
    bne     .Leffect_second_pivots
    cmpi.w  #256,8(%a2)
    bne     .Leffect_second_pivots
    move.w  10(%a1),10(%a0)
    move.w  12(%a1),12(%a0)
    bra     .Leffect_flags
.Leffect_second_pivots:
    move.w  10(%a2),10(%a0)
    move.w  12(%a2),12(%a0)

.Leffect_flags:
    /* FLIP_X/FLIP_Y compose by XOR; HIDDEN composes by OR. */
    moveq   #0,%d0
    moveq   #0,%d1
    move.b  14(%a1),%d0
    move.b  14(%a2),%d1
    eor.b   %d1,%d0
    andi.b  #0x03,%d0
    move.b  14(%a1),%d1
    or.b    14(%a2),%d1
    andi.b  #0x04,%d1
    or.b    %d1,%d0
    move.b  %d0,14(%a0)

    move.l  (%sp)+,%a2
    rts

#endif
