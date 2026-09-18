/**
 * @file config.h
 * @brief Compile-time capacities for gameplay hit detection.
 */

#ifndef UNSIGNED_COLLISION_CONFIG_H
#define UNSIGNED_COLLISION_CONFIG_H

#ifndef UNSIGNED_COLLISION_HIT_CAPACITY
#define UNSIGNED_COLLISION_HIT_CAPACITY 128
#endif

#if UNSIGNED_COLLISION_HIT_CAPACITY < 1 || UNSIGNED_COLLISION_HIT_CAPACITY > 65535
#error "UNSIGNED_COLLISION_HIT_CAPACITY must fit the non-zero u16 hit counter capacity"
#endif

#endif
