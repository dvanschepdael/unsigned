/**
 * @file blink_label.h
 * @brief Blinking text label widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_BLINK_LABEL_H
#define UNSIGNED_DISPLAY_UI_WIDGET_BLINK_LABEL_H

#include "display/ui/blink.h"
#include "display/ui/config.h"
#include "display/ui/widget/label.h"

typedef struct UUIBlinkLabelConfig {
    UUIRect bounds;
    UUIStyleId style;
    const char *text;
    u16 interval_frames;
    u16 clear_width;
    bool initially_visible;
} UUIBlinkLabelConfig;

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
 * @param config Immutable widget configuration.
 * @pre `blink_label`, `config`, and `config->text` are valid.
 * @pre `config->clear_width <= UNSIGNED_UI_BLINK_LABEL_CLEAR_CAPACITY`.
 */
void unsigned_ui_blink_label_init(UUIBlinkLabel *blink_label, const UUIBlinkLabelConfig *config);

/**
 * @brief Sets text on the UI blink label and refreshes its visible/clear presentation when changed.
 *
 * Reapplying the same text pointer is a no-op; blink ticks/reset remain responsible for visibility.
 *
 * @param blink_label Blinking label widget to configure or update.
 * @param text Text content consumed by formatting or rendering.
 * @pre `blink_label` and `text` are valid.
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
