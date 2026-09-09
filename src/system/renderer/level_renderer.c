/**
 * @file level_renderer.c
 * @brief Allocates non-overlapping Neo Geo sprite ranges for backgrounds and visible level actors.
 */

#include "system/renderer/level_renderer.h"

#include "display/sprite/limits.h"
#include "display/sprite/palette.h"
#include "level/level.h"
#include "level/level_runtime.h"
#include "system/renderer/actor_renderer.h"
#include "system/renderer/renderer_backend.h"
#include "system/renderer/sprite_renderer.h"

/** Returns whether two inclusive hardware sprite ranges overlap. */
static bool level_ranges_overlap(u16 first_a, u16 count_a, u16 first_b, u16 count_b) {
    if (count_a == 0u || count_b == 0u) {
        return false;
    }

    u32 last_a = (u32)first_a + count_a - 1u;
    u32 last_b = (u32)first_b + count_b - 1u;
    return (u32)first_a <= last_b && (u32)first_b <= last_a;
}

/** Clears the level renderer actor span state without freeing caller-owned storage. */
static void level_renderer_clear_actor_span(ULevelRenderer *renderer) {
    if (renderer->rendered_actor_sprite_count != 0u) {
        unsigned_sprite_renderer_clear_range(renderer->rendered_actor_first_sprite, renderer->rendered_actor_sprite_count);
    }
    renderer->rendered_actor_sprite_count = 0u;
}

/** Renders the level actor span from its current logical state. */
static bool level_actor_render_span(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport, u16 *first_sprite, u16 *sprite_count) {
    u16 initial_first_sprite = UINT16_MAX;
    u16 total = 0u;

    if (renderer == NULL || level == NULL || viewport == NULL || first_sprite == NULL || sprite_count == NULL || level->actors.instances == NULL) {
        return false;
    }

    if (!unsigned_actor_renderer_prepare(&level->actors, viewport, &total, renderer->actor_first_sprite_valid ? NULL : &initial_first_sprite)) {
        return false;
    }

    if (total == 0u) {
        *first_sprite = renderer->actor_first_sprite_valid ? renderer->actor_first_sprite : UNSIGNED_SPRITE_FIRST;
        *sprite_count = 0u;
        return true;
    }

    if (!renderer->actor_first_sprite_valid) {
        if (initial_first_sprite == UINT16_MAX) {
            return false;
        }
        renderer->actor_first_sprite = initial_first_sprite;
        renderer->actor_first_sprite_valid = true;
    }

    if ((u32)renderer->actor_first_sprite + total - 1u > UNSIGNED_SPRITE_LAST) {
        return false;
    }

    *first_sprite = renderer->actor_first_sprite;
    *sprite_count = total;
    return true;
}

/** Renders the level actor overlaps background from its current logical state. */
static bool level_actor_render_overlaps_background(const ULevel *level, const UViewport *viewport, u16 first_sprite, u16 sprite_count) {
    if (sprite_count == 0u) {
        return false;
    }

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        const UBackgroundLayer *layer = &level->background.layers[i];

        if (!layer->active || layer->definition == NULL) {
            continue;
        }

        u8 background_columns = unsigned_background_renderer_column_count(layer, viewport);
        if (background_columns == 0u) {
            return true;
        }

        if (level_ranges_overlap(first_sprite, sprite_count, layer->definition->first_sprite, background_columns)) {
            return true;
        }
    }

    return false;
}

/** Computes the level sorted actors layout without drawing it. */
static bool level_layout_sorted_actors(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport, u16 *first_sprite, u16 *sprite_count) {
    if (level->actors.count == 0u) {
        *first_sprite = renderer->actor_first_sprite_valid ? renderer->actor_first_sprite : UNSIGNED_SPRITE_FIRST;
        *sprite_count = 0u;
        return true;
    }

    if (!level_actor_render_span(renderer, level, viewport, first_sprite, sprite_count) || level_actor_render_overlaps_background(level, viewport, *first_sprite, *sprite_count)) {
        return false;
    }

    return unsigned_actor_renderer_layout_prepared(&level->actors, *first_sprite);
}

/** Hides hardware sprite slots that belonged to the previous actor span but are no longer owned this frame. */
static void level_renderer_sync_old_actor_span(ULevelRenderer *renderer, u16 first_sprite, u16 sprite_count) {
    const u16 old_first = renderer->rendered_actor_first_sprite;
    const u16 old_count = renderer->rendered_actor_sprite_count;

    if (old_count == 0u) {
        return;
    }

    if (sprite_count == 0u || old_first != first_sprite) {
        level_renderer_clear_actor_span(renderer);
        return;
    }

    if (old_count > sprite_count) {
        unsigned_sprite_renderer_clear_range((u16)(first_sprite + sprite_count), (u16)(old_count - sprite_count));
    }
}

void unsigned_level_renderer_init(ULevelRenderer *renderer) {
    if (renderer == NULL) {
        return;
    }

    *renderer = (ULevelRenderer){ 0 };
    unsigned_background_renderer_init(&renderer->background);
}

void unsigned_level_renderer_render(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport) {
    u16 first_sprite;
    u16 sprite_count;

    if (renderer == NULL || level == NULL || !unsigned_viewport_is_valid(viewport)) {
        return;
    }

    unsigned_renderer_backend_begin();

    if (level->definition == NULL) {
        level_renderer_clear_actor_span(renderer);
        unsigned_background_renderer_hide(&renderer->background);
        renderer->rendered_definition = NULL;
        renderer->actor_first_sprite_valid = false;
        renderer->actor_layout_complete = false;
        unsigned_renderer_backend_end();
        return;
    }

    if (renderer->rendered_definition != level->definition) {
        level_renderer_clear_actor_span(renderer);
        renderer->actor_first_sprite_valid = false;
        renderer->actor_layout_complete = false;
        renderer->rendered_definition = level->definition;
        unsigned_sprite_palette_set_backdrop_color(level->definition->backdrop_color);
    }

    renderer->actor_layout_complete = level_layout_sorted_actors(renderer, level, viewport, &first_sprite, &sprite_count);

    if (!renderer->actor_layout_complete) {
        first_sprite = renderer->rendered_actor_first_sprite;
        sprite_count = renderer->rendered_actor_sprite_count;
    } else {
        level_renderer_sync_old_actor_span(renderer, first_sprite, sprite_count);
        renderer->rendered_actor_first_sprite = first_sprite;
        renderer->rendered_actor_sprite_count = sprite_count;
    }

    unsigned_background_renderer_draw(&renderer->background, &level->background, viewport);

    if (renderer->actor_layout_complete) {
        for (u8 i = 0u; i < level->actors.count; ++i) {
            unsigned_actor_renderer_draw_prepared(level->actors.instances[i], viewport);
        }
    }

    unsigned_renderer_backend_end();
}

void unsigned_level_renderer_hide(ULevelRenderer *renderer) {
    bool background_visible = false;

    if (renderer == NULL) {
        return;
    }

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        if (renderer->background.layers[i].rendered) {
            background_visible = true;
            break;
        }
    }

    if (renderer->rendered_actor_sprite_count == 0u && !background_visible) {
        renderer->rendered_definition = NULL;
        renderer->actor_first_sprite_valid = false;
        renderer->actor_layout_complete = false;
        return;
    }

    unsigned_renderer_backend_begin();
    level_renderer_clear_actor_span(renderer);
    unsigned_background_renderer_hide(&renderer->background);
    unsigned_renderer_backend_end();

    renderer->rendered_definition = NULL;
    renderer->actor_first_sprite_valid = false;
    renderer->actor_layout_complete = false;
}
