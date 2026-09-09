/**
 * @file runtime.h
 * @brief Neo Geo BIOS-owned lifecycle runner and convenience umbrella for platform services.
 *
 * This is not a conventional self-owned `while (1)` game loop: ngdevkit/BIOS enters cartridge
 * code through USER requests. The runtime translates those entries into ATTRACT/TITLE/GAME/
 * GAME_OVER phases while keeping BIOS callbacks authoritative for player start/session state.
 */

#ifndef UNSIGNED_SYSTEM_NEO_GEO_RUNTIME_H
#define UNSIGNED_SYSTEM_NEO_GEO_RUNTIME_H

#include "audio/audio_types.h"
#include "core/types.h"
#include "input/input.h"

typedef enum UNeoGeoPhase {
    U_NEO_GEO_PHASE_ATTRACT = 0,
    U_NEO_GEO_PHASE_TITLE,
    U_NEO_GEO_PHASE_GAME,
    U_NEO_GEO_PHASE_GAME_OVER,
    U_NEO_GEO_PHASE_COUNT,
} UNeoGeoPhase;

typedef void (*UNeoGeoPhaseCallback)(void *context, UNeoGeoPhase phase);
typedef bool (*UNeoGeoInitializeCallback)(void *context);

/**
 * Filter one BIOS PLAYER_START request.
 *
 * Bits 0..3 correspond to P1..P4. The callback may only clear bits from requested_players.
 * PLAYER_START is disabled while initialize() and shutdown() run, so this callback only sees a
 * fully initialized runtime context.
 */
typedef u8 (*UNeoGeoStartRequestCallback)(void *context, u8 requested_players);

/**
 * Application callbacks for one BIOS USER entry.
 *
 * initialize/shutdown bracket USER 2 and USER 3 only. All other callbacks are optional. GAME stays
 * active while at least one player is PLAYING or CONTINUE.
 */
typedef struct UNeoGeoRuntimeDefinition {
    void *context;
    UInputManager *input;
    UNeoGeoInitializeCallback initialize;
    UCallbackFunc shutdown;
    UCallbackFunc eye_catcher;
    UNeoGeoStartRequestCallback accept_start;
    UCallbackFunc start_game;
    UCallbackFunc tick;
    UCallbackFunc render;
    UNeoGeoPhaseCallback enter_phase;
    UNeoGeoPhaseCallback render_phase;
} UNeoGeoRuntimeDefinition;

/** Run ngdevkit main() for USER 1/2. Returns 1 only for invalid arguments or failed initialization. */
int unsigned_neo_geo_main(const UNeoGeoRuntimeDefinition *definition, USoundCommand coin_sound_command);

/** Run ngdevkit main_mvs_title() for USER 3. */
int unsigned_neo_geo_main_mvs(const UNeoGeoRuntimeDefinition *definition, USoundCommand coin_sound_command);

/** End USER 2 attract mode and return lifecycle control to the BIOS. */
void unsigned_neo_geo_end_attract(void);

/*
 * Convenience umbrella: existing callers of runtime.h keep access to the small public Neo Geo
 * services, while focused modules may include session.h, credits.h or settings.h directly.
 */
#include "system/neogeo/credits.h"
#include "system/neogeo/session.h"
#include "system/neogeo/settings.h"

#endif
