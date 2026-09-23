/**
 * @file ui_renderer.c
 * @brief Binds generic UI draw callbacks to the Neo Geo FIX-layer backend.
 */

#include "renderer/ui_renderer.h"

#include "display/ui/widget/label.h"
#include "display/ui/widget/progress_bar.h"
#include "display/ui/widget/selector.h"
#include "system/fix.h"

/** Converts a UI pixel-space Y coordinate to the corresponding Neo Geo FIX row. */
static u8 neo_geo_ui_row(const UUIElement *element) {
    return (u8)(element->bounds.y / 8);
}

/** Append one zero-terminated fragment to the renderer's bounded text scratch. */
static u8 neo_geo_ui_text_append(char *target, u8 length, const char *text) {
    const u8 last = (u8)(UNSIGNED_NEO_GEO_UI_TEXT_CAPACITY - 1u);
    while (*text != '\0' && length < last) {
        target[length++] = *text++;
    }
    target[length] = '\0';
    return length;
}

/** Append up to `count` spaces while preserving room for the trailing NUL. */
static u8 neo_geo_ui_text_append_spaces(char *target, u8 length, u8 count) {
    const u8 last = (u8)(UNSIGNED_NEO_GEO_UI_TEXT_CAPACITY - 1u);
    while (count > 0u && length < last) {
        target[length++] = ' ';
        --count;
    }
    target[length] = '\0';
    return length;
}

/** Draws a centered label on the FIX layer, selecting normal or tall text from the configured style id. */
static void neo_geo_ui_draw_label(void *context, const UUITheme *theme, const UUILabel *label) {
    UNeoGeoUIRenderer *backend = context;
    (void)theme;

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

    const char *selected = unsigned_ui_selector_selected_text(selector);
    if (selected == NULL) {
        selected = "";
    }
    const u8 width = backend->config.selector_option_width;
    u8 length = 0u;
    u8 selected_length = 0u;

    backend->text[0] = '\0';
    length = neo_geo_ui_text_append(backend->text, length, selector->label != NULL ? selector->label : "");
    length = neo_geo_ui_text_append(backend->text, length, selector->element.focused ? "  < " : "    ");

    /* snprintf("%-*s") only pads when the option is shorter than the configured width; a longer
     * option is emitted in full. Count while copying so no strlen/printf machinery is needed. */
    const u8 last = (u8)(UNSIGNED_NEO_GEO_UI_TEXT_CAPACITY - 1u);
    while (*selected != '\0' && length < last) {
        backend->text[length++] = *selected++;
        ++selected_length;
    }
    backend->text[length] = '\0';
    if (selected_length < width) {
        length = neo_geo_ui_text_append_spaces(backend->text, length, (u8)(width - selected_length));
    }
    (void)neo_geo_ui_text_append(backend->text, length, selector->element.focused ? " >" : "  ");

    unsigned_neo_geo_fix_center_text(neo_geo_ui_row(&selector->element), backend->config.palette, backend->text);
}

/** Draws two rows of precomposed frame/fill tiles; clipping preserves the original progress. */
static void neo_geo_ui_draw_progress_bar(void *context, const UUITheme *theme, const UUIProgressBar *progress_bar) {
    UNeoGeoUIRenderer *backend = context;
    (void)theme;

    if (backend->config.progress_bar_tile_base == 0u) {
        return;
    }

    const UUIRect *bounds = &progress_bar->element.bounds;
    const u16 columns = bounds->width / U_UI_PROGRESS_BAR_TILE_SIZE;
    if (bounds->x < 0 || bounds->y < 0) {
        return;
    }
    const u16 x = (u16)bounds->x / U_UI_PROGRESS_BAR_TILE_SIZE;
    const u16 y = (u16)bounds->y / U_UI_PROGRESS_BAR_TILE_SIZE;
    if (x >= UNSIGNED_NEO_GEO_FIX_COLUMNS || y >= UNSIGNED_NEO_GEO_FIX_ROWS) {
        return;
    }

    /* Fill is restricted to the framed interior: x=1..width-2. */
    const u16 interior_width = (u16)(columns * U_UI_PROGRESS_BAR_TILE_SIZE - U_NEO_GEO_UI_PROGRESS_INSET_LEFT - U_NEO_GEO_UI_PROGRESS_INSET_RIGHT);
    const u16 progress = unsigned_ui_progress_bar_progress(progress_bar);
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
    *backend = (UNeoGeoUIRenderer){
        .config = *config,
    };
    unsigned_ui_renderer_init(renderer, backend, theme);
    renderer->draw_label = neo_geo_ui_draw_label;
    renderer->draw_progress_bar = neo_geo_ui_draw_progress_bar;
    renderer->draw_selector = neo_geo_ui_draw_selector;
}
