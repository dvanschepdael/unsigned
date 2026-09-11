/**
 * @file credits_internal.h
 * @brief Runtime-only MVS credit/start preparation used by BIOS PLAYER_START handling.
 *
 * Public code may query credits through `credits.h`; only the BIOS callback path prepares the
 * decrement fields used by the BIOS handshake.
 */

#ifndef UNSIGNED_SYSTEM_CREDITS_INTERNAL_H
#define UNSIGNED_SYSTEM_CREDITS_INTERNAL_H

#include "core/types.h"

void neo_geo_credits_init(void);
void neo_geo_credits_prepare_start(u8 accepted_players);

#endif
