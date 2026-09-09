/**
 * @file screen.c
 * @brief Implements ordered collection of renderable UI elements.
 */

#include "display/ui/screen.h"

#include "display/ui/widget/button.h"
#include "display/ui/widget/image.h"
#include "display/ui/widget/label.h"
#include "display/ui/widget/panel.h"
#include "display/ui/widget/progress_bar.h"
#include "display/ui/widget/selector.h"

/** Renders the UI screen element from its current logical state. */
static void unsigned_ui_screen_draw_element(const UUIElement *element, const UUIRenderer *renderer) {
    if (element == NULL || renderer == NULL || !element->visible) {
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
            break;
    }
}

void unsigned_ui_screen_init(UUIScreen *screen) {
    if (screen == NULL) {
        return;
    }

    *screen = (UUIScreen){ 0 };
}

UUIResult unsigned_ui_screen_add(UUIScreen *screen, UUIElement *element) {
    if (screen == NULL || element == NULL) {
        return U_UI_RESULT_INVALID_ARGUMENT;
    }

    if (screen->count >= UNSIGNED_UI_SCREEN_CAPACITY) {
        return U_UI_RESULT_FULL;
    }

    screen->elements[screen->count] = element;
    ++screen->count;
    return U_UI_RESULT_OK;
}

UUIResult unsigned_ui_screen_remove(UUIScreen *screen, UUIElement *element) {
    if (screen == NULL || element == NULL) {
        return U_UI_RESULT_INVALID_ARGUMENT;
    }

    for (u8 i = 0; i < screen->count; ++i) {
        if (screen->elements[i] == element) {
            for (u8 j = i; j + 1 < screen->count; ++j) {
                screen->elements[j] = screen->elements[j + 1];
            }
            --screen->count;
            screen->elements[screen->count] = NULL;
            return U_UI_RESULT_OK;
        }
    }

    return U_UI_RESULT_NOT_FOUND;
}

void unsigned_ui_screen_render(const UUIScreen *screen, const UUIRenderer *renderer) {
    if (screen == NULL || renderer == NULL) {
        return;
    }

    for (u8 i = 0; i < screen->count; ++i) {
        unsigned_ui_screen_draw_element(screen->elements[i], renderer);
    }
}
