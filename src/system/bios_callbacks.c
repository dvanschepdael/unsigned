#include "audio/audio_types.h"
#include "system/audio_backend_internal.h"
#include "system/bios_callbacks_internal.h"
#include "system/bios_state_internal.h"
#include "system/credits.h"
#include "system/credits_internal.h"
#include "system/session_internal.h"
#include "system/settings.h"

#include <ngdevkit/bios-ram.h>

typedef enum UNeoGeoRuntimeBindingState {
    U_NEO_GEO_RUNTIME_UNBOUND = 0,
    U_NEO_GEO_RUNTIME_INITIALIZING,
    U_NEO_GEO_RUNTIME_READY,
} UNeoGeoRuntimeBindingState;

static volatile USoundCommand neo_geo_coin_sound_command;
static volatile UNeoGeoBiosRequest neo_geo_runtime_request = U_NEO_GEO_BIOS_REQUEST_INVALID;
static volatile u8 neo_geo_runtime_binding_state = U_NEO_GEO_RUNTIME_UNBOUND;
static const UNeoGeoRuntimeDefinition *neo_geo_runtime_definition;

void neo_geo_bios_callbacks_init(USoundCommand coin_sound_command, UNeoGeoBiosRequest request) {
    neo_geo_runtime_binding_state = U_NEO_GEO_RUNTIME_UNBOUND;
    neo_geo_runtime_definition = NULL;
    neo_geo_runtime_request = request;
    neo_geo_coin_sound_command = (unsigned_audio_command_is_game(coin_sound_command) && coin_sound_command < 128u) ? coin_sound_command : U_AUDIO_COMMAND_NONE;
    neo_geo_credits_init();
}

void neo_geo_bios_begin_runtime(const UNeoGeoRuntimeDefinition *definition) {
    neo_geo_runtime_definition = definition;
    neo_geo_runtime_binding_state = U_NEO_GEO_RUNTIME_INITIALIZING;
}

void neo_geo_bios_enable_start_requests(void) {
    if (neo_geo_runtime_definition != NULL) {
        neo_geo_runtime_binding_state = U_NEO_GEO_RUNTIME_READY;
    }
}

void neo_geo_bios_end_runtime(void) {
    neo_geo_runtime_binding_state = U_NEO_GEO_RUNTIME_UNBOUND;
    neo_geo_runtime_definition = NULL;
}

static bool neo_geo_bios_forced_start_handoff_pending(void) {
    if (neo_geo_runtime_request != U_NEO_GEO_BIOS_REQUEST_DEMO || unsigned_neo_geo_system() != U_NEO_GEO_SYSTEM_MVS || bios_user_mode != (u8)U_NEO_GEO_MODE_DEMO || !unsigned_neo_geo_game_start_compulsion_enabled()) {
        return false;
    }

    /*
     * BIOS revisions do not all expose the transition at exactly the same instruction boundary.
     * USER_REQUEST=TITLE is the strongest signal. An active compulsion timer or a newly available
     * credit are safe fallbacks while we are still executing the USER 2 runtime. A partial coin
     * that has not produced a credit keeps all three signals false and remains on the immediate
     * COIN_SOUND path.
     */
    return bios_user_request == (u8)U_NEO_GEO_BIOS_REQUEST_TITLE || bios_compulsion_timer_over == 0u || bios_compulsion_timer != 0u || unsigned_neo_geo_player_credits(U_NEO_GEO_PLAYER_1) != 0u || unsigned_neo_geo_player_credits(U_NEO_GEO_PLAYER_2) != 0u;
}

void neo_geo_bios_play_coin_sound(void) {
    const USoundCommand command = neo_geo_coin_sound_command;
    if (command == U_AUDIO_COMMAND_NONE) {
        return;
    }

    /*
     * In forced-start mode the first attract credit causes USER 3. The BIOS may switch to board
     * FIX/SM1 on that path; nullsound command 1 then silences YM2610 by design. Playing the coin now
     * would therefore start a sample that is immediately cut. Preserve the event instead and let
     * USER 3 replay it after CRTFIX/M1 and sound-driver command 3.
     *
     * Credits inserted in GAME or an already-running TITLE do not cause this USER 2 -> USER 3
     * handoff and must remain immediate.
     */
    if (neo_geo_bios_forced_start_handoff_pending()) {
        neo_geo_audio_defer_coin_sound();
        return;
    }

    neo_geo_audio_play_bios_sound(command);
}

void neo_geo_bios_process_start(void) {
    const u8 requested = (u8)(bios_start_flag & 0x0fu);
    const u8 already_playing = neo_geo_session_playing_player_mask();
    const u8 candidates = (u8)(requested & (u8)~already_playing);
    u8 accepted = 0u;

    if (neo_geo_runtime_binding_state == U_NEO_GEO_RUNTIME_READY && neo_geo_runtime_definition != NULL) {
        accepted = candidates;
        if (neo_geo_runtime_definition->accept_start != NULL) {
            accepted = neo_geo_runtime_definition->accept_start(neo_geo_runtime_definition->context, candidates);
            accepted &= candidates;
        }
    }

    bios_start_flag = accepted;
    neo_geo_credits_prepare_start(accepted);

    if (accepted != 0u) {
        neo_geo_session_activate_players(accepted);
        bios_user_mode = U_NEO_GEO_MODE_GAME;
    }
}

void player_start(void) {
    neo_geo_bios_process_start();
}

void coin_sound(void) {
    neo_geo_bios_play_coin_sound();
}
