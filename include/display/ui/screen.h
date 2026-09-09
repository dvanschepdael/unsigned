/**
 * @file screen.h
 * @brief Ordered collection of renderable UI elements.
 */

#ifndef UNSIGNED_DISPLAY_UI_SCREEN_H
#define UNSIGNED_DISPLAY_UI_SCREEN_H

#include "display/ui/config.h"
#include "display/ui/element.h"
#include "display/ui/renderer.h"
#include "display/ui/types.h"

typedef struct UUIScreen {
    UUIElement *elements[UNSIGNED_UI_SCREEN_CAPACITY];
    u8 count;
} UUIScreen;

/**
 * @brief Initializes the UI screen to a valid empty runtime state.
 *
 * @param screen UI screen whose ordered element collection is processed.
 */
void unsigned_ui_screen_init(UUIScreen *screen);

/**
 * @brief Adds the supplied entry to the UI screen.
 *
 * @param screen UI screen whose ordered element collection is processed.
 * @param element UI element being configured, focused or rendered.
 * @return `U_UI_RESULT_OK` on success, `U_UI_RESULT_FULL` at screen capacity, or `U_UI_RESULT_INVALID_ARGUMENT` for null input.
 */
UUIResult unsigned_ui_screen_add(UUIScreen *screen, UUIElement *element);

/**
 * @brief Removes the supplied entry from the UI screen.
 *
 * @param screen UI screen whose ordered element collection is processed.
 * @param element UI element being configured, focused or rendered.
 * @return `U_UI_RESULT_OK` when removed, `U_UI_RESULT_NOT_FOUND` when absent, or `U_UI_RESULT_INVALID_ARGUMENT` for null input.
 */
UUIResult unsigned_ui_screen_remove(UUIScreen *screen, UUIElement *element);

/**
 * @brief Renders the UI screen from its current logical state.
 *
 * @param screen UI screen whose ordered element collection is processed.
 * @param renderer Renderer abstraction/state used to draw the logical presentation.
 */
void unsigned_ui_screen_render(const UUIScreen *screen, const UUIRenderer *renderer);

#endif
