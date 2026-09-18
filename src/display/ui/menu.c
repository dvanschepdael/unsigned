/**
 * @file menu.c
 * @brief Implements focusable menu navigation and cancel handling.
 */

#include "display/ui/menu.h"

#include "display/ui/widget/button.h"
#include "display/ui/widget/selector.h"

#pragma region Focus navigation

/** Returns whether the menu element at the requested index can currently receive focus. */
static bool unsigned_ui_menu_index_focusable(const UUIMenu *menu, u8 index) {
    return menu != NULL && index < menu->count && unsigned_ui_element_can_focus(menu->items[index]);
}

/** Searches circularly from the current index for the next enabled, visible and focusable menu item. */
static bool unsigned_ui_menu_find_next(const UUIMenu *menu, u8 start_index, bool forward, u8 *result_index) {
    if (menu == NULL || result_index == NULL || menu->count == 0) {
        return false;
    }

    for (u8 step = 1; step <= menu->count; ++step) {
        u8 index;

        if (forward) {
            index = (u8)((start_index + step) % menu->count);
        } else {
            index = (u8)((start_index + menu->count - (step % menu->count)) % menu->count);
        }

        if (unsigned_ui_menu_index_focusable(menu, index)) {
            *result_index = index;
            return true;
        }
    }

    return false;
}

/** Transfers exclusive focus to one validated menu item and clears the previous focus owner. */
static void unsigned_ui_menu_set_focus(UUIMenu *menu, u8 index) {
    if (menu == NULL || !unsigned_ui_menu_index_focusable(menu, index)) {
        return;
    }

    if (menu->has_focus && menu->focused_index < menu->count) {
        menu->items[menu->focused_index]->focused = false;
    }

    menu->focused_index = index;
    menu->items[index]->focused = true;
    menu->has_focus = true;
}

/** Repairs menu focus after structural or enabled/visible state changes. */
static void unsigned_ui_menu_repair_focus(UUIMenu *menu) {
    u8 index;

    if (menu == NULL || menu->count == 0) {
        return;
    }

    if (menu->has_focus && unsigned_ui_menu_index_focusable(menu, menu->focused_index)) {
        menu->items[menu->focused_index]->focused = true;
        return;
    }

    if (menu->has_focus && menu->focused_index < menu->count) {
        menu->items[menu->focused_index]->focused = false;
    }
    menu->has_focus = false;
    if (unsigned_ui_menu_find_next(menu, (u8)(menu->count - 1), true, &index)) {
        unsigned_ui_menu_set_focus(menu, index);
    }
}

/** Routes confirm/value-change input to the focused widget according to its element type. */
static void unsigned_ui_menu_activate_focused(UUIMenu *menu, const UUIInput *input) {
    if (menu == NULL || input == NULL || !menu->has_focus) {
        return;
    }

    UUIElement *element = menu->items[menu->focused_index];
    if (!unsigned_ui_element_can_focus(element)) {
        return;
    }

    switch (element->type) {
        case U_UI_ELEMENT_BUTTON:
            if (input->confirm_pressed) {
                unsigned_ui_button_press((UUIButton *)element);
            }
            break;

        case U_UI_ELEMENT_SELECTOR:
            if (input->value_previous_pressed) {
                unsigned_ui_selector_previous((UUISelector *)element);
            }
            if (input->value_next_pressed) {
                unsigned_ui_selector_next((UUISelector *)element);
            }
            break;

        default:
            break;
    }
}

#pragma endregion

#pragma region Menu content and focus lifecycle
void unsigned_ui_menu_init(UUIMenu *menu) {
    if (menu == NULL) {
        return;
    }

    *menu = (UUIMenu){ 0 };
}

UUIResult unsigned_ui_menu_add(UUIMenu *menu, UUIElement *element) {
    if (menu == NULL || element == NULL || !element->focusable) {
        return U_UI_RESULT_INVALID_ARGUMENT;
    }

    if (menu->count >= UNSIGNED_UI_MENU_CAPACITY) {
        return U_UI_RESULT_FULL;
    }

    menu->items[menu->count] = element;
    ++menu->count;

    if (!menu->has_focus && unsigned_ui_element_can_focus(element)) {
        unsigned_ui_menu_set_focus(menu, (u8)(menu->count - 1));
    }

    return U_UI_RESULT_OK;
}

UUIResult unsigned_ui_menu_remove(UUIMenu *menu, UUIElement *element) {
    if (menu == NULL || element == NULL) {
        return U_UI_RESULT_INVALID_ARGUMENT;
    }

    for (u8 i = 0; i < menu->count; ++i) {
        if (menu->items[i] == element) {
            element->focused = false;
            for (u8 j = i; j + 1 < menu->count; ++j) {
                menu->items[j] = menu->items[j + 1];
            }

            --menu->count;
            menu->items[menu->count] = NULL;
            menu->has_focus = false;
            unsigned_ui_menu_repair_focus(menu);
            return U_UI_RESULT_OK;
        }
    }

    return U_UI_RESULT_NOT_FOUND;
}

void unsigned_ui_menu_set_cancel_handler(UUIMenu *menu, UUIMenuCancelCallback callback, void *context) {
    if (menu == NULL) {
        return;
    }

    menu->on_cancel = callback;
    menu->cancel_context = context;
}

void unsigned_ui_menu_focus_first(UUIMenu *menu) {
    u8 index;

    if (menu == NULL || menu->count == 0) {
        return;
    }

    if (unsigned_ui_menu_find_next(menu, (u8)(menu->count - 1), true, &index)) {
        unsigned_ui_menu_set_focus(menu, index);
    }
}

void unsigned_ui_menu_clear_focus(UUIMenu *menu) {
    if (menu == NULL) {
        return;
    }

    if (menu->has_focus && menu->focused_index < menu->count) {
        menu->items[menu->focused_index]->focused = false;
    }

    menu->has_focus = false;
}

UUIElement *unsigned_ui_menu_focused(const UUIMenu *menu) {
    if (menu == NULL || !menu->has_focus || menu->focused_index >= menu->count) {
        return NULL;
    }

    return menu->items[menu->focused_index];
}

#pragma endregion

#pragma region Input handling
void unsigned_ui_menu_update(UUIMenu *menu, const UUIInput *input) {
    u8 next_index;

    if (menu == NULL || input == NULL) {
        return;
    }

    unsigned_ui_menu_repair_focus(menu);

    if (menu->has_focus && input->navigate_previous_pressed) {
        if (unsigned_ui_menu_find_next(menu, menu->focused_index, false, &next_index)) {
            unsigned_ui_menu_set_focus(menu, next_index);
        }
    } else if (menu->has_focus && input->navigate_next_pressed) {
        if (unsigned_ui_menu_find_next(menu, menu->focused_index, true, &next_index)) {
            unsigned_ui_menu_set_focus(menu, next_index);
        }
    }

    unsigned_ui_menu_activate_focused(menu, input);

    if (input->cancel_pressed && menu->on_cancel != NULL) {
        menu->on_cancel(menu->cancel_context);
    }
}

#pragma endregion
