/**
 * @file ui_renderer.h
 * @brief Neo Geo implementation of the generic UI renderer.
 */

#ifndef UNSIGNED_SYSTEM_UI_RENDERER_H
#define UNSIGNED_SYSTEM_UI_RENDERER_H

#include "display/ui/renderer.h"
#include "display/ui/types.h"

#define UNSIGNED_NEO_GEO_UI_TEXT_CAPACITY 64u

typedef struct UNeoGeoUIRendererConfig {
    UUIStyleId tall_label_style;
    u8 palette;
    u8 selector_option_width;
} UNeoGeoUIRendererConfig;

typedef struct UNeoGeoUIRenderer {
    UNeoGeoUIRendererConfig config;
    char text[UNSIGNED_NEO_GEO_UI_TEXT_CAPACITY];
} UNeoGeoUIRenderer;

/**
 * @brief Initializes the Neo Geo UI renderer to a valid empty runtime state.
 *
 * @param renderer Renderer abstraction/state used to draw the logical presentation.
 * @param backend Neo Geo-specific UI backend state bound to the generic renderer callbacks.
 * @param theme Theme containing the style callbacks/assets used by UI rendering.
 * @param config Neo Geo UI style/palette configuration copied into `backend`.
 */
void unsigned_neo_geo_ui_renderer_init(UUIRenderer *renderer, UNeoGeoUIRenderer *backend, const UUITheme *theme, const UNeoGeoUIRendererConfig *config);

#endif
