/**
 * @file renderer.c
 * @brief Implements UI renderer abstraction and theme binding.
 */

#include "display/ui/renderer.h"

#include <stddef.h>

void unsigned_ui_renderer_init(UUIRenderer *renderer, void *context, const UUITheme *theme) {
    if (renderer == NULL) {
        return;
    }

    *renderer = (UUIRenderer){
        .context = context,
        .theme = theme,
    };
}
