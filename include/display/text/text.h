/**
 * @file text.h
 * @brief Localized text catalog and formatting service.
 */

#ifndef UNSIGNED_DISPLAY_TEXT_H
#define UNSIGNED_DISPLAY_TEXT_H

#include "core/types.h"

typedef struct UText {
    const char *code;
    const char *name;
    const char *const *strings;
} UText;

/**
 * @brief Immutable localized string catalog installed for the lifetime of the text service.
 *
 * @invariant `languages` contains every authored language and `default_language` identifies one of them.
 * @invariant Every language owns a string table covering the authored string-id range.
 * @invariant The default language owns every authored string entry; other languages may store NULL entries to request fallback.
 */
typedef struct UTextConfig {
    const UText *languages;
    u8 default_language;
} UTextConfig;

/**
 * @brief Installs a localized text catalog and selects its configured default language.
 *
 * @param config Caller-owned text catalog configuration; it must remain valid after initialization.
 * @pre `config` is valid and satisfies UTextConfig invariants; authored catalog relationships are not rescanned at initialization.
 */
void unsigned_text_init(const UTextConfig *config);

/**
 * @brief Selects the active language used by subsequent text lookups.
 *
 * @param language Language index in the installed catalog.
 * @pre `language` addresses an authored entry in the installed language table.
 */
void unsigned_text_set_language(u8 language);

/**
 * @brief Returns the currently selected language index.
 * @return Active language index.
 * @pre The text service has been initialized.
 */
u8 unsigned_text_get_language(void);

/**
 * @brief Returns metadata/string-table information for one language.
 *
 * @param language Language index in the installed catalog.
 * @return Pointer to the catalog language entry.
 * @pre The service is initialized and `language` addresses an authored language entry.
 */
const UText *unsigned_text_get_language_info(u8 language);

/**
 * @brief Returns a localized string with automatic fallback to the configured default language.
 *
 * @param string_id String-table index to look up.
 * @return Localized string, falling back to the authored default language when this language omits it.
 * @pre The service is initialized and `string_id` addresses an authored string entry.
 */
const char *unsigned_text_get(u16 string_id);

/**
 * @brief Formats a localized catalog string into caller-provided storage using printf-style arguments.
 *
 * @param buffer Destination character buffer.
 * @param capacity Destination capacity including the terminating NUL byte.
 * @param string_id Catalog string index used as the printf format string.
 *        This parameter uses `int` because it is the final named argument of a variadic C function;
 *        catalog identifiers remain authored in the `u16` range.
 * @pre `buffer` is valid, `capacity > 0`, the service is initialized and `string_id` addresses an authored string entry.
 */
void unsigned_text_format(char *buffer, size_t capacity, int string_id, ...);

#endif
