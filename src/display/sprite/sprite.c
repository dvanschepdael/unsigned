/**
 * @file sprite.c
 * @brief Implements logical animation, frame and sprite presentation state.
 */

#include "display/sprite/sprite.h"

#pragma region Animation and playback

/** Advance the explicit playback-selection revision without reserving zero as a special value. */
static void sprite_playback_revision_advance(USprite *sprite) {
    ++sprite->playback_revision;
    if (sprite->playback_revision == 0u) {
        sprite->playback_revision = 1u;
    }
}

/** Returns whether batched frame advancement can preserve all gameplay-visible semantics. */
static bool sprite_animation_can_batch(const UAnimation *animation) {
    for (u8 i = 0u; i < animation->count; ++i) {
        const UFrame *frame = &animation->frames[i];
        if (frame->hitbox != NULL || frame->hurtbox != NULL || (frame->callbacks != NULL && frame->callbacks->count > 0u)) {
            return false;
        }
    }
    return true;
}

/** Select one authored animation frame and publish the common playback/render metadata. */
static void sprite_select_frame(USprite *sprite, const UAnimation *animation, u8 animation_index, u8 frame_index) {
    const UFrame *frame = &animation->frames[frame_index];
    const bool frame_changed = sprite->current_frame != frame;

    sprite->animation_index = animation_index;
    sprite->frame_index = frame_index;
    sprite->current_animation = animation;
    sprite->current_frame = frame;
    sprite->frame_ticks = 0u;
    sprite->batchable_animation = sprite_animation_can_batch(animation);
    sprite_playback_revision_advance(sprite);
    if (frame_changed) {
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_TILES);
    }
}

const UFrame *unsigned_sprite_current_frame(const USprite *sprite) {
    return sprite->current_frame;
}

/** Invokes authored callbacks attached to the newly entered animation frame. */
static void sprite_notify(const UFrame *frame, void *context) {
    if (frame->callbacks == NULL || frame->callbacks->count == 0u) {
        return;
    }

    for (u8 i = 0u; i < frame->callbacks->count; ++i) {
        frame->callbacks->instances[i](context);
    }
}

void unsigned_sprite_init(USprite *sprite, const USpriteDefinition *definition, u16 first_sprite) {
    const UAnimation *animation = &definition->animations->instances[0];
    const UFrame *frame = &animation->frames[0];

    *sprite = (USprite){
        .definition = definition,
        .current_animation = animation,
        .current_frame = frame,
        .render =
            {
                .layout =
                    {
                        .first_sprite = first_sprite,
                        .sprite_count = definition->width_tiles,
                        .assigned = true,
                    },
                .shrink_x = 0x0fu,
                .shrink_y = 0xffu,
                .dirty = U_SPRITE_RENDER_DIRTY_ALL,
            },
        .palette = definition->palette,
        .state = U_SPRITE_STOPPED,
        .batchable_animation = sprite_animation_can_batch(animation),
        .playback_revision = 1u,
    };
}

void unsigned_sprite_play(USprite *sprite, u8 animation_index, UAnimationPlayback playback) {
    const UAnimation *animation = &sprite->definition->animations->instances[animation_index];
    sprite_select_frame(sprite, animation, animation_index, 0u);
    sprite->playback = playback;
    sprite->state = U_SPRITE_PLAYING;
}

void unsigned_sprite_set_static_frame(USprite *sprite, u8 animation_index, u8 frame_index) {
    const UAnimation *animation = &sprite->definition->animations->instances[animation_index];
    sprite_select_frame(sprite, animation, animation_index, frame_index);
    sprite->state = U_SPRITE_STOPPED;
}

void unsigned_sprite_set_flip_x(USprite *sprite, bool flip_x) {
    const u8 value = flip_x ? 1u : 0u;
    if (sprite->flip_x != value) {
        sprite->flip_x = value;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_GRAPHICS);
    }
}

void unsigned_sprite_set_flip_y(USprite *sprite, bool flip_y) {
    const u8 value = flip_y ? 1u : 0u;
    if (sprite->flip_y != value) {
        sprite->flip_y = value;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_GRAPHICS);
    }
}

void unsigned_sprite_set_palette(USprite *sprite, u8 palette) {
    if (sprite->palette != palette) {
        sprite->palette = palette;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_PALETTE);
    }
}

void unsigned_sprite_set_shrink(USprite *sprite, u8 shrink_x, u8 shrink_y) {
    if (sprite->render.shrink_x != shrink_x || sprite->render.shrink_y != shrink_y) {
        sprite->render.shrink_x = shrink_x;
        sprite->render.shrink_y = shrink_y;
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_SCALE);
    }
}

void unsigned_sprite_invalidate_render_state(USprite *sprite) {
    unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_ALL);
}

void unsigned_sprite_tick(USprite *sprite, void *context) {
    unsigned_effect_tick(&sprite->effect);
    if (sprite->state != U_SPRITE_PLAYING) {
        return;
    }

    const UAnimation *animation = sprite->current_animation;
    const UFrame *frame = sprite->current_frame;
    const u8 duration = frame->duration;

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
        if (animation->count > 1u) {
            unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_TILES);
        }
        sprite->frame_ticks = 1u;
        sprite_notify(sprite->current_frame, context);
    } else {
        sprite->state = U_SPRITE_COMPLETED;
    }
}

bool unsigned_sprite_can_batch_ticks(const USprite *sprite) {
    return sprite->state != U_SPRITE_PLAYING || sprite->batchable_animation;
}

void unsigned_sprite_tick_batch(USprite *sprite, u16 ticks) {
    if (ticks == 0u) {
        return;
    }

    unsigned_effect_tick_frames(&sprite->effect, ticks);
    if (sprite->state != U_SPRITE_PLAYING) {
        return;
    }

    const UAnimation *animation = sprite->current_animation;
    const UFrame *initial_frame = sprite->current_frame;

    while (ticks > 0u && sprite->state == U_SPRITE_PLAYING) {
        const UFrame *frame = sprite->current_frame;
        const u8 duration = frame->duration;

        if (sprite->frame_ticks == 0u) {
            sprite->frame_ticks = 1u;
            --ticks;
            continue;
        }

        const u16 until_transition = (u16)duration - sprite->frame_ticks + 1u;
        if (ticks < until_transition) {
            sprite->frame_ticks = (u8)(sprite->frame_ticks + ticks);
            break;
        }

        ticks = (u16)(ticks - until_transition);
        if ((u8)(sprite->frame_index + 1u) < animation->count) {
            ++sprite->frame_index;
            sprite->current_frame = &animation->frames[sprite->frame_index];
            sprite->frame_ticks = 1u;
        } else if (sprite->playback == U_SPRITE_PLAY_LOOP) {
            sprite->frame_index = 0u;
            sprite->current_frame = &animation->frames[0];
            sprite->frame_ticks = 1u;
        } else {
            sprite->state = U_SPRITE_COMPLETED;
        }
    }

    if (sprite->current_frame != initial_frame) {
        unsigned_sprite_render_mark_dirty(&sprite->render, U_SPRITE_RENDER_DIRTY_TILES);
    }
}

#pragma endregion
