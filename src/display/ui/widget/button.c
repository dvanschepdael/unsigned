#include "display/ui/widget/button.h"

void unsigned_ui_button_init(UUIButton *button, UUIRect bounds, UUIStyleId style, const char *text, UUIButtonPressedCallback on_pressed, void *context) {
    *button = (UUIButton){
        .text = text,
        .on_pressed = on_pressed,
        .context = context,
    };
    unsigned_ui_element_init(&button->element, U_UI_ELEMENT_BUTTON, bounds, style, true);
}

void unsigned_ui_button_press(UUIButton *button) {
    if (!button->element.enabled || !button->element.visible) {
        return;
    }

    if (button->on_pressed != NULL) {
        button->on_pressed(button, button->context);
    }
}
