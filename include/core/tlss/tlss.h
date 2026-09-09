/**
 * @file tlss.h
 * @brief Temporal level-of-simulation scheduling.
 */

#ifndef UNSIGNED_CORE_TLSS_H
#define UNSIGNED_CORE_TLSS_H

#include "core/types.h"

typedef enum UTLSSScale {
    U_TLSS_SCALE_1 = 0,
    U_TLSS_SCALE_2 = 1,
    U_TLSS_SCALE_4 = 2,
    U_TLSS_SCALE_8 = 3,
    U_TLSS_SCALE_16 = 4,
} UTLSSScale;

typedef struct UTLSSScaleConfig {
    UTLSSScale ai;
    UTLSSScale collision;
} UTLSSScaleConfig;

typedef struct UTLSS {
    u16 frame;
    UTLSSScaleConfig scales;
} UTLSS;

typedef struct UTLSSNode {
    u16 last_tick;
    u8 phase;
    u8 mask;
} UTLSSNode;

/**
 * @brief Validates that both configured TLSS scales are supported power-of-two scheduling cadences.
 *
 * @param config AI/collision scale configuration to validate.
 * @return true when config is non-NULL and both scales are in U_TLSS_SCALE_1..U_TLSS_SCALE_16; false otherwise.
 */
bool unsigned_tlss_scale_config_is_valid(const UTLSSScaleConfig *config);

/**
 * @brief Initializes TLSS at the pre-first-frame sentinel with full-rate AI and collision scheduling.
 *
 * @param tlss Scheduler state to initialize; NULL is ignored.
 */
void unsigned_tlss_init(UTLSS *tlss);

/**
 * @brief Advances the global TLSS frame counter once at the start of an engine frame.
 *
 * @param tlss Scheduler whose frame index is advanced; NULL is ignored.
 */
void unsigned_tlss_begin_frame(UTLSS *tlss);

/**
 * @brief Initializes one TLSS node and distributes its phase among peer entries sharing the same cadence.
 *
 * @param tlss Scheduler supplying the current frame for last-tick initialization.
 * @param node Per-entity scheduling state to initialize.
 * @param index Stable index of this entity within its peer group.
 * @param count Number of peer entries across which phases are distributed.
 * @param scale Requested update cadence: every 1, 2, 4, 8 or 16 frames.
 */
void unsigned_tlss_node_init(const UTLSS *tlss, UTLSSNode *node, u16 index, u16 count, UTLSSScale scale);

/**
 * @brief Changes a TLSS node cadence and redistributes its phase from a stable peer index/count.
 *
 * @param node Per-entity scheduling state to update.
 * @param index Stable index of this entity within its peer group.
 * @param count Number of peer entries across which phases are distributed.
 * @param scale Requested update cadence.
 */
void unsigned_tlss_node_set_scale(UTLSSNode *node, u16 index, u16 count, UTLSSScale scale);

/**
 * @brief Changes a TLSS node cadence using a caller-provided stable slot directly as the phase source.
 *
 * @param node Per-entity scheduling state to update.
 * @param slot Stable scheduling slot; masked into the selected cadence period.
 * @param scale Requested update cadence.
 */
void unsigned_tlss_node_set_scale_slot(UTLSSNode *node, u16 slot, UTLSSScale scale);

/**
 * @brief Tests whether a TLSS node mask already represents the requested cadence.
 *
 * @param node Scheduling node to inspect.
 * @param scale Cadence to compare against the node mask.
 * @return true when node is valid and already configured for scale; false otherwise.
 */
bool unsigned_tlss_node_matches_scale(const UTLSSNode *node, UTLSSScale scale);

/**
 * @brief Tests whether the current global frame matches a TLSS node distributed phase.
 *
 * @param tlss Scheduler supplying the current frame.
 * @param node Scheduling node containing cadence mask and phase.
 * @return true when this node is scheduled on the current frame; false for invalid inputs or another phase.
 */
bool unsigned_tlss_should_tick(const UTLSS *tlss, const UTLSSNode *node);

/**
 * @brief Commits a scheduled TLSS update and returns how many engine frames elapsed since that node last ran.
 *
 * @param tlss Scheduler supplying the current frame.
 * @param node Scheduling node whose last_tick is updated.
 * @return Elapsed frame delta, including uint16 wrap semantics; 0 for invalid inputs.
 */
uint16_t unsigned_tlss_tick(const UTLSS *tlss, UTLSSNode *node);

#endif
