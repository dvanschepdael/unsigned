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

/** Maximum number of local player targets tracked by the lightweight Beat'Em Up AI coordinator. */
#ifndef UNSIGNED_NPC_AI_MAX_TARGETS
#define UNSIGNED_NPC_AI_MAX_TARGETS 4u
#endif

/** Maximum authored tactical positions around one player target. */
#ifndef UNSIGNED_NPC_AI_MAX_ATTACK_SLOTS
#define UNSIGNED_NPC_AI_MAX_ATTACK_SLOTS 8u
#endif

/* Authoring contract: activity margins are non-negative, off-screen scales are valid UTLSSScale
 * values, the game player capacity fits UNSIGNED_NPC_AI_MAX_TARGETS and authored AI slot counts
 * fit UNSIGNED_NPC_AI_MAX_ATTACK_SLOTS. */

#endif
