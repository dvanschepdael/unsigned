/**
 * @file state_graph_clock.h
 * @brief Optional frame clock for timed state-graph transitions.
 *
 * UStateGraph owns only logical execution state. This companion runtime owns temporal state so
 * graphs that do not use timeout transitions pay no per-instance timing cost.
 */

#ifndef UNSIGNED_CORE_STATE_GRAPH_CLOCK_H
#define UNSIGNED_CORE_STATE_GRAPH_CLOCK_H

#include "core/state/state_graph.h"

typedef struct UStateGraphClock {
    /** State whose elapsed time is currently tracked; used to detect event/task transitions. */
    const UStateGraphNode *state;
    /** Frames spent in state, saturated at UINT16_MAX. */
    u16 elapsed_frames;
} UStateGraphClock;

/** Reset timing and synchronize the clock with the graph's current state. */
void unsigned_state_graph_clock_reset(UStateGraphClock *clock, const UStateGraph *graph);

/**
 * Tick graph logic, then advance timeout timing only if the same state remained active.
 * A state entered by a task transition starts at zero and is first timed on the next call.
 */
void unsigned_state_graph_clock_tick(UStateGraphClock *clock, UStateGraph *graph);

#endif
