#include "display/effect/zoom.h"

#include "display/effect/effect_internal.h"

/** Linear interpolation with Q8 progress; 256 reaches the target exactly. */
static u16 zoom_lerp_q8(u16 from, u16 to, u16 progress_q8) {
    const s32 delta = (s32)to - from;
    const u32 magnitude = delta < 0 ? (u32)(-delta) : (u32)delta;
    const u16 step = (u16)((magnitude * progress_q8 + 128u) >> 8u);

    return delta < 0 ? (u16)(from - step) : (u16)(from + step);
}

/** Return a 0..256..0 triangle over the complete 8-bit effect phase. */
static u16 zoom_ping_pong_q8(u8 phase) {
    if (phase == 127u || phase == 128u) {
        return U_EFFECT_SCALE_ONE;
    }
    return phase < 128u ? (u16)phase << 1u : (u16)(255u - phase) << 1u;
}

static void zoom_sample(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UZoomEffect *zoom = context;
    const u16 progress = zoom_ping_pong_q8(phase);
    (void)column;
    (void)column_count;

    sample->scale_x = zoom_lerp_q8(unsigned_effect_clamp_q8(zoom->min_scale_x),
                                   unsigned_effect_clamp_q8(zoom->max_scale_x), progress);
    sample->scale_y = zoom_lerp_q8(unsigned_effect_clamp_q8(zoom->min_scale_y),
                                   unsigned_effect_clamp_q8(zoom->max_scale_y), progress);
    sample->pivot_x = unsigned_effect_clamp_q8(zoom->pivot_x);
    sample->pivot_y = unsigned_effect_clamp_q8(zoom->pivot_y);
}

void unsigned_effect_set_zoom(UEffect *effect, const UZoomEffect *zoom, u8 speed) {
    unsigned_effect_bind(effect, zoom_sample, zoom, speed, U_EFFECT_LAYOUT_UNIFORM, unsigned_effect_zero_bounds);
}
