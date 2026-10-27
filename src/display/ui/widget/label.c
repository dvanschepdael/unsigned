/**
 * @file label.c
 * @brief Implements text label widget.
 */

#include "display/ui/widget/label.h"

void unsigned_ui_label_init(UUILabel *label, UUIRect bounds, UUIStyleId style, const char *text) {
    *label = (UUILabel){
        .text = text,
    };
    unsigned_ui_element_init(&label->element, U_UI_ELEMENT_LABEL, bounds, style, false);
}

void unsigned_ui_label_set_text(UUILabel *label, const char *text) {
    label->text = text;
}
