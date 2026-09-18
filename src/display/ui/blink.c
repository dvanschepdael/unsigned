/**
 * @file blink.c
 * @brief Implements frame-based visibility blinking helper.
 */

#include "display/ui/blink.h"

void unsigned_ui_blink_init(UUIBlink *blink, u16 interval_frames, bool initially_visible) {
    if (blink == NULL) {
        return;
    }

    *blink = (UUIBlink){
        .interval_frames = interval_frames,
        .visible = initially_visible,
    };
}

void unsigned_ui_blink_reset(UUIBlink *blink, bool visible) {
    if (blink == NULL) {
        return;
    }

    blink->elapsed_frames = 0u;
    blink->visible = visible;
}

void unsigned_ui_blink_tick(UUIBlink *blink) {
    if (blink == NULL || blink->interval_frames == 0u) {
        return;
    }

    ++blink->elapsed_frames;
    if (blink->elapsed_frames >= blink->interval_frames) {
        blink->elapsed_frames = 0u;
        blink->visible = !blink->visible;
    }
}

bool unsigned_ui_blink_is_visible(const UUIBlink *blink) {
    return blink != NULL && blink->visible;
}
