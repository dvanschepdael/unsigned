/**
 * @file projectile_collision.h
 * @brief Projectile overlap resolution against actor collision data.
 */

#ifndef UNSIGNED_COLLISION_PROJECTILE_H
#define UNSIGNED_COLLISION_PROJECTILE_H

#include "collision/actor_index.h"
#include "core/pool/pool.h"
#include "core/tlss/tlss.h"
#include "physics/collision.h"

/**
 * @brief Resolves active projectile overlaps and maps collision boxes back to their owning actors.
 *
 * @param collisions Collision manager containing the registered world/gameplay boxes.
 * @param actors Actor container/index used to map collision results to gameplay entities.
 * @param projectiles Projectile pool whose active instances are tested/resolved.
 * @param tlss TLSS scheduler that determines temporal update cadence.
 */
void unsigned_collision_projectiles_resolve(UCollisionManager *collisions, const UCollisionActorIndex *actors, UPoolInstanceContainer *projectiles, const UTLSS *tlss);

#endif
