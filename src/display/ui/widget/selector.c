/**
 * @file selector.c
 * @brief Implements focusable option selector widget.
 */

#include "display/ui/widget/selector.h"

#include "core/math/math.h"

/** Notifies the caller after the selector changes to a different option index. */
static void unsigned_ui_selector_notify(UUISelector *selector) {
    if (selector->on_changed != NULL) {
        selector->on_changed(selector, selector->selected_index, selector->context);
    }
}

void unsigned_ui_selector_init(UUISelector *selector, UUIRect bounds, UUIStyleId style, const char *label, const char *const *options, u8 option_count, u8 selected_index, bool wrap, UUISelectorChangedCallback on_changed, void *context) {
    if (selector == NULL) {
        return;
    }

    *selector = (UUISelector){
        .label = label,
        .options = options,
        .option_count = option_count,
        .wrap = wrap,
        .on_changed = on_changed,
        .context = context,
    };
    unsigned_ui_element_init(&selector->element, U_UI_ELEMENT_SELECTOR, bounds, style, true);

    if (option_count > 0) {
        selector->selected_index = unsigned_math_min_u8(selected_index, (u8)(option_count - 1));
    }
}

bool unsigned_ui_selector_previous(UUISelector *selector) {
    if (selector == NULL || selector->option_count == 0) {
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
    if (selector == NULL || selector->option_count == 0) {
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
    if (selector == NULL || selector->option_count == 0) {
        return;
    }

    selected_index = unsigned_math_min_u8(selected_index, (u8)(selector->option_count - 1));

    if (selector->selected_index == selected_index) {
        return;
    }

    selector->selected_index = selected_index;
    unsigned_ui_selector_notify(selector);
}

const char *unsigned_ui_selector_selected_text(const UUISelector *selector) {
    if (selector == NULL || selector->options == NULL || selector->option_count == 0 || selector->selected_index >= selector->option_count) {
        return NULL;
    }

    return selector->options[selector->selected_index];
}
