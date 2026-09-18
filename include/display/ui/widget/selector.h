/**
 * @file selector.h
 * @brief Focusable option selector widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_SELECTOR_H
#define UNSIGNED_DISPLAY_UI_WIDGET_SELECTOR_H

#include "display/ui/element.h"

typedef struct UUISelector UUISelector;
typedef void (*UUISelectorChangedCallback)(UUISelector *selector, u8 selected_index, void *context);

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
 * @param bounds Bounds used for layout, culling, collision, or effect calculations.
 * @param style UI style identifier resolved by the active theme.
 * @param label Label text/widget to configure.
 * @param options Caller-owned option strings available to the selector.
 * @param option_count Number of valid entries in the option array.
 * @param selected_index Zero-based option index to select.
 * @param wrap Whether navigation wraps between the first and last option.
 * @param on_changed Callback invoked after the selected option changes.
 * @param context Opaque caller context passed back to callbacks.
 */
void unsigned_ui_selector_init(UUISelector *selector, UUIRect bounds, UUIStyleId style, const char *label, const char *const *options, u8 option_count, u8 selected_index, bool wrap, UUISelectorChangedCallback on_changed, void *context);

/**
 * @brief Moves selection to the previous option, honoring wrap policy and firing on_changed only when the index changes.
 *
 * @param selector Selector widget to navigate.
 * @return true when selection changed; false for invalid/empty selectors or when already at the first non-wrapping option.
 */
bool unsigned_ui_selector_previous(UUISelector *selector);

/**
 * @brief Moves selection to the next option, honoring wrap policy and firing on_changed only when the index changes.
 *
 * @param selector Selector widget to navigate.
 * @return true when selection changed; false for invalid/empty selectors or when already at the last non-wrapping option.
 */
bool unsigned_ui_selector_next(UUISelector *selector);

/**
 * @brief Sets selected on the UI selector.
 *
 * @param selector Selector widget whose option state is read or changed.
 * @param selected_index Zero-based option index to select.
 */
void unsigned_ui_selector_set_selected(UUISelector *selector, u8 selected_index);

/**
 * @brief Returns the text of the currently selected option, or NULL when the selector has no valid option.
 *
 * @param selector Selector widget whose option state is read or changed.
 * @return Selected option text, or NULL when the selector/options/index are invalid.
 */
const char *unsigned_ui_selector_selected_text(const UUISelector *selector);

#endif
