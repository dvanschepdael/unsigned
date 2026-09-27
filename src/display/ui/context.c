/**
 * @file context.c
 * @brief Implements UI page stack, update and render orchestration.
 */

#include "display/ui/context.h"

void unsigned_ui_context_init(UUIContext *ui, const UUIRenderer *renderer) {
    *ui = (UUIContext){
        .renderer = *renderer,
    };
}

void unsigned_ui_push_page(UUIContext *ui, UUIPage *page) {
    ui->pages[ui->page_count] = page;
    ++ui->page_count;

    if (page->menu != NULL) {
        unsigned_ui_menu_focus_first(page->menu);
    }

    if (page->on_enter != NULL) {
        page->on_enter(page->context);
    }
}

void unsigned_ui_pop_page(UUIContext *ui) {
    --ui->page_count;
    UUIPage *page = ui->pages[ui->page_count];

    if (page->on_leave != NULL) {
        page->on_leave(page->context);
    }
}

UUIPage *unsigned_ui_current_page(const UUIContext *ui) {
    if (ui->page_count == 0) {
        return NULL;
    }

    return ui->pages[ui->page_count - 1];
}

void unsigned_ui_update(UUIContext *ui, const UUIInput *input) {
    if (ui->page_count == 0u) {
        return;
    }

    UUIPage *page = ui->pages[ui->page_count - 1u];
    if (page->menu != NULL) {
        unsigned_ui_menu_update(page->menu, input);
    }
}

void unsigned_ui_render(const UUIContext *ui) {
    if (ui->page_count == 0) {
        return;
    }

    if (ui->renderer.begin != NULL) {
        ui->renderer.begin(ui->renderer.context, ui->renderer.theme);
    }

    u8 first_page = (u8)(ui->page_count - 1);
    while (first_page > 0 && ui->pages[first_page]->overlay) {
        --first_page;
    }

    for (u8 i = first_page; i < ui->page_count; ++i) {
        UUIPage *page = ui->pages[i];
        if (page->screen != NULL) {
            unsigned_ui_screen_render(page->screen, &ui->renderer);
        }
    }

    if (ui->renderer.end != NULL) {
        ui->renderer.end(ui->renderer.context, ui->renderer.theme);
    }
}
