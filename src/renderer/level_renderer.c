/**
 * @file level_renderer.c
 * @brief Builds a CPU-side frame plan, then commits Neo Geo VRAM work in priority order.
 */

#include "renderer/level_renderer.h"

#include "display/sprite/limits.h"
#include "display/sprite/palette.h"
#include "level/level.h"
#include "level/level_runtime.h"
#include "renderer/actor_renderer.h"
#include "renderer/sprite_renderer.h"
#include "system/renderer_backend.h"

static bool level_ranges_overlap(u16 first_a, u16 count_a, u16 first_b, u16 count_b) {
    if (count_a == 0u || count_b == 0u) {
        return false;
    }

    const u32 last_a = (u32)first_a + count_a - 1u;
    const u32 last_b = (u32)first_b + count_b - 1u;
    return (u32)first_a <= last_b && (u32)first_b <= last_a;
}

/** Immediate helper retained for teardown paths that do not use a frame plan. */
static void level_renderer_clear_actor_span(ULevelRenderer *renderer) {
    if (renderer->rendered_actor_sprite_count != 0u) {
        unsigned_sprite_renderer_clear_range(renderer->rendered_actor_first_sprite, renderer->rendered_actor_sprite_count);
    }
    renderer->rendered_actor_sprite_count = 0u;
}

static bool level_actor_render_span(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport,
                                    u16 *first_sprite, u16 *sprite_count) {
    u16 initial_first_sprite = UINT16_MAX;
    u16 total = 0u;

    if (renderer == NULL || level == NULL || viewport == NULL || first_sprite == NULL || sprite_count == NULL || level->actors.instances == NULL) {
        return false;
    }

    if (!unsigned_actor_renderer_prepare(&level->actors, viewport, &total,
                                         renderer->actor_first_sprite_valid ? NULL : &initial_first_sprite)) {
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

static bool level_actor_render_overlaps_background(const ULevel *level, const UViewport *viewport,
                                                   u16 first_sprite, u16 sprite_count) {
    if (sprite_count == 0u) {
        return false;
    }

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        const UBackgroundLayer *layer = &level->background.layers[i];

        if (!layer->active || layer->definition == NULL) {
            continue;
        }

        const u8 background_columns = unsigned_background_renderer_column_count(layer, viewport);
        if (background_columns == 0u) {
            return true;
        }

        if (level_ranges_overlap(first_sprite, sprite_count, layer->definition->first_sprite, background_columns)) {
            return true;
        }
    }

    return false;
}

static bool level_layout_sorted_actors(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport,
                                       u16 *first_sprite, u16 *sprite_count, u16 *relocation_count) {
    if (level->actors.count == 0u) {
        *first_sprite = renderer->actor_first_sprite_valid ? renderer->actor_first_sprite : UNSIGNED_SPRITE_FIRST;
        *sprite_count = 0u;
        if (relocation_count != NULL) {
            *relocation_count = 0u;
        }
        return true;
    }

    if (!level_actor_render_span(renderer, level, viewport, first_sprite, sprite_count) ||
        level_actor_render_overlaps_background(level, viewport, *first_sprite, *sprite_count)) {
        return false;
    }

    return unsigned_actor_renderer_layout_prepared(&level->actors, *first_sprite, relocation_count);
}

/** Plan the minimum SCB3 clear needed to retire the previous actor span. */
static void level_renderer_plan_old_actor_clear(const ULevelRenderer *renderer, ULevelRenderPlan *plan) {
    const u16 old_first = renderer->rendered_actor_first_sprite;
    const u16 old_count = renderer->rendered_actor_sprite_count;

    if (old_count == 0u) {
        return;
    }

    if (plan->definition_changed || plan->actor_sprite_count == 0u || old_first != plan->actor_first_sprite) {
        plan->clear_actor_range = true;
        plan->clear_actor_first_sprite = old_first;
        plan->clear_actor_sprite_count = old_count;
    } else if (old_count > plan->actor_sprite_count) {
        plan->clear_actor_range = true;
        plan->clear_actor_first_sprite = (u16)(plan->actor_first_sprite + plan->actor_sprite_count);
        plan->clear_actor_sprite_count = (u16)(old_count - plan->actor_sprite_count);
    }

    if (plan->clear_actor_range) {
        unsigned_render_plan_add_words(&plan->frame, U_RENDER_PRIORITY_CRITICAL, plan->clear_actor_sprite_count);
    }
}

void unsigned_level_renderer_init(ULevelRenderer *renderer) {
    if (renderer == NULL) {
        return;
    }

    *renderer = (ULevelRenderer){ 0 };
    unsigned_background_renderer_init(&renderer->background);
}

bool unsigned_level_renderer_prepare(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport) {
    ULevelRenderPlan *plan;
    u16 relocation_count = 0u;

    if (renderer == NULL || level == NULL || !unsigned_viewport_is_valid(viewport)) {
        return false;
    }

    plan = &renderer->plan;
    *plan = (ULevelRenderPlan){ 0 };
    unsigned_render_plan_reset(&plan->frame);
    plan->level = level;
    plan->viewport = viewport;
    plan->definition = level->definition;

    if (level->definition == NULL) {
        plan->hide_all = true;
        plan->frame.valid = true;
        return true;
    }

    plan->definition_changed = renderer->rendered_definition != level->definition;
    if (plan->definition_changed) {
        /* Allocator policy changes now; hardware ownership is retired later in CRITICAL commit. */
        renderer->actor_first_sprite_valid = false;
        renderer->actor_layout_complete = false;
    }

    plan->actor_layout_complete = level_layout_sorted_actors(renderer, level, viewport,
                                                             &plan->actor_first_sprite,
                                                             &plan->actor_sprite_count,
                                                             &relocation_count);
    plan->frame.stats.actor_relocations = relocation_count;

    if (plan->actor_layout_complete) {
        if (plan->definition_changed) {
            /* Same hardware indices may be reused by a new stage; force a complete authoritative rewrite. */
            unsigned_actor_renderer_force_rebuild_prepared(&level->actors);
        }
        level_renderer_plan_old_actor_clear(renderer, plan);
        unsigned_actor_renderer_prepare_draws(&level->actors, viewport, &plan->frame);
    } else if (plan->definition_changed && renderer->rendered_actor_sprite_count != 0u) {
        plan->clear_actor_range = true;
        plan->clear_actor_first_sprite = renderer->rendered_actor_first_sprite;
        plan->clear_actor_sprite_count = renderer->rendered_actor_sprite_count;
        unsigned_render_plan_add_words(&plan->frame, U_RENDER_PRIORITY_CRITICAL, plan->clear_actor_sprite_count);
    }

    if (!unsigned_background_renderer_prepare(&renderer->background, &level->background, viewport,
                                              &plan->background, &plan->frame)) {
        return false;
    }

    plan->frame.valid = true;
    return true;
}

/** CRITICAL: retire stale slots and sever every relocated sticky chain before heavier uploads. */
static void level_renderer_commit_critical(ULevelRenderer *renderer, ULevelRenderPlan *plan) {
    if (plan->clear_actor_range && plan->clear_actor_sprite_count != 0u) {
        unsigned_sprite_renderer_clear_range(plan->clear_actor_first_sprite, plan->clear_actor_sprite_count);
    }

    if (plan->actor_layout_complete) {
        unsigned_actor_renderer_precommit_chain_boundaries(&plan->level->actors);
    }

    (void)renderer;
}

/** HIGH: fully rebuild/move actor sprites before any normal-priority background traffic. */
static void level_renderer_commit_high(ULevelRenderPlan *plan) {
    if (!plan->actor_layout_complete) {
        return;
    }

    for (u8 i = 0u; i < plan->level->actors.count; ++i) {
        unsigned_actor_renderer_draw_underlay_prepared(plan->level->actors.instances[i], plan->viewport);
    }
    for (u8 i = 0u; i < plan->level->actors.count; ++i) {
        unsigned_actor_renderer_draw_body_prepared(plan->level->actors.instances[i], plan->viewport);
    }
}

/** NORMAL: scrolling background uploads/transforms run after actor state is coherent. */
static void level_renderer_commit_normal(ULevelRenderer *renderer, ULevelRenderPlan *plan) {
    unsigned_background_renderer_commit(&renderer->background, &plan->background);
}

void unsigned_level_renderer_commit(ULevelRenderer *renderer) {
    ULevelRenderPlan *plan;

    if (renderer == NULL) {
        return;
    }
    plan = &renderer->plan;
    if (!plan->frame.valid) {
        return;
    }

    unsigned_renderer_backend_begin();

    if (plan->hide_all) {
        level_renderer_clear_actor_span(renderer);
        unsigned_background_renderer_hide(&renderer->background);
        renderer->rendered_definition = NULL;
        renderer->actor_first_sprite_valid = false;
        renderer->actor_layout_complete = false;
        unsigned_renderer_backend_end();
        plan->frame.valid = false;
        return;
    }

    level_renderer_commit_critical(renderer, plan);

    if (plan->definition_changed) {
        unsigned_sprite_palette_set_backdrop_color(plan->definition->backdrop_color);
    }

    level_renderer_commit_high(plan);
    level_renderer_commit_normal(renderer, plan);

    unsigned_renderer_backend_end();

    if (plan->actor_layout_complete) {
        renderer->rendered_actor_first_sprite = plan->actor_first_sprite;
        renderer->rendered_actor_sprite_count = plan->actor_sprite_count;
    } else if (plan->definition_changed) {
        renderer->rendered_actor_sprite_count = 0u;
    }
    renderer->actor_layout_complete = plan->actor_layout_complete;
    renderer->rendered_definition = plan->definition;
    plan->frame.valid = false;
}

void unsigned_level_renderer_render(ULevelRenderer *renderer, ULevel *level, const UViewport *viewport) {
    if (unsigned_level_renderer_prepare(renderer, level, viewport)) {
        unsigned_level_renderer_commit(renderer);
    }
}

const URenderFrameStats *unsigned_level_renderer_stats(const ULevelRenderer *renderer) {
    return renderer != NULL ? &renderer->plan.frame.stats : NULL;
}

bool unsigned_level_renderer_has_prepared_frame(const ULevelRenderer *renderer) {
    return renderer != NULL && renderer->plan.frame.valid;
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
        renderer->plan.frame.valid = false;
        return;
    }

    unsigned_renderer_backend_begin();
    level_renderer_clear_actor_span(renderer);
    unsigned_background_renderer_hide(&renderer->background);
    unsigned_renderer_backend_end();

    renderer->rendered_definition = NULL;
    renderer->actor_first_sprite_valid = false;
    renderer->actor_layout_complete = false;
    renderer->plan.frame.valid = false;
}
