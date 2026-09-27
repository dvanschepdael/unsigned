/**
 * @file session_internal.h
 * @brief Internal session transitions layered over BIOS PLAYER_MOD state.
 *
 * Public code gets controlled player/session operations from `session.h`; the runtime uses this
 * surface for masks and lifecycle bookkeeping that should not become game-level API.
 */

#ifndef UNSIGNED_SYSTEM_SESSION_INTERNAL_H
#define UNSIGNED_SYSTEM_SESSION_INTERNAL_H

#include "system/session.h"

u8 unsigned_system_session_player_mask(void);
bool unsigned_system_session_player_is_playing(UNeoGeoPlayer player);
bool unsigned_system_session_has_players(void);
void unsigned_system_session_activate_players(u8 player_mask);
void unsigned_system_session_reset(void);
void unsigned_system_session_begin(void);
bool unsigned_system_session_has_ended(void);

#endif
