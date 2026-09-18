/**
 * @file context.h
 * @brief UI page stack, update and render orchestration.
 */

#ifndef UNSIGNED_UI_CONTEXT_H
#define UNSIGNED_UI_CONTEXT_H

#include "display/ui/config.h"
#include "display/ui/input.h"
#include "display/ui/page.h"
#include "display/ui/renderer.h"
#include "display/ui/types.h"

typedef struct UUIContext {
    UUIRenderer renderer;
    UUIPage *pages[UNSIGNED_UI_PAGE_STACK_CAPACITY];
    u8 page_count;
} UUIContext;

/**
 * @brief Initializes the UI context to a valid empty runtime state.
 *
 * @param ui UI context that owns the active page stack.
 * @param renderer Renderer abstraction/state used to draw the logical presentation.
 */
void unsigned_ui_context_init(UUIContext *ui, const UUIRenderer *renderer);

/**
 * @brief Pushes a page on the UI stack and makes it the active update/render target.
 *
 * @param ui UI context that owns the active page stack.
 * @param page UI page pushed, popped or queried in the context stack.
 * @return `U_UI_RESULT_OK` on success, `U_UI_RESULT_FULL` when the stack is full, or `U_UI_RESULT_INVALID_ARGUMENT` for null input.
 */
UUIResult unsigned_ui_push_page(UUIContext *ui, UUIPage *page);

/**
 * @brief Removes the top UI page and restores the previous page when one exists.
 *
 * @param ui UI context that owns the active page stack.
 * @return `U_UI_RESULT_OK` on success, `U_UI_RESULT_NOT_FOUND` when the stack is empty, or `U_UI_RESULT_INVALID_ARGUMENT` for a null context.
 */
UUIResult unsigned_ui_pop_page(UUIContext *ui);

/**
 * @brief Returns the current page.
 *
 * @param ui UI context that owns the active page stack.
 * @return Top page, or NULL when the context/page stack is empty or invalid.
 */
UUIPage *unsigned_ui_current_page(const UUIContext *ui);

/**
 * @brief Updates only the current UI page/menu from the supplied navigation input.
 *
 * @param ui UI context that owns the active page stack.
 * @param input Input snapshot sampled or consumed by this API.
 */
void unsigned_ui_update(UUIContext *ui, const UUIInput *input);

/**
 * @brief Renders the UI from its current logical state.
 *
 * @param ui UI context that owns the active page stack.
 */
void unsigned_ui_render(const UUIContext *ui);

#endif
