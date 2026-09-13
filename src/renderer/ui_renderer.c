/**
 * @file ui_renderer.c
 * @brief Binds generic UI draw callbacks to the Neo Geo FIX-layer backend.
 */

#include "renderer/ui_renderer.h"

#include "display/ui/widget/label.h"
#include "display/ui/widget/progress_bar.h"
#include "display/ui/widget/selector.h"
#include "system/fix.h"

#include <stdio.h>

/** Converts a UI pixel-space Y coordinate to the corresponding Neo Geo FIX row. */
static u8 neo_geo_ui_row(const UUIElement *element) {
    return element != NULL ? (u8)(element->bounds.y / 8) : 0u;
}

/** Draws a centered label on the FIX layer, selecting normal or tall text from the configured style id. */
static void neo_geo_ui_draw_label(void *context, const UUITheme *theme, const UUILabel *label) {
    UNeoGeoUIRenderer *backend = context;
    (void)theme;

    if (backend == NULL || label == NULL || label->text == NULL) {
        return;
    }

    if (label->element.style == backend->config.tall_label_style) {
        unsigned_neo_geo_fix_center_text_tall(neo_geo_ui_row(&label->element), backend->config.palette, label->text);
    } else {
        unsigned_neo_geo_fix_center_text(neo_geo_ui_row(&label->element), backend->config.palette, label->text);
    }
}

/** Formats the selector label/current option and draws focus arrows around the active value on the FIX layer. */
static void neo_geo_ui_draw_selector(void *context, const UUITheme *theme, const UUISelector *selector) {
    UNeoGeoUIRenderer *backend = context;
    (void)theme;

    if (backend == NULL || selector == NULL) {
        return;
    }

    const char *selected = unsigned_ui_selector_selected_text(selector);
    if (selected == NULL) {
        selected = "";
    }
    int width = backend->config.selector_option_width == 0u ? 12 : (int)backend->config.selector_option_width;

    (void)snprintf(backend->text, sizeof(backend->text), selector->element.focused ? "%s  < %-*s >" : "%s    %-*s  ", selector->label != NULL ? selector->label : "", width, selected);

    unsigned_neo_geo_fix_center_text(neo_geo_ui_row(&selector->element), backend->config.palette, backend->text);
}

/** Draws two rows of precomposed frame/fill tiles; clipping preserves the original progress. */
static void neo_geo_ui_draw_progress_bar(void *context, const UUITheme *theme, const UUIProgressBar *progress_bar) {
    UNeoGeoUIRenderer *backend = context;
    (void)theme;

    if (backend == NULL || progress_bar == NULL || backend->config.progress_bar_tile_base == 0u || backend->config.progress_bar_tile_base > 0x1000u - U_NEO_GEO_UI_PROGRESS_TILE_COUNT) {
        return;
    }

    const UUIRect *bounds = &progress_bar->element.bounds;
    const u16 columns = bounds->width / U_UI_PROGRESS_BAR_TILE_SIZE;
    if (columns < 2u || bounds->height < U_UI_PROGRESS_BAR_HEIGHT || bounds->x < 0 || bounds->y < 0) {
        return;
    }
    const u16 x = (u16)bounds->x / U_UI_PROGRESS_BAR_TILE_SIZE;
    const u16 y = (u16)bounds->y / U_UI_PROGRESS_BAR_TILE_SIZE;
    if (x >= UNSIGNED_NEO_GEO_FIX_COLUMNS || y >= UNSIGNED_NEO_GEO_FIX_ROWS) {
        return;
    }

    /* Fill starts at x=1 and includes the final column (x=23 in the source bar). */
    const u16 interior_width = (u16)(columns * U_UI_PROGRESS_BAR_TILE_SIZE - U_NEO_GEO_UI_PROGRESS_INSET_LEFT - U_NEO_GEO_UI_PROGRESS_INSET_RIGHT);
    u16 progress = unsigned_ui_progress_bar_progress(progress_bar);
    if (progress > U_UI_PROGRESS_BAR_PROGRESS_ONE) {
        progress = U_UI_PROGRESS_BAR_PROGRESS_ONE;
    }
    const u16 fill_end = (u16)(U_NEO_GEO_UI_PROGRESS_INSET_LEFT + (((u32)interior_width * progress + U_UI_PROGRESS_BAR_PROGRESS_ONE / 2u) >> 8u));
    const u16 available_columns = (u16)(UNSIGNED_NEO_GEO_FIX_COLUMNS - x);
    const u16 drawable = columns < available_columns ? columns : available_columns;
    u8 tiles[UNSIGNED_NEO_GEO_FIX_COLUMNS];

    for (u8 row = 0u; row < 2u && y + row < UNSIGNED_NEO_GEO_FIX_ROWS; ++row) {
        for (u16 column = 0u; column < drawable; ++column) {
            const u8 part = column == 0u ? 0u : (column == columns - 1u ? 2u : 1u);
            const u16 left = (u16)(column * U_UI_PROGRESS_BAR_TILE_SIZE);
            u16 fill = fill_end > left ? (u16)(fill_end - left) : 0u;
            if (fill > U_UI_PROGRESS_BAR_TILE_SIZE) {
                fill = U_UI_PROGRESS_BAR_TILE_SIZE;
            }
            tiles[column] = (u8)(((row * 3u) + part) * U_NEO_GEO_UI_PROGRESS_VARIANTS + fill);
        }
        unsigned_neo_geo_fix_draw_tile_strip((u8)x, (u8)(y + row), progress_bar->palette, backend->config.progress_bar_tile_base, tiles, (u8)drawable);
    }
}

void unsigned_neo_geo_ui_renderer_init(UUIRenderer *renderer, UNeoGeoUIRenderer *backend, const UUITheme *theme, const UNeoGeoUIRendererConfig *config) {
    if (renderer == NULL || backend == NULL || config == NULL) {
        return;
    }

    *backend = (UNeoGeoUIRenderer){
        .config = *config,
    };
    unsigned_ui_renderer_init(renderer, backend, theme);
    renderer->draw_label = neo_geo_ui_draw_label;
    renderer->draw_progress_bar = neo_geo_ui_draw_progress_bar;
    renderer->draw_selector = neo_geo_ui_draw_selector;
}
