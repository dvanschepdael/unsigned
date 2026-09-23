/**
 * @file config.h
 * @brief Screen defaults and storage-capacity helpers for UGameInstance.
 * storage is prepared for a game instance. Runtime code should use the typed functions declared
 * by `game/game.h`; the remaining function-like macros exist only where ISO C requires a
 * compile-time constant expression for file-scope/static array bounds.
 */

#ifndef UNSIGNED_GAME_RUNTIME_CONFIG_H
#define UNSIGNED_GAME_RUNTIME_CONFIG_H

#ifndef UNSIGNED_GAME_SCREEN_WIDTH
#define UNSIGNED_GAME_SCREEN_WIDTH 320
#endif

#ifndef UNSIGNED_GAME_SCREEN_HEIGHT
#define UNSIGNED_GAME_SCREEN_HEIGHT 224
#endif

/**
 * Compile-time actor-capacity formula for file-scope/static array bounds.
 * This formula is the actor-view storage contract used by static game composition.
 */
#define U_GAME_ACTOR_CAPACITY(players, npcs, objects, projectiles) ((players) + (npcs) + (objects) + (projectiles))

/**
 * Compile-time collision-layer storage formula for file-scope/static array bounds.
 * This formula is the storage contract used by static game composition.
 */
#define U_GAME_COLLISION_LAYER_STORAGE_CAPACITY(players, npcs, objects, projectiles) ((3u * (players)) + (3u * (npcs)) + (6u * (objects)) + (2u * (projectiles)) + (2u * U_GAME_ACTOR_CAPACITY((players), (npcs), (objects), (projectiles))))

#endif
