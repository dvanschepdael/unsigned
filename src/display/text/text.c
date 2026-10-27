/**
 * @file text.c
 * @brief Implements localized text catalog and formatting service.
 */

#include "display/text/text.h"

#include <stdarg.h>
#include <stdio.h>

static const UTextConfig *text_config;
static u8 current_language;

void unsigned_text_init(const UTextConfig *config) {
    text_config = config;
    current_language = config->default_language;
}

void unsigned_text_set_language(u8 language) {
    current_language = language;
}

u8 unsigned_text_get_language(void) {
    return current_language;
}

const UText *unsigned_text_get_language_info(u8 language) {
    return &text_config->languages[language];
}

/** Returns the localized catalog string at the requested string-table index. */
static const char *text_string_at(u8 language, u16 string_id) {
    return text_config->languages[language].strings[string_id];
}

const char *unsigned_text_get(u16 string_id) {
    const char *text = text_string_at(current_language, string_id);
    return text != NULL ? text : text_string_at(text_config->default_language, string_id);
}

void unsigned_text_format(char *buffer, size_t capacity, int string_id, ...) {
    va_list arguments;

    va_start(arguments, string_id);
    (void)vsnprintf(buffer, capacity, unsigned_text_get((u16)string_id), arguments);
    va_end(arguments);
}
