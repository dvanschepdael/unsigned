/**
 * @file menu.c
 * @brief Implements focusable menu navigation and cancel handling.
 */

#include "display/ui/menu.h"

#include "display/ui/collection_internal.h"

#include "display/ui/widget/button.h"
#include "display/ui/widget/selector.h"

#pragma region Focus navigation

/** Searches circularly from the current index for the next enabled, visible and focusable menu item. */
static bool unsigned_ui_menu_find_next(const UUIMenu *menu, u8 start_index, bool forward, u8 *result_index) {
    if (menu->count == 0) {
        return false;
    }

    u8 index = start_index;
    for (u8 step = 0u; step < menu->count; ++step) {
        if (forward) {
            ++index;
            if (index == menu->count) {
                index = 0u;
            }
        } else {
            if (index == 0u) {
                index = menu->count;
            }
            --index;
        }

        if (unsigned_ui_element_can_focus(menu->items[index])) {
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
    if (menu->count == 0) {
        return;
    }

    if (menu->focused_index != UINT8_MAX && unsigned_ui_element_can_focus(menu->items[menu->focused_index])) {
        return;
    }

    unsigned_ui_menu_clear_focus(menu);
    unsigned_ui_menu_focus_first(menu);
}


#pragma endregion

#pragma region Menu content and focus lifecycle
void unsigned_ui_menu_init(UUIMenu *menu) {
    *menu = (UUIMenu){.focused_index = UINT8_MAX};
}

void unsigned_ui_menu_add(UUIMenu *menu, UUIElement *element) {
    unsigned_ui_collection_add(menu->items, &menu->count, element);

    if (menu->focused_index == UINT8_MAX && unsigned_ui_element_can_focus(element)) {
        unsigned_ui_menu_set_focus(menu, (u8)(menu->count - 1));
    }
}

void unsigned_ui_menu_remove(UUIMenu *menu, UUIElement *element) {
    unsigned_ui_menu_clear_focus(menu);
    element->focused = false;
    menu->count = unsigned_ui_collection_remove(menu->items, menu->count, element);
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

    if (menu->focused_index != UINT8_MAX) {
        if (input->navigate_previous_pressed) {
            if (unsigned_ui_menu_find_next(menu, menu->focused_index, false, &next_index)) {
                unsigned_ui_menu_set_focus(menu, next_index);
            }
        } else if (input->navigate_next_pressed) {
            if (unsigned_ui_menu_find_next(menu, menu->focused_index, true, &next_index)) {
                unsigned_ui_menu_set_focus(menu, next_index);
            }
        }

        UUIElement *element = menu->items[menu->focused_index];
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

    if (input->cancel_pressed && menu->on_cancel != NULL) {
        menu->on_cancel(menu->cancel_context);
    }
}

#pragma endregion
