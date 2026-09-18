/**
 * @file menu.h
 * @brief Focusable menu navigation and cancel handling.
 */

#ifndef UNSIGNED_DISPLAY_UI_MENU_H
#define UNSIGNED_DISPLAY_UI_MENU_H

#include "display/ui/config.h"
#include "display/ui/element.h"
#include "display/ui/input.h"
#include "display/ui/types.h"

typedef void (*UUIMenuCancelCallback)(void *context);

typedef struct UUIMenu {
    UUIElement *items[UNSIGNED_UI_MENU_CAPACITY];
    u8 count;
    u8 focused_index;
    bool has_focus;
    UUIMenuCancelCallback on_cancel;
    void *cancel_context;
} UUIMenu;

/**
 * @brief Initializes the UI menu to a valid empty runtime state.
 *
 * @param menu Menu whose entries/focus state are updated.
 */
void unsigned_ui_menu_init(UUIMenu *menu);

/**
 * @brief Adds the supplied entry to the UI menu.
 *
 * @param menu Menu whose entries/focus state are updated.
 * @param element UI element being configured, focused or rendered.
 * @return `U_UI_RESULT_OK` on success, `U_UI_RESULT_FULL` at menu capacity, or `U_UI_RESULT_INVALID_ARGUMENT` for invalid/non-focusable input.
 */
UUIResult unsigned_ui_menu_add(UUIMenu *menu, UUIElement *element);

/**
 * @brief Removes the supplied entry from the UI menu.
 *
 * @param menu Menu whose entries/focus state are updated.
 * @param element UI element being configured, focused or rendered.
 * @return `U_UI_RESULT_OK` when removed, `U_UI_RESULT_NOT_FOUND` when absent, or `U_UI_RESULT_INVALID_ARGUMENT` for null input.
 */
UUIResult unsigned_ui_menu_remove(UUIMenu *menu, UUIElement *element);

/**
 * @brief Installs the callback/context invoked when the menu handles a cancel action.
 *
 * @param menu Menu whose entries/focus state are updated.
 * @param callback Optional callback invoked when cancel input is accepted by this menu.
 * @param context Opaque caller context passed back to callbacks.
 */
void unsigned_ui_menu_set_cancel_handler(UUIMenu *menu, UUIMenuCancelCallback callback, void *context);

/**
 * @brief Moves focus to the first currently focusable menu element.
 *
 * @param menu Menu whose entries/focus state are updated.
 */
void unsigned_ui_menu_focus_first(UUIMenu *menu);

/**
 * @brief Clears the UI menu focus state without freeing caller-owned storage.
 *
 * @param menu Menu whose entries/focus state are updated.
 */
void unsigned_ui_menu_clear_focus(UUIMenu *menu);

/**
 * @brief Returns the menu element that currently owns focus, or NULL when none is valid.
 *
 * @param menu Menu whose entries/focus state are updated.
 * @return Currently focused element, or NULL when no valid focusable entry owns focus.
 */
UUIElement *unsigned_ui_menu_focused(const UUIMenu *menu);

/**
 * @brief Processes directional, accept and cancel input for the menu focus state.
 *
 * @param menu Menu whose entries/focus state are updated.
 * @param input Input snapshot sampled or consumed by this API.
 */
void unsigned_ui_menu_update(UUIMenu *menu, const UUIInput *input);

#endif
