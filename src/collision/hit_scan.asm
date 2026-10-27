/*
 * Native Motorola 68000 actor hit candidate scan.
 *
 * C ABI contract:
 *   - arguments are passed on the stack by GCC m68k;
 *   - scalar count/mask/offset arguments are 32-bit;
 *   - d0-d1/a0-a1 are call-clobbered;
 *   - d2-d7/a2-a3 are preserved because this routine uses them;
 *   - UCollisionBox begins with signed 16-bit x/y/w/h words;
 *   - UCollisionHit is two 32-bit actor pointers, asserted by the C header.
 */

#if defined(__m68k__)

    .text

/*
 * u32 unsigned_collision_hit_scan(
 *     UCollisionHit *hits,
 *     UActor *const *targets,
 *     u32 count,
 *     UActor *attacker,
 *     u32 target_mask,
 *     const UCollisionBox *hitbox,
 *     u32 hurtbox_offset,
 *     u32 hurtbox_channel_offset);
 */
    .globl unsigned_collision_hit_scan
unsigned_collision_hit_scan:
    move.l  %d2,-(%sp)
    move.l  %d3,-(%sp)
    move.l  %d4,-(%sp)
    move.l  %d5,-(%sp)
    move.l  %d6,-(%sp)
    move.l  %d7,-(%sp)
    move.l  %a2,-(%sp)
    move.l  %a3,-(%sp)

    move.l  36(%sp),%a0
    move.l  %a0,%a3
    move.l  40(%sp),%a1
    move.l  44(%sp),%d6
    move.l  52(%sp),%d7

    /* Precompute the attack rectangle as signed 32-bit edges. */
    move.l  56(%sp),%a2
    move.w  0(%a2),%d2
    ext.l   %d2
    move.l  %d2,%d3
    move.w  4(%a2),%d0
    ext.l   %d0
    add.l   %d0,%d3

    move.w  2(%a2),%d4
    ext.l   %d4
    move.l  %d4,%d5
    move.w  6(%a2),%d0
    ext.l   %d0
    add.l   %d0,%d5

    subq.w  #1,%d6

.Lhit_scan_loop:
    move.l  (%a1)+,%d0
    cmp.l   48(%sp),%d0
    beq     .Lhit_scan_next

    /* Reject actors without a compatible hurtbox channel. */
    move.l  %d0,%a2
    adda.l  64(%sp),%a2
    moveq   #0,%d1
    move.b  (%a2),%d1
    cmpi.b  #16,%d1
    beq     .Lhit_scan_next
    btst    %d1,%d7
    beq     .Lhit_scan_next

    /* a2 = target hurtbox, using the C-supplied UActor offset. */
    move.l  %d0,%a2
    adda.l  60(%sp),%a2

    /* other_right <= hit_left */
    move.w  0(%a2),%d0
    ext.l   %d0
    move.w  4(%a2),%d1
    ext.l   %d1
    add.l   %d1,%d0
    cmp.l   %d2,%d0
    ble     .Lhit_scan_next

    /* other_left >= hit_right */
    move.w  0(%a2),%d0
    ext.l   %d0
    cmp.l   %d3,%d0
    bge     .Lhit_scan_next

    /* other_bottom <= hit_top */
    move.w  2(%a2),%d0
    ext.l   %d0
    move.w  6(%a2),%d1
    ext.l   %d1
    add.l   %d1,%d0
    cmp.l   %d4,%d0
    ble     .Lhit_scan_next

    /* other_top >= hit_bottom */
    move.w  2(%a2),%d0
    ext.l   %d0
    cmp.l   %d5,%d0
    bge     .Lhit_scan_next

    /* Preserve traversal order exactly: append { attacker, target }. */
    move.l  48(%sp),%d0
    move.l  %d0,(%a0)+
    move.l  -4(%a1),%d0
    move.l  %d0,(%a0)+

.Lhit_scan_next:
    dbra    %d6,.Lhit_scan_loop

    /* Two pointers (8 bytes) were written per hit. */
    move.l  %a0,%d0
    sub.l   %a3,%d0
    lsr.l   #3,%d0

    move.l  (%sp)+,%a3
    move.l  (%sp)+,%a2
    move.l  (%sp)+,%d7
    move.l  (%sp)+,%d6
    move.l  (%sp)+,%d5
    move.l  (%sp)+,%d4
    move.l  (%sp)+,%d3
    move.l  (%sp)+,%d2
    rts

#endif
