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
    return unsigned_ui_element_can_focus(menu->items[index]);
}

/** Searches circularly from the current index for the next enabled, visible and focusable menu item. */
static bool unsigned_ui_menu_find_next(const UUIMenu *menu, u8 start_index, bool forward, u8 *result_index) {
    if (menu->count == 0) {
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

/** Transfers exclusive focus to one menu item and clears the previous focus owner. */
static void unsigned_ui_menu_set_focus(UUIMenu *menu, u8 index) {
    if (menu->focused_index != UINT8_MAX) {
        menu->items[menu->focused_index]->focused = false;
    }

    menu->focused_index = index;
    menu->items[index]->focused = true;
}

/** Repairs menu focus after structural or enabled/visible state changes. */
static void unsigned_ui_menu_repair_focus(UUIMenu *menu) {
    u8 index;

    if (menu->count == 0) {
        return;
    }

    if (menu->focused_index != UINT8_MAX && unsigned_ui_menu_index_focusable(menu, menu->focused_index)) {
        menu->items[menu->focused_index]->focused = true;
        return;
    }

    if (menu->focused_index != UINT8_MAX) {
        menu->items[menu->focused_index]->focused = false;
    }
    menu->focused_index = UINT8_MAX;
    if (unsigned_ui_menu_find_next(menu, (u8)(menu->count - 1), true, &index)) {
        unsigned_ui_menu_set_focus(menu, index);
    }
}

/** Routes confirm/value-change input to the focused widget according to its element type. */
static void unsigned_ui_menu_activate_focused(UUIMenu *menu, const UUIInput *input) {
    if (menu->focused_index == UINT8_MAX) {
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
        U_UNREACHABLE();
    }
}

#pragma endregion

#pragma region Menu content and focus lifecycle
void unsigned_ui_menu_init(UUIMenu *menu) {
    *menu = (UUIMenu){.focused_index = UINT8_MAX};
}

void unsigned_ui_menu_add(UUIMenu *menu, UUIElement *element) {
    menu->items[menu->count] = element;
    ++menu->count;

    if (menu->focused_index == UINT8_MAX && unsigned_ui_element_can_focus(element)) {
        unsigned_ui_menu_set_focus(menu, (u8)(menu->count - 1));
    }
}

void unsigned_ui_menu_remove(UUIMenu *menu, UUIElement *element) {
    u8 index = 0u;
    while (menu->items[index] != element) {
        ++index;
    }

    if (menu->focused_index != UINT8_MAX) {
        menu->items[menu->focused_index]->focused = false;
    }
    element->focused = false;
    for (u8 i = index; i + 1u < menu->count; ++i) {
        menu->items[i] = menu->items[i + 1u];
    }

    --menu->count;
    menu->focused_index = UINT8_MAX;
    unsigned_ui_menu_repair_focus(menu);
}

void unsigned_ui_menu_set_cancel_handler(UUIMenu *menu, UUIMenuCancelCallback callback, void *context) {
    menu->on_cancel = callback;
    menu->cancel_context = context;
}

void unsigned_ui_menu_focus_first(UUIMenu *menu) {
    u8 index;

    if (menu->count == 0) {
        return;
    }

    if (unsigned_ui_menu_find_next(menu, (u8)(menu->count - 1), true, &index)) {
        unsigned_ui_menu_set_focus(menu, index);
    }
}

void unsigned_ui_menu_clear_focus(UUIMenu *menu) {
    if (menu->focused_index != UINT8_MAX) {
        menu->items[menu->focused_index]->focused = false;
    }

    menu->focused_index = UINT8_MAX;
}

UUIElement *unsigned_ui_menu_focused(const UUIMenu *menu) {
    return menu->focused_index == UINT8_MAX ? NULL : menu->items[menu->focused_index];
}

#pragma endregion

#pragma region Input handling
void unsigned_ui_menu_update(UUIMenu *menu, const UUIInput *input) {
    u8 next_index;

    unsigned_ui_menu_repair_focus(menu);

    if (menu->focused_index != UINT8_MAX && input->navigate_previous_pressed) {
        if (unsigned_ui_menu_find_next(menu, menu->focused_index, false, &next_index)) {
            unsigned_ui_menu_set_focus(menu, next_index);
        }
    } else if (menu->focused_index != UINT8_MAX && input->navigate_next_pressed) {
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
