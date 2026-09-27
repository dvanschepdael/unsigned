/**
 * @file npc_ai.h
 * @brief Lightweight Beat'Em Up NPC decision runtime and TLSS scheduling.
 *
 * The reusable engine policy mirrors the usual arcade loop without importing a pathfinder or a
 * heavyweight behavior-tree runtime:
 *
 *   select target -> reserve tactical slot -> approach -> attack -> step back -> release -> roam
 *           \-----------------------------------------------------------------------/
 *                                      retry
 *
 * Target choice, slot reservation and ability activation remain separate decisions. Immutable
 * tuning lives in UNpcAiProfile; mutable per-NPC state stays in UNpcAiRuntime; the level-owned
 * UNpcAiWorld keeps only the tiny reservation table shared by all enemies.
 */

#ifndef UNSIGNED_ACTOR_NPC_AI_H
#define UNSIGNED_ACTOR_NPC_AI_H

#include "actor/npc_config.h"
#include "core/pool/pool.h"
#include "core/tlss/tlss.h"
#include "gameplay/tag.h"

#include <stdint.h>

#define UNSIGNED_NPC_AI_TARGET_NONE UINT8_MAX
#define UNSIGNED_NPC_AI_SLOT_NONE UINT8_MAX
#define UNSIGNED_NPC_AI_ABILITY_NONE UINT8_MAX
#define UNSIGNED_NPC_AI_ANIMATION_NONE UINT8_MAX

struct UAbilityPool;
struct UCharacter;
struct UMovementBounds;
struct UNpc;

/**
 * Immutable content tuning for the built-in Beat'Em Up NPC state graph.
 *
 * Slot offsets are world-plane offsets relative to the target actor origin. They are reservations,
 * not path nodes: movement remains direct integer steering appropriate for a 2D belt-scroller.
 * Optional tag sets model the UE-style global Hurt/Die interruption without an event-listener
 * object per NPC: control blockers temporarily pause decision work, while stop tags cancel the
 * tracked attack, release the tactical slot and stop the graph until the tag disappears.
 *
 * @invariant `attack_slot_count` is in 1..UNSIGNED_NPC_AI_MAX_ATTACK_SLOTS and
 *            `attack_slot_offsets` contains that many entries.
 * @invariant `attack_ability_tag != UNSIGNED_GAMEPLAY_TAG_NONE` and resolves in the NPC catalog.
 * @invariant movement speeds/tolerances/radii/distances are non-negative,
 *            `roam_min_distance <= roam_radius_x` and
 *            `step_back_min_distance <= step_back_max_distance`.
 * @invariant animation indices are valid for the NPC sprite when they are not
 *            UNSIGNED_NPC_AI_ANIMATION_NONE.
 */
typedef struct UNpcAiProfile {
    const Vec2 *attack_slot_offsets;
    const struct UMovementBounds *movement_bounds;
    const UGameplayTagContainer *target_excluded_tags;
    const UGameplayTagContainer *control_blocked_tags;
    const UGameplayTagContainer *stop_tags;
    UGameplayTag attack_ability_tag;
    s16 horizontal_move_speed;
    s16 vertical_move_speed;
    s16 arrival_tolerance_x;
    s16 arrival_tolerance_y;
    /** Minimum horizontal target standoff used while waiting for an attack slot. */
    s16 roam_min_distance;
    /** Maximum horizontal target standoff used while waiting for an attack slot. */
    s16 roam_radius_x;
    /** Maximum target-relative depth variation used while waiting for an attack slot. */
    s16 roam_radius_y;
    s16 step_back_min_distance;
    s16 step_back_max_distance;
    s16 step_back_depth;
    u16 wait_frames;
    u8 attack_slot_count;
    u8 idle_animation;
    u8 move_animation;
} UNpcAiProfile;

/** Reservation state attached to one physical player-pool slot. */
typedef struct UNpcAiTargetSlots {
    /** Player reservation generation currently represented by `owners`; zero means no active target. */
    u16 generation;
    /** Zero means free; otherwise the value is the stable NPC pool slot + 1. */
    u8 owners[UNSIGNED_NPC_AI_MAX_ATTACK_SLOTS];
} UNpcAiTargetSlots;

/**
 * Small level-owned coordinator shared by all built-in Beat'Em Up NPCs.
 *
 * It borrows the fixed player and ability pools. Player pool revisions are used to clear slot
 * reservations only when player membership changes, avoiding a per-frame rebuild.
 */
typedef struct UNpcAiWorld {
    UPoolInstanceContainer *players;
    struct UAbilityPool *abilities;
    u16 player_revision;
    UNpcAiTargetSlots targets[UNSIGNED_NPC_AI_MAX_TARGETS];
} UNpcAiWorld;

/** Mutable state required only by NPCs configured for the built-in Beat'Em Up policy. */
typedef struct UNpcAiRuntime {
    UNpcAiWorld *world;
    const UNpcAiProfile *profile;
    /** Optional content-owned arguments forwarded to the configured attack ability. */
    void *attack_args;
    /** Target-relative goal used by retreat/wait movement and re-resolved as the target moves. */
    Vec2 target_offset;
    u16 target_generation;
    u16 attack_generation;
    u16 random_state;
    /** Wall-clock frames left in the no-target retry state. */
    u16 wait_remaining;
    /** Engine-frame delta represented by the current TLSS AI update. */
    u16 scheduled_elapsed;
    u8 target_index;
    u8 assigned_slot;
    u8 attack_index;
    /** Stable NPC pool slot encoded as slot + 1 for the shared reservation table. */
    u8 owner_token;
} UNpcAiRuntime;

/**
 * @brief Initializes the level-owned target/ability coordinator over existing fixed pools.
 * @pre `world`, `players` and `abilities` are valid for the complete level lifetime.
 * @pre `players->capacity <= UNSIGNED_NPC_AI_MAX_TARGETS`.
 */
void unsigned_actor_npc_ai_world_init(UNpcAiWorld *world, UPoolInstanceContainer *players, struct UAbilityPool *abilities);

/** Reset transient target reservations while preserving the borrowed pool pointers. */
void unsigned_actor_npc_ai_world_reset(UNpcAiWorld *world);

/**
 * Synchronize player reservation generations after player pool membership changes.
 * Call once before ticking Beat'Em Up NPC AI for the frame.
 */
void unsigned_actor_npc_ai_world_sync(UNpcAiWorld *world);

/**
 * @brief Configures one NPC to use the built-in Beat'Em Up state graph.
 *
 * Call from the NPC spawn/content init callback before unsigned_actor_npc_init() / pool reserve.
 * The profile is immutable shared content; `attack_args` may be NULL when the attack ability does
 * not require caller context.
 *
 * @pre `npc`, `world` and `profile` are valid and the profile satisfies its authoring invariants.
 * @pre `random_seed != 0`.
 */
void unsigned_actor_npc_ai_configure(struct UNpc *npc, UNpcAiWorld *world, const UNpcAiProfile *profile, void *attack_args, u16 random_seed);

/**
 * @brief Return the currently selected player character, or NULL when the target was released,
 *        reused or became excluded by the configured target tags.
 */
struct UCharacter *unsigned_actor_npc_ai_target(const struct UNpc *npc);

/**
 * @brief Releases Beat'Em Up AI-owned transient resources before an NPC slot is destroyed/reused.
 *
 * Passive/custom-state-graph NPCs have no built-in AI resources and therefore require no work.
 */
void unsigned_actor_npc_ai_detach(struct UNpc *npc);

/**
 * @brief Advances one NPC state graph only on the TLSS frames assigned to its current activity level.
 *
 * Passive/custom state graphs retain the previous scheduling behavior. NPCs configured with
 * unsigned_actor_npc_ai_configure() additionally execute the shared Beat'Em Up target/slot/attack
 * policy and use the elapsed TLSS delta for movement/wait timing so off-screen throttling does not
 * make their simulated speed 16x slower.
 *
 * @param npc NPC runtime whose character/state-graph scheduling state is updated.
 * @param tlss TLSS scheduler that determines temporal update cadence.
 * @param slot Stable NPC pool slot used to phase-distribute TLSS work across frames.
 * @pre `npc`, `tlss` and any configured state graph are valid runtime objects.
 * @pre The configured UNpcAiWorld has been synchronized for the current player-pool revision.
 */
void unsigned_actor_npc_ai_tick(struct UNpc *npc, const UTLSS *tlss, u16 slot);

#endif
