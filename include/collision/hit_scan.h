/**
 * @file hit_scan.h
 * @brief Target-specific inner loop for actor hitbox/hurtbox overlap scans.
 */

#ifndef UNSIGNED_COLLISION_HIT_SCAN_H
#define UNSIGNED_COLLISION_HIT_SCAN_H

#include "actor/actor.h"
#include "collision/hit_detection.h"

/*
 * Low-level candidate scan boundary. Production links hit_scan.asm;
 * host tests link test/mock/hit_scan.c. Scalar arguments are widened
 * to 32 bits so the target ABI is independent of -mshort. Actor field offsets
 * are supplied by C, keeping the assembly independent of the large UActor
 * layout while retaining one tight candidate loop.
 *
 * @pre 0 < count <= 65536 and every target pointer is valid.
 * @pre hits has room for every overlap produced by this candidate span.
 * @return number of UCollisionHit entries appended to hits.
 */
u32 unsigned_collision_hit_scan(UCollisionHit *hits, UActor *const *targets, u32 count, UActor *attacker, u32 target_mask, const UCollisionBox *hitbox, u32 hurtbox_offset, u32 hurtbox_channel_offset);

#if defined(__m68k__)
/* hit_scan.asm streams UCollisionHit as two 32-bit actor pointers. */
typedef char UCollisionHitAsmAttackerOffsetMustBeZero[(__builtin_offsetof(UCollisionHit, attacker) == 0u) ? 1 : -1];
typedef char UCollisionHitAsmTargetOffsetMustBeFour[(__builtin_offsetof(UCollisionHit, target) == 4u) ? 1 : -1];
typedef char UCollisionHitAsmSizeMustBeEight[(sizeof(UCollisionHit) == 8u) ? 1 : -1];
typedef char UCollisionChannelCountAsmMustBeSixteen[(U_COLLISION_CHANNEL_COUNT == 16u) ? 1 : -1];
typedef char UCollisionBoxAsmXOffsetMustBeZero[(__builtin_offsetof(UCollisionBox, x) == 0u) ? 1 : -1];
typedef char UCollisionBoxAsmYOffsetMustBeTwo[(__builtin_offsetof(UCollisionBox, y) == 2u) ? 1 : -1];
typedef char UCollisionBoxAsmWOffsetMustBeFour[(__builtin_offsetof(UCollisionBox, w) == 4u) ? 1 : -1];
typedef char UCollisionBoxAsmHOffsetMustBeSix[(__builtin_offsetof(UCollisionBox, h) == 6u) ? 1 : -1];
#endif

#endif
