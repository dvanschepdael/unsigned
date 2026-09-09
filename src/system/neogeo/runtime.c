#include "system/neogeo/runtime.h"

#include "system/neogeo/audio_backend_internal.h"
#include "system/neogeo/bios_callbacks_internal.h"
#include "system/neogeo/bios_state_internal.h"
#include "system/neogeo/input.h"
#include "system/neogeo/session_internal.h"
#include "system/neogeo/video.h"

#include <ngdevkit/bios-ram.h>
#include <ngdevkit/ng-video.h>

static bool neo_geo_attract_ended = true;

static UNeoGeoBiosRequest neo_geo_bios_request(void) {
    return bios_user_request <= U_NEO_GEO_BIOS_REQUEST_TITLE ? (UNeoGeoBiosRequest)bios_user_request : U_NEO_GEO_BIOS_REQUEST_INVALID;
}

/**
 * Bootstrap one ngdevkit C entry before any potentially long project initialization.
 *
 * MVS can temporarily switch from the cartridge FIX/M1 ROMs to the board FIX/SM1 ROMs while
 * entering forced-start TITLE (USER 3). nullsound's command 1 intentionally silences the YM2610
 * and waits in Z80 RAM for that ROM handoff. Once ngdevkit has selected CRTFIX/M1 again, command 3
 * is required to initialize the cartridge sound driver.
 *
 * A COIN_SOUND raised during USER 2 with GAME START COMPULSION is therefore deferred across the C
 * runtime reset. USER 3 resets nullsound first, then arms the deferred coin event for the first
 * post-VBlank transport slot, when the newly selected M1 driver has had time to initialize.
 */
static void neo_geo_init(USoundCommand coin_sound_command, UNeoGeoBiosRequest request) {
    neo_geo_audio_transport_init();

    if (request == U_NEO_GEO_BIOS_REQUEST_DEMO) {
        /* A fresh attract entry invalidates any stale handoff left by an interrupted boot/session. */
        neo_geo_audio_discard_deferred_coin_sounds();
        neo_geo_audio_reset();
    } else if (request == U_NEO_GEO_BIOS_REQUEST_TITLE) {
        neo_geo_audio_reset();
        neo_geo_audio_resume_deferred_coin_sounds(coin_sound_command);
    }

    neo_geo_bios_callbacks_init(coin_sound_command, request);
    neo_geo_session_reset();
    unsigned_video_init();
}

static void neo_geo_enter_phase(const UNeoGeoRuntimeDefinition *definition, UNeoGeoPhase phase) {
    if (definition->enter_phase != NULL) {
        definition->enter_phase(definition->context, phase);
    }
}

/**
 * Execute one complete game frame.
 *
 * ngdevkit's VBlank handler runs SYSTEM_IO, which updates BIOS controller state and dispatches BIOS
 * callbacks such as PLAYER_START. The frame therefore consumes the BIOS-maintained input snapshot;
 * it never tries to synthesize a START event itself.
 */
static void neo_geo_frame(const UNeoGeoRuntimeDefinition *definition, UNeoGeoPhase phase) {
    unsigned_neo_geo_input_poll(definition->input);

    if (definition->tick != NULL) {
        definition->tick(definition->context);
    }
    if (definition->render != NULL) {
        definition->render(definition->context);
    }
    if (definition->render_phase != NULL) {
        definition->render_phase(definition->context, phase);
    }

    ng_wait_vblank();
    neo_geo_audio_transport_tick();
}

/**
 * Run GAME while a player is PLAYING or waiting in CONTINUE.
 * CONTINUE keeps the session alive so a later BIOS PLAYER_START can restore that slot to PLAYING.
 * GAME_OVER begins only when every participating slot has reached a terminal player mode.
 */
static void neo_geo_run_game(const UNeoGeoRuntimeDefinition *definition) {
    if (!unsigned_neo_geo_game_started()) {
        return;
    }

    neo_geo_session_begin();
    if (definition->start_game != NULL) {
        definition->start_game(definition->context);
    }

    neo_geo_enter_phase(definition, U_NEO_GEO_PHASE_GAME);
    while (!neo_geo_session_has_ended() && neo_geo_session_has_participating_players()) {
        neo_geo_frame(definition, U_NEO_GEO_PHASE_GAME);
    }

    if (neo_geo_session_has_ended()) {
        return;
    }

    neo_geo_enter_phase(definition, U_NEO_GEO_PHASE_GAME_OVER);
    while (!neo_geo_session_has_ended()) {
        neo_geo_frame(definition, U_NEO_GEO_PHASE_GAME_OVER);
    }
}

/**
 * Run USER 2 attract presentation.
 *
 * ATTRACT has two valid exits: PLAYER_START changes BIOS_USER_MODE to GAME, or the presentation calls
 * unsigned_neo_geo_end_attract(). In the latter case this function returns so ngdevkit can execute
 * SYSTEM_RETURN and let the BIOS decide what happens next (important for normal MVS slot rotation).
 */
static void neo_geo_run_demo(const UNeoGeoRuntimeDefinition *definition) {
    bios_user_mode = U_NEO_GEO_MODE_DEMO;
    neo_geo_attract_ended = false;

    neo_geo_enter_phase(definition, U_NEO_GEO_PHASE_ATTRACT);
    while (!neo_geo_attract_ended && !unsigned_neo_geo_game_started()) {
        neo_geo_frame(definition, U_NEO_GEO_PHASE_ATTRACT);
    }

    if (unsigned_neo_geo_game_started()) {
        neo_geo_run_game(definition);
    }
}

static void neo_geo_run_title(const UNeoGeoRuntimeDefinition *definition) {
    if (unsigned_neo_geo_system() != U_NEO_GEO_SYSTEM_MVS) {
        return;
    }

    /* Some MVS BIOS revisions otherwise request USER 3 again after game over with credits present. */
    bios_title_mode = 1u;
    bios_user_mode = U_NEO_GEO_MODE_DEMO;

    neo_geo_enter_phase(definition, U_NEO_GEO_PHASE_TITLE);
    while (!unsigned_neo_geo_game_started()) {
        neo_geo_frame(definition, U_NEO_GEO_PHASE_TITLE);
    }
    neo_geo_run_game(definition);
}

static int neo_geo_run_initialized(const UNeoGeoRuntimeDefinition *definition, void (*run)(const UNeoGeoRuntimeDefinition *)) {
    bool initialized = true;

    neo_geo_bios_begin_runtime(definition);
    if (definition->initialize != NULL) {
        initialized = definition->initialize(definition->context);
    }

    if (initialized) {
        neo_geo_bios_enable_start_requests();
        run(definition);
    }

    neo_geo_bios_end_runtime();
    if (definition->shutdown != NULL) {
        definition->shutdown(definition->context);
    }

    return initialized ? 0 : 1;
}

int unsigned_neo_geo_main(const UNeoGeoRuntimeDefinition *definition, USoundCommand coin_sound_command) {
    if (definition == NULL) {
        return 1;
    }

    const UNeoGeoBiosRequest request = neo_geo_bios_request();
    neo_geo_init(coin_sound_command, request);

    switch (request) {
        case U_NEO_GEO_BIOS_REQUEST_EYE_CATCHER:
            bios_user_mode = U_NEO_GEO_MODE_BOOT;
            if (definition->eye_catcher != NULL) {
                definition->eye_catcher(definition->context);
            }
            return 0;

        case U_NEO_GEO_BIOS_REQUEST_DEMO:
            return neo_geo_run_initialized(definition, neo_geo_run_demo);

        /* USER 0 uses rom_mvs_startup_init; USER 3 uses main_mvs_title(). */
        case U_NEO_GEO_BIOS_REQUEST_INIT:
        case U_NEO_GEO_BIOS_REQUEST_TITLE:
        case U_NEO_GEO_BIOS_REQUEST_INVALID:
        default:
            return 0;
    }
}

int unsigned_neo_geo_main_mvs(const UNeoGeoRuntimeDefinition *definition, USoundCommand coin_sound_command) {
    if (definition == NULL) {
        return 1;
    }

    const UNeoGeoBiosRequest request = neo_geo_bios_request();
    if (request != U_NEO_GEO_BIOS_REQUEST_TITLE) {
        return 0;
    }

    neo_geo_init(coin_sound_command, request);
    return neo_geo_run_initialized(definition, neo_geo_run_title);
}

void unsigned_neo_geo_end_attract(void) {
    neo_geo_attract_ended = true;
}
