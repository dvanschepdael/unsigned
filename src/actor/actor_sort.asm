/*
 * Native Motorola 68000 actor sort.
 *
 * C ABI contract:
 *   - arguments are passed on the stack by GCC m68k;
 *   - scalar count/order arguments are passed as 32-bit values;
 *   - d0-d1/a0-a1 are call-clobbered;
 *   - d2-d7/a2-a6 are preserved when used here;
 *   - UActor::position is the first field and Vec2 is { s16 x, s16 y }.
 */

#if defined(__m68k__)

    .text

/*
 * u32 unsigned_actor_sort(UActor **instances, u32 count, u32 order);
 *
 * Stable insertion sort is retained deliberately: the actor list is already
 * sorted from the previous frame, so beat-'em-up depth changes are normally
 * local and the common path performs one neighbour comparison per actor.
 */
    .globl unsigned_actor_sort
unsigned_actor_sort:
    move.l  %d2,-(%sp)
    move.l  %d3,-(%sp)
    move.l  %a2,-(%sp)
    move.l  %a3,-(%sp)
    move.l  %a4,-(%sp)

    move.l  24(%sp),%a0
    move.l  28(%sp),%d2
    move.l  32(%sp),%d1

    moveq   #0,%d0
    subq.w  #2,%d2
    lea     4(%a0),%a1

.Lactor_sort_outer:
    move.l  (%a1),%a2
    move.l  %a1,%a3

.Lactor_sort_inner:
    cmpa.l  %a0,%a3
    beq     .Lactor_sort_place

    move.l  -4(%a3),%a4

    /* Signed Y comparison: previous Y < current Y means already ordered. */
    move.w  2(%a4),%d3
    cmp.w   2(%a2),%d3
    blt     .Lactor_sort_place
    bgt     .Lactor_sort_shift

    /* Equal Y stays stable for Y-only order. */
    tst.w   %d1
    beq     .Lactor_sort_place

    /* Y/X order is also stable: equal X does not move the previous actor. */
    move.w  (%a4),%d3
    cmp.w   (%a2),%d3
    ble     .Lactor_sort_place

.Lactor_sort_shift:
    move.l  %a4,(%a3)
    subq.l  #4,%a3
    moveq   #1,%d0
    bra     .Lactor_sort_inner

.Lactor_sort_place:
    move.l  %a2,(%a3)
    addq.l  #4,%a1
    dbra    %d2,.Lactor_sort_outer

    move.l  (%sp)+,%a4
    move.l  (%sp)+,%a3
    move.l  (%sp)+,%a2
    move.l  (%sp)+,%d3
    move.l  (%sp)+,%d2
    rts

#endif
