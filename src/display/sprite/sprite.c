/**
 * @file sprite.c
 * @brief Implements logical animation, frame and sprite presentation state.
 */

#include "display/sprite/sprite.h"

#include "display/sprite/limits.h"

#pragma region Animation and playback

/** Returns the animation definition at the requested sprite animation index. */
static const UAnimation *sprite_animation_at(const USpriteDefinition *definition, u8 animation_index) {
    if (definition == NULL) {
        return NULL;
    }

    const UAnimationContainer *animations = definition->animations;
    if (animations == NULL || animations->instances == NULL || animation_index >= animations->count) {
        return NULL;
    }

    return &animations->instances[animation_index];
}

/** Returns the frame definition at the requested index within an animation. */
static const UFrame *sprite_frame_at(const UAnimation *animation, u8 frame_index) {
    if (animation == NULL || animation->frames == NULL || frame_index >= animation->count) {
        return NULL;
    }

    return &animation->frames[frame_index];
}

const UAnimation *unsigned_sprite_current_animation(const USprite *sprite) {
    return sprite == NULL ? NULL : sprite->current_animation;
}

const UFrame *unsigned_sprite_current_frame(const USprite *sprite) {
    return sprite == NULL ? NULL : sprite->current_frame;
}

/** Invokes every callback attached to the newly entered animation frame. */
static void sprite_notify(const UFrame *frame, void *context) {
    if (frame == NULL || frame->callbacks == NULL || frame->callbacks->instances == NULL) {
        return;
    }

    for (u8 i = 0; i < frame->callbacks->count; ++i) {
        UCallbackFunc callback = frame->callbacks->instances[i];
        if (callback != NULL) {
            callback(context);
        }
    }
}

bool unsigned_sprite_init(USprite *sprite, const USpriteDefinition *definition, u16 first_sprite) {
    if (sprite == NULL || definition == NULL || definition->animations == NULL || definition->width_tiles == 0u || !unsigned_sprite_height_is_valid(definition->height_tiles) || !unsigned_sprite_range_is_valid(first_sprite, definition->width_tiles)) {
        return false;
    }

    const UAnimation *animation = sprite_animation_at(definition, 0u);
    const UFrame *frame = sprite_frame_at(animation, 0u);
    if (frame == NULL) {
        return false;
    }

    *sprite = (USprite){
        .definition = definition,
        .current_animation = animation,
        .current_frame = frame,
        .render = {
            .first_sprite = first_sprite,
            .sprite_count = definition->width_tiles,
            .shrink_x = 0x0fu,
            .shrink_y = 0xffu,
            .dirty = U_SPRITE_RENDER_DIRTY_ALL,
        },
        .palette = definition->palette,
        .state = U_SPRITE_STOPPED,
    };
    return true;
}

bool unsigned_sprite_play(USprite *sprite, u8 animation_index, UAnimationPlayback playback) {
    if (sprite == NULL) {
        return false;
    }

    const UAnimation *animation = sprite_animation_at(sprite->definition, animation_index);
    const UFrame *frame = sprite_frame_at(animation, 0u);
    if (frame == NULL) {
        return false;
    }

    sprite->animation_index = animation_index;
    sprite->frame_index = 0u;
    sprite->current_animation = animation;
    sprite->current_frame = frame;
    sprite->frame_ticks = 0u;
    sprite->playback = playback;
    sprite->state = U_SPRITE_PLAYING;
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_TILES);
    return true;
}

void unsigned_sprite_set_flip_x(USprite *sprite, bool flip_x) {
    const u8 value = flip_x ? 1u : 0u;

    if (sprite == NULL) {
        return;
    }

    if (sprite->flip_x != value) {
        sprite->flip_x = value;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_GRAPHICS);
    }
}

void unsigned_sprite_set_palette(USprite *sprite, u8 palette) {
    if (sprite == NULL) {
        return;
    }

    if (sprite->palette != palette) {
        sprite->palette = palette;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_PALETTE);
    }
}

void unsigned_sprite_set_shrink(USprite *sprite, u8 shrink_x, u8 shrink_y) {
    if (sprite == NULL) {
        return;
    }

    shrink_x &= 0x0fu;
    if (sprite->render.shrink_x != shrink_x || sprite->render.shrink_y != shrink_y) {
        sprite->render.shrink_x = shrink_x;
        sprite->render.shrink_y = shrink_y;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_SCALE);
    }
}

void unsigned_sprite_invalidate_render_state(USprite *sprite) {
    if (sprite == NULL) {
        return;
    }

    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_ALL);
}

void unsigned_sprite_tick(USprite *sprite, void *context) {
    if (sprite == NULL || sprite->state != U_SPRITE_PLAYING) {
        return;
    }

    const UAnimation *animation = sprite->current_animation;
    const UFrame *frame = sprite->current_frame;
    if (frame == NULL) {
        sprite->state = U_SPRITE_STOPPED;
        return;
    }

    u8 duration = frame->duration == 0u ? 1u : frame->duration;

    if (sprite->frame_ticks == 0u) {
        sprite->frame_ticks = 1u;
        sprite_notify(frame, context);
        return;
    }

    if (sprite->frame_ticks < duration) {
        ++sprite->frame_ticks;
        return;
    }

    if ((u8)(sprite->frame_index + 1u) < animation->count) {
        ++sprite->frame_index;
        sprite->current_frame = &animation->frames[sprite->frame_index];
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_TILES);
        sprite->frame_ticks = 1u;
        sprite_notify(sprite->current_frame, context);
        return;
    }

    if (sprite->playback == U_SPRITE_PLAY_LOOP) {
        sprite->frame_index = 0u;
        sprite->current_frame = &animation->frames[0];
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_TILES);
        sprite->frame_ticks = 1u;
        sprite_notify(sprite->current_frame, context);
    } else {
        sprite->state = U_SPRITE_COMPLETED;
    }
}

#pragma endregion
