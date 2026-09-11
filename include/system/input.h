/**
 * @file input.h
 * @brief Neo Geo BIOS controller-snapshot adapter.
 *
 * The adapter updates UInputManager from BIOS-maintained state after SYSTEM_IO. It does not perform
 * hardware probing and does not generate PLAYER_START lifecycle events.
 */

#ifndef UNSIGNED_SYSTEM_INPUT_H
#define UNSIGNED_SYSTEM_INPUT_H

#include "input/input.h"

/**
 * Advance each configured controller from the current BIOS snapshot.
 * input may be NULL; at most input->player_count slots are updated.
 */
void unsigned_neo_geo_input_poll(UInputManager *input);

#endif
