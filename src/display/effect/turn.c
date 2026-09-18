#include "display/effect/turn.h"

#include "display/effect/effect_internal.h"

/** Map one 64-phase quarter-turn to 0..256 without runtime division. */
static u16 turn_edge_progress_q8(u8 local) {
    return local >= 63u ? U_EFFECT_SCALE_ONE : (u16)((u16)local << 2u);
}

static u16 turn_scale(u16 minimum, u8 phase) {
    const u8 quadrant = phase >> 6u;
    const u8 local = phase & 63u;
    const u8 toward_edge = (quadrant == 0u || quadrant == 2u) ? local : (u8)(63u - local);
    const u16 range = (u16)(U_EFFECT_SCALE_ONE - minimum);
    const u16 progress = turn_edge_progress_q8(toward_edge);

    return (u16)(U_EFFECT_SCALE_ONE - (((u32)range * progress + 128u) >> 8u));
}

static void turn_sample(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UTurnEffect *turn = context;
    const u16 minimum = unsigned_effect_clamp_q8(turn->min_scale);
    const u16 scale = turn_scale(minimum, phase);
    const u8 quadrant = phase >> 6u;
    const bool mirrored = quadrant == 1u || quadrant == 2u;
    (void)column;
    (void)column_count;

    sample->pivot_x = unsigned_effect_clamp_q8(turn->pivot_x);
    sample->pivot_y = unsigned_effect_clamp_q8(turn->pivot_y);
    if (turn->axis == U_TURN_AXIS_X) {
        sample->scale_y = scale;
        if (mirrored) {
            sample->flags |= U_EFFECT_SAMPLE_FLIP_Y;
        }
    } else {
        sample->scale_x = scale;
        if (mirrored) {
            sample->flags |= U_EFFECT_SAMPLE_FLIP_X;
        }
    }
}

void unsigned_effect_set_turn(UEffect *effect, const UTurnEffect *turn, u8 speed) {
    unsigned_effect_bind(effect, turn_sample, turn, speed, U_EFFECT_LAYOUT_UNIFORM, unsigned_effect_zero_bounds);
}
