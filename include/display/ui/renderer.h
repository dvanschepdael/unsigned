/**
 * @file renderer.h
 * @brief UI renderer abstraction and theme binding.
 */

#ifndef UNSIGNED_DISPLAY_UI_RENDERER_H
#define UNSIGNED_DISPLAY_UI_RENDERER_H

#include "display/ui/theme.h"

struct UUIPanel;
struct UUILabel;
struct UIImage;
struct UUIButton;
struct UUIProgressBar;
struct UUISelector;

typedef struct UUIRenderer {
    void *context;
    const UUITheme *theme;

    void (*begin)(void *context, const UUITheme *theme);
    void (*draw_panel)(void *context, const UUITheme *theme, const struct UUIPanel *panel);
    void (*draw_label)(void *context, const UUITheme *theme, const struct UUILabel *label);
    void (*draw_image)(void *context, const UUITheme *theme, const struct UIImage *image);
    void (*draw_button)(void *context, const UUITheme *theme, const struct UUIButton *button);
    void (*draw_progress_bar)(void *context, const UUITheme *theme, const struct UUIProgressBar *progress_bar);
    void (*draw_selector)(void *context, const UUITheme *theme, const struct UUISelector *selector);
    void (*end)(void *context, const UUITheme *theme);
} UUIRenderer;

/**
 * @brief Initializes the UI renderer to a valid empty runtime state.
 *
 * @param renderer Renderer abstraction/state used to draw the logical presentation.
 * @param context Opaque caller context passed back to callbacks.
 * @param theme Theme containing the style callbacks/assets used by UI rendering.
 */
void unsigned_ui_renderer_init(UUIRenderer *renderer, void *context, const UUITheme *theme);

#endif
