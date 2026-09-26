#include "display/ui/widget/button.h"

void unsigned_ui_button_init(UUIButton *button, const UUIButtonConfig *config) {
    *button = (UUIButton){
        .text = config->text,
        .on_pressed = config->on_pressed,
        .context = config->context,
    };
    unsigned_ui_element_init(&button->element, U_UI_ELEMENT_BUTTON, config->bounds, config->style, true);
}

void unsigned_ui_button_press(UUIButton *button) {
    if (!button->element.enabled || !button->element.visible) {
        return;
    }

    if (button->on_pressed != NULL) {
        button->on_pressed(button, button->context);
    }
}
