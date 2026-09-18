/**
 * @file blink.h
 * @brief Frame-based visibility blinking helper.
 */

#ifndef UNSIGNED_DISPLAY_UI_BLINK_H
#define UNSIGNED_DISPLAY_UI_BLINK_H

#include "core/types.h"

typedef struct UUIBlink {
    u16 interval_frames;
    u16 elapsed_frames;
    bool visible;
} UUIBlink;

/**
 * @brief Initializes the UI blink to a valid empty runtime state.
 *
 * @param blink Blink timer/state controlling periodic visibility.
 * @param interval_frames Number of frames between visibility toggles.
 * @param initially_visible Initial visibility state before the first blink interval elapses.
 */
void unsigned_ui_blink_init(UUIBlink *blink, u16 interval_frames, bool initially_visible);

/**
 * @brief Resets the blink phase and immediately applies the requested visibility state.
 *
 * @param blink Blink timer/state controlling periodic visibility.
 * @param visible Requested visibility state.
 */
void unsigned_ui_blink_reset(UUIBlink *blink, bool visible);

/**
 * @brief Advances the UI blink by one scheduled engine frame.
 *
 * @param blink Blink timer/state controlling periodic visibility.
 */
void unsigned_ui_blink_tick(UUIBlink *blink);

/**
 * @brief Returns whether the UI blink is visible.
 *
 * @param blink Blink timer/state controlling periodic visibility.
 * @return true while the blink phase says the owning UI element should currently be drawn; false otherwise.
 */
bool unsigned_ui_blink_is_visible(const UUIBlink *blink);

#endif
