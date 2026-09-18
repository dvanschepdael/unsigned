/**
 * @file text.h
 * @brief Localized text catalog and formatting service.
 */

#ifndef UNSIGNED_DISPLAY_TEXT_H
#define UNSIGNED_DISPLAY_TEXT_H

#include "core/types.h"

#define UNSIGNED_LANGUAGE_INVALID 0xff

typedef struct UText {
    const char *code;
    const char *name;
    const char *const *strings;
} UText;

typedef struct UTextConfig {
    const UText *languages;
    u8 language_count;
    u16 string_count;
    u8 default_language;
} UTextConfig;

/**
 * @brief Installs a localized text catalog and selects its configured default language.
 *
 * @param config Caller-owned text catalog configuration; it must remain valid after initialization.
 * @return true when language storage/default index and default-language string table are valid; false otherwise.
 */
bool unsigned_text_init(const UTextConfig *config);

/**
 * @brief Selects the active language used by subsequent text lookups.
 *
 * @param language Language index in the installed catalog; invalid indices are ignored.
 */
void unsigned_text_set_language(u8 language);

/**
 * @brief Returns the currently selected language index.
 * @return Active language index, or UNSIGNED_LANGUAGE_INVALID before successful initialization.
 */
u8 unsigned_text_get_language(void);

/**
 * @brief Returns metadata/string-table information for one language.
 *
 * @param language Language index in the installed catalog.
 * @return Pointer to the catalog language entry, or NULL when text is uninitialized or the index is out of range.
 */
const UText *unsigned_text_get_language_info(u8 language);

/**
 * @brief Returns a localized string with automatic fallback to the configured default language.
 *
 * @param string_id String-table index to look up.
 * @return Localized/default-language string, or an empty string when no valid text is available.
 */
const char *unsigned_text_get(u16 string_id);

/**
 * @brief Formats a localized catalog string into caller-provided storage using printf-style arguments.
 *
 * @param buffer Destination character buffer; cleared on validation/format errors.
 * @param capacity Destination capacity including the terminating NUL byte.
 * @param string_id Catalog string index used as the printf format string.
 * @return true when the formatted result fits completely in buffer; false for invalid input, format failure or truncation.
 */
bool unsigned_text_format(char *buffer, size_t capacity, unsigned int string_id, ...);

#endif
