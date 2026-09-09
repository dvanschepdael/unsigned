/**
 * @file credits.h
 * @brief Public MVS credit queries and player-facing credit prompts.
 */

#ifndef UNSIGNED_SYSTEM_NEO_GEO_CREDITS_H
#define UNSIGNED_SYSTEM_NEO_GEO_CREDITS_H

#include "system/neogeo/session.h"

typedef enum UNeoGeoPrompt {
    U_NEO_GEO_PROMPT_NONE = 0,
    U_NEO_GEO_PROMPT_PRESS_START,
    U_NEO_GEO_PROMPT_INSERT_COIN,
} UNeoGeoPrompt;

/** True on standard JP MVS credit routing; US/EU MVS use separate P1/P2 counters. */
bool unsigned_neo_geo_credits_are_shared(void);

/** Return the standard BIOS BCD credit count for P1/P2. AES and P3/P4 return zero. */
u8 unsigned_neo_geo_player_credits(UNeoGeoPlayer player);

/** Prompt appropriate for attract/title presentation. */
UNeoGeoPrompt unsigned_neo_geo_attract_prompt(void);

/** Prompt appropriate for one player. P3/P4 return NONE on standard MVS credit routing. */
UNeoGeoPrompt unsigned_neo_geo_player_prompt(UNeoGeoPlayer player);

#endif
