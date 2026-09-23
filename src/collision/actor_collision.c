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
    unsigned_physics_collision_box_set_position(target, position);
}

/** Return whether the actor-owned transformed boxes still match current presentation state. */
static bool actor_collision_transform_is_current(const UActorCollisionState *collision, const UFrame *frame, const Vec2 *position, bool flip_x) {
    return collision->transform_frame == frame && collision->transform_position.x == position->x && collision->transform_position.y == position->y && collision->transform_flip_x == (u8)(flip_x ? 1u : 0u);
}

/** Publish the transform key after current-frame boxes have been refreshed. */
static void actor_collision_cache_transform(UActorCollisionState *collision, const UFrame *frame, const Vec2 *position, bool flip_x) {
    collision->transform_frame = frame;
    collision->transform_position = *position;
    collision->transform_flip_x = flip_x ? 1u : 0u;
}

/** Resets per-frame registration flags before reading collision boxes from the current animation frame. */
static void actor_collision_begin_frame(UActorCollisionState *collision) {
    collision->hitbox_channel = U_COLLISION_CHANNEL_NONE;
    collision->hurtbox_channel = U_COLLISION_CHANNEL_NONE;
    collision->resolve_immediately = false;
}

/** Routes one transformed box to dynamic or persistent collision storage without duplicating frame handling. */
static void actor_collision_register_box(UCollisionManager *manager, UPhysicsCollisionChannel channel, const UCollisionBox *box, bool persistent) {
    if (persistent) {
        unsigned_physics_collision_register_static(manager, channel, box);
    } else {
        unsigned_physics_collision_register(manager, channel, box);
    }
}

void unsigned_actor_collision_deactivate_frame(UActor *actor) {
    UActorCollisionState *collision = &actor->collision;
    if (collision->hitbox_channel == U_COLLISION_CHANNEL_NONE && collision->hurtbox_channel == U_COLLISION_CHANNEL_NONE && !collision->resolve_immediately) {
        return;
    }
    actor_collision_begin_frame(collision);
}

bool unsigned_actor_collision_probe(UActor *actor) {
    /* Collision owns actor presentation state, so the hot probe reads the cached frame directly. */
    const UFrame *frame = actor->sprite.current_frame;
    const bool has_hitbox = frame->hitbox != NULL;

    /* Keep the attack-edge cache correct while full collision materialization is sleeping. */
    if (!has_hitbox && actor->collision.hitbox_channel != U_COLLISION_CHANNEL_NONE) {
        actor->collision.hitbox_channel = U_COLLISION_CHANNEL_NONE;
        actor->collision.resolve_immediately = false;
    }
    return has_hitbox;
}

/**
 * Decide whether collision resolution is due for this actor under TLSS.
 * A hitbox that just became active bypasses the normal temporal schedule once so the first
 * active attack frame cannot be skipped. The immediate flag is consumed by this call.
 */
bool unsigned_actor_collision_should_resolve(UActor *actor, const UTLSS *tlss, u16 slot) {
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
    const UFrame *frame = actor->sprite.current_frame;
    if (frame->hitbox == NULL) {
        return NULL;
    }

    const bool flip_x = actor->sprite.flip_x != 0u;
    if (actor->collision.hitbox_channel != U_COLLISION_CHANNEL_NONE && actor_collision_transform_is_current(&actor->collision, frame, position, flip_x)) {
        return &actor->collision.hitbox;
    }

    actor_collision_place_box(&actor->collision.hitbox, frame->hitbox, position, flip_x);
    return &actor->collision.hitbox;
}

static void actor_collision_refresh(UActor *actor) {
    const UAnimation *animation = actor->sprite.current_animation;
    const UFrame *frame = actor->sprite.current_frame;
    const bool flip_x = actor->sprite.flip_x != 0u;
    const bool transform_current = actor_collision_transform_is_current(&actor->collision, frame, &actor->position, flip_x);
    const bool hitbox_expected = frame->hitbox != NULL;
    const bool hurtbox_expected = frame->hurtbox != NULL;
    const bool hitbox_active = actor->collision.hitbox_channel != U_COLLISION_CHANNEL_NONE;
    const bool hurtbox_active = actor->collision.hurtbox_channel != U_COLLISION_CHANNEL_NONE;

    /* Immutable frame geometry is already materialized in actor-owned world-space storage.
     * On the overwhelmingly common idle frame, preserve the registration metadata too instead
     * of clearing and rebuilding it. Channel comparisons keep this safe even if two animations
     * intentionally share a UFrame object but use different collision channels. */
    if (transform_current && hitbox_active == hitbox_expected && hurtbox_active == hurtbox_expected && (!hitbox_expected || actor->collision.hitbox_channel == animation->hitbox_channel) &&
        (!hurtbox_expected || actor->collision.hurtbox_channel == animation->hurtbox_channel)) {
        return;
    }

    const bool hitbox_was_active = hitbox_active;
    actor_collision_begin_frame(&actor->collision);

    if (hurtbox_expected) {
        if (!transform_current) {
            actor_collision_place_box(&actor->collision.hurtbox, frame->hurtbox, &actor->position, flip_x);
        }
        actor->collision.hurtbox_channel = animation->hurtbox_channel;
    }

    if (hitbox_expected) {
        if (!transform_current) {
            actor_collision_place_box(&actor->collision.hitbox, frame->hitbox, &actor->position, flip_x);
        }
        actor->collision.hitbox_channel = animation->hitbox_channel;
        actor->collision.resolve_immediately = !hitbox_was_active;
    }

    if (!transform_current) {
        actor_collision_cache_transform(&actor->collision, frame, &actor->position, flip_x);
    }

    return;
}

/** Register the refreshed frame geometry into dynamic or persistent manager storage. */
void unsigned_actor_collision_register(UActor *actor, UCollisionManager *manager, bool persistent) {
    actor_collision_refresh(actor);

    if (actor->collision.hurtbox_channel != U_COLLISION_CHANNEL_NONE) {
        actor_collision_register_box(manager, actor->collision.hurtbox_channel, &actor->collision.hurtbox, persistent);
    }

    if (actor->collision.hitbox_channel != U_COLLISION_CHANNEL_NONE) {
        actor_collision_register_box(manager, actor->collision.hitbox_channel, &actor->collision.hitbox, persistent);
    }
}
