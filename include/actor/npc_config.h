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

#if UNSIGNED_NPC_ACTIVE_MARGIN_X < 0 || UNSIGNED_NPC_ACTIVE_MARGIN_Y < 0
#error "NPC activity margins must be non-negative"
#endif

#endif
