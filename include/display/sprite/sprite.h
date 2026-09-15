/**
 * @file sprite.h
 * @brief Logical animation, frame and sprite presentation state.
 */

#ifndef UNSIGNED_DISPLAY_SPRITE_H
#define UNSIGNED_DISPLAY_SPRITE_H

#include "core/types.h"
#include "display/effect/effect.h"
#include "display/sprite/config.h"
#include "display/sprite/render_state.h"
#include "physics/collision.h"

typedef struct UAnimationNotifyCallbackContainer {
    u8 count;
    const UCallbackFunc *instances;
} UAnimationNotifyCallbackContainer;

typedef struct UFrame {
    /** Tile offset relative to `USpriteDefinition.first_tile` for this animation frame. */
    u16 tile_offset;
    /** Duration in engine ticks; zero is normalized to one tick during playback. */
    u8 duration;
    /** Optional callbacks fired once when the frame becomes current. */
    const UAnimationNotifyCallbackContainer *callbacks;
    /** Optional local-space attack box transformed into actor collision state for this frame. */
    const UCollisionBox *hitbox;
    /** Optional local-space vulnerable box transformed into actor collision state for this frame. */
    const UCollisionBox *hurtbox;
} UFrame;

typedef struct UAnimation {
    u8 count;
    const UFrame *frames;
    UPhysicsCollisionChannel hitbox_channel;
    UPhysicsCollisionChannel hurtbox_channel;
} UAnimation;

typedef struct UAnimationContainer {
    u8 count;
    const UAnimation *instances;
} UAnimationContainer;

typedef struct USpriteDefinition {
    const UAnimationContainer *animations;
    u16 first_tile;
    /**
     * Optional fully transparent tile used to guard unused SCB1 rows.
     *
     * Neo Geo vertical shrinking samples tile-map rows beyond the visible
     * height because SCB3 defines a fixed display window. When
     * `clear_unused_rows` is enabled, the backend fills every unused row with
     * this tile whenever the sprite hardware layout is rebuilt. This prevents
     * stale tile-map data from becoming visible while shrink_y is below 0xff.
     */
    u16 transparent_tile;
    u8 width_tiles;
    u8 height_tiles;
    u8 palette;
    u8 auto_animation;
    /** Whether unused SCB1 tile-map rows must be initialized with `transparent_tile`. */
    bool clear_unused_rows;
} USpriteDefinition;

typedef enum UAnimationPlayback {
    U_SPRITE_PLAY_ONCE = 0,
    U_SPRITE_PLAY_LOOP = 1,
} UAnimationPlayback;

typedef enum UAnimationState {
    U_SPRITE_STOPPED = 0,
    U_SPRITE_PLAYING = 1,
    U_SPRITE_COMPLETED = 2,
} UAnimationState;

typedef struct USprite {
    const USpriteDefinition *definition;
    const UAnimation *current_animation;
    const UFrame *current_frame;
    USpriteRenderState render;
    /** Optional presentation effect sampled by the sprite renderer and advanced by unsigned_sprite_tick(). */
    UEffect effect;
    Vec2 offset;
    u8 animation_index;
    u8 frame_index;
    u8 frame_ticks;
    u8 flip_x;
    u8 flip_y;
    u8 palette;
    UAnimationPlayback playback;
    UAnimationState state;
} USprite;

/**
 * @brief Initializes a logical sprite from its first animation/frame and assigns its Neo Geo hardware sprite range.
 *
 * @param sprite Sprite runtime state to initialize.
 * @param definition Caller-owned immutable sprite definition that must remain valid while the sprite is used.
 * @param first_sprite First hardware sprite index reserved for the sprite columns.
 * @return true when definition dimensions, hardware range and first animation/frame are valid; false otherwise.
 */
bool unsigned_sprite_init(USprite *sprite, const USpriteDefinition *definition, u16 first_sprite);

/**
 * @brief Starts an animation at frame zero and marks sprite tiles dirty for the renderer.
 *
 * @param sprite Initialized logical sprite to update.
 * @param animation_index Animation index in sprite->definition->animations.
 * @param playback Playback policy used when the last frame is reached.
 * @return true when the animation exists and contains a valid first frame; false otherwise.
 */
bool unsigned_sprite_play(USprite *sprite, u8 animation_index, UAnimationPlayback playback);

/**
 * @brief Changes horizontal mirroring and dirties graphics only when the value actually changes.
 *
 * @param sprite Logical sprite whose animation/render state is processed.
 * @param flip_x Whether the sprite is horizontally mirrored. Actor collision follows this base facing flip; effect-only flips remain presentation-only.
 */
void unsigned_sprite_set_flip_x(USprite *sprite, bool flip_x);

/** Changes vertical presentation mirroring. Sprite effects may also toggle it temporarily; collision is unchanged. */
void unsigned_sprite_set_flip_y(USprite *sprite, bool flip_y);

/**
 * @brief Changes the sprite palette and dirties only palette state when necessary.
 *
 * @param sprite Logical sprite whose animation/render state is processed.
 * @param palette Neo Geo sprite palette index.
 */
void unsigned_sprite_set_palette(USprite *sprite, u8 palette);

/**
 * @brief Updates Neo Geo shrink values and dirties only scale state; horizontal shrink is masked to 4 bits.
 *
 * @param sprite Logical sprite whose animation/render state is processed.
 * @param shrink_x Neo Geo horizontal shrink value applied by the renderer.
 * @param shrink_y Neo Geo vertical shrink value applied by the renderer.
 */
void unsigned_sprite_set_shrink(USprite *sprite, u8 shrink_x, u8 shrink_y);

/**
 * @brief Forces a full hardware-state rebuild on the next renderer draw.
 *
 * @param sprite Logical sprite whose animation/render state is processed.
 */
void unsigned_sprite_invalidate_render_state(USprite *sprite);

/**
 * @brief Advances the sprite by one scheduled engine frame.
 *
 * @param sprite Logical sprite whose animation/render state is processed.
 * @param context Opaque caller context passed back to callbacks.
 */
void unsigned_sprite_tick(USprite *sprite, void *context);

/**
 * @brief Returns the current animation.
 *
 * @param sprite Logical sprite whose animation/render state is processed.
 * @return Current immutable animation definition, or NULL for a NULL/uninitialized sprite.
 */
const UAnimation *unsigned_sprite_current_animation(const USprite *sprite);

/**
 * @brief Returns the current frame.
 *
 * @param sprite Logical sprite whose animation/render state is processed.
 * @return Current immutable frame definition, or NULL for a NULL/uninitialized sprite.
 */
const UFrame *unsigned_sprite_current_frame(const USprite *sprite);

#endif
