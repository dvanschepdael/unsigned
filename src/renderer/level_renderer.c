/**
 * @file level_renderer.c
 * @brief Builds a CPU-side frame plan, then commits Neo Geo VRAM work in priority order.
 */

#include "renderer/level_renderer.h"

#include "display/sprite/limits.h"
#include "display/sprite/palette.h"
#include "game/config.h"
#include "level/level.h"
#include "level/level_runtime.h"
#include "renderer/actor_renderer.h"
#include "renderer/sprite_renderer.h"
#include "system/renderer_backend.h"

/** Immediate helper retained for teardown paths that do not use a frame plan. */
static void level_renderer_clear_actor_span(ULevelRenderer *renderer) {
    if (renderer->rendered_actor_sprite_count != 0u) {
        unsigned_sprite_renderer_clear_range(renderer->rendered_actor_first_sprite, renderer->rendered_actor_sprite_count);
    }
    renderer->rendered_actor_sprite_count = 0u;
}

/** Compute the contiguous actor hardware span selected by culling/stable-layout policy.
 * @pre Authored actor ranges fit the Neo Geo hardware range and do not overlap background ranges.
 */
static void level_actor_render_span(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport, u16 *first_sprite, u16 *sprite_count) {
    u16 initial_first_sprite = UINT16_MAX;
    u16 total = 0u;

    const bool stable_ranges = level->definition->stable_actor_sprite_ranges;
    unsigned_actor_renderer_prepare(&level->actors, viewport, &total, &initial_first_sprite, stable_ranges);

    if (total == 0u) {
        *first_sprite = renderer->actor_first_sprite != UINT16_MAX ? renderer->actor_first_sprite : UNSIGNED_SPRITE_FIRST;
        *sprite_count = 0u;
        return;
    }

    if (renderer->actor_first_sprite == UINT16_MAX) {
        renderer->actor_first_sprite = initial_first_sprite;
    }

    *first_sprite = renderer->actor_first_sprite;
    *sprite_count = total;
}

static u16 level_layout_sorted_actors(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport, u16 *first_sprite, u16 *sprite_count) {
    if (level->actors.count == 0u) {
        *first_sprite = renderer->actor_first_sprite != UINT16_MAX ? renderer->actor_first_sprite : UNSIGNED_SPRITE_FIRST;
        *sprite_count = 0u;
        return 0u;
    }

    level_actor_render_span(renderer, level, viewport, first_sprite, sprite_count);

    const bool stable_ranges = level->definition->stable_actor_sprite_ranges;
    if (stable_ranges && renderer->rendered_definition == level->definition && renderer->rendered_actor_layout_revision == level->actors.layout_revision && renderer->rendered_actor_first_sprite == *first_sprite &&
        renderer->rendered_actor_sprite_count == *sprite_count) {
        return 0u;
    }

    return unsigned_actor_renderer_layout_prepared(&level->actors, *first_sprite, stable_ranges);
}

/** Plan the minimum SCB3 clear needed to retire the previous actor span. */
static void level_renderer_plan_old_actor_clear(const ULevelRenderer *renderer, ULevelRenderPlan *plan) {
    const u16 old_first = renderer->rendered_actor_first_sprite;
    const u16 old_count = renderer->rendered_actor_sprite_count;

    if (old_count == 0u) {
        return;
    }

    if (renderer->rendered_definition != plan->level->definition || plan->actor_sprite_count == 0u || old_first != plan->actor_first_sprite) {
        plan->clear_actor_first_sprite = old_first;
        plan->clear_actor_sprite_count = old_count;
    } else if (old_count > plan->actor_sprite_count) {
        plan->clear_actor_first_sprite = (u16)(plan->actor_first_sprite + plan->actor_sprite_count);
        plan->clear_actor_sprite_count = (u16)(old_count - plan->actor_sprite_count);
    }

    unsigned_render_plan_add_words(&plan->frame, U_RENDER_PRIORITY_CRITICAL, plan->clear_actor_sprite_count);
}

#if UNSIGNED_RENDERER_DIAGNOSTICS
/** Add one hardware-sprite span to the visible raster-line pressure accumulator. */
static void level_renderer_scanline_add(u16 *scanlines, s16 y, u16 height_pixels, u16 columns) {
    if (height_pixels == 0u || columns == 0u) {
        return;
    }

    s32 first = y;
    s32 last = (s32)y + height_pixels;
    if (last <= 0 || first >= UNSIGNED_GAME_SCREEN_HEIGHT) {
        return;
    }
    if (first < 0) {
        first = 0;
    }
    if (last > UNSIGNED_GAME_SCREEN_HEIGHT) {
        last = UNSIGNED_GAME_SCREEN_HEIGHT;
    }

    for (s32 line = first; line < last; ++line) {
        const u32 next = (u32)scanlines[line] + columns;
        scanlines[line] = next > UINT16_MAX ? UINT16_MAX : (u16)next;
    }
}

/** Count one actor sprite exactly as the Neo Geo active-list parser sees it. */
static void level_renderer_measure_actor_sprite(u16 *scanlines, const USprite *sprite) {
    if (!sprite->render.layout.visible || sprite->render.prepared.hidden) {
        return;
    }

    /* Vertical shrink changes sampled pixels but not the SCB3 active height seen by the hardware
     * sprite parser. Per-column effects can move columns independently, so use the base screen Y
     * as a conservative diagnostic in that uncommon path. */
    const s16 y = sprite->render.prepared.per_column ? sprite->render.prepared.screen_y : sprite->render.prepared.draw_y;
    level_renderer_scanline_add(scanlines, y, (u16)sprite->definition->height_tiles * 16u, sprite->render.layout.sprite_count);
}

/** Measure hardware sprite pressure for the fully prepared frame without touching VRAM. */
static void level_renderer_measure_scanline_pressure(ULevelRenderPlan *plan) {
    u16 scanlines[UNSIGNED_GAME_SCREEN_HEIGHT] = {0};

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        const UBackgroundLayerRenderPlan *layer_plan = &plan->background.layers[i];
        if (layer_plan->next_state.rendered_definition == NULL || layer_plan->columns == 0u) {
            continue;
        }
        const UBackgroundLayerDefinition *definition = layer_plan->layer->definition;
        level_renderer_scanline_add(scanlines, layer_plan->y, (u16)definition->height_tiles * 16u, layer_plan->columns);
    }

    {
        for (u8 i = 0u; i < plan->level->actors.count; ++i) {
            const UActor *actor = plan->level->actors.instances[i];
            if (actor->underlay != NULL) {
                level_renderer_measure_actor_sprite(scanlines, actor->underlay);
            }
            level_renderer_measure_actor_sprite(scanlines, &actor->sprite);
        }
    }

    for (u16 line = 0u; line < UNSIGNED_GAME_SCREEN_HEIGHT; ++line) {
        const u16 load = scanlines[line];
        if (load > plan->frame.stats.max_sprite_columns_per_scanline) {
            plan->frame.stats.max_sprite_columns_per_scanline = load;
        }
        if (load > UNSIGNED_SPRITE_MAX_PER_SCANLINE) {
            ++plan->frame.stats.sprite_overflow_scanlines;
        }
    }
}
#endif

void unsigned_level_renderer_init(ULevelRenderer *renderer) {
    *renderer = (ULevelRenderer){.actor_first_sprite = UINT16_MAX};
    unsigned_background_renderer_init(&renderer->background);
}

void unsigned_level_renderer_prepare(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport) {
    ULevelRenderPlan *plan;

    plan = &renderer->plan;
    /* Do not clear the 1.5 KiB nested background work arrays every frame. Their counts/modes are
     * reset by the background renderer and every consumed element is overwritten before commit. */
    unsigned_render_plan_reset(&plan->frame);
    unsigned_sprite_column_plan_reset(&plan->sprite_columns);
    plan->level = level;
    plan->actor_first_sprite = 0u;
    plan->actor_sprite_count = 0u;
    plan->clear_actor_first_sprite = 0u;
    plan->clear_actor_sprite_count = 0u;
    if (level->definition == NULL) {
        return;
    }

    const bool definition_changed = renderer->rendered_definition != level->definition;
    if (definition_changed) {
        /* Allocator policy changes now; hardware ownership is retired later in CRITICAL commit. */
        renderer->actor_first_sprite = UINT16_MAX;
        renderer->rendered_actor_layout_revision = 0u;
    }

#if UNSIGNED_RENDERER_DIAGNOSTICS
    plan->frame.stats.actor_relocations = level_layout_sorted_actors(renderer, level, viewport, &plan->actor_first_sprite, &plan->actor_sprite_count);
#else
    (void)level_layout_sorted_actors(renderer, level, viewport, &plan->actor_first_sprite, &plan->actor_sprite_count);
#endif

    if (definition_changed) {
        /* Same hardware indices may be reused by a new stage; force a complete authoritative rewrite. */
        unsigned_actor_renderer_force_rebuild_prepared(&level->actors);
    }
    level_renderer_plan_old_actor_clear(renderer, plan);
    unsigned_actor_renderer_prepare_draws(&level->actors, viewport, &plan->frame, &plan->sprite_columns);

    unsigned_background_renderer_prepare(&renderer->background, &level->background, viewport, &plan->background, &plan->frame);

#if UNSIGNED_RENDERER_DIAGNOSTICS
    level_renderer_measure_scanline_pressure(plan);
#endif
}

/** CRITICAL: retire stale slots and sever every relocated sticky chain before heavier uploads. */
static void level_renderer_commit_critical(ULevelRenderPlan *plan) {
    if (plan->clear_actor_sprite_count != 0u) {
        unsigned_sprite_renderer_clear_range(plan->clear_actor_first_sprite, plan->clear_actor_sprite_count);
    }

    unsigned_actor_renderer_precommit_chain_boundaries(&plan->level->actors);
}

void unsigned_level_renderer_commit(ULevelRenderer *renderer) {
    ULevelRenderPlan *plan;

    plan = &renderer->plan;
    /* A fully unchanged frame needs no Neo Geo renderer transaction at all. Preparation has
     * already updated CPU-side ownership metadata and proved that every renderer priority has
     * zero writes. Keep definition changes out of this path because backdrop publication is an
     * uncounted hardware write performed below. */
    const ULevelDefinition *definition = plan->level->definition;
    const bool hide_all = definition == NULL;
    const bool definition_changed = renderer->rendered_definition != definition;

    if (!hide_all && !definition_changed && !plan->frame.has_work) {
        renderer->rendered_actor_first_sprite = plan->actor_first_sprite;
        renderer->rendered_actor_sprite_count = plan->actor_sprite_count;
        renderer->rendered_actor_layout_revision = plan->level->actors.layout_revision;
        renderer->rendered_definition = definition;
        return;
    }

    unsigned_renderer_backend_begin();

    if (hide_all) {
        level_renderer_clear_actor_span(renderer);
        unsigned_background_renderer_hide(&renderer->background);
        renderer->rendered_definition = NULL;
        renderer->actor_first_sprite = UINT16_MAX;
        unsigned_renderer_backend_end();
        return;
    }

    level_renderer_commit_critical(plan);

    if (definition_changed) {
        unsigned_sprite_palette_set_backdrop_color(definition->backdrop_color);
    }

    /* HIGH: fully rebuild/move actor sprites before any normal-priority background traffic. */
    unsigned_actor_renderer_commit_prepared(&plan->level->actors);

    /* NORMAL: scrolling background uploads/transforms run after actor state is coherent. */
    unsigned_background_renderer_commit(&renderer->background, &plan->background);

    unsigned_renderer_backend_end();

    renderer->rendered_actor_first_sprite = plan->actor_first_sprite;
    renderer->rendered_actor_sprite_count = plan->actor_sprite_count;
    renderer->rendered_actor_layout_revision = plan->level->actors.layout_revision;
    renderer->rendered_definition = definition;
}

const URenderFrameStats *unsigned_level_renderer_stats(const ULevelRenderer *renderer) {
    return &renderer->plan.frame.stats;
}

void unsigned_level_renderer_hide(ULevelRenderer *renderer) {
    bool background_visible = false;

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        if (renderer->background.layers[i].rendered_definition != NULL) {
            background_visible = true;
            break;
        }
    }

    if (renderer->rendered_actor_sprite_count == 0u && !background_visible) {
        renderer->rendered_definition = NULL;
        renderer->actor_first_sprite = UINT16_MAX;
        return;
    }

    unsigned_renderer_backend_begin();
    level_renderer_clear_actor_span(renderer);
    unsigned_background_renderer_hide(&renderer->background);
    unsigned_renderer_backend_end();

    renderer->rendered_definition = NULL;
    renderer->actor_first_sprite = UINT16_MAX;
}
