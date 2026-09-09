/**
 * @file config.h
 * @brief Compile-time state-graph limits.
 */

#ifndef UNSIGNED_CORE_STATE_CONFIG_H
#define UNSIGNED_CORE_STATE_CONFIG_H

#ifndef UNSIGNED_STATE_GRAPH_MAX_NODES
#define UNSIGNED_STATE_GRAPH_MAX_NODES 32
#endif

#if UNSIGNED_STATE_GRAPH_MAX_NODES < 1 || UNSIGNED_STATE_GRAPH_MAX_NODES > 255
#error "UNSIGNED_STATE_GRAPH_MAX_NODES must fit the non-zero u8 node count"
#endif

#endif
