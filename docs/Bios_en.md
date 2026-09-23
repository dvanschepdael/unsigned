# Neo Geo BIOS integration

Unsigned keeps BIOS lifecycle rules inside `engine/system/`. Game code works with runtime phases and typed player state; it does not read BIOS RAM or recreate BIOS events from controller input.

## Lifecycle boundary

| BIOS request | ngdevkit entry | Runtime behavior |
| --- | --- | --- |
| USER 0 | `rom_mvs_startup_init` | handled by ngdevkit startup code |
| USER 1 | `main()` | optional eye catcher; no game initialization |
| USER 2 | `main()` | initialize, ATTRACT, optional GAME/GAME_OVER, shutdown |
| USER 3 | `main_mvs_title()` | initialize, TITLE, optional GAME/GAME_OVER, shutdown |

`BIOS_USER_REQUEST` is interpreted through the internal `UNeoGeoBiosRequest` type shared by the Neo Geo runtime/callback layer. `UNeoGeoPhase` is a game-facing concept: one USER request can run several phases.

## BIOS callbacks

`PLAYER_START` and `COIN_SOUND` are authoritative BIOS events. Unsigned does not synthesize them from controller changes or `BIOS_COMPULSION_TIMER`.

A runtime binding has three internal states:

- `UNBOUND`: no USER 2/3 runtime is active; START requests are rejected;
- `INITIALIZING`: `initialize()` is running; START requests are rejected;
- `READY`: initialization completed; START requests may be filtered by `accept_start`.

This prevents a VBlank/BIOS callback from entering GAME with a partially initialized application context.

### PLAYER_START handshake

`neo_geo_bios_process_start()` performs the cartridge side of the handshake:

1. read P1..P4 request bits from `BIOS_START_FLAG`;
2. remove players already in `PLAYING`;
3. reject the whole request unless the runtime is `READY`;
4. let `accept_start` clear additional requested bits;
5. write only accepted bits back to `BIOS_START_FLAG`;
6. prepare `BIOS_CREDIT_DEC1..4` for accepted players;
7. move accepted players to `PLAYING`;
8. set `BIOS_USER_MODE = GAME` when at least one player was accepted.

The BIOS owns the actual MVS credit decrement after the callback returns.

## Player state

Unsigned uses the BIOS `PLAYER_MOD` values directly:

| State | Meaning |
| --- | --- |
| `NEVER_PLAYED` | slot has not participated |
| `PLAYING` | active gameplay |
| `CONTINUE` | waiting for a continue; keeps the GAME session alive |
| `GAME_OVER` | terminal for that player |

Game code changes player state through controlled transitions:

- `unsigned_neo_geo_player_begin_continue()`: `PLAYING -> CONTINUE`;
- `unsigned_neo_geo_player_game_over()`: `PLAYING/CONTINUE -> GAME_OVER`;
- a later accepted BIOS `PLAYER_START` moves `CONTINUE -> PLAYING`.

`unsigned_neo_geo_request_game_over()` applies the terminal transition to every participating player. `unsigned_neo_geo_end_session()` additionally closes the local runtime session.

## MVS credits

The generic runtime models the standard P1/P2 credit counters only.

- Japan uses the shared P1 credit pool;
- US and Europe MVS use separate P1/P2 pools;
- P3/P4 credit ownership is not guessed because 4-player routing depends on the BIOS/system extension.

`engine/system/credits.c` only reads credit counters for UI and writes `BIOS_CREDIT_DEC1..4`. It never decrements backup-RAM credit counters directly.

`unsigned_neo_geo_set_game_start_compulsion(bool enabled)` is the explicit exception for cabinet configuration. On MVS it unlocks BIOS backup RAM, writes the persistent GAME START COMPULSION setting, then locks backup RAM again. `false` selects `WITHOUT`; `true` enables forced-start behavior. It is a no-op on AES. Because this changes a cabinet-wide persistent BIOS setting, games should only call it deliberately.

The matching cabinet helpers are:

- `unsigned_neo_geo_game_start_compulsion_enabled()` reads the current forced-start setting.
- `unsigned_neo_geo_game_start_compulsion_seconds()` exposes the BIOS BCD timer as normal seconds.
- `unsigned_neo_geo_set_game_start_compulsion_seconds()` stores a 0..99 second value as BIOS BCD.
- `unsigned_neo_geo_demo_sound_enabled()` reports the cabinet-level demo-sound mute (`true` on AES).

The demo-sound helper does not replace game-specific soft DIP logic. All setters above change persistent cabinet BIOS backup RAM and should therefore be used deliberately.

## Sound commands

The sound driver reserves commands `0..3`:

| Command | Use |
| --- | --- |
| 0 | unused/no command |
| 1 | ROM switch preparation |
| 2 | ngdevkit eye catcher |
| 3 | driver/YM2610 reset |
| 4+ | game commands |

Generic audio accepts only commands `>= U_AUDIO_COMMAND_FIRST_GAME`. Platform lifecycle code reaches reserved commands only through Neo Geo backend helpers such as `neo_geo_audio_reset()`.

The 68k `REG_SOUND` register is a one-byte latch, so Unsigned does not let gameplay write it directly. Normal audio commands enter the Neo Geo transport queue and at most one is written immediately after each completed VBlank. A `COIN_SOUND` that cannot trigger a BIOS ROM handoff is still sent immediately and reserves the next normal transport slot. The nullsound driver already copies accepted game commands into its own Z80-side FIFO.

MVS GAME START COMPULSION is different. SNK's forced-start lifecycle requests USER 3/Title, and the board can temporarily select the board FIX/SM1 ROMs. nullsound command 1 is specifically designed for that ROM switch: it stops sound, resets YM2610 and waits in Z80 RAM. The same hardware selection also changes the FIX source, which is why MAME can show a brief visual flash at exactly the point where an eagerly played first-credit sample is cut.

Unsigned therefore treats the coin as an event, not as playback that must survive the ROM switch:

1. while USER 2/ATTRACT is active with GAME START COMPULSION, the callback checks BIOS transition signals (`BIOS_USER_REQUEST`, compulsion state and current credits). Only a coin that is actually leading into forced-start TITLE is deferred; partial coinage that has not produced a credit remains immediate. A deferred event increments a tiny handoff token in the cartridge backup block (`.bss.bram`), which ngdevkit does not clear during USER 3 C-runtime initialization;
2. USER 3 starts after ngdevkit has selected `CRTFIX`/the cartridge M1 again;
3. USER 3 sends BIOS sound command 3, as required to initialize the newly selected nullsound M1 driver;
4. the persistent token is consumed immediately and armed as volatile transport state;
5. on the first completed VBlank, the coin command is sent before normal TITLE/GAME audio. This frame boundary is a protocol boundary after command 3, not an arbitrary sleep/delay.

With GAME START COMPULSION set to `WITHOUT`, USER 3 is not part of this first-credit forced-start handoff, so the normal immediate `COIN_SOUND` path remains unchanged.

`UAudioManager` keeps one resolved command locally when the platform queue is full and retries it later, so transport back-pressure never silently drops a music transition or SFX command.

Both USER 2 and USER 3 reset the sound driver after `CRTFIX`/M1 selection. USER 2 also clears stale handoff state. USER 3 then replays any coin event deferred specifically for the forced-start ROM transition.

## Internal responsibilities

| File | Responsibility |
| --- | --- |
| `engine/system/runtime.c` | BIOS USER dispatch and ATTRACT/TITLE/GAME/GAME_OVER sequencing |
| `engine/system/bios_callbacks.c` | cartridge ABI callbacks and runtime binding state |
| `engine/system/session.c` | `PLAYER_MOD` access, player transitions and session lifetime |
| `engine/system/credits.c` | start cost and standard P1/P2 MVS credit routing |
| `engine/system/input.c` | BIOS controller snapshots -> `UInputManager` |
| `engine/system/audio_backend.c` | serialized `REG_SOUND` transport, BIOS coin priority and reserved platform audio commands |
| `engine/system/save_backend.c` | memory-card / backup-RAM BIOS operations |
| `engine/system/renderer_backend.h` + sprite/background/palette backends | global render transaction plus Neo Geo VRAM/palette/sprite hardware access |

Internal headers follow the same boundaries: `engine/system/bios_callbacks_internal.h`, `engine/system/bios_state_internal.h`, `engine/system/session_internal.h` and `engine/system/credits_internal.h`.

## ngdevkit compatibility

The build is pinned to ngdevkit package version `0.5` through `NGDEVKIT_REQUIRED_VERSION` in the Makefile. When changing ngdevkit version, re-check at least:

- `<ngdevkit>/runtime/ngdevkit-crt0.S` USER dispatch;
- `PLAYER_START` and `COIN_SOUND` wrappers;
- `<ngdevkit>/include/ngdevkit/bios-ram.h`;
- the official credits-management and memory-card examples.

Source/nightly ngdevkit builds can share the same package version; release builds should also record the exact ngdevkit git revision used by the toolchain.
