#include "display/effect/blink.h"

#include "display/effect/effect_internal.h"

static void blink_sample(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UBlinkEffect *blink = context;
    const u16 cycle = (u16)blink->visible_phase + blink->hidden_phase;
    (void)column;
    (void)column_count;

    if (blink->hidden_phase == 0u) {
        return;
    }
    if (blink->visible_phase == 0u || (u16)(phase % cycle) >= blink->visible_phase) {
        sample->flags |= U_EFFECT_SAMPLE_HIDDEN;
    }
}

void unsigned_effect_set_blink(UEffect *effect, const UBlinkEffect *blink, u8 speed) {
    unsigned_effect_bind(effect, blink_sample, blink, speed, U_EFFECT_LAYOUT_UNIFORM, unsigned_effect_zero_bounds);
}
