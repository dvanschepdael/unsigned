/**
 * @file progress_bar.h
 * @brief Attribute-bound fixed-point progress bar widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_PROGRESS_BAR_H
#define UNSIGNED_DISPLAY_UI_WIDGET_PROGRESS_BAR_H

#include "display/ui/element.h"
#include "gameplay/attribute.h"

/** Q8-style normalized progress: 0 is empty, 256 is completely filled. */
#define U_UI_PROGRESS_BAR_PROGRESS_ONE 256u
#define U_UI_PROGRESS_BAR_TILE_SIZE 8u
#define U_UI_PROGRESS_BAR_HEIGHT 16u

typedef struct UUIProgressBar {
    UUIElement element;
    const UGameplayAttribute *attribute;
    u16 progress;
    u8 palette;
} UUIProgressBar;

/**
 * @brief Initializes a framed progress bar and optionally binds it to a gameplay attribute.
 *
 * @details
 * The visual progression is stored in 0..256 fixed-point units so the Neo Geo renderer can turn
 * it into FIX-tile fill segments without a division every frame. When `attribute` is non-NULL the
 * initial progression is synchronized immediately. Call unsigned_ui_progress_bar_sync() after the
 * bound attribute changes.
 *
 * The bound attribute uses `attribute->min_value->current_value` and
 * `attribute->max_value->current_value` when those pointers are present. A missing minimum defaults
 * to zero and a missing maximum defaults to `attribute->base_value`.
 *
 * @param progress_bar Progress-bar widget to initialize.
 * @param x Left position in pixels, snapped down to the FIX grid when rendered.
 * @param y Top position in pixels, snapped down to the FIX grid when rendered.
 * @param length Total width in 8-pixel tiles, including both corners. Values below 2 become 2.
 * @param palette Neo Geo FIX palette index used by the bar artwork.
 * @param attribute Optional caller-owned gameplay attribute to display.
 */
void unsigned_ui_progress_bar_init(UUIProgressBar *progress_bar, s16 x, s16 y, u8 length, u8 palette, const UGameplayAttribute *attribute);

/**
 * @brief Rebinds the progress bar to another gameplay attribute and synchronizes it immediately.
 *
 * @param progress_bar Progress-bar widget to update.
 * @param attribute Optional caller-owned gameplay attribute; NULL leaves the bar unbound.
 */
void unsigned_ui_progress_bar_set_attribute(UUIProgressBar *progress_bar, const UGameplayAttribute *attribute);

/**
 * @brief Changes the FIX palette used by the progress bar.
 *
 * @param progress_bar Progress-bar widget to update.
 * @param palette Neo Geo FIX palette index; only the low four bits are retained.
 */
void unsigned_ui_progress_bar_set_palette(UUIProgressBar *progress_bar, u8 palette);

/**
 * @brief Sets progress from a zero-based current/maximum pair.
 *
 * @details This only changes the visual progression; it never mutates the bound gameplay attribute.
 *
 * @param progress_bar Progress-bar widget to update.
 * @param current Current amount. Values at or above `maximum` render as full.
 * @param maximum Maximum amount. Zero renders as empty.
 */
void unsigned_ui_progress_bar_set_progress(UUIProgressBar *progress_bar, u16 current, u16 maximum);

/**
 * @brief Sets progress from an arbitrary signed minimum/current/maximum range.
 *
 * @param progress_bar Progress-bar widget to update.
 * @param current Current amount.
 * @param minimum Inclusive amount corresponding to an empty bar.
 * @param maximum Inclusive amount corresponding to a full bar.
 */
void unsigned_ui_progress_bar_set_progress_range(UUIProgressBar *progress_bar, s16 current, s16 minimum, s16 maximum);

/**
 * @brief Sets progress from an integer percentage.
 *
 * @param progress_bar Progress-bar widget to update.
 * @param percent Percentage in [0, 100]; larger values are clamped to 100.
 */
void unsigned_ui_progress_bar_set_percent(UUIProgressBar *progress_bar, u8 percent);

/**
 * @brief Synchronizes the cached visual progression from the bound gameplay attribute.
 *
 * @details This is intentionally explicit so attribute changes do not introduce a division in the
 * renderer every frame. A NULL/unbound attribute produces an empty bar.
 *
 * @param progress_bar Progress-bar widget to synchronize.
 */
void unsigned_ui_progress_bar_sync(UUIProgressBar *progress_bar);

/**
 * @brief Returns normalized visual progress in [0, U_UI_PROGRESS_BAR_PROGRESS_ONE].
 *
 * @param progress_bar Progress-bar widget to query.
 * @return Cached 0..256 progression, or zero for NULL.
 */
u16 unsigned_ui_progress_bar_progress(const UUIProgressBar *progress_bar);

/**
 * @brief Returns the visual progression rounded to an integer percentage.
 *
 * @param progress_bar Progress-bar widget to query.
 * @return Percentage in [0, 100], or zero for NULL.
 */
u8 unsigned_ui_progress_bar_percent(const UUIProgressBar *progress_bar);

#endif
