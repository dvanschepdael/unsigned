/**
 * @file blink_label.c
 * @brief Implements blinking text label widget.
 */

#include "display/ui/widget/blink_label.h"

/** Synchronizes label visibility with the embedded blink state. */
static void unsigned_ui_blink_label_update(UUIBlinkLabel *blink_label) {
    const bool show_text = unsigned_ui_blink_is_visible(&blink_label->blink) && blink_label->visible_text[0] != '\0';

    unsigned_ui_label_set_text(&blink_label->label, show_text ? blink_label->visible_text : blink_label->clear_text);
}

void unsigned_ui_blink_label_init(UUIBlinkLabel *blink_label, const UUIBlinkLabelConfig *config) {
    for (u16 column = 0u; column < config->clear_width; ++column) {
        blink_label->clear_text[column] = ' ';
    }
    blink_label->clear_text[config->clear_width] = '\0';
    blink_label->visible_text = config->text;
    unsigned_ui_blink_init(&blink_label->blink, config->interval_frames, config->initially_visible);
    unsigned_ui_label_init(&blink_label->label, config->bounds, config->style, "");
    unsigned_ui_blink_label_update(blink_label);
}

void unsigned_ui_blink_label_set_text(UUIBlinkLabel *blink_label, const char *text) {
    if (blink_label->visible_text == text) {
        return;
    }
    blink_label->visible_text = text;
    unsigned_ui_blink_label_update(blink_label);
}

void unsigned_ui_blink_label_reset(UUIBlinkLabel *blink_label, bool visible) {
    unsigned_ui_blink_reset(&blink_label->blink, visible);
    unsigned_ui_blink_label_update(blink_label);
}

void unsigned_ui_blink_label_tick(UUIBlinkLabel *blink_label) {
    unsigned_ui_blink_tick(&blink_label->blink);
    unsigned_ui_blink_label_update(blink_label);
}
