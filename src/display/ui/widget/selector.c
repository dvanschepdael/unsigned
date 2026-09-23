/**
 * @file selector.c
 * @brief Implements focusable option selector widget.
 */

#include "display/ui/widget/selector.h"

/** Notifies the caller after the selector changes to a different option index. */
static void unsigned_ui_selector_notify(UUISelector *selector) {
    if (selector->on_changed != NULL) {
        selector->on_changed(selector, selector->selected_index, selector->context);
    }
}

void unsigned_ui_selector_init(UUISelector *selector, const UUISelectorConfig *config) {
    *selector = (UUISelector){
        .label = config->label,
        .options = config->options,
        .option_count = config->option_count,
        .wrap = config->wrap,
        .on_changed = config->on_changed,
        .context = config->context,
    };
    unsigned_ui_element_init(&selector->element, U_UI_ELEMENT_SELECTOR, config->bounds, config->style, true);

    selector->selected_index = config->option_count == 0u ? 0u : config->selected_index;
}

bool unsigned_ui_selector_previous(UUISelector *selector) {
    if (selector->option_count == 0) {
        return false;
    }

    if (selector->selected_index > 0) {
        --selector->selected_index;
    } else if (selector->wrap && selector->option_count > 1) {
        selector->selected_index = (u8)(selector->option_count - 1);
    } else {
        return false;
    }

    unsigned_ui_selector_notify(selector);
    return true;
}

bool unsigned_ui_selector_next(UUISelector *selector) {
    if (selector->option_count == 0) {
        return false;
    }

    if ((u8)(selector->selected_index + 1) < selector->option_count) {
        ++selector->selected_index;
    } else if (selector->wrap && selector->option_count > 1) {
        selector->selected_index = 0;
    } else {
        return false;
    }

    unsigned_ui_selector_notify(selector);
    return true;
}

void unsigned_ui_selector_set_selected(UUISelector *selector, u8 selected_index) {
    if (selector->selected_index == selected_index) {
        return;
    }

    selector->selected_index = selected_index;
    unsigned_ui_selector_notify(selector);
}

const char *unsigned_ui_selector_selected_text(const UUISelector *selector) {
    if (selector->option_count == 0u) {
        return NULL;
    }

    return selector->options[selector->selected_index];
}
