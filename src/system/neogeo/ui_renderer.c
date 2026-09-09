/**
 * @file ui_renderer.c
 * @brief Binds generic UI draw callbacks to the Neo Geo FIX-layer backend.
 */

#include "system/neogeo/ui_renderer.h"

#include "display/ui/widget/label.h"
#include "display/ui/widget/selector.h"
#include "system/neogeo/fix.h"

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

void unsigned_neo_geo_ui_renderer_init(UUIRenderer *renderer, UNeoGeoUIRenderer *backend, const UUITheme *theme, const UNeoGeoUIRendererConfig *config) {
    if (renderer == NULL || backend == NULL || config == NULL) {
        return;
    }

    *backend = (UNeoGeoUIRenderer){
        .config = *config,
    };
    unsigned_ui_renderer_init(renderer, backend, theme);
    renderer->draw_label = neo_geo_ui_draw_label;
    renderer->draw_selector = neo_geo_ui_draw_selector;
}
