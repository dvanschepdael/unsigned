/**
 * @file screen.c
 * @brief Implements ordered collection of renderable UI elements.
 */

#include "display/ui/screen.h"

#include "display/ui/collection_internal.h"

#include "display/ui/widget/button.h"
#include "display/ui/widget/image.h"
#include "display/ui/widget/label.h"
#include "display/ui/widget/panel.h"
#include "display/ui/widget/progress_bar.h"
#include "display/ui/widget/selector.h"

/** Renders the UI screen element from its current logical state. */
static void unsigned_ui_screen_draw_element(const UUIElement *element, const UUIRenderer *renderer) {
    if (!element->visible) {
        return;
    }

    switch (element->type) {
    case U_UI_ELEMENT_PANEL:
        if (renderer->draw_panel != NULL) {
            renderer->draw_panel(renderer->context, renderer->theme, (const UUIPanel *)element);
        }
        break;

    case U_UI_ELEMENT_LABEL:
        if (renderer->draw_label != NULL) {
            renderer->draw_label(renderer->context, renderer->theme, (const UUILabel *)element);
        }
        break;

    case U_UI_ELEMENT_IMAGE:
        if (renderer->draw_image != NULL) {
            renderer->draw_image(renderer->context, renderer->theme, (const UIImage *)element);
        }
        break;

    case U_UI_ELEMENT_BUTTON:
        if (renderer->draw_button != NULL) {
            renderer->draw_button(renderer->context, renderer->theme, (const UUIButton *)element);
        }
        break;

    case U_UI_ELEMENT_PROGRESS_BAR:
        if (renderer->draw_progress_bar != NULL) {
            renderer->draw_progress_bar(renderer->context, renderer->theme, (const UUIProgressBar *)element);
        }
        break;

    case U_UI_ELEMENT_SELECTOR:
        if (renderer->draw_selector != NULL) {
            renderer->draw_selector(renderer->context, renderer->theme, (const UUISelector *)element);
        }
        break;

    default:
        U_UNREACHABLE();
    }
}

void unsigned_ui_screen_init(UUIScreen *screen) {
    *screen = (UUIScreen){0};
}

void unsigned_ui_screen_add(UUIScreen *screen, UUIElement *element) {
    unsigned_ui_collection_add(screen->elements, &screen->count, element);
}

void unsigned_ui_screen_remove(UUIScreen *screen, UUIElement *element) {
    screen->count = unsigned_ui_collection_remove(screen->elements, screen->count, element);
}

void unsigned_ui_screen_render(const UUIScreen *screen, const UUIRenderer *renderer) {
    for (u8 i = 0; i < screen->count; ++i) {
        unsigned_ui_screen_draw_element(screen->elements[i], renderer);
    }
}
