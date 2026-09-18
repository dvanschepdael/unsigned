/**
 * @file save.h
 * @brief Public persistence API umbrella.
 *
 * Include this header when a caller needs the save subsystem without depending on
 * its record codec or platform backend internals. Operations are synchronous and
 * non-reentrant because storage, block and set helpers reuse fixed scratch buffers.
 */

#ifndef UNSIGNED_SAVE_H
#define UNSIGNED_SAVE_H

#include "save/block.h"
#include "save/set.h"
#include "save/storage.h"

#endif
