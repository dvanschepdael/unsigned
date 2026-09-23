/**
 * @file renderer_backend.h
 * @brief Global renderer transaction boundary for the active video backend.
 *
 * @details
 * Domain-specific sprite, background and palette operations live in their dedicated backend headers.
 */

#ifndef UNSIGNED_SYSTEM_RENDERER_BACKEND_H
#define UNSIGNED_SYSTEM_RENDERER_BACKEND_H

/** Begin a renderer batch. VRAMMOD writes may be cached until the matching end call. */
void unsigned_renderer_backend_begin(void);

/** End the current renderer batch and invalidate backend-side write caches. */
void unsigned_renderer_backend_end(void);

#endif
