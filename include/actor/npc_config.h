/**
 * @file npc_config.h
 * @brief NPC AI scheduling defaults.
 */

#ifndef UNSIGNED_ACTOR_NPC_CONFIG_H
#define UNSIGNED_ACTOR_NPC_CONFIG_H

#include "core/tlss/tlss.h"

#ifndef UNSIGNED_NPC_ACTIVE_MARGIN_X
#define UNSIGNED_NPC_ACTIVE_MARGIN_X 64
#endif

#ifndef UNSIGNED_NPC_ACTIVE_MARGIN_Y
#define UNSIGNED_NPC_ACTIVE_MARGIN_Y 32
#endif

#ifndef UNSIGNED_NPC_OFFSCREEN_AI_SCALE
#define UNSIGNED_NPC_OFFSCREEN_AI_SCALE U_TLSS_SCALE_16
#endif

#ifndef UNSIGNED_NPC_OFFSCREEN_PRESENTATION_SCALE
#define UNSIGNED_NPC_OFFSCREEN_PRESENTATION_SCALE U_TLSS_SCALE_16
#endif

/* Authoring contract: activity margins are non-negative and off-screen scales are valid UTLSSScale values. */

#endif
