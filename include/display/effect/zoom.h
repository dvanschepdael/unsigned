/**
 * @file zoom.h
 * @brief Repeating zoom effect.
 */
#ifndef UNSIGNED_DISPLAY_EFFECT_ZOOM_H
#define UNSIGNED_DISPLAY_EFFECT_ZOOM_H

#include "display/effect/effect.h"

/** Ping-pongs between two sprite scales around a stable pivot. */
typedef struct UZoomEffect {
    u16 min_scale_x;
    u16 min_scale_y;
    u16 max_scale_x;
    u16 max_scale_y;
    u16 pivot_x;
    u16 pivot_y;
} UZoomEffect;

/** Bind a repeating zoom-in/zoom-out effect.
 * @pre `effect` and `zoom` are valid; min/max scales and pivots are in 0..U_EFFECT_SCALE_ONE.
 * @pre Each minimum scale is less than or equal to its matching maximum scale.
 */
void unsigned_effect_set_zoom(UEffect *effect, const UZoomEffect *zoom, u8 speed);

#endif
