/**
 * @file prepared_column.h
 * @brief Pre-encoded Neo Geo sprite-control words prepared before VBlank.
 */

#ifndef UNSIGNED_RENDERER_PREPARED_COLUMN_H
#define UNSIGNED_RENDERER_PREPARED_COLUMN_H

#include "core/types.h"

/** Final Neo Geo SCB2/SCB3/SCB4 words streamed during the commit phase. */
typedef struct UPreparedColumn {
    u16 scb2;
    u16 scb3;
    u16 scb4;
} UPreparedColumn;

#endif
