/**
 * @file progress_bar.c
 * @brief Implements bounded progress-value widget.
 */

#include "display/ui/widget/progress_bar.h"

#include "core/math/math.h"

void unsigned_ui_progress_bar_init(UUIProgressBar *progress_bar, UUIRect bounds, UUIStyleId style, u16 min_value, u16 max_value, u16 value) {
    if (progress_bar == NULL) {
        return;
    }

    if (max_value < min_value) {
        u16 tmp = min_value;
        min_value = max_value;
        max_value = tmp;
    }

    *progress_bar = (UUIProgressBar){
        .min_value = min_value,
        .max_value = max_value,
        .value = unsigned_math_clamp_u16(value, min_value, max_value),
    };
    unsigned_ui_element_init(&progress_bar->element, U_UI_ELEMENT_PROGRESS_BAR, bounds, style, false);
}

void unsigned_ui_progress_bar_set_value(UUIProgressBar *progress_bar, u16 value) {
    if (progress_bar == NULL) {
        return;
    }

    progress_bar->value = unsigned_math_clamp_u16(value, progress_bar->min_value, progress_bar->max_value);
}

u16 unsigned_ui_progress_bar_value(const UUIProgressBar *progress_bar) {
    if (progress_bar == NULL) {
        return 0;
    }

    return progress_bar->value;
}
