# Neo Geo BIOS integration

French version: [`bios_fr.md`](bios_fr.md)

Unsigned keeps Neo Geo BIOS lifecycle code under `src/system/` and exposes the corresponding public platform APIs under `include/system/`. Game code works with runtime phases and typed player/session state while BIOS RAM access, callbacks, credit routing, sound transport and persistent cabinet settings remain in the system layer.

## Lifecycle boundary

| BIOS request | ngdevkit entry | Unsigned behavior |
| --- | --- | --- |
| USER 0 | `rom_mvs_startup_init` | handled by ngdevkit startup code |
| USER 1 | `main()` | optional eye catcher; no USER 2/3 runtime initialization |
| USER 2 | `main()` | initialize, ATTRACT, optional GAME/GAME_OVER, shutdown |
| USER 3 | `main_mvs_title()` | initialize, TITLE, GAME/GAME_OVER, shutdown |

`BIOS_USER_REQUEST` is interpreted through the internal `UNeoGeoBiosRequest` type declared in `include/system/bios_state_internal.h`. `UNeoGeoPhase`, declared in `include/system/runtime.h`, is the application-facing phase model. A single BIOS USER request can therefore contain several Unsigned phases.

`src/system/runtime.c` translates those BIOS entries into the runtime sequence. VBlank remains the synchronization boundary: ngdevkit runs `SYSTEM_IO`, BIOS-maintained controller state and callbacks are updated, then Unsigned consumes that state during the following frame.

## BIOS callbacks

`PLAYER_START` and `COIN_SOUND` are authoritative BIOS callbacks. Their cartridge-side implementations live in `src/system/bios_callbacks.c`.

A runtime binding has three internal states:

- `UNBOUND`: no USER 2/3 application runtime is active;
- `INITIALIZING`: `initialize()` is running;
- `READY`: initialization completed and `PLAYER_START` requests may be filtered through `accept_start`.

This state machine keeps `PLAYER_START` from entering GAME with a partially initialized application context.

### `PLAYER_START` handshake

`neo_geo_bios_process_start()` performs the cartridge side of the handshake:

1. read P1..P4 request bits from `BIOS_START_FLAG`;
2. remove players already in `PLAYING`;
3. accept requests only while the runtime binding is `READY`;
4. pass candidate bits to `accept_start` when the application provides that callback;
5. write the accepted mask back to `BIOS_START_FLAG`;
6. prepare `BIOS_CREDIT_DEC1..4` for accepted players;
7. move accepted players to `PLAYING`;
8. set `BIOS_USER_MODE = GAME` when at least one player is accepted.

The BIOS performs the actual MVS credit decrement after the cartridge callback returns.

## Player and session state

Unsigned uses the BIOS `PLAYER_MOD` values directly through `src/system/session.c`:

| State | Meaning |
| --- | --- |
| `NEVER_PLAYED` | slot has not participated |
| `PLAYING` | active gameplay |
| `CONTINUE` | waiting for a continue; still participates in the GAME session |
| `GAME_OVER` | terminal state for that player |

Public transitions are exposed by `include/system/session.h`:

- `unsigned_neo_geo_player_begin_continue()`: `PLAYING -> CONTINUE`;
- `unsigned_neo_geo_player_game_over()`: `PLAYING/CONTINUE -> GAME_OVER`;
- a later accepted BIOS `PLAYER_START` moves the selected slot to `PLAYING` again;
- `unsigned_neo_geo_request_game_over()` applies the terminal transition to every participating player;
- `unsigned_neo_geo_end_session()` also closes the local runtime session.

GAME stays active while at least one player is either `PLAYING` or `CONTINUE`. GAME_OVER starts after participating players have left those two states, unless the local session has already been explicitly ended.

## MVS credits

The generic runtime models the standard P1/P2 credit counters.

- Japan uses the shared P1 credit pool;
- US and Europe use split P1/P2 pools;
- P3/P4 credit ownership is left to BIOS/system-specific extensions rather than inferred by the generic API.

`src/system/credits.c` reads the BIOS credit counters for prompts/UI and prepares `BIOS_CREDIT_DEC1..4` for accepted starts. It does not directly decrement the BIOS credit counters.

The public helpers in `include/system/credits.h` expose credit sharing, player credit values and the standard INSERT COIN / PRESS START prompt selection.

## MVS cabinet settings

`src/system/settings.c` models the cabinet settings currently needed by Unsigned.

`unsigned_neo_geo_set_game_start_compulsion(bool enabled)` updates the persistent MVS GAME START COMPULSION setting in BIOS backup RAM. AES is left unchanged.

Related helpers are:

- `unsigned_neo_geo_game_start_compulsion_enabled()`;
- `unsigned_neo_geo_game_start_compulsion_seconds()`;
- `unsigned_neo_geo_set_game_start_compulsion_seconds()`;
- `unsigned_neo_geo_demo_sound_enabled()`.

The compulsion delay is exposed as normal seconds while the BIOS value is stored as BCD. Setter values above 99 seconds are clamped to 99. These APIs change cabinet-wide persistent BIOS settings, so application code should treat them as configuration operations rather than per-session gameplay state.

## Sound-command boundary

The Neo Geo BIOS reserves sound commands `0..3`:

| Command | Meaning in nullsound |
| --- | --- |
| 0 | unused/no command |
| 1 | prepare for ROM switch and wait in Z80 RAM |
| 2 | reset driver and start eye-catcher music supplied by the game ROM |
| 3 | initialize/reset the sound driver |
| 4+ | game-defined commands |

Unsigned reflects that boundary with `U_AUDIO_COMMAND_FIRST_GAME = 4` in `include/audio/audio_types.h`.

The 68k `REG_SOUND` register is a one-byte latch, not a queue. `src/system/audio_backend.c` therefore owns the hardware write and serializes normal game commands through a small transport queue. `neo_geo_audio_transport_tick()` sends at most one queued command after a completed VBlank.

A regular BIOS `COIN_SOUND` is written immediately and reserves the following normal transport slot. This gives the Z80 NMI path a full transport boundary before another game command may replace the latch value.

`UAudioManager` also handles backend back-pressure. If `unsigned_audio_backend_send()` cannot accept an already resolved music/SFX command, `src/audio/audio.c` stores it in `pending_backend_command` and retries it on a later audio tick.

## GAME START COMPULSION and the first-credit audio handoff

MVS GAME START COMPULSION can transition from USER 2/ATTRACT into USER 3/TITLE. During that lifecycle, the board can temporarily select the board FIX/SM1 ROMs. nullsound command 1 exists specifically for this ROM switch: it stops sound, resets the YM2610 and waits from Z80 RAM until the cartridge sound ROM becomes active again. ngdevkit then restores cartridge FIX/M1 before calling `main_mvs_title()`.

Playing the first-credit sample immediately before this handoff can therefore start audio that command 1 intentionally cuts. Unsigned preserves the **coin event** across the USER 2 -> USER 3 C-runtime reset and replays it once the cartridge sound driver has been restarted.

### Detection in USER 2

`neo_geo_bios_forced_start_handoff_pending()` only participates while all of the following are true:

- the active request is USER 2 / DEMO;
- the system is MVS;
- `BIOS_USER_MODE` is still DEMO;
- GAME START COMPULSION is enabled.

Inside that context, the implementation treats the following as forced-start transition indicators:

- `BIOS_USER_REQUEST` already requests TITLE;
- the BIOS compulsion timer state indicates an active/completed transition window;
- a standard P1/P2 credit is available while USER 2 is still executing.

A partial coin insertion that has not produced a usable credit and has not activated those transition indicators remains on the immediate `COIN_SOUND` path.

### Persistent handoff token

When the forced-start handoff is detected, `neo_geo_audio_defer_coin_sound()` increments a tiny persistent token declared with ngdevkit's `_backup_ram` attribute. The structure stored by `src/system/audio_backend.c` contains:

- a magic value;
- `pending_coin_count`;
- an inverted count used as a validity check.

The `_backup_ram` attribute places the token in `.bss.bram`. ngdevkit's USER 3 C-runtime initialization clears normal `.bss`, while this cartridge backup block remains available for the short USER 2 -> USER 3 handoff.

### USER 3 restart sequence

The current sequence is:

1. USER 2 receives the BIOS `COIN_SOUND` callback;
2. a forced-start transition is detected and the coin event is persisted instead of played;
3. the BIOS/ngdevkit lifecycle enters USER 3 and restores cartridge FIX/M1;
4. `src/system/runtime.c` initializes the Neo Geo transport and sends sound command 3 through `neo_geo_audio_reset()`;
5. `neo_geo_audio_resume_deferred_coin_sounds()` consumes the persistent token immediately and arms volatile playback state;
6. after the first completed VBlank, `neo_geo_audio_transport_tick()` sends the deferred coin command before normal TITLE/GAME audio.

The VBlank boundary is used as a protocol boundary after command 3; the implementation does not use an arbitrary sleep or busy delay.

USER 2 clears stale deferred state when a fresh attract runtime starts. USER 3 consumes the token before arming playback so a later reset cannot replay the same event again.

With GAME START COMPULSION disabled, this USER 2 -> USER 3 first-credit path does not apply and the normal immediate `COIN_SOUND` transport remains in use.

## Internal responsibilities

| File | Responsibility |
| --- | --- |
| `src/system/runtime.c` | BIOS USER dispatch and ATTRACT/TITLE/GAME/GAME_OVER sequencing |
| `src/system/bios_callbacks.c` | cartridge callback implementations and runtime binding state |
| `src/system/session.c` | `PLAYER_MOD` access, player transitions and session lifetime |
| `src/system/credits.c` | standard P1/P2 credit routing and start decrement preparation |
| `src/system/settings.c` | persistent MVS cabinet settings used by Unsigned |
| `src/system/input.c` | BIOS controller snapshots to `UInputManager` |
| `src/system/audio_backend.c` | serialized `REG_SOUND` transport, BIOS coin priority, driver reset and forced-start handoff |
| `src/system/save_backend.c` | memory-card / backup-RAM save backend operations |
| `src/system/*_backend.c`, `src/system/fix.c`, `src/system/video.c` | Neo Geo rendering/video hardware access |

The related internal contracts are under `include/system/`, including `bios_callbacks_internal.h`, `bios_state_internal.h`, `session_internal.h`, `credits_internal.h` and `audio_backend_internal.h`.

## ngdevkit compatibility

Unsigned relies on ngdevkit lifecycle and ABI behavior around:

- `runtime/ngdevkit-crt0.S` USER dispatch and C-runtime reinitialization;
- `PLAYER_START` and `COIN_SOUND` wrappers;
- `include/ngdevkit/bios-ram.h` and BIOS backup-RAM definitions;
- `_backup_ram` / `.bss.bram` handling;
- nullsound BIOS commands 1, 2 and 3.

The Unsigned repository itself currently contains `unsigned.mk` rather than the parent game's complete ngdevkit toolchain configuration, so this document does not claim a specific ngdevkit package or git revision. A release build should record the exact ngdevkit revision/toolchain used by the parent project and re-check these integration points when that dependency changes.
