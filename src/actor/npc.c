/**
 * @file npc.c
 * @brief Implements NPC runtime state, state graph and activity scheduling.
 */

#include "actor/npc.h"

#include "actor/npc_config.h"

#include <stdint.h>

/** Synchronize the animation catch-up clock when gameplay explicitly changes/restarts playback. */
static void npc_presentation_sync_playback_revision(UNpc *npc, const UTLSS *tlss) {
    const USprite *sprite = &npc->character->actor.sprite;
    if (npc->presentation_playback_revision == sprite->playback_revision) {
        return;
    }

    npc->presentation_playback_revision = sprite->playback_revision;
    /* Playback selected after the prior presentation step begins advancing on this frame. */
    npc->presentation_animation_last_tick = (u16)(tlss->frame - 1u);
}

/** Advance deferred effect time and animation time, which may differ after a playback restart. */
static void npc_presentation_batch_to_current_frame(UNpc *npc, const UTLSS *tlss) {
    USprite *sprite = &npc->character->actor.sprite;
    const u16 effect_elapsed = unsigned_tlss_tick(tlss, &npc->tlss_presentation);
    u16 animation_elapsed = (u16)(tlss->frame - npc->presentation_animation_last_tick);
    npc->presentation_animation_last_tick = tlss->frame;

    /* Effect phase is independent from animation playback. If gameplay restarted the animation
     * between presentation phases, preserve the full effect clock and only bound animation time. */
    if (animation_elapsed > effect_elapsed) {
        animation_elapsed = effect_elapsed;
    }
    if (effect_elapsed > animation_elapsed) {
        unsigned_effect_tick_frames(&sprite->effect, (u16)(effect_elapsed - animation_elapsed));
    }
    unsigned_actor_tick_batch(&npc->character->actor, animation_elapsed);
}

void unsigned_npc_init(UNpc *npc) {
    npc->tlss_ai = (UTLSSNode){.last_tick = UINT16_MAX};
    npc->tlss_presentation = (UTLSSNode){.last_tick = UINT16_MAX};
    npc->presentation_animation_last_tick = UINT16_MAX;
    npc->presentation_playback_revision = npc->character->actor.sprite.playback_revision;

    /* A NULL initial node intentionally represents a passive NPC: it still participates in
     * actor rendering/collision but owns no AI state machine and therefore schedules no AI work. */
    if (npc->state_graph.initial != NULL) {
        unsigned_state_graph_init(&npc->state_graph, npc->state_graph.global, npc->state_graph.initial, npc);
    }
}

void unsigned_npc_tick_scheduled(UNpc *npc, const UTLSS *tlss, u16 slot) {
    npc_presentation_sync_playback_revision(npc, tlss);

    if (npc->activity == U_NPC_ACTIVITY_DORMANT) {
        npc->tlss_presentation.last_tick = tlss->frame;
        npc->presentation_animation_last_tick = tlss->frame;
        return;
    }

    if (!unsigned_sprite_can_batch_ticks(&npc->character->actor.sprite)) {
        /* Gameplay-bearing animations are never collapsed: callbacks/collision frame timing stay exact. */
        unsigned_actor_tick(&npc->character->actor);
        npc->tlss_presentation.last_tick = tlss->frame;
        npc->presentation_animation_last_tick = tlss->frame;
        npc->presentation_playback_revision = npc->character->actor.sprite.playback_revision;
        return;
    }

    if (npc->activity == U_NPC_ACTIVITY_ACTIVE) {
        /* An NPC may become visible between its off-screen phases. Catch it up immediately so
         * rendering never exposes a stale presentation frame/effect phase. */
        npc_presentation_batch_to_current_frame(npc, tlss);
        return;
    }

    const UTLSSScale scale = UNSIGNED_NPC_OFFSCREEN_PRESENTATION_SCALE;
    if (scale == U_TLSS_SCALE_1) {
        npc_presentation_batch_to_current_frame(npc, tlss);
        return;
    }

    if (!unsigned_tlss_node_matches_scale(&npc->tlss_presentation, scale)) {
        unsigned_tlss_node_set_scale_slot(&npc->tlss_presentation, slot, scale);
    }
    if (!unsigned_tlss_should_tick(tlss, &npc->tlss_presentation)) {
        return;
    }

    npc_presentation_batch_to_current_frame(npc, tlss);
}

void unsigned_npc_destroy(UNpc *npc) {
    unsigned_state_graph_stop(&npc->state_graph);
    unsigned_actor_destroy(&npc->character->actor);
}
