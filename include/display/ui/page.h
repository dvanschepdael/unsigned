/**
 * @file page.h
 * @brief UI page composition and overlay policy.
 */

#ifndef UNSIGNED_DISPLAY_UI_PAGE_H
#define UNSIGNED_DISPLAY_UI_PAGE_H

#include "display/ui/menu.h"
#include "display/ui/screen.h"
#include "display/ui/types.h"

typedef void (*UUIPageCallback)(void *context);

typedef struct UUIPage {
    UUIScreen *screen;
    UUIMenu *menu;
    bool overlay;
    UUIPageCallback on_enter;
    UUIPageCallback on_leave;
    void *context;
} UUIPage;

/**
 * @brief Initializes the UI page to a valid empty runtime state.
 *
 * @param page UI page pushed, popped or queried in the context stack.
 * @param screen UI screen whose ordered element collection is processed.
 * @param menu Optional menu that owns focus/navigation for this page.
 * @param overlay Whether the page renders as an overlay instead of replacing the previous page.
 */
void unsigned_ui_page_init(UUIPage *page, UUIScreen *screen, UUIMenu *menu, bool overlay);

#endif
