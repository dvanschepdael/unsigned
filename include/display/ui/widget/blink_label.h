/**
 * @file blink_label.h
 * @brief Blinking text label widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_BLINK_LABEL_H
#define UNSIGNED_DISPLAY_UI_WIDGET_BLINK_LABEL_H

#include "display/ui/blink.h"
#include "display/ui/config.h"
#include "display/ui/widget/label.h"

typedef struct UUIBlinkLabel {
    UUILabel label;
    UUIBlink blink;
    const char *visible_text;
    char clear_text[UNSIGNED_UI_BLINK_LABEL_CLEAR_CAPACITY + 1];
} UUIBlinkLabel;

/**
 * @brief Initializes the UI blink label to a valid empty runtime state.
 *
 * @param blink_label Blinking label widget to configure or update.
 * @param bounds Bounds used for layout, culling, collision, or effect calculations.
 * @param style UI style identifier resolved by the active theme.
 * @param text Text content consumed by formatting or rendering.
 * @param interval_frames Number of frames between visibility toggles.
 * @param initially_visible Initial visibility state before the first blink interval elapses.
 * @param clear_width Text width cleared when the blinking label becomes hidden.
 */
void unsigned_ui_blink_label_init(UUIBlinkLabel *blink_label, UUIRect bounds, UUIStyleId style, const char *text, u16 interval_frames, bool initially_visible, u16 clear_width);

/**
 * @brief Sets text on the UI blink label.
 *
 * @param blink_label Blinking label widget to configure or update.
 * @param text Text content consumed by formatting or rendering.
 */
void unsigned_ui_blink_label_set_text(UUIBlinkLabel *blink_label, const char *text);

/**
 * @brief Resets the embedded blink state and immediately applies the requested label visibility.
 *
 * @param blink_label Blinking label widget to configure or update.
 * @param visible Requested visibility state.
 */
void unsigned_ui_blink_label_reset(UUIBlinkLabel *blink_label, bool visible);

/**
 * @brief Advances the UI blink label by one scheduled engine frame.
 *
 * @param blink_label Blinking label widget to configure or update.
 */
void unsigned_ui_blink_label_tick(UUIBlinkLabel *blink_label);

#endif
