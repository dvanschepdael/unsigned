/**
 * @file text.c
 * @brief Implements localized text catalog and formatting service.
 */

#include "display/text/text.h"

#include <stdarg.h>
#include <stdio.h>

static const UTextConfig *text_config;
static u8 current_language = UNSIGNED_LANGUAGE_INVALID;

/** Returns whether the text config is valid. */
static bool text_config_is_valid(const UTextConfig *config) {
    if (config == NULL || config->languages == NULL || config->language_count == 0u || config->default_language >= config->language_count) {
        return false;
    }

    if (config->string_count != 0u && config->languages[config->default_language].strings == NULL) {
        return false;
    }

    return true;
}

bool unsigned_text_init(const UTextConfig *config) {
    text_config = NULL;
    current_language = UNSIGNED_LANGUAGE_INVALID;

    if (!text_config_is_valid(config)) {
        return false;
    }

    text_config = config;
    current_language = config->default_language;
    return true;
}

void unsigned_text_set_language(u8 language) {
    if (text_config == NULL || language >= text_config->language_count) {
        return;
    }

    current_language = language;
}

u8 unsigned_text_get_language(void) {
    return current_language;
}

const UText *unsigned_text_get_language_info(u8 language) {
    if (text_config == NULL || text_config->languages == NULL || language >= text_config->language_count) {
        return NULL;
    }

    return &text_config->languages[language];
}

/** Returns the localized catalog string at the requested string-table index. */
static const char *text_string_at(u8 language, u16 string_id) {
    if (text_config == NULL || language >= text_config->language_count || string_id >= text_config->string_count) {
        return NULL;
    }

    const UText *text = &text_config->languages[language];
    if (text->strings == NULL) {
        return NULL;
    }

    return text->strings[string_id];
}

const char *unsigned_text_get(u16 string_id) {
    if (text_config == NULL || current_language == UNSIGNED_LANGUAGE_INVALID) {
        return "";
    }

    const char *text = text_string_at(current_language, string_id);
    if (text != NULL) {
        return text;
    }

    text = text_string_at(text_config->default_language, string_id);
    return text != NULL ? text : "";
}

bool unsigned_text_format(char *buffer, size_t capacity, unsigned int string_id, ...) {
    va_list arguments;

    if (buffer == NULL || capacity == 0u) {
        return false;
    }

    buffer[0] = '\0';
    if (text_config == NULL || string_id > UINT16_MAX || string_id >= text_config->string_count) {
        return false;
    }

    va_start(arguments, string_id);
    int length = vsnprintf(buffer, capacity, unsigned_text_get((u16)string_id), arguments);
    va_end(arguments);

    if (length < 0) {
        buffer[0] = '\0';
        return false;
    }

    return (size_t)length < capacity;
}
