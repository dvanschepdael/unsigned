/**
 * @file selector.h
 * @brief Focusable option selector widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_SELECTOR_H
#define UNSIGNED_DISPLAY_UI_WIDGET_SELECTOR_H

#include "display/ui/element.h"

typedef struct UUISelector UUISelector;
typedef void (*UUISelectorChangedCallback)(UUISelector *selector, u8 selected_index, void *context);

typedef struct UUISelectorConfig {
    UUIRect bounds;
    UUIStyleId style;
    const char *label;
    const char *const *options;
    u8 option_count;
    u8 selected_index;
    bool wrap;
    UUISelectorChangedCallback on_changed;
    void *context;
} UUISelectorConfig;

struct UUISelector {
    UUIElement element;
    const char *label;
    const char *const *options;
    u8 option_count;
    u8 selected_index;
    bool wrap;
    UUISelectorChangedCallback on_changed;
    void *context;
};

/**
 * @brief Initializes the UI selector to a valid empty runtime state.
 *
 * @param selector Selector widget whose option state is read or changed.
 * @param config Caller-owned initialization values consumed during the call.
 * @pre `config->options != NULL` when `config->option_count > 0`.
 * @pre `config->selected_index < config->option_count` when `config->option_count > 0`.
 */
void unsigned_ui_selector_init(UUISelector *selector, const UUISelectorConfig *config);

/**
 * @brief Moves selection to the previous option, honoring wrap policy and firing on_changed only when the index changes.
 *
 * @param selector Selector widget to navigate.
 * @return true when selection changed; false when empty or already at the first non-wrapping option.
 */
bool unsigned_ui_selector_previous(UUISelector *selector);

/**
 * @brief Moves selection to the next option, honoring wrap policy and firing on_changed only when the index changes.
 *
 * @param selector Selector widget to navigate.
 * @return true when selection changed; false when empty or already at the last non-wrapping option.
 */
bool unsigned_ui_selector_next(UUISelector *selector);

/**
 * @brief Sets selected on the UI selector.
 *
 * @param selector Selector widget whose option state is read or changed.
 * @param selected_index Zero-based option index to select.
 * @pre `selector->option_count > 0` and `selected_index < selector->option_count`.
 */
void unsigned_ui_selector_set_selected(UUISelector *selector, u8 selected_index);

/**
 * @brief Returns the text of the currently selected option, or NULL when the selector has no valid option.
 *
 * @param selector Selector widget whose option state is read or changed.
 * @return Selected option text, or NULL when the selector has no options.
 */
const char *unsigned_ui_selector_selected_text(const UUISelector *selector);

#endif
