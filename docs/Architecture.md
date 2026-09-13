# Unsigned Technical Architecture

This document describes the **current** organization of the Unsigned engine and demo project. Its central design principle is to keep gameplay rules and simulation separate from Neo Geo-specific details so that most of the engine can be tested on a host platform.

## 1. Dependency Direction

The overall dependency flow goes from game content to engine contracts, then to hardware backends:

```text
src/  (game, characters, levels, menus, HUD)
 |
 v
engine/game
 |
 +----------------+----------------+----------------+
 v                v                v                v
level            actor          gameplay          input/audio
 |                |                |
 +--------+-------+--------+-------+
          v                v
      collision         display
          |                |
          v                v
       physics         renderer
                           |
                           v
                        system
                           |
                           v
                    Neo Geo / BIOS / VRAM

core = shared generic types and low-level engine building blocks
save = generic persistent storage + platform backend
```

Dependencies are not strictly vertical in every case, but this direction defines the rule to preserve:

- `src` describes the game and composes the engine.
- `engine/display` describes what must be presented.
- `engine/renderer` decides how presentation state becomes rendering operations.
- `engine/system` performs Neo Geo- and BIOS-specific operations.

Gameplay code should therefore never write directly to VRAM or read BIOS RAM merely to bypass engine APIs.

## 2. Current Directory Layout

### Engine

```text
engine/
├── actor/       actors, characters, players, NPCs, objects, projectiles, pools
├── audio/       events, music, resolution, and logical audio commands
├── collision/   actor index, hit detection, projectiles, gameplay collision
├── core/        types, math, pools, state graph, timers, TLSS
├── display/     camera, sprites, text, UI, and viewport
├── game/        UGameInstance composition root
├── gameplay/    attributes, tags, abilities, effects, cues, and runtime
├── input/       generic input state
├── level/       level definition/runtime, spawns, AI, collisions, backgrounds
├── physics/     low-level geometry/collision and trajectories
├── renderer/    actor/background/level/UI rendering and render caches
├── save/        records, blocks, sets, and storage abstraction
└── system/      BIOS runtime and Neo Geo backends
```

Important subdirectories include:

```text
engine/core/
├── math/
├── pool/
├── state/
├── timer/
└── tlss/

engine/display/
├── camera/
├── sprite/
├── text/
├── ui/
│   └── widget/
└── viewport/

engine/level/
└── background/
```

### Demo Game

```text
src/
├── audio/                  project Z80 / sound driver
├── characters/
│   ├── npc/
│   └── player/demo/        demo character and abilities
├── game/                   composition, flow, loop, and Neo Geo runtime adaptation
├── hud/                    in-game HUD
├── levels/
│   ├── common/
│   ├── forest_edge/
│   ├── old_road/
│   └── ruin_gate/
├── localizations/          demo text resources
└── menu/main/              main menu / selection
```

`main.c` intentionally remains thin. It delegates ngdevkit entry points to `engine/system/runtime.c` and provides the `DEMO_NEO_GEO_RUNTIME` application runtime description.

## 3. Composition Root: `UGameInstance`

`UGameInstance`, declared in `engine/game/game.h`, gathers the generic subsystems required by a running game:

- audio;
- input;
- timers;
- gameplay runtime;
- actor pools;
- level runtime;
- level renderer;
- level manager;
- viewport.

`unsigned_game_instance_init()` broadly follows this order:

1. validate configuration and capacities;
2. initialize the renderer;
3. wire actor pools and the gameplay runtime;
4. wire the level runtime and collision buffers;
5. initialize input;
6. initialize audio and timers;
7. initialize the viewport;
8. start level flow through either `ULevelGraph` or a direct `initial_level`.

Variable-capacity memory is supplied by the application through `UGameInstanceStorage`. The engine does not own these arrays and never frees them.

If initialization fails, `unsigned_game_instance_destroy()` resets the instance to an empty state while leaving all externally owned buffers untouched.

## 4. Frame Loop

### Neo Geo Layer

`engine/system/runtime.c` owns the BIOS/USER lifecycle and executes a Neo Geo frame roughly as follows:

```text
SYSTEM_IO / BIOS callbacks during the previous VBlank
        |
        v
unsigned_neo_geo_input_poll()
        |
        v
application tick callback
        |
        v
application render callback
        |
        v
render_phase callback
        |
        v
ng_wait_vblank()
        |
        v
audio transport
```

The game therefore does not own a simple standalone `while (1)` loop. The BIOS remains authoritative over part of the runtime lifecycle.

### Application Layer

In the demo, `src/game/demo_loop.c` orchestrates:

```text
demo_loop_tick
  -> unsigned_game_instance_tick
  -> demo flow
  -> scene/stage logic
  -> menu

demo_loop_render
  -> unsigned_game_instance_render
  -> stage/scene overlays
  -> menu
  -> HUD
```

### Engine Layer

`unsigned_game_instance_tick()` in `engine/game/game.c` performs:

1. timer tick;
2. level manager tick;
3. active level tick, when a level is running;
4. viewport tick;
5. audio tick.

Level rendering is kept separate in `unsigned_game_instance_render()`.

## 5. Level Lifecycle

`ULevelDefinition` (`engine/level/level_definition.h`) describes stable level content. `ULevel` (`engine/level/level_runtime.h`) contains mutable runtime state.

### Loading

`unsigned_level_load()`:

1. unloads the previous level;
2. preserves runtime buffers and dependencies supplied to the level;
3. resets TLSS, collision, and background runtime state;
4. installs declared background layers;
5. calls `load`, when provided;
6. instantiates declared spawns;
7. builds static collision data;
8. calls `enter`.

If a step fails after `load` has started, the engine calls `unload` when required and rolls back the affected pools, gameplay state, and collision state.

### Unloading

`unsigned_level_unload()` performs:

1. `exit`;
2. `unload`;
3. cleanup of mutable level state.

### Level Tick

The actual order in `engine/level/level.c` is:

1. begin TLSS frame;
2. remove previous dynamic collision registrations;
3. tick actors and player input;
4. classify NPC activity and tick NPC AI;
5. tick active abilities;
6. register collision state and detect hits/projectiles;
7. call `resolve_hits` when hits are present;
8. tick effects;
9. tick cues;
10. tick the background runtime.

The engine detects interactions. The active level definition decides their game-specific meaning through `resolve_hits`.

## 6. Actors, Characters, and Pools

The hierarchy uses **explicit C composition**:

```text
UActor
  position
  sprite
  collision
    |
    +--> UObject
    |
    +--> UCharacter
           attributes
           abilities
           tags
           facing
             |
             +--> UPlayer
             |      input + ability bindings
             |
             +--> UNpc
                    state graph + activity + TLSS AI

UProjectile -> references a UActor + trajectory/lifetime
```

`UActorContainer` is a non-owning view over active actors gathered from the specialized pools.

Pools use fixed slots. `UPoolInstance.generation` detects slot reuse: the same memory address does not necessarily represent the same logical instance over time.

## 7. Gameplay: Attributes, Abilities, Effects, Cues, and Tags

The aggregated gameplay runtime is `UGameplayRuntime` (`engine/gameplay/runtime.h`):

```text
UGameplayRuntime
├── UAbilityPool
├── UEffectPool
└── UCuePool
```

The main concepts are:

- **Attribute**: mutable gameplay value, optionally constrained by bounds/relationships and update callbacks.
- **Ability**: active action that can be triggered by the player or game logic.
- **Effect**: instant, temporary, or persistent modification.
- **Cue**: timed gameplay event.
- **Tag**: reference-counted identifier used to grant, track, or block gameplay states.

Gameplay callbacks may release or reuse a pool slot while that callback is still executing. Generation and identity checks prevent the engine from continuing work on an instance that has become stale.

## 8. TLSS

TLSS lives in `engine/core/tlss/`.

It provides simulation cadences of 1/2/4/8/16 frames and distributes phases across entries so that all throttled tasks do not wake on the same frame.

The level maintains separate TLSS configuration for:

- AI;
- collision resolution.

NPCs may be classified as `ACTIVE`, `OFFSCREEN`, or `DORMANT`. `engine/level/level_ai.c` selects an activity state according to the viewport, then `engine/actor/npc_ai.c` applies the corresponding TLSS cadence.

Collision also uses TLSS during hit/projectile resolution. A newly active hitbox can force immediate resolution so that its first active frame is not lost because of throttling.

## 9. Collision: Physics vs Gameplay Meaning

### `engine/physics`

This layer handles geometry and collision layers without knowing about players, NPCs, or attacks.

### `engine/collision`

This layer adds gameplay semantics:

- actor hitbox/hurtbox transformation;
- actor indexing;
- hit detection;
- projectile resolution.

### `engine/level/level_collision.c`

The level orchestrates the frame by:

1. preserving static collision built during level load;
2. rebuilding dynamic collision registrations;
3. resynchronizing the global actor view;
4. sorting actors according to `actor_order`;
5. building `UCollisionActorIndex`;
6. resolving projectiles;
7. detecting attacker/target pairs.

An incomplete registration invalidates collision results for the entire frame. This prevents fixed-buffer saturation from producing insertion-order-dependent partial results.

## 10. Display, Renderer, and System

The current architecture explicitly separates three responsibilities.

### `engine/display`

Contains backend-independent presentation structures and behavior:

- camera;
- sprite and render state;
- text;
- UI;
- viewport and viewport effects.

Generic UI code lives in `engine/display/ui/`. Reusable widgets live in `engine/display/ui/widget/`, for example `progress_bar.c`.

### `engine/renderer`

Contains rendering policy:

- `actor_renderer`;
- `sprite_renderer`;
- `background_renderer`;
- `level_renderer`;
- `palette_renderer`;
- `ui_renderer`.

The renderer decides visibility, sprite allocation, dirty-state handling, background ring/cache behavior, and command emission order.

`engine/renderer/ui_renderer.c` currently adapts the generic UI renderer to the Neo Geo FIX layer, including labels, selectors, and progress bars.

### `engine/system`

Contains platform-specific details:

- BIOS runtime;
- BIOS callbacks;
- credits/session/settings;
- Neo Geo input;
- FIX;
- video;
- audio backend;
- sprite/background/palette backends;
- VRAM writer;
- save backend.

`engine/system/renderer_backend.h` is the boundary used by renderers to request hardware operations without exposing encoding details to level or gameplay code.

## 11. Backgrounds and Sprites

`engine/renderer/background_renderer.c` treats backgrounds as reusable hardware-sprite strips. Its cache avoids rewriting the entire screen for a simple scroll operation.

`engine/renderer/sprite_renderer.c` uses sprite dirty state to push only modified properties to the backend, such as graphics, position, shrink, and related state.

`engine/renderer/level_renderer.c` wraps rendering as follows:

```text
unsigned_renderer_backend_begin()
    -> prepare/sort actors
    -> background
    -> actors
unsigned_renderer_backend_end()
```

## 12. UI and HUD

UI follows the same logical/backend separation:

```text
engine/display/ui/*
    elements, layouts, screens, pages, widgets
          |
          v
UUIRenderer
          |
          v
engine/renderer/ui_renderer.c
          |
          v
engine/system/fix.c
```

Current example: `src/hud/demo_hud.c` builds a `UUIScreen`, `UUIProgressBar`, `UUIRenderer`, and `UNeoGeoUIRenderer`, then renders them on the FIX layer.

A progress bar can be bound to a `UGameplayAttribute`. The widget stores normalized progress in the 0..256 range so that the renderer does not need to recompute a division every frame.

## 13. Audio

`engine/audio` operates on logical concepts such as catalogs, events, music, cooldowns, and commands.

`engine/system/audio_backend.c` performs Neo Geo transport to the sound driver. BIOS-specific details and reserved commands remain inside the system layer.

The project sound driver currently lives in `src/audio/sound_driver.s`.

## 14. Save System

`engine/save` separates:

- `storage`: access to a versioned physical record;
- `block`: application block stored in a slot;
- `set`: data distributed across multiple consecutive slots.

The platform backend lives in `engine/system/save_backend.c`.

Sets use a consistent generation across blocks so that an interrupted write cannot be mistaken for a complete valid save.

## 15. Neo Geo Runtime and BIOS

The runtime now lives directly under `engine/system/`.

The main application phases are:

- `ATTRACT`;
- `TITLE`;
- `GAME`;
- `GAME_OVER`.

These phases are built on top of BIOS USER requests. `PLAYER_START` remains authoritative for accepting a player and consuming credits.

The USER 1/2/3 lifecycle, GAME START COMPULSION, audio transport, and credit behavior are documented in `engine/system/BIOS_WORKFLOW.md`.

## 16. Engine vs Game Boundary

Use this rule when deciding where new code belongs:

- reusable behavior suitable for multiple games: `engine/`;
- content or rules specific to Unsigned/the demo: `src/`;
- logical presentation representation: `engine/display/`;
- rendering policy: `engine/renderer/`;
- BIOS/hardware access: `engine/system/`;
- concrete scene or level: `src/levels/` or `src/game/`;
- concrete character: `src/characters/`;
- concrete interface: `src/menu/` or `src/hud/`.

Preserving this separation is the main architectural rule for future engine evolution.
