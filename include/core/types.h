/**
 * @file types.h
 * @brief Defines the fundamental fixed-width types and tiny shared value types used by Unsigned.
 */

#ifndef UNSIGNED_CORE_TYPES_H
#define UNSIGNED_CORE_TYPES_H

#include <ngdevkit/types.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Returns the number of elements in a real C array.
 *
 * @details
 * This intentionally remains a macro because a function parameter would decay the array to a
 * pointer before `sizeof` is evaluated, losing the element count.
 */
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define ARRAY_COUNT_U8(array) ((u8)ARRAY_COUNT(array))

/**
 * @brief Marks a control-flow path forbidden by the caller/content contract.
 *
 * Use only after exhaustive handling of values whose validity is already guaranteed by an API
 * precondition. Unlike a defensive fallback this emits no recovery behavior in the runtime path.
 */
#define U_UNREACHABLE() __builtin_unreachable()

typedef struct Vec2 {
    s16 x;
    s16 y;
} Vec2;

typedef u8 UEvent;
typedef void (*UCallbackFunc)(void *args);

#endif
