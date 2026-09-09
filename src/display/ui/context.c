/**
 * @file context.c
 * @brief Implements UI page stack, update and render orchestration.
 */

#include "display/ui/context.h"

void unsigned_ui_context_init(UUIContext *ui, const UUIRenderer *renderer) {
    if (ui == NULL || renderer == NULL) {
        return;
    }

    *ui = (UUIContext){
        .renderer = *renderer,
    };
}

UUIResult unsigned_ui_push_page(UUIContext *ui, UUIPage *page) {
    if (ui == NULL || page == NULL) {
        return U_UI_RESULT_INVALID_ARGUMENT;
    }

    if (ui->page_count >= UNSIGNED_UI_PAGE_STACK_CAPACITY) {
        return U_UI_RESULT_FULL;
    }

    ui->pages[ui->page_count] = page;
    ++ui->page_count;

    if (page->menu != NULL) {
        unsigned_ui_menu_focus_first(page->menu);
    }

    if (page->on_enter != NULL) {
        page->on_enter(page->context);
    }

    return U_UI_RESULT_OK;
}

UUIResult unsigned_ui_pop_page(UUIContext *ui) {
    if (ui == NULL) {
        return U_UI_RESULT_INVALID_ARGUMENT;
    }

    if (ui->page_count == 0) {
        return U_UI_RESULT_NOT_FOUND;
    }

    --ui->page_count;
    UUIPage *page = ui->pages[ui->page_count];
    ui->pages[ui->page_count] = NULL;

    if (page != NULL && page->on_leave != NULL) {
        page->on_leave(page->context);
    }

    return U_UI_RESULT_OK;
}

UUIPage *unsigned_ui_current_page(const UUIContext *ui) {
    if (ui == NULL || ui->page_count == 0) {
        return NULL;
    }

    return ui->pages[ui->page_count - 1];
}

void unsigned_ui_update(UUIContext *ui, const UUIInput *input) {
    if (ui == NULL || input == NULL) {
        return;
    }

    UUIPage *page = unsigned_ui_current_page(ui);
    if (page != NULL && page->menu != NULL) {
        unsigned_ui_menu_update(page->menu, input);
    }
}

void unsigned_ui_render(const UUIContext *ui) {
    if (ui == NULL || ui->page_count == 0) {
        return;
    }

    if (ui->renderer.begin != NULL) {
        ui->renderer.begin(ui->renderer.context, ui->renderer.theme);
    }

    u8 first_page = (u8)(ui->page_count - 1);
    while (first_page > 0 && ui->pages[first_page] != NULL && ui->pages[first_page]->overlay) {
        --first_page;
    }

    for (u8 i = first_page; i < ui->page_count; ++i) {
        UUIPage *page = ui->pages[i];
        if (page != NULL && page->screen != NULL) {
            unsigned_ui_screen_render(page->screen, &ui->renderer);
        }
    }

    if (ui->renderer.end != NULL) {
        ui->renderer.end(ui->renderer.context, ui->renderer.theme);
    }
}
