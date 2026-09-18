/**
 * @file types.h
 * @brief Defines shared data types for the UI subsystem.
 */

#ifndef UNSIGNED_UI_TYPES_H
#define UNSIGNED_UI_TYPES_H

#include "core/types.h"

typedef u8 UUIStyleId;
typedef u16 UUIAssetId;

typedef struct UUIRect {
    s16 x;
    s16 y;
    u16 width;
    u16 height;
} UUIRect;

typedef enum UUIElementType {
    U_UI_ELEMENT_PANEL = 0,
    U_UI_ELEMENT_LABEL,
    U_UI_ELEMENT_IMAGE,
    U_UI_ELEMENT_BUTTON,
    U_UI_ELEMENT_PROGRESS_BAR,
    U_UI_ELEMENT_SELECTOR,
} UUIElementType;

typedef enum UUILayoutDirection {
    U_UI_LAYOUT_VERTICAL = 0,
    U_UI_LAYOUT_HORIZONTAL,
} UUILayoutDirection;

typedef enum UUIResult {
    U_UI_RESULT_OK = 0,
    U_UI_RESULT_FULL,
    U_UI_RESULT_INVALID_ARGUMENT,
    U_UI_RESULT_NOT_FOUND,
} UUIResult;

#endif
