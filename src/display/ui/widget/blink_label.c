/**
 * @file blink_label.c
 * @brief Implements blinking text label widget.
 */

#include "display/ui/widget/blink_label.h"

/** Synchronizes label visibility with the embedded blink state. */
static void unsigned_ui_blink_label_update(UUIBlinkLabel *blink_label) {
    const bool show_text = unsigned_ui_blink_is_visible(&blink_label->blink) && blink_label->visible_text != NULL && blink_label->visible_text[0] != '\0';

    unsigned_ui_label_set_text(&blink_label->label, show_text ? blink_label->visible_text : blink_label->clear_text);
}

void unsigned_ui_blink_label_init(UUIBlinkLabel *blink_label, UUIRect bounds, UUIStyleId style, const char *text, u16 interval_frames, bool initially_visible, u16 clear_width) {
    if (blink_label == NULL) {
        return;
    }
    if (clear_width > UNSIGNED_UI_BLINK_LABEL_CLEAR_CAPACITY) {
        clear_width = UNSIGNED_UI_BLINK_LABEL_CLEAR_CAPACITY;
    }

    for (u16 column = 0u; column < clear_width; ++column) {
        blink_label->clear_text[column] = ' ';
    }
    blink_label->clear_text[clear_width] = '\0';
    blink_label->visible_text = text;
    unsigned_ui_blink_init(&blink_label->blink, interval_frames, initially_visible);
    unsigned_ui_label_init(&blink_label->label, bounds, style, "");
    unsigned_ui_blink_label_update(blink_label);
}

void unsigned_ui_blink_label_set_text(UUIBlinkLabel *blink_label, const char *text) {
    if (blink_label == NULL) {
        return;
    }

    blink_label->visible_text = text;
    unsigned_ui_blink_label_update(blink_label);
}

void unsigned_ui_blink_label_reset(UUIBlinkLabel *blink_label, bool visible) {
    if (blink_label == NULL) {
        return;
    }

    unsigned_ui_blink_reset(&blink_label->blink, visible);
    unsigned_ui_blink_label_update(blink_label);
}

void unsigned_ui_blink_label_tick(UUIBlinkLabel *blink_label) {
    if (blink_label == NULL) {
        return;
    }

    unsigned_ui_blink_tick(&blink_label->blink);
    unsigned_ui_blink_label_update(blink_label);
}
