/**
 * @file actor_collision.c
 * @brief Builds and registers collision state for one actor at a time.
 */

#include "collision/actor_collision.h"

#include "actor/actor.h"
#include "actor/character.h"
#include "actor/npc.h"
#include "actor/player.h"
#include "display/sprite/sprite.h"

/** Converts a frame-local box to world space; horizontal flip mirrors it around the actor origin. */
static void actor_collision_place_box(UCollisionBox *target, const UCollisionBox *source, const Vec2 *position, bool flip_x) {
    *target = *source;
    if (flip_x) {
        target->offset_x = (s16)(-source->offset_x - source->w);
    }
    target->x = (s16)(target->offset_x + position->x);
    target->y = (s16)(target->offset_y + position->y);
}

/** Resets per-frame registration flags before reading collision boxes from the current animation frame. */
static void actor_collision_begin_frame(UActorCollisionState *collision) {
    collision->hitbox_channel = U_COLLISION_CHANNEL_NONE;
    collision->hurtbox_channel = U_COLLISION_CHANNEL_NONE;
    collision->hitbox_active = false;
    collision->hurtbox_active = false;
    collision->resolve_immediately = false;
}

/** Routes one transformed box to dynamic or persistent collision storage without duplicating frame handling. */
static bool actor_collision_register_box(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box, bool persistent) {
    return persistent ? unsigned_physics_collision_register_static(manager, channel, box) : unsigned_physics_collision_register(manager, channel, box);
}

/**
 * Decide whether collision resolution is due for this actor under TLSS.
 * A hitbox that just became active bypasses the normal temporal schedule once so the first
 * active attack frame cannot be skipped. The immediate flag is consumed by this call.
 */
bool unsigned_actor_collision_should_resolve(UActor *actor, const UTLSS *tlss, u16 slot) {
    if (actor == NULL || !actor->active) {
        return false;
    }

    if (tlss == NULL) {
        actor->collision.resolve_immediately = false;
        return true;
    }

    UTLSSNode *node = &actor->collision.tlss_collision;
    if (!unsigned_tlss_node_matches_scale(node, tlss->scales.collision)) {
        unsigned_tlss_node_set_scale_slot(node, slot, tlss->scales.collision);
    }

    if (actor->collision.resolve_immediately) {
        actor->collision.resolve_immediately = false;
        (void)unsigned_tlss_tick(tlss, node);
        return true;
    }

    if (!unsigned_tlss_should_tick(tlss, node)) {
        return false;
    }

    (void)unsigned_tlss_tick(tlss, node);
    return true;
}

/**
 * Place the current animation frame hitbox at an arbitrary world position.
 * The result is written into actor-owned scratch storage and accounts for sprite flip.
 */
const UCollisionBox *unsigned_actor_collision_current_hitbox(UActor *actor, const Vec2 *position) {
    if (actor == NULL || position == NULL) {
        return NULL;
    }

    const UFrame *frame = unsigned_sprite_current_frame(&actor->sprite);
    if (frame == NULL || frame->hitbox == NULL) {
        return NULL;
    }

    actor_collision_place_box(&actor->collision.hitbox, frame->hitbox, position, actor->sprite.flip_x != 0u);
    return &actor->collision.hitbox;
}

/**
 * Register the current frame's hurtbox and hitbox for one active actor.
 * Registration is best-effort per box: if one layer is full the other box can still be
 * registered, while the function returns false to expose the incomplete frame state.
 */
static bool actor_collision_register(UActor *actor, UCollisionManager *manager, bool persistent) {
    bool complete = true;

    if (actor == NULL || manager == NULL || !actor->active) {
        return false;
    }

    bool hitbox_was_active = actor->collision.hitbox_active;
    actor_collision_begin_frame(&actor->collision);

    const UAnimation *animation = unsigned_sprite_current_animation(&actor->sprite);
    const UFrame *frame = unsigned_sprite_current_frame(&actor->sprite);
    if (animation == NULL || frame == NULL) {
        return false;
    }

    if (frame->hurtbox != NULL) {
        actor_collision_place_box(&actor->collision.hurtbox, frame->hurtbox, &actor->position, actor->sprite.flip_x != 0u);
        if (actor_collision_register_box(manager, animation->hurtbox_channel, &actor->collision.hurtbox, persistent)) {
            actor->collision.hurtbox_channel = animation->hurtbox_channel;
            actor->collision.hurtbox_active = true;
        } else {
            complete = false;
        }
    }

    if (frame->hitbox != NULL) {
        actor_collision_place_box(&actor->collision.hitbox, frame->hitbox, &actor->position, actor->sprite.flip_x != 0u);
        if (actor_collision_register_box(manager, animation->hitbox_channel, &actor->collision.hitbox, persistent)) {
            actor->collision.hitbox_channel = animation->hitbox_channel;
            actor->collision.hitbox_active = true;
            actor->collision.resolve_immediately = !hitbox_was_active;
        } else {
            complete = false;
        }
    }

    return complete;
}

bool unsigned_actor_collision_register(UActor *actor, UCollisionManager *manager) {
    return actor_collision_register(actor, manager, false);
}

bool unsigned_actor_collision_register_static(UActor *actor, UCollisionManager *manager) {
    return actor_collision_register(actor, manager, true);
}

bool unsigned_character_collision_register(UCharacter *character, UCollisionManager *manager) {
    if (character == NULL) {
        return false;
    }

    return unsigned_actor_collision_register(&character->actor, manager);
}

bool unsigned_player_collision_register(UPlayer *player, UCollisionManager *manager) {
    if (player == NULL || player->character == NULL) {
        return false;
    }

    return unsigned_character_collision_register(player->character, manager);
}

bool unsigned_npc_collision_register(UNpc *npc, UCollisionManager *manager) {
    if (npc == NULL || npc->character == NULL) {
        return false;
    }

    return unsigned_character_collision_register(npc->character, manager);
}
