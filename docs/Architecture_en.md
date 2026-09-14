# Unsigned Technical Architecture

This document describes the **current** organization of the engine and the demo project. The core principle is to keep game rules and simulation separate from Neo Geo-specific details so that most of the engine can be tested on the host.

## 1. Dependency direction

The general dependency flow goes from game content to engine contracts, then to hardware backends:

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

core = types and generic building blocks shared by engine layers
save = generic persistent storage + platform backend
```

Dependencies are not all strictly vertical, but this direction defines the rule to preserve:

- `src` describes the game and composes the engine;
- `engine/display` describes what must be presented;
- `engine/renderer` decides how to transform that state into rendering operations;
- `engine/system` performs Neo Geo- and BIOS-specific operations.

Gameplay code must therefore not write directly to VRAM or read BIOS RAM to bypass engine APIs.

## 2. Current directory structure

### Engine

```text
engine/
├── actor/       actors, characters, players, NPCs, objects, projectiles, pools
├── audio/       events, music, resolution and logical audio commands
├── collision/   actor index, hit detection, projectiles, gameplay collision
├── core/        types, math, pools, state graph, timers, TLSS
├── display/     camera, sprites, text, UI and viewport
├── game/        UGameInstance composition root
├── gameplay/    attributes, tags, abilities, effects, cues and runtime
├── input/       generic input state
├── level/       level definition/runtime, spawns, AI, collisions, backgrounds
├── physics/     low-level geometry/collision, movement constraints and trajectories
├── renderer/    actor/background/level/UI rendering and render caches
├── save/        records, blocks, sets and storage abstraction
└── system/      BIOS runtime and Neo Geo backends
```

Some important subdirectories:

```text
engine/core/
├── math/
├── pool/
├── state/       state_graph + state_graph_clock
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

### Demo game

```text
src/
├── audio/                  project Z80 / sound driver
├── characters/
│   ├── npc/
│   └── player/             demo player runtime + concrete characters (including Arthur)
├── game/                   composition, flow, loop and Neo Geo runtime adaptation
├── hud/                    in-game HUD
├── levels/
│   ├── common/
│   ├── forest_edge/
│   ├── old_road/
│   └── ruin_gate/
├── localizations/          demo text
└── menu/main/              main menu / selection
```

`main.c` intentionally stays thin: it delegates ngdevkit entry points to `engine/system/runtime.c` and provides `DEMO_NEO_GEO_RUNTIME`.

## 3. Composition root: `UGameInstance`

`UGameInstance` in `engine/game/game.h` groups the generic subsystems required by a game session:

- audio;
- input;
- timers;
- gameplay runtime;
- actor pools;
- level;
- level renderer;
- level manager;
- viewport.

`unsigned_game_instance_init()` essentially follows this order:

1. validate configuration and capacities;
2. initialize the renderer;
3. wire actor pools and gameplay runtime;
4. wire the level runtime and its collision buffers;
5. initialize input;
6. initialize audio and timers;
7. initialize the viewport;
8. start level flow, either through `ULevelGraph` or a direct `initial_level`.

Variable-capacity memory is provided by the application through `UGameInstanceStorage`. The engine does not own these arrays and does not free them.

On failure, `unsigned_game_instance_destroy()` returns the instance to an empty state while leaving buffers owned by their original owner.

## 4. One-frame loop

### Neo Geo layer

`engine/system/runtime.c` owns the BIOS/USER lifecycle and executes a Neo Geo frame as follows:

```text
SYSTEM_IO / BIOS callbacks during previous VBlank
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

The loop is therefore not a simple `while (1)` fully owned by the game: the BIOS remains responsible for part of the lifecycle.

### Application layer

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

### Engine layer

`unsigned_game_instance_tick()` in `engine/game/game.c`:

1. ticks timers;
2. ticks the level manager;
3. if a level is active: ticks the level;
4. ticks the viewport;
5. ticks audio.

Level rendering is handled separately by `unsigned_game_instance_render()`.

## 5. Level lifecycle

`ULevelDefinition` (`engine/level/level_definition.h`) describes stable content. `ULevel` (`engine/level/level_runtime.h`) contains mutable state.

### Loading

`unsigned_level_load()`:

1. unloads the previous level;
2. preserves runtime buffers and dependencies provided to the level;
3. resets TLSS, collisions and background state;
4. installs declared background layers;
5. calls `load` if present;
6. instantiates declared spawns;
7. builds static collisions;
8. calls `enter`.

If a step fails after `load` has started, the engine calls `unload` when required, then cleans up the affected pools, gameplay state and collisions.

### Unloading

`unsigned_level_unload()` calls:

1. `exit`;
2. `unload`;
3. cleanup of the level's mutable state.

### Level tick

The actual order in `engine/level/level.c` is:

1. begin TLSS frame;
2. remove previous dynamic collisions;
3. tick actors + player input;
4. classify and tick NPC AI;
5. tick active abilities;
6. register/detect collisions;
7. call `resolve_hits` if hits exist;
8. tick effects;
9. tick cues;
10. tick the background.

The engine detects interactions; the level definition decides their gameplay meaning through `resolve_hits`.

`resolve_hits` must consume results already produced by the engine through `unsigned_level_collision_hits()`. Gameplay code must not rebuild hitboxes/hurtboxes in world space to perform a second detection pass: geometry belongs to the collision pipeline, while damage, guard, knockdown and combo rules belong to the game.

## 6. Actors, characters and pools

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

`UActorContainer` is a non-owning view over active actors gathered from specialized pools.

`UActor` intentionally has no concept of playable area or bounds. It stores a world position; `UCharacter` provides generic movement/orientation operations, but the decision to constrain that movement remains external to the actor.

### Movement and spatial constraints

The current separation is:

```text
UInputState
   |
   v
unsigned_input_direction()       engine/input
   |
   v
unsigned_character_move()        engine/actor
   |
   v
Vec2 world position
   |
   +--> unsigned_physics_movement_constrain()  engine/physics
            ^
            |
       UMovementBounds chosen by gameplay/level
```

- `unsigned_input_direction()` converts the D-pad into `-1/0/1` axes; opposite directions cancel each other out;
- `unsigned_character_set_facing()` and `unsigned_character_move()` handle generic character behavior;
- `UMovementBounds` and `unsigned_physics_movement_constrain()` live in `engine/physics/movement.h` / `engine/physics/movement.c`;
- the owner of the movement explicitly decides whether these bounds apply. A projectile or NPC can therefore remain free to leave the playable area.

This separation prevents `UActor` from owning level policy.

Pools use fixed slots. `UPoolInstance.generation` detects reuse of the same slot: an identical address does not necessarily mean that it is still the same logical instance.

## 7. Gameplay: attributes, abilities, effects, cues and tags

The aggregate runtime is `UGameplayRuntime` (`engine/gameplay/runtime.h`):

```text
UGameplayRuntime
├── UAbilityPool
├── UEffectPool
└── UCuePool
```

The main concepts are:

- **Attribute**: mutable gameplay value, with optional bounds/relationships and an update callback;
- **Ability**: active action that can be triggered by the player or the game;
- **Effect**: instant, temporary or persistent modification;
- **Cue**: timed gameplay event;
- **Tag**: reference-counted identifier used to grant or block states.

Gameplay callbacks may release or reuse a slot during their own execution. Generation and identity checks in pools then prevent work from continuing on an instance that has become stale.

### Input to abilities

`UGameplayAbilityBinding` separates the trigger (`DOWN`, `PRESSED`, `RELEASED`, `HOLD`) from the matching policy:

- `U_INPUT_MATCH_ALL`: every button in the mask must match;
- `U_INPUT_MATCH_ANY`: at least one button in the mask is enough.

The demo movement therefore uses a single `ANY` binding covering the entire D-pad. A diagonal consequently creates one movement ability instead of one ability per direction.

`UPlayer.input_state` exposes the current snapshot to abilities that need continuous control, such as Arthur's jump.

### Owner-level interruption/replacement

`UAbilityPool` provides owner-level operations so a character does not inspect the pool's internal arrays:

- `unsigned_gameplay_ability_pool_release_owner()`;
- `unsigned_gameplay_ability_pool_replace_owner()`.

Replacement pre-validates pool capacity and tag capacity for the prospective state before removing existing abilities. Reactions such as Hurt/Knockdown/Caught can therefore interrupt a character without coupling character code to the pool's internal representation.

### Attributes

`unsigned_gameplay_attribute_set_current_value()` applies bounds and only notifies `on_change` when the final value actually changes. `unsigned_gameplay_attribute_add_current_value()` adds a signed delta with `s16` saturation, then reuses the same clamp/notification rules.

## 8. State graph: logic and time separated

`UStateGraph` (`engine/core/state/state_graph.h` / `engine/core/state/state_graph.c`) contains only logical execution state: context, global/initial/current nodes, tasks and transitions. It owns no time counter.

Nodes may declare `duration_frames`, but elapsed time is owned by the optional `UStateGraphClock` component (`engine/core/state/state_graph_clock.h` / `engine/core/state/state_graph_clock.c`):

```text
UStateGraphNode.duration_frames   stable definition
              |
              v
UStateGraph                    current logical state
              ^
              |
UStateGraphClock               optional temporal state
  state
  elapsed_frames
```

`unsigned_state_graph_clock_tick()` executes the graph, advances the counter if the same state remains active, then signals expiration through `unsigned_state_graph_timeout()`. A task/event transition naturally resets the clock for the new state.

This separation has two consequences:

- a graph that does not use timeouts does not have to store `elapsed_frames`;
- the runtime owner decides whether it needs a clock. `ULevelManager` owns one in graph-driven mode; direct mode does not depend on it.

## 9. TLSS

TLSS lives in `engine/core/tlss/`.

It provides simulation cadences of 1/2/4/8/16 frames and distributes phases across entries so all throttled tasks are not woken on the same frame.

The level contains separate configuration for:

- AI;
- collision resolution.

NPCs can be classified as `ACTIVE`, `OFFSCREEN` or `DORMANT`. `engine/level/level_ai.c` selects their activity from the viewport, then `engine/actor/npc_ai.c` applies the matching TLSS cadence.

Collision also uses TLSS when resolving hits/projectiles. A newly active hitbox can force immediate resolution so its first active frame is not lost.

## 10. Collision: physics versus gameplay meaning

### `engine/physics`

This layer handles physical primitives independent of gameplay:

- geometry and box layers;
- `UMovementBounds` and explicit constraint of a `Vec2`;
- `x/depth/height` trajectories;
- trajectory projection;
- `unsigned_physics_trajectory_parabola_height()` for generic parabolic height.

It knows nothing about players, NPCs or attacks. An ability such as Arthur's jump chooses its height and charge rules, then reuses the engine trajectory primitive.

### `engine/collision`

This layer adds gameplay collision semantics:

- transformation of actor hitboxes/hurtboxes;
- actor indexing;
- hit detection;
- projectile resolution.

### `engine/level/level_collision.c`

The level orchestrates the frame:

1. preserves static collisions built at load time;
2. rebuilds dynamic collisions;
3. resynchronizes the global actor view;
4. sorts actors according to `actor_order`;
5. builds `UCollisionActorIndex`;
6. resolves projectiles;
7. detects attacker/target pairs.

Incomplete registration invalidates the frame's collision results. This prevents results from becoming insertion-order dependent when fixed buffers are saturated.

Detected pairs are exposed as frame-local `UCollisionHit` values. `unsigned_level_collision_hits()` lets the `resolve_hits` callback iterate over them. Gameplay code can then apply damage/guard/reactions or deduplicate an activation, but it must not repeat the geometric intersection that the engine has already resolved.

## 11. Display, renderer and system

The current structure explicitly separates three responsibilities.

### `engine/display`

Contains backend-independent presentation structures and behavior:

- camera;
- sprite and render state;
- text;
- UI;
- viewport and viewport effects.

Generic UI lives in `engine/display/ui/`. Reusable widgets live in `engine/display/ui/widget/`, for example `progress_bar.c`.

### `engine/renderer`

Contains rendering policy:

- `actor_renderer`;
- `sprite_renderer`;
- `background_renderer`;
- `level_renderer`;
- `palette_renderer`;
- `ui_renderer`.

The renderer determines visibility, sprite allocation, dirty flags, background ring/cache behavior and command emission order.

`engine/renderer/ui_renderer.c` currently adapts the generic UI renderer to Neo Geo FIX, notably for labels, selectors and progress bars.

### `engine/system`

Contains platform details:

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

`engine/system/renderer_backend.h` is the boundary called by renderers to request hardware operations without exposing their encoding details to level logic.

## 12. Backgrounds and sprites

`engine/renderer/background_renderer.c` treats backgrounds as reusable strips of hardware sprites. The cache avoids rewriting the entire screen for a simple scroll.

`engine/renderer/sprite_renderer.c` uses sprite dirty state so only modified parts are pushed to the backend: graphics, position, shrink, and so on.

`engine/renderer/level_renderer.c` brackets rendering as follows:

```text
unsigned_renderer_backend_begin()
    -> actor preparation/sorting
    -> background
    -> actors
unsigned_renderer_backend_end()
```

## 13. UI and HUD

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

Current example: `src/hud/demo_hud.c` builds a `UUIScreen`, a `UUIProgressBar`, a `UUIRenderer` and a `UNeoGeoUIRenderer`, then renders the result on the FIX layer.

The progress bar can be bound to a `UGameplayAttribute`. The widget stores normalized progress in the 0..256 range so the renderer does not need to recompute a division every frame. The current HUD intentionally owns the `on_change` callbacks of Health and its bounds while bound; this policy remains in `src/hud/` and is not an engine responsibility.

For menus, `unsigned_ui_input_from_controller()` transforms a `UInputState` into standard UI commands (navigation, value, confirm, cancel). Controller mapping is therefore centralized in `engine/display/ui/input.h` / `engine/display/ui/input.c`, while the meaning of a concrete screen remains in `src/menu/`.

## 14. Audio

`engine/audio` handles logical concepts: catalog, events, music, cooldowns and commands.

`engine/system/audio_backend.c` performs Neo Geo transport to the sound driver. BIOS details and reserved commands remain in the system layer.

The project driver currently lives in `src/audio/sound_driver.s`.

## 15. Save system

`engine/save` separates:

- `storage`: access to a versioned physical record;
- `block`: application block in a slot;
- `set`: data spread across several consecutive slots.

The platform backend is in `engine/system/save_backend.c`.

Sets use a coherent generation across blocks so an interrupted write is not mistaken for a complete valid save.

## 16. Neo Geo runtime and BIOS

The runtime now lives directly under `engine/system/`.

The main application phases are:

- `ATTRACT`;
- `TITLE`;
- `GAME`;
- `GAME_OVER`.

They are built on top of BIOS USER requests. `PLAYER_START` remains authoritative for player acceptance and credit consumption.

The details of USER 1/2/3 lifecycle, GAME START COMPULSION, audio transport and credits are documented in `engine/system/BIOS_WORKFLOW.md`.

### Demo flow

`src/game/demo_flow.c` remains an application rule. It describes scenes through a rule table (timeout/event/failure owner) instead of scattering these transitions across large `switch` statements. It uses `UTimerPool` for presentation durations and to expose the countdown to the HUD.

This flow must not be confused with `UStateGraphClock`: the former orchestrates the concrete demo scenario; the latter is an optional generic component used to time a `UStateGraph`.

## 17. Boundary between engine and game

A practical rule helps decide where new code belongs:

- behavior reusable across several games: `engine/`;
- content or rules specific to Unsigned/demo: `src/`;
- logical representation of presentation: `engine/display/`;
- rendering policy: `engine/renderer/`;
- BIOS/hardware access: `engine/system/`;
- concrete scene or level: `src/levels/` or `src/game/`;
- concrete character: `src/characters/`;
- concrete interface: `src/menu/` or `src/hud/`.

This separation is the foundation to preserve as the architecture evolves.
