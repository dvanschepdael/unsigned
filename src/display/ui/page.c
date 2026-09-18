/**
 * @file page.c
 * @brief Implements UI page composition and overlay policy.
 */

#include "display/ui/page.h"

void unsigned_ui_page_init(UUIPage *page, UUIScreen *screen, UUIMenu *menu, bool overlay) {
    if (page == NULL) {
        return;
    }

    *page = (UUIPage){
        .screen = screen,
        .menu = menu,
        .overlay = overlay,
    };
}
