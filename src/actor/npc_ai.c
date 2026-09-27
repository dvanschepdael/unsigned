/**
 * @file npc_ai.c
 * @brief Implements a compact Beat'Em Up NPC state graph plus TLSS scheduling.
 */

#include "actor/npc_ai.h"

#include "actor/npc.h"
#include "actor/player.h"
#include "core/math/math.h"
#include "gameplay/ability_pool.h"
#include "physics/movement.h"

#include <stdint.h>

enum {
    NPC_AI_SLOT_ROUTE_DIRECT = 0u,
    NPC_AI_SLOT_ROUTE_CLEAR_X = 1u,
    NPC_AI_SLOT_ROUTE_CLEAR_DEPTH = 2u,
    NPC_AI_SLOT_ROUTE_CROSS = 3u,
    NPC_AI_SLOT_ROUTE_FINAL = 4u,
};

/* ------------------------------------------------------------------------- */
/* Shared level coordinator                                                  */
/* ------------------------------------------------------------------------- */

/** Clear tactical owners for one physical player target slot. */
static void npc_ai_target_slots_clear(UNpcAiTargetSlots *target) {
    for (u8 i = 0u; i < UNSIGNED_PLAYER_MAX_SLOTS; ++i) {
        target->owners[i] = 0u;
    }
}

void unsigned_actor_npc_ai_world_init(UNpcAiWorld *world, UPoolInstanceContainer *players, UAbilityPool *abilities) {
    *world = (UNpcAiWorld){
        .players = players,
        .abilities = abilities,
    };
    unsigned_actor_npc_ai_world_reset(world);
}

void unsigned_actor_npc_ai_world_reset(UNpcAiWorld *world) {
    /* Force the next sync for every possible wrapped u16 pool revision. */
    world->player_revision = (u16)(world->players->revision - 1u);

    for (u8 i = 0u; i < UNSIGNED_PLAYER_MAX; ++i) {
        world->targets[i].generation = 0u;
        npc_ai_target_slots_clear(&world->targets[i]);
    }
}

void unsigned_actor_npc_ai_world_sync(UNpcAiWorld *world) {
    UPoolInstanceContainer *players = world->players;
    if (world->player_revision == players->revision) {
        return;
    }

    world->player_revision = players->revision;
    for (u8 i = 0u; i < players->capacity; ++i) {
        const UPoolInstance *instance = &players->instances[i];
        const u16 generation = instance->active ? instance->generation : 0u;
        UNpcAiTargetSlots *target = &world->targets[i];

        if (target->generation == generation) {
            continue;
        }

        target->generation = generation;
        npc_ai_target_slots_clear(target);
    }
}

/* ------------------------------------------------------------------------- */
/* Runtime handles and shared helpers                                        */
/* ------------------------------------------------------------------------- */

/** Absolute signed-32 difference represented without narrowing. */
static u32 npc_ai_abs_s32(s32 value) {
    return value < 0 ? (u32)(-(value + 1)) + 1u : (u32)value;
}

/** Cheap Manhattan distance; selection/reservation do not need square roots or multiplication. */
static u32 npc_ai_distance(Vec2 left, Vec2 right) {
    return npc_ai_abs_s32((s32)left.x - right.x) + npc_ai_abs_s32((s32)left.y - right.y);
}

/** Resolve the selected physical player slot only while its generation and target policy still match. */
static UPlayer *npc_ai_target_player(const UNpc *npc) {
    const UNpcAiRuntime *ai = &npc->ai;
    if (ai->target_index == UNSIGNED_NPC_AI_TARGET_NONE) {
        return NULL;
    }

    const UPoolInstance *instance = &ai->world->players->instances[ai->target_index];
    if (!instance->active || instance->generation != ai->target_generation) {
        return NULL;
    }

    UPlayer *player = instance->args;
    const UGameplayTagContainer *excluded = ai->profile->target_excluded_tags;
    if (excluded != NULL && unsigned_gameplay_tag_has_any(&player->character->tags, excluded)) {
        return NULL;
    }

    return player;
}

UCharacter *unsigned_actor_npc_ai_target(const UNpc *npc) {
    if (npc->ai.profile == NULL) {
        return NULL;
    }

    UPlayer *player = npc_ai_target_player(npc);
    return player == NULL ? NULL : player->character;
}

/** Resolve one authored slot offset to its current target-relative world position. */
static Vec2 npc_ai_slot_world_position(const UNpc *npc, const UCharacter *target, u8 slot) {
    const Vec2 offset = npc->ai.profile->attack_slot_offsets[slot];
    Vec2 position = {
        .x = unsigned_math_saturate_s16((s32)target->actor.position.x + offset.x),
        .y = unsigned_math_saturate_s16((s32)target->actor.position.y + offset.y),
    };
    if (npc->ai.profile->movement_bounds != NULL) {
        unsigned_physics_movement_constrain(&position, npc->ai.profile->movement_bounds);
    }
    return position;
}

/** Release only the reservation still owned by this NPC/target generation. */
static void npc_ai_release_slot(UNpc *npc) {
    UNpcAiRuntime *ai = &npc->ai;
    if (ai->assigned_slot == UNSIGNED_NPC_AI_SLOT_NONE) {
        return;
    }

    if (ai->target_index != UNSIGNED_NPC_AI_TARGET_NONE) {
        UNpcAiTargetSlots *target = &ai->world->targets[ai->target_index];
        if (target->generation == ai->target_generation && target->owners[ai->assigned_slot] == ai->owner_token) {
            target->owners[ai->assigned_slot] = 0u;
        }
    }

    ai->assigned_slot = UNSIGNED_NPC_AI_SLOT_NONE;
    ai->slot_route_phase = NPC_AI_SLOT_ROUTE_DIRECT;
    ai->slot_route_side = 0;
    ai->slot_route_depth_side = 0;
}

/** Return whether the tracked attack slot still represents the same active ability reservation. */
static bool npc_ai_attack_is_current(const UNpc *npc) {
    const UNpcAiRuntime *ai = &npc->ai;
    if (ai->attack_index == UNSIGNED_NPC_AI_ABILITY_NONE) {
        return false;
    }

    const UAbilityPoolInstance *instance = &ai->world->abilities->instances[ai->attack_index];
    return instance->active && instance->generation == ai->attack_generation;
}

/** Cancel the tracked attack when still live, then invalidate the compact handle. */
static void npc_ai_cancel_attack(UNpc *npc) {
    if (npc_ai_attack_is_current(npc)) {
        unsigned_gameplay_ability_pool_release(npc->ai.world->abilities, &npc->ai.world->abilities->instances[npc->ai.attack_index]);
    }
    npc->ai.attack_index = UNSIGNED_NPC_AI_ABILITY_NONE;
}

/** Switch a configured loop animation only when it is not already playing. */
static void npc_ai_play_loop(UNpc *npc, u8 animation) {
    if (animation == UNSIGNED_NPC_AI_ANIMATION_NONE) {
        return;
    }

    USprite *sprite = &npc->character->actor.sprite;
    if (sprite->animation_index == animation && sprite->state == U_SPRITE_PLAYING && sprite->playback == U_SPRITE_PLAY_LOOP) {
        return;
    }

    unsigned_sprite_play(sprite, animation, U_SPRITE_PLAY_LOOP);
}

/** Face the target on the horizontal axis while preserving the previous direction at exact overlap. */
static void npc_ai_face_target(UNpc *npc, const UCharacter *target) {
    const s32 delta = (s32)target->actor.position.x - npc->character->actor.position.x;
    const s16 direction = delta > 0 ? 1 : (delta < 0 ? -1 : 0);
    unsigned_actor_character_set_facing(npc->character, direction);
}

/** Move one axis by the current TLSS wall-clock delta without overshooting the goal. */
static s16 npc_ai_move_axis(s16 current, s16 goal, s16 speed, u16 elapsed) {
    const s32 delta = (s32)goal - current;
    const s32 max_step = (s32)speed * elapsed;

    if (delta > 0) {
        const s32 step = delta < max_step ? delta : max_step;
        return unsigned_math_saturate_s16((s32)current + step);
    }
    if (delta < 0) {
        const s32 magnitude = -delta;
        const s32 step = magnitude < max_step ? magnitude : max_step;
        return unsigned_math_saturate_s16((s32)current - step);
    }
    return current;
}

/** Direct belt-scroller steering: no navigation mesh, heap object or path search. */
static void npc_ai_move_toward(UNpc *npc, Vec2 goal) {
    const UNpcAiProfile *profile = npc->ai.profile;
    Vec2 *position = &npc->character->actor.position;

    position->x = npc_ai_move_axis(position->x, goal.x, profile->horizontal_move_speed, npc->ai.scheduled_elapsed);
    position->y = npc_ai_move_axis(position->y, goal.y, profile->vertical_move_speed, npc->ai.scheduled_elapsed);
}

/** Test the authored rectangular acceptance radius used by approach/roam/back-step movement. */
static bool npc_ai_has_reached(const UNpc *npc, Vec2 goal) {
    const Vec2 position = npc->character->actor.position;
    return npc_ai_abs_s32((s32)goal.x - position.x) <= 1 && npc_ai_abs_s32((s32)goal.y - position.y) <= 1;
}

/** Generate an integer offset in [-radius, radius] without modulo/division. */
static s16 npc_ai_random_offset(u16 *state, s16 radius) {
    const u16 magnitude = unsigned_math_random_bounded_u16(state, (u16)radius + 1u);
    return (unsigned_math_random_u16(state) & 1u) != 0u ? (s16)magnitude : (s16) - (s16)magnitude;
}

/** Resolve a stored target-relative movement offset against the target's current position. */
static Vec2 npc_ai_target_relative_goal(const UNpc *npc, const UCharacter *target) {
    Vec2 goal = {
        .x = unsigned_math_saturate_s16((s32)target->actor.position.x + npc->ai.target_offset.x),
        .y = unsigned_math_saturate_s16((s32)target->actor.position.y + npc->ai.target_offset.y),
    };
    if (npc->ai.profile->movement_bounds != NULL) {
        unsigned_physics_movement_constrain(&goal, npc->ai.profile->movement_bounds);
    }
    return goal;
}

/** Return a signed side relative to the target without forcing an NPC to cross through the player. */
static s16 npc_ai_target_side(const UNpc *npc, const UCharacter *target) {
    const s32 delta = (s32)npc->character->actor.position.x - target->actor.position.x;
    if (delta < 0) {
        return -1;
    }
    if (delta > 0) {
        return 1;
    }
    return npc->character->facing_right ? -1 : 1;
}

/** Return the authored horizontal side of one attack slot relative to the target. */
static s16 npc_ai_slot_side(const UNpc *npc, u8 slot) {
    const s16 x = npc->ai.profile->attack_slot_offsets[slot].x;
    if (x < 0) {
        return -1;
    }
    if (x > 0) {
        return 1;
    }
    return 0;
}

/** Return whether one depth side can preserve the configured bypass clearance in bounds. */
static bool npc_ai_slot_bypass_depth_has_clearance(const UNpcAiProfile *profile, const UCharacter *target, s16 depth_side) {
    if (profile->movement_bounds == NULL) {
        return true;
    }

    if (depth_side < 0) {
        return (s32)target->actor.position.y - profile->movement_bounds->min_y >= profile->slot_bypass_clearance_y;
    }
    return (s32)profile->movement_bounds->max_y - target->actor.position.y >= profile->slot_bypass_clearance_y;
}

/** Pick the shortest safe depth side, preferring a side that has the full authored clearance. */
static s16 npc_ai_choose_slot_bypass_depth_side(UNpc *npc, const UCharacter *target) {
    const UNpcAiProfile *profile = npc->ai.profile;
    const s32 delta_y = (s32)npc->character->actor.position.y - target->actor.position.y;
    s16 preferred = delta_y < 0 ? -1 : (delta_y > 0 ? 1 : ((unsigned_math_random_u16(&npc->ai.random_state) & 1u) != 0u ? 1 : -1));

    if (profile->movement_bounds == NULL) {
        return preferred;
    }

    const s32 above = (s32)target->actor.position.y - profile->movement_bounds->min_y;
    const s32 below = (s32)profile->movement_bounds->max_y - target->actor.position.y;

    if (npc_ai_slot_bypass_depth_has_clearance(profile, target, preferred)) {
        return preferred;
    }
    return above >= below ? -1 : 1;
}

/** Resolve one target-relative detour corner while preserving movement bounds. */
static Vec2 npc_ai_slot_bypass_goal(const UNpc *npc, const UCharacter *target, s16 horizontal_side) {
    const UNpcAiProfile *profile = npc->ai.profile;
    const Vec2 slot_offset = profile->attack_slot_offsets[npc->ai.assigned_slot];
    const s16 slot_distance = (s16)npc_ai_abs_s32(slot_offset.x);
    const s16 clearance_x = slot_distance > profile->slot_bypass_clearance_x ? slot_distance : profile->slot_bypass_clearance_x;
    Vec2 goal = {
        .x = unsigned_math_saturate_s16((s32)target->actor.position.x + (s32)horizontal_side * clearance_x),
        .y = unsigned_math_saturate_s16((s32)target->actor.position.y + (s32)npc->ai.slot_route_depth_side * profile->slot_bypass_clearance_y),
    };

    if (profile->movement_bounds != NULL) {
        unsigned_physics_movement_constrain(&goal, profile->movement_bounds);
    }
    return goal;
}

/** Start an orthogonal route: clear X if needed, move to the safe depth lane, cross, then return to the slot. */
static void npc_ai_begin_slot_bypass(UNpc *npc, const UCharacter *target, s16 current_side) {
    npc->ai.slot_route_side = (s8)current_side;
    npc->ai.slot_route_depth_side = (s8)npc_ai_choose_slot_bypass_depth_side(npc, target);
    npc->ai.slot_route_phase = NPC_AI_SLOT_ROUTE_CLEAR_X;
}

/* ------------------------------------------------------------------------- */
/* State tasks                                                               */
/* ------------------------------------------------------------------------- */

static UTaskState npc_ai_task_face_target(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    UPlayer *target = npc_ai_target_player(npc);
    (void)graph;

    if (target != NULL) {
        npc_ai_face_target(npc, target->character);
    }
    return U_TASK_RUNNING;
}

static UTaskState npc_ai_task_select_target(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    UNpcAiRuntime *ai = &npc->ai;
    UPoolInstanceContainer *players = ai->world->players;
    const Vec2 position = npc->character->actor.position;
    u32 best_distance = UINT32_MAX;
    u8 best_index = UNSIGNED_NPC_AI_TARGET_NONE;
    (void)graph;

    ai->target_index = UNSIGNED_NPC_AI_TARGET_NONE;
    ai->target_generation = 0u;

    for (u8 i = 0u; i < unsigned_pool_iteration_end(players); ++i) {
        const UPoolInstance *instance = &players->instances[i];
        if (!instance->active) {
            continue;
        }

        UPlayer *player = instance->args;
        const UGameplayTagContainer *excluded = ai->profile->target_excluded_tags;
        if (excluded != NULL && unsigned_gameplay_tag_has_any(&player->character->tags, excluded)) {
            continue;
        }

        const u32 distance = npc_ai_distance(position, player->character->actor.position);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }

    if (best_index == UNSIGNED_NPC_AI_TARGET_NONE) {
        return U_TASK_FAILED;
    }

    ai->target_index = best_index;
    ai->target_generation = players->instances[best_index].generation;
    return U_TASK_SUCCESS;
}

static UTaskState npc_ai_task_acquire_slot(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    UNpcAiRuntime *ai = &npc->ai;
    UPlayer *target_player = npc_ai_target_player(npc);
    (void)graph;

    if (target_player == NULL) {
        return U_TASK_FAILED;
    }

    UNpcAiTargetSlots *target_slots = &ai->world->targets[ai->target_index];
    const Vec2 position = npc->character->actor.position;
    u32 best_distance = UINT32_MAX;
    u8 best_slot = UNSIGNED_NPC_AI_SLOT_NONE;

    for (u8 i = 0u; i < ai->profile->attack_slot_count; ++i) {
        if (target_slots->owners[i] != 0u) {
            continue;
        }

        const Vec2 goal = npc_ai_slot_world_position(npc, target_player->character, i);
        const u32 distance = npc_ai_distance(position, goal);
        if (distance < best_distance) {
            best_distance = distance;
            best_slot = i;
        }
    }

    if (best_slot == UNSIGNED_NPC_AI_SLOT_NONE) {
        return U_TASK_FAILED;
    }

    target_slots->owners[best_slot] = ai->owner_token;
    ai->assigned_slot = best_slot;
    ai->slot_route_phase = NPC_AI_SLOT_ROUTE_DIRECT;

    const s16 slot_side = npc_ai_slot_side(npc, best_slot);
    const s16 current_side = npc_ai_target_side(npc, target_player->character);
    if (slot_side != 0 && current_side != slot_side) {
        npc_ai_begin_slot_bypass(npc, target_player->character, current_side);
    }
    return U_TASK_SUCCESS;
}

static UTaskState npc_ai_task_move_to_slot(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    UNpcAiRuntime *ai = &npc->ai;
    UPlayer *target = npc_ai_target_player(npc);
    (void)graph;

    if (target == NULL) {
        return U_TASK_FAILED;
    }

    const UCharacter *target_character = target->character;
    const s16 slot_side = npc_ai_slot_side(npc, ai->assigned_slot);
    const s16 current_side = npc_ai_target_side(npc, target_character);

    /* A direct approach is valid only while the NPC already sits on the slot side. Once a bypass
     * starts, keep its orthogonal route instead of collapsing back to a diagonal as soon as the
     * horizontal crossing passes the player's center line. */
    if (slot_side != 0) {
        if (ai->slot_route_phase == NPC_AI_SLOT_ROUTE_DIRECT && current_side != slot_side) {
            npc_ai_begin_slot_bypass(npc, target_character, current_side);
        } else if ((ai->slot_route_phase == NPC_AI_SLOT_ROUTE_CLEAR_X ||
                    ai->slot_route_phase == NPC_AI_SLOT_ROUTE_CLEAR_DEPTH) &&
                   current_side != 0 && current_side != ai->slot_route_side) {
            npc_ai_begin_slot_bypass(npc, target_character, current_side);
        }

        /* If the selected arena edge no longer leaves the authored depth clearance, rebuild the
         * U-shaped route on the opposite depth side. Clearing X again keeps the lane change away
         * from touch/grab range. */
        if (ai->slot_route_phase != NPC_AI_SLOT_ROUTE_DIRECT &&
            !npc_ai_slot_bypass_depth_has_clearance(ai->profile, target_character, ai->slot_route_depth_side) &&
            npc_ai_slot_bypass_depth_has_clearance(ai->profile, target_character, (s16)-ai->slot_route_depth_side)) {
            const s16 replan_side = current_side != 0 ? current_side : ai->slot_route_side;
            ai->slot_route_side = (s8)replan_side;
            ai->slot_route_depth_side = (s8)-ai->slot_route_depth_side;
            ai->slot_route_phase = NPC_AI_SLOT_ROUTE_CLEAR_X;
        }
    }

    Vec2 *position = &npc->character->actor.position;
    const Vec2 slot_goal = npc_ai_slot_world_position(npc, target_character, ai->assigned_slot);

    if (ai->slot_route_phase == NPC_AI_SLOT_ROUTE_DIRECT) {
        if (npc_ai_has_reached(npc, slot_goal)) {
            *position = slot_goal;
            return U_TASK_SUCCESS;
        }
        npc_ai_move_toward(npc, slot_goal);
        if (npc_ai_has_reached(npc, slot_goal)) {
            *position = slot_goal;
            return U_TASK_SUCCESS;
        }
        return U_TASK_RUNNING;
    }

    const Vec2 route_corner = npc_ai_slot_bypass_goal(npc, target_character, ai->slot_route_side);

    if (ai->slot_route_phase == NPC_AI_SLOT_ROUTE_CLEAR_X) {
        /* First move only horizontally away from the player when the NPC is inside the authored
         * side clearance. If it is already farther out, preserve that X and start the vertical leg. */
        const bool x_is_clear = ai->slot_route_side < 0 ? position->x <= route_corner.x : position->x >= route_corner.x;
        if (x_is_clear) {
            ai->slot_route_phase = NPC_AI_SLOT_ROUTE_CLEAR_DEPTH;
            return U_TASK_RUNNING;
        }

        position->x = npc_ai_move_axis(position->x, route_corner.x, ai->profile->horizontal_move_speed, ai->scheduled_elapsed);
        if (npc_ai_abs_s32((s32)route_corner.x - position->x) <= 1u) {
            position->x = route_corner.x;
            ai->slot_route_phase = NPC_AI_SLOT_ROUTE_CLEAR_DEPTH;
        }
        return U_TASK_RUNNING;
    }

    if (ai->slot_route_phase == NPC_AI_SLOT_ROUTE_CLEAR_DEPTH) {
        /* Second leg is vertical only: enter the safe lane without drifting toward the player. */
        position->y = npc_ai_move_axis(position->y, route_corner.y, ai->profile->vertical_move_speed, ai->scheduled_elapsed);
        if (npc_ai_abs_s32((s32)route_corner.y - position->y) <= 1u) {
            position->y = route_corner.y;
            ai->slot_route_phase = NPC_AI_SLOT_ROUTE_CROSS;
        }
        return U_TASK_RUNNING;
    }

    if (ai->slot_route_phase == NPC_AI_SLOT_ROUTE_CROSS) {
        /* Keep the lane target-relative. If the player changes depth, restore the lane before
         * moving X. The crossing itself is then a single straight horizontal segment. */
        if (npc_ai_abs_s32((s32)route_corner.y - position->y) > 1u) {
            position->y = npc_ai_move_axis(position->y, route_corner.y, ai->profile->vertical_move_speed, ai->scheduled_elapsed);
            if (npc_ai_abs_s32((s32)route_corner.y - position->y) <= 1u) {
                position->y = route_corner.y;
            }
            return U_TASK_RUNNING;
        }

        position->y = route_corner.y;
        position->x = npc_ai_move_axis(position->x, slot_goal.x, ai->profile->horizontal_move_speed, ai->scheduled_elapsed);
        if (npc_ai_abs_s32((s32)slot_goal.x - position->x) <= 1u) {
            position->x = slot_goal.x;
            ai->slot_route_phase = NPC_AI_SLOT_ROUTE_FINAL;
        }
        return U_TASK_RUNNING;
    }

    /* Final leg returns vertically to the exact authored slot. If the player moves horizontally,
     * correct X first while still away in depth, then continue the vertical return. */
    if (npc_ai_abs_s32((s32)slot_goal.x - position->x) > 1u) {
        position->x = npc_ai_move_axis(position->x, slot_goal.x, ai->profile->horizontal_move_speed, ai->scheduled_elapsed);
        if (npc_ai_abs_s32((s32)slot_goal.x - position->x) <= 1u) {
            position->x = slot_goal.x;
        }
        return U_TASK_RUNNING;
    }

    position->x = slot_goal.x;
    position->y = npc_ai_move_axis(position->y, slot_goal.y, ai->profile->vertical_move_speed, ai->scheduled_elapsed);
    if (!npc_ai_has_reached(npc, slot_goal)) {
        return U_TASK_RUNNING;
    }

    *position = slot_goal;
    ai->slot_route_phase = NPC_AI_SLOT_ROUTE_DIRECT;
    return U_TASK_SUCCESS;
}

static UTaskState npc_ai_task_attack(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    UNpcAiRuntime *ai = &npc->ai;
    UAbilityPool *abilities = ai->world->abilities;
    (void)graph;

    if (npc_ai_target_player(npc) == NULL) {
        npc_ai_cancel_attack(npc);
        return U_TASK_FAILED;
    }

    if (ai->attack_index != UNSIGNED_NPC_AI_ABILITY_NONE) {
        if (npc_ai_attack_is_current(npc)) {
            return U_TASK_RUNNING;
        }

        ai->attack_index = UNSIGNED_NPC_AI_ABILITY_NONE;
        return U_TASK_SUCCESS;
    }

    const UGameplayAbility *ability = unsigned_gameplay_ability_find(&npc->character->abilities, ai->profile->attack_ability_tag);
    if (!unsigned_gameplay_ability_can_activate(&npc->character->tags, ability)) {
        return U_TASK_FAILED;
    }

    /* Pool exhaustion is a legitimate runtime outcome when many actors fire abilities together. */
    if (abilities->pool.count == abilities->pool.capacity) {
        return U_TASK_FAILED;
    }

    UAbilityPoolInstance *instance = unsigned_gameplay_ability_pool_reserve(abilities, ability, &npc->character->tags, ai->attack_args);
    ai->attack_index = instance->index;
    ai->attack_generation = instance->generation;
    return U_TASK_RUNNING;
}

static UTaskState npc_ai_task_move_relative_to_target(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    UPlayer *target = npc_ai_target_player(npc);
    (void)graph;

    if (target == NULL) {
        return U_TASK_FAILED;
    }

    /* Re-resolve on every scheduled AI tick. A moving player therefore drags the standoff goal
     * with them instead of leaving the NPC committed to a stale world-space destination. */
    const Vec2 goal = npc_ai_target_relative_goal(npc, target->character);
    if (npc_ai_has_reached(npc, goal)) {
        return U_TASK_SUCCESS;
    }

    npc_ai_move_toward(npc, goal);
    return npc_ai_has_reached(npc, goal) ? U_TASK_SUCCESS : U_TASK_RUNNING;
}

static UTaskState npc_ai_task_release_slot(UStateGraph *graph, void *context) {
    (void)graph;
    npc_ai_release_slot(context);
    return U_TASK_SUCCESS;
}

static UTaskState npc_ai_task_wait(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    (void)graph;

    if (npc->ai.wait_remaining <= npc->ai.scheduled_elapsed) {
        npc->ai.wait_remaining = 0u;
        return U_TASK_SUCCESS;
    }

    npc->ai.wait_remaining = (u16)(npc->ai.wait_remaining - npc->ai.scheduled_elapsed);
    return U_TASK_RUNNING;
}

/* ------------------------------------------------------------------------- */
/* State entry callbacks                                                     */
/* ------------------------------------------------------------------------- */

static void npc_ai_enter_idle(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    (void)graph;
    npc_ai_play_loop(npc, npc->ai.profile->idle_animation);
}

static void npc_ai_enter_move(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    (void)graph;
    npc_ai_play_loop(npc, npc->ai.profile->move_animation);
}

static void npc_ai_enter_wait(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    (void)graph;

    npc->ai.wait_remaining = npc->ai.profile->wait_frames;
    npc_ai_play_loop(npc, npc->ai.profile->idle_animation);
}

static void npc_ai_enter_step_back(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    UNpcAiRuntime *ai = &npc->ai;
    const UNpcAiProfile *profile = ai->profile;
    UPlayer *target = npc_ai_target_player(npc);
    (void)graph;

    if (target != NULL) {
        const UCharacter *target_character = target->character;
        const s16 side = npc_ai_target_side(npc, target_character);
        const u16 distance_span = (u16)(profile->step_back_max_distance - profile->step_back_min_distance + 1);
        const s16 additional_distance = (s16)(profile->step_back_min_distance + (s16)unsigned_math_random_bounded_u16(&ai->random_state, distance_span));
        const u32 current_distance = npc_ai_abs_s32((s32)npc->character->actor.position.x - target_character->actor.position.x);
        const s32 standoff = (s32)current_distance + additional_distance;

        npc_ai_face_target(npc, target_character);
        ai->target_offset = (Vec2){
            .x = unsigned_math_saturate_s16((s32)side * standoff),
            .y = unsigned_math_saturate_s16((s32)npc->character->actor.position.y - target_character->actor.position.y + npc_ai_random_offset(&ai->random_state, profile->step_back_depth)),
        };
    }
    npc_ai_play_loop(npc, profile->move_animation);
}

static void npc_ai_enter_move_around(UStateGraph *graph, void *context) {
    UNpc *npc = context;
    UNpcAiRuntime *ai = &npc->ai;
    const UNpcAiProfile *profile = ai->profile;
    UPlayer *target = npc_ai_target_player(npc);
    (void)graph;

    if (target != NULL) {
        const s16 side = npc_ai_target_side(npc, target->character);
        const u16 distance_span = (u16)(profile->roam_radius_x - profile->roam_min_distance + 1);
        const s16 distance = (s16)(profile->roam_min_distance + (s16)unsigned_math_random_bounded_u16(&ai->random_state, distance_span));

        ai->target_offset = (Vec2){
            .x = (s16)(side * distance),
            .y = npc_ai_random_offset(&ai->random_state, profile->roam_radius_y),
        };
    }
    npc_ai_play_loop(npc, profile->move_animation);
}

/* ------------------------------------------------------------------------- */
/* Immutable shared state graph                                              */
/* ------------------------------------------------------------------------- */

static const UStateGraphNode npc_ai_global_node;
static const UStateGraphNode npc_ai_select_target_node;
static const UStateGraphNode npc_ai_acquire_slot_node;
static const UStateGraphNode npc_ai_move_to_slot_node;
static const UStateGraphNode npc_ai_attack_node;
static const UStateGraphNode npc_ai_step_back_node;
static const UStateGraphNode npc_ai_release_slot_node;
static const UStateGraphNode npc_ai_move_around_node;
static const UStateGraphNode npc_ai_wait_node;

static const UStateGraphTask npc_ai_face_tasks[] = { npc_ai_task_face_target };
static const UStateGraphTask npc_ai_select_target_tasks[] = { npc_ai_task_select_target };
static const UStateGraphTask npc_ai_acquire_slot_tasks[] = { npc_ai_task_acquire_slot };
static const UStateGraphTask npc_ai_move_to_slot_tasks[] = { npc_ai_task_move_to_slot };
static const UStateGraphTask npc_ai_attack_tasks[] = { npc_ai_task_attack };
static const UStateGraphTask npc_ai_step_back_tasks[] = { npc_ai_task_move_relative_to_target };
static const UStateGraphTask npc_ai_release_slot_tasks[] = { npc_ai_task_release_slot };
static const UStateGraphTask npc_ai_move_around_tasks[] = { npc_ai_task_move_relative_to_target };
static const UStateGraphTask npc_ai_wait_tasks[] = { npc_ai_task_wait };

#define NPC_AI_TASKS(name) { .count = ARRAY_COUNT_U8(name), .instances = name }

static const UStateGraphTransition npc_ai_select_success[] = { { .target = &npc_ai_acquire_slot_node } };
static const UStateGraphTransition npc_ai_select_failed[] = { { .target = &npc_ai_wait_node } };
static const UStateGraphTransition npc_ai_acquire_success[] = { { .target = &npc_ai_move_to_slot_node } };
static const UStateGraphTransition npc_ai_acquire_failed[] = { { .target = &npc_ai_move_around_node } };
static const UStateGraphTransition npc_ai_move_to_slot_success[] = { { .target = &npc_ai_attack_node } };
static const UStateGraphTransition npc_ai_move_to_slot_failed[] = { { .target = &npc_ai_release_slot_node } };
static const UStateGraphTransition npc_ai_attack_success[] = { { .target = &npc_ai_step_back_node } };
static const UStateGraphTransition npc_ai_attack_failed[] = { { .target = &npc_ai_release_slot_node } };
static const UStateGraphTransition npc_ai_step_back_completed[] = { { .target = &npc_ai_release_slot_node } };
static const UStateGraphTransition npc_ai_release_completed[] = { { .target = &npc_ai_move_around_node } };
static const UStateGraphTransition npc_ai_move_around_completed[] = { { .target = &npc_ai_select_target_node } };
static const UStateGraphTransition npc_ai_wait_completed[] = { { .target = &npc_ai_select_target_node } };

#define NPC_AI_TRANSITIONS(name) { .count = ARRAY_COUNT_U8(name), .instances = name }

static const UStateGraphTransitionContainer npc_ai_select_success_container = NPC_AI_TRANSITIONS(npc_ai_select_success);
static const UStateGraphTransitionContainer npc_ai_select_failed_container = NPC_AI_TRANSITIONS(npc_ai_select_failed);
static const UStateGraphTransitionContainer npc_ai_acquire_success_container = NPC_AI_TRANSITIONS(npc_ai_acquire_success);
static const UStateGraphTransitionContainer npc_ai_acquire_failed_container = NPC_AI_TRANSITIONS(npc_ai_acquire_failed);
static const UStateGraphTransitionContainer npc_ai_move_to_slot_success_container = NPC_AI_TRANSITIONS(npc_ai_move_to_slot_success);
static const UStateGraphTransitionContainer npc_ai_move_to_slot_failed_container = NPC_AI_TRANSITIONS(npc_ai_move_to_slot_failed);
static const UStateGraphTransitionContainer npc_ai_attack_success_container = NPC_AI_TRANSITIONS(npc_ai_attack_success);
static const UStateGraphTransitionContainer npc_ai_attack_failed_container = NPC_AI_TRANSITIONS(npc_ai_attack_failed);
static const UStateGraphTransitionContainer npc_ai_step_back_completed_container = NPC_AI_TRANSITIONS(npc_ai_step_back_completed);
static const UStateGraphTransitionContainer npc_ai_release_completed_container = NPC_AI_TRANSITIONS(npc_ai_release_completed);
static const UStateGraphTransitionContainer npc_ai_move_around_completed_container = NPC_AI_TRANSITIONS(npc_ai_move_around_completed);
static const UStateGraphTransitionContainer npc_ai_wait_completed_container = NPC_AI_TRANSITIONS(npc_ai_wait_completed);

static const UStateGraphNode npc_ai_global_node = {
    .tasks = NPC_AI_TASKS(npc_ai_face_tasks),
};

static const UStateGraphNode npc_ai_select_target_node = {
    .parent = &npc_ai_global_node,
    .enter = npc_ai_enter_idle,
    .tasks = NPC_AI_TASKS(npc_ai_select_target_tasks),
    .transitions = {
        [U_TRANSITION_ON_SUCCESS] = &npc_ai_select_success_container,
        [U_TRANSITION_ON_FAILED] = &npc_ai_select_failed_container,
    },
};

static const UStateGraphNode npc_ai_acquire_slot_node = {
    .parent = &npc_ai_global_node,
    .enter = npc_ai_enter_idle,
    .tasks = NPC_AI_TASKS(npc_ai_acquire_slot_tasks),
    .transitions = {
        [U_TRANSITION_ON_SUCCESS] = &npc_ai_acquire_success_container,
        [U_TRANSITION_ON_FAILED] = &npc_ai_acquire_failed_container,
    },
};

static const UStateGraphNode npc_ai_move_to_slot_node = {
    .parent = &npc_ai_global_node,
    .enter = npc_ai_enter_move,
    .tasks = NPC_AI_TASKS(npc_ai_move_to_slot_tasks),
    .transitions = {
        [U_TRANSITION_ON_SUCCESS] = &npc_ai_move_to_slot_success_container,
        [U_TRANSITION_ON_FAILED] = &npc_ai_move_to_slot_failed_container,
    },
};

static const UStateGraphNode npc_ai_attack_node = {
    .parent = &npc_ai_global_node,
    .enter = npc_ai_enter_idle,
    .tasks = NPC_AI_TASKS(npc_ai_attack_tasks),
    .transitions = {
        [U_TRANSITION_ON_SUCCESS] = &npc_ai_attack_success_container,
        [U_TRANSITION_ON_FAILED] = &npc_ai_attack_failed_container,
    },
};

static const UStateGraphNode npc_ai_step_back_node = {
    .parent = &npc_ai_global_node,
    .enter = npc_ai_enter_step_back,
    .tasks = NPC_AI_TASKS(npc_ai_step_back_tasks),
    .transitions = {
        [U_TRANSITION_ON_COMPLETED] = &npc_ai_step_back_completed_container,
    },
};

static const UStateGraphNode npc_ai_release_slot_node = {
    .parent = &npc_ai_global_node,
    .tasks = NPC_AI_TASKS(npc_ai_release_slot_tasks),
    .transitions = {
        [U_TRANSITION_ON_COMPLETED] = &npc_ai_release_completed_container,
    },
};

static const UStateGraphNode npc_ai_move_around_node = {
    .parent = &npc_ai_global_node,
    .enter = npc_ai_enter_move_around,
    .tasks = NPC_AI_TASKS(npc_ai_move_around_tasks),
    .transitions = {
        [U_TRANSITION_ON_COMPLETED] = &npc_ai_move_around_completed_container,
    },
};

static const UStateGraphNode npc_ai_wait_node = {
    .parent = &npc_ai_global_node,
    .enter = npc_ai_enter_wait,
    .tasks = NPC_AI_TASKS(npc_ai_wait_tasks),
    .transitions = {
        [U_TRANSITION_ON_COMPLETED] = &npc_ai_wait_completed_container,
    },
};

#undef NPC_AI_TASKS
#undef NPC_AI_TRANSITIONS

/* ------------------------------------------------------------------------- */
/* Configuration / scheduling                                                */
/* ------------------------------------------------------------------------- */

void unsigned_actor_npc_ai_configure(UNpc *npc, UNpcAiWorld *world, const UNpcAiProfile *profile, void *attack_args, u16 random_seed) {
    npc->ai = (UNpcAiRuntime){
        .world = world,
        .profile = profile,
        .attack_args = attack_args,
        .random_state = random_seed,
        .target_index = UNSIGNED_NPC_AI_TARGET_NONE,
        .assigned_slot = UNSIGNED_NPC_AI_SLOT_NONE,
        .attack_index = UNSIGNED_NPC_AI_ABILITY_NONE,
    };
    npc->state_graph.global = &npc_ai_global_node;
    npc->state_graph.initial = &npc_ai_select_target_node;
}

void unsigned_actor_npc_ai_detach(UNpc *npc) {
    if (npc->ai.profile == NULL) {
        return;
    }

    npc_ai_cancel_attack(npc);
    npc_ai_release_slot(npc);
    npc->ai.target_index = UNSIGNED_NPC_AI_TARGET_NONE;
    npc->ai.target_generation = 0u;
}

/** Stop a sleeping/dead built-in graph and release work that must not survive suspension. */
static void npc_ai_suspend(UNpc *npc) {
    npc_ai_cancel_attack(npc);
    npc_ai_release_slot(npc);
    if (npc->state_graph.current != NULL) {
        unsigned_state_graph_stop(&npc->state_graph);
    }
}

void unsigned_actor_npc_ai_tick(UNpc *npc, const UTLSS *tlss, u16 slot) {
    if (npc->activity == U_NPC_ACTIVITY_DORMANT) {
        if (npc->ai.profile != NULL && npc->state_graph.current != NULL) {
            npc_ai_suspend(npc);
        }
        return;
    }
    if (npc->state_graph.initial == NULL) {
        return;
    }

    /* Off-screen NPCs may run less often than the global AI cadence, never more often. */
    const UTLSSScale offscreen_scale = UNSIGNED_NPC_OFFSCREEN_AI_SCALE;
    const UTLSSScale scale = npc->activity == U_NPC_ACTIVITY_OFFSCREEN && (unsigned int)offscreen_scale > (unsigned int)tlss->scales.ai ? offscreen_scale : tlss->scales.ai;
    if (!unsigned_tlss_node_matches_scale(&npc->tlss_ai, scale)) {
        unsigned_tlss_node_set_scale_slot(&npc->tlss_ai, slot, scale);
    }
    if (!unsigned_tlss_should_tick(tlss, &npc->tlss_ai)) {
        return;
    }

    const u16 elapsed = unsigned_tlss_tick(tlss, &npc->tlss_ai);
    if (npc->ai.profile == NULL) {
        unsigned_state_graph_tick(&npc->state_graph);
        return;
    }

    UNpcAiRuntime *ai = &npc->ai;
    ai->scheduled_elapsed = elapsed;
    ai->owner_token = (u8)(slot + 1u);

    const UGameplayTagContainer *stop_tags = ai->profile->stop_tags;
    if (stop_tags != NULL && unsigned_gameplay_tag_has_any(&npc->character->tags, stop_tags)) {
        npc_ai_suspend(npc);
        return;
    }

    if (npc->state_graph.current == NULL) {
        unsigned_state_graph_try_start(&npc->state_graph);
    }

    const UGameplayTagContainer *blocked = ai->profile->control_blocked_tags;
    if (blocked != NULL && unsigned_gameplay_tag_has_any(&npc->character->tags, blocked)) {
        return;
    }

    unsigned_state_graph_tick(&npc->state_graph);
}
