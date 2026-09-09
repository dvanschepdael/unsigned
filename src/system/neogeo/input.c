/**
 * @file input.c
 * @brief Converts BIOS-maintained controller snapshots into the engine input state.
 *
 * @details
 * Direction/A/B/C/D come from BIOS_PxCURRENT. START/SELECT live in BIOS status bits and are folded
 * into the same UInputMask for gameplay/UI consumption. These START bits are ordinary input state
 * only: lifecycle start acceptance remains exclusively in the BIOS PLAYER_START callback.
 */

#include "system/neogeo/input.h"

#include <ngdevkit/bios-ram.h>

/** Combines raw Neo Geo controller buttons with BIOS status/start state for one player. */
static UInputMask input_with_status_buttons(u8 player, u8 buttons, u8 status) {
    UInputMask result = buttons;
    const u8 start_mask = (u8)(1u << (u8)(player * 2u));
    const u8 select_mask = (u8)(start_mask << 1u);

    if ((status & start_mask) != 0u) {
        result |= U_INPUT_BUTTON_START;
    }
    if ((status & select_mask) != 0u) {
        result |= U_INPUT_BUTTON_SELECT;
    }

    return result;
}

void unsigned_neo_geo_input_poll(UInputManager *input) {
    if (input == NULL) {
        return;
    }

    const u8 status = bios_statcurnt;
    const u8 current[U_INPUT_PLAYER_CAPACITY] = {
        bios_p1current,
        bios_p2current,
        bios_p3current,
        bios_p4current,
    };

    for (u8 player = 0u; player < input->player_count; ++player) {
        const UInputMask buttons = input_with_status_buttons(player, current[player], status);
        unsigned_input_controller_tick(&input->players[player], buttons, input->hold_frames);
    }
}
