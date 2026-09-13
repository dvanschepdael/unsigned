/**
 * @file progress_bar.c
 * @brief Implements the attribute-bound fixed-point progress bar widget.
 */

#include "display/ui/widget/progress_bar.h"

/** Converts a zero-based unsigned ratio to the progress bar's 0..256 fixed-point range. */
static u16 progress_bar_from_ratio(u16 current, u16 maximum) {
    if (maximum == 0u || current == 0u) {
        return 0u;
    }
    if (current >= maximum) {
        return U_UI_PROGRESS_BAR_PROGRESS_ONE;
    }

    return (u16)(((u32)current * U_UI_PROGRESS_BAR_PROGRESS_ONE) / maximum);
}

/** Converts an arbitrary signed range to the progress bar's 0..256 fixed-point range. */
static u16 progress_bar_from_range(s16 current, s16 minimum, s16 maximum) {
    if (maximum <= minimum || current <= minimum) {
        return 0u;
    }
    if (current >= maximum) {
        return U_UI_PROGRESS_BAR_PROGRESS_ONE;
    }

    const u16 position = (u16)((s32)current - minimum);
    const u16 span = (u16)((s32)maximum - minimum);
    return progress_bar_from_ratio(position, span);
}

void unsigned_ui_progress_bar_init(UUIProgressBar *progress_bar, s16 x, s16 y, u8 length, u8 palette, const UGameplayAttribute *attribute) {
    if (progress_bar == NULL) {
        return;
    }

    *progress_bar = (UUIProgressBar){
        .attribute = attribute,
        .palette = (u8)(palette & 0x0fu),
    };
    const UUIRect bounds = {
        .x = x,
        .y = y,
        .width = (u16)((length < 2u ? 2u : length) * U_UI_PROGRESS_BAR_TILE_SIZE),
        .height = U_UI_PROGRESS_BAR_HEIGHT,
    };
    unsigned_ui_element_init(&progress_bar->element, U_UI_ELEMENT_PROGRESS_BAR, bounds, 0u, false);
    unsigned_ui_progress_bar_sync(progress_bar);
}

void unsigned_ui_progress_bar_set_attribute(UUIProgressBar *progress_bar, const UGameplayAttribute *attribute) {
    if (progress_bar == NULL) {
        return;
    }

    progress_bar->attribute = attribute;
    unsigned_ui_progress_bar_sync(progress_bar);
}

void unsigned_ui_progress_bar_set_palette(UUIProgressBar *progress_bar, u8 palette) {
    if (progress_bar != NULL) {
        progress_bar->palette = (u8)(palette & 0x0fu);
    }
}

void unsigned_ui_progress_bar_set_progress(UUIProgressBar *progress_bar, u16 current, u16 maximum) {
    if (progress_bar != NULL) {
        progress_bar->progress = progress_bar_from_ratio(current, maximum);
    }
}

void unsigned_ui_progress_bar_set_progress_range(UUIProgressBar *progress_bar, s16 current, s16 minimum, s16 maximum) {
    if (progress_bar != NULL) {
        progress_bar->progress = progress_bar_from_range(current, minimum, maximum);
    }
}

void unsigned_ui_progress_bar_set_percent(UUIProgressBar *progress_bar, u8 percent) {
    if (progress_bar == NULL) {
        return;
    }

    if (percent >= 100u) {
        progress_bar->progress = U_UI_PROGRESS_BAR_PROGRESS_ONE;
        return;
    }

    progress_bar->progress = (u16)(((u16)percent * U_UI_PROGRESS_BAR_PROGRESS_ONE + 50u) / 100u);
}

void unsigned_ui_progress_bar_sync(UUIProgressBar *progress_bar) {
    if (progress_bar == NULL) {
        return;
    }
    if (progress_bar->attribute == NULL) {
        progress_bar->progress = 0u;
        return;
    }

    const UGameplayAttribute *attribute = progress_bar->attribute;
    const s16 minimum = attribute->min_value != NULL ? attribute->min_value->current_value : 0;
    const s16 maximum = attribute->max_value != NULL ? attribute->max_value->current_value : attribute->base_value;
    progress_bar->progress = progress_bar_from_range(attribute->current_value, minimum, maximum);
}

u16 unsigned_ui_progress_bar_progress(const UUIProgressBar *progress_bar) {
    return progress_bar != NULL ? progress_bar->progress : 0u;
}

u8 unsigned_ui_progress_bar_percent(const UUIProgressBar *progress_bar) {
    if (progress_bar == NULL) {
        return 0u;
    }

    return (u8)(((u32)progress_bar->progress * 100u + (U_UI_PROGRESS_BAR_PROGRESS_ONE / 2u)) / U_UI_PROGRESS_BAR_PROGRESS_ONE);
}
