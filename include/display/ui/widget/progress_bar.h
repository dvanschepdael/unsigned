/**
 * @file progress_bar.h
 * @brief Bounded progress-value widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_PROGRESS_BAR_H
#define UNSIGNED_DISPLAY_UI_WIDGET_PROGRESS_BAR_H

#include "display/ui/element.h"

typedef struct UUIProgressBar {
    UUIElement element;
    u16 min_value;
    u16 max_value;
    u16 value;
} UUIProgressBar;

/**
 * @brief Initializes the UI progress bar to a valid empty runtime state.
 *
 * @param progress_bar Progress-bar widget whose bounded value is read or changed.
 * @param bounds Bounds used for layout, culling, collision, or effect calculations.
 * @param style UI style identifier resolved by the active theme.
 * @param min_value Inclusive lower bound.
 * @param max_value Inclusive upper bound.
 * @param value Initial value; clamped to [`min_value`, `max_value`].
 */
void unsigned_ui_progress_bar_init(UUIProgressBar *progress_bar, UUIRect bounds, UUIStyleId style, u16 min_value, u16 max_value, u16 value);

/**
 * @brief Sets value on the UI progress bar.
 *
 * @param progress_bar Progress-bar widget whose bounded value is read or changed.
 * @param value New value; clamped to the progress bar range.
 */
void unsigned_ui_progress_bar_set_value(UUIProgressBar *progress_bar, u16 value);

/**
 * @brief Returns the progress bar value.
 *
 * @param progress_bar Progress-bar widget whose bounded value is read or changed.
 * @return The current bounded value, or 0 when `progress_bar` is NULL.
 */
u16 unsigned_ui_progress_bar_value(const UUIProgressBar *progress_bar);

#endif
