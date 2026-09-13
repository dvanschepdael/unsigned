# Getting Started with Unsigned

This guide is intended for developers who know the basics of C but are new to the Unsigned engine and/or the Neo Geo. The goal is not to memorize every file. Instead, focus on understanding **who owns the data**, **who decides behavior**, **who decides rendering**, and **who actually writes to the hardware**.

## 1. Five Core Concepts to Know

### `UGameInstance`: the engine root

`UGameInstance` (`engine/game/game.h`) gathers the generic subsystems required by a running game: input, timers, gameplay, actor pools, level runtime, renderer, level manager, viewport, and audio.

`unsigned_game_instance_init()` receives preallocated buffers through `UGameInstanceStorage`. If a capacity is insufficient, it should be fixed during startup rather than hidden behind dynamic allocation during gameplay.

### `ULevelDefinition`: declared content

`ULevelDefinition` (`engine/level/level_definition.h`) contains stable level content:

- backgrounds;
- NPC/object spawns;
- `load`, `enter`, `exit`, and `unload` callbacks;
- actor ordering;
- game-specific hit resolution;
- backdrop color.

The definition must remain valid for as long as the level uses it.

### `ULevel`: mutable runtime state

`ULevel` (`engine/level/level_runtime.h`) contains state that changes during gameplay:

- active actors;
- associated pools;
- TLSS state;
- collision state;
- background runtime;
- camera;
- gameplay runtime;
- active definition and application context.

Keeping `ULevelDefinition` and `ULevel` separate prevents static content data from becoming mixed with mutable runtime state.

### `UActor` / `UCharacter`: world presence and combat state

`UActor` represents presence in the game world: position, sprite, and collision.

`UCharacter` adds:

- attributes;
- abilities;
- tags;
- facing direction.

`UPlayer` and `UNpc` then wrap a `UCharacter` with their respective control logic.

### `engine/system`: the Neo Geo boundary

BIOS access, VRAM, FIX, video, platform input, audio transport, and Neo Geo storage are grouped under `engine/system/`.

Level and character code should not bypass this layer to access hardware registers directly.

## 2. Reading the Directory Tree Without Getting Lost

Start with this map:

```text
engine/
  core/       generic building blocks
  actor/      entities and pools
  gameplay/   attributes/tags/abilities/effects/cues
  level/      level lifecycle and orchestration
  physics/    low-level geometric collision
  collision/  gameplay collision
  display/    presentation data, UI, viewport
  renderer/   rendering policy and caches
  system/     Neo Geo/BIOS/hardware backends
  audio/      logical audio layer
  save/       generic save system
  input/      generic input
  game/       UGameInstance

src/
  game/       demo composition and flow
  characters/ concrete characters
  levels/     concrete levels
  menu/       menus
  hud/        HUD
  localizations/
  audio/
```

A simple rule helps keep the architecture clear: `engine/` should remain reusable, while `src/` may depend on game-specific requirements.

## 3. From BIOS to a Game Frame

The main path is:

```text
BIOS / VBlank / USER request
        |
        v
engine/system/runtime.c
        |
        +--> unsigned_neo_geo_input_poll()
        |
        +--> demo_loop_tick()
        |      |
        |      +--> unsigned_game_instance_tick()
        |      |      +--> timers
        |      |      +--> level manager
        |      |      +--> level tick
        |      |      +--> viewport
        |      |      +--> audio
        |      |
        |      +--> demo flow / stage / menu
        |
        +--> demo_loop_render()
        |      +--> unsigned_game_instance_render()
        |      +--> overlays / menu / HUD
        |
        +--> ng_wait_vblank()
        +--> audio transport
```

The BIOS remains authoritative over several system transitions, so the runtime should not be treated like a fully autonomous PC-style main loop.

## 4. Reading a Level Frame

In `engine/level/level.c`, simulation follows this order:

```text
TLSS begin frame
    |
clear dynamic collision
    |
actors + player input
    |
NPC activity + AI
    |
abilities
    |
collision registration + hit/projectile detection
    |
level resolve_hits
    |
effects
    |
cues
    |
background tick
```

This order is part of the engine's behavioral contract. Moving one step can change gameplay even if the code still compiles.

For example, `resolve_hits` runs after abilities but before end-of-frame effects and cues.

## 5. Understanding Fixed-Capacity Memory

The engine uses fixed-capacity pools. A `UPoolInstance` contains a `generation` value.

Why is this needed? A callback may:

1. release a slot;
2. reuse it immediately;
3. leave the same physical pointer visible to the caller.

The generation value distinguishes the old logical instance from the new one.

### Practical Rule

Do not keep a pointer to a pool slot and assume it still represents the same logical object later. Check the subsystem contract and, where available, use the `generation` mechanism.

## 6. Where Should New Code Go?

### A new reusable character mechanic

Start by looking at:

- `engine/actor/character.h`;
- `engine/gameplay/`;
- optionally `engine/physics/` or `engine/collision/`.

If the mechanic only belongs to one game character, place it under `src/characters/` instead.

### A new player action

1. define or reuse a `UGameplayAbility`;
2. define its tags and activation conditions;
3. create a `UGameplayAbilityBinding`;
4. choose the `UInputTrigger`;
5. define how the ability ends or is cancelled;
6. test activation failure when the pool is full.

Useful files:

- `engine/actor/player.c`;
- `engine/gameplay/ability.c`;
- `engine/gameplay/ability_pool.c`;
- `engine/gameplay/gameplay_pool.c`;
- examples under `src/characters/player/demo/`.

### A new attribute

Attributes are defined in `engine/gameplay/attribute.h`.

For a character-specific example, see `src/characters/player/demo/player_health.c`.

An `on_update` callback can synchronize UI state, as the current HUD does for player health.

### A new NPC

1. define its content/spawn data;
2. provide its `UCharacter`;
3. configure its state graph;
4. call `unsigned_npc_init()`;
5. let `level_ai.c` and TLSS manage update cadence according to activity.

Useful files:

- `engine/actor/npc.h`;
- `engine/actor/npc_ai.c`;
- `engine/level/level_ai.c`;
- `engine/core/tlss/tlss.h`.

### A new level

Concrete level content belongs under `src/levels/<level_name>/`.

1. declare a `ULevelDefinition`;
2. define backgrounds and spawns with sufficient lifetime;
3. use `load` for setup that cannot be expressed directly as data;
4. use `enter` / `exit` for state changes around active gameplay;
5. use `unload` to reverse work performed by `load`;
6. expose the definition through `src/game/demo_scenes.c` or the relevant flow code.

Level loading is transactional: if an error occurs, already-installed content is rolled back.

## 7. Do Not Confuse Display, Renderer, and System

This distinction is central to the current architecture.

### `engine/display`

Describes backend-independent presentation concepts:

- sprites;
- text;
- viewport;
- UI elements;
- widgets;
- layout/screen/page structures.

### `engine/renderer`

Decides how those concepts are rendered:

- visibility;
- ordering;
- sprite allocation;
- dirty state;
- background caching;
- Neo Geo UI adaptation.

### `engine/system`

Performs operations that are actually Neo Geo-specific:

- VRAM access;
- FIX;
- palettes;
- video;
- BIOS interaction;
- platform input;
- audio transport;
- save backend.

### Example: Rendering an Actor

```text
UActor / USprite
    -> engine/renderer/actor_renderer.c
    -> engine/renderer/sprite_renderer.c
    -> engine/system/renderer_backend.h
    -> engine/system/sprite_backend.c
    -> VRAM / SCB
```

### Example: Rendering a Widget

```text
UUIProgressBar
    -> UUIRenderer
    -> engine/renderer/ui_renderer.c
    -> engine/system/fix.c
    -> FIX layer
```

## 8. Adding or Modifying UI

Generic UI code lives under `engine/display/ui/`.

Reusable widgets live under `engine/display/ui/widget/`:

- button;
- image;
- label;
- panel;
- progress bar;
- selector;
- blink label.

Concrete game UI should generally live under `src/menu/` or `src/hud/`.

### Example: Current Health Bar

`src/hud/demo_hud.c`:

1. creates a `UUIProgressBar`;
2. binds it to a health `UGameplayAttribute`;
3. connects `on_update` to the attribute and its bounds;
4. calls `unsigned_ui_progress_bar_sync()` when the value changes;
5. renders the `UUIScreen` through `UUIRenderer` + `UNeoGeoUIRenderer`.

The widget stores normalized progress in the 0..256 range. The renderer therefore does not need to repeat the full attribute division every frame.

## 9. Collision: Two Layers to Keep Separate

`engine/physics` knows about boxes and collision layers. It does not know what an attack is.

`engine/collision` adds gameplay concepts such as:

- hitboxes/hurtboxes;
- actors;
- projectiles;
- hit detection.

`engine/level/level_collision.c` then orchestrates them for the active level.

Static collision is built during level loading. Dynamic collision is cleared and registered again each frame.

If registration buffers become saturated, the entire collision frame is invalidated rather than partially processed.

## 10. TLSS: Reduced-Frequency Simulation

TLSS lives in `engine/core/tlss/`.

It can spread work over 1/2/4/8/16 frames. Two current uses are configurable through `UGameInstanceConfig`:

- AI;
- collision.

Off-screen or dormant NPCs can therefore cost less CPU time without reducing the global frame rate.

Important: collision can force immediate resolution for a hitbox that has just become active, preventing reduced TLSS cadence from missing its first active attack frame.

## 11. Neo Geo: Do Not Reimplement BIOS Responsibilities

On MVS/AES hardware, the BIOS owns part of the application lifecycle.

The system runtime handles, among other things:

- USER requests;
- `ATTRACT` / `TITLE` / `GAME` / `GAME_OVER` phases;
- `PLAYER_START`;
- player session state;
- credits;
- GAME START COMPULSION;
- associated audio handoff.

Before modifying this area, read `engine/system/BIOS_WORKFLOW.md` and the relevant public interfaces under `engine/system/`.

Do not turn a generic START button press directly into an MVS session-start decision. BIOS `PLAYER_START` remains authoritative.

## 12. Reading the Demo Project

To understand how the layers are assembled, read the project in this order:

1. `main.c`;
2. `src/game/demo_game.c`;
3. `src/game/demo_loop.c`;
4. `src/game/demo_flow.c`;
5. `src/game/demo_scenes.c`;
6. one level under `src/levels/`;
7. the player implementation under `src/characters/player/demo/`;
8. `src/hud/demo_hud.c` and `src/menu/main/demo_menu.c`.

This path shows the boundary between reusable engine code and game-specific content more clearly than reading every file under `engine/` in sequence.

## 13. Recommended Modification Workflow

For a contribution:

1. find the subsystem's public `.h` API;
2. read the corresponding implementation;
3. read at least one related test under `test/`;
4. identify fixed capacities and pointer lifetimes;
5. make the smallest necessary change;
6. add or update a host-side test;
7. verify on MAME/hardware when the change depends on Neo Geo-specific behavior.

When a comment is needed, document the invariant, lifetime rule, performance reason, or BIOS constraint. Avoid comments that merely restate what the code already says.
