/**
 * @file input.c
 * @brief Implements UI navigation/action input snapshot.
 */

#include "display/ui/input.h"

#include <stddef.h>

void unsigned_ui_input_clear(UUIInput *input) {
    if (input == NULL) {
        return;
    }

    *input = (UUIInput){ 0 };
}
