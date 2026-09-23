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
├── display/     camera, presentation effects, sprites, text, UI and viewport
├── game/        UGameInstance composition root
├── gameplay/    attributes, tags, abilities, effects, cues and runtime
├── input/       generic input state
├── level/       definition/runtime, Beat'Em Up camera, spawns, AI, collisions, backgrounds
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
├── effect/      generic effects shared by viewports and sprites
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

1. derive the collision-manager capacities from the authored actor-pool capacities;
2. reset the runtime and initialize the level renderer;
3. wire fixed actor pools and the gameplay runtime;
4. wire the level runtime to caller-owned spawn/collision/actor storage;
5. initialize input, audio and timers;
6. initialize the viewport;
7. initialize level flow either in graph mode (`ULevelGraph`) or direct mode (`initial_level`).

Variable-capacity memory is provided by the application through `UGameInstanceStorage`. The engine does not own or allocate these arrays. Configuration is trusted authored data: capacities, storage and mutually exclusive level-flow choices are construction contracts documented in `engine/game/game.h`, not defensive branches repeated during initialization.

`unsigned_game_instance_destroy()` hides renderer-owned hardware state, stops level flow, clears runtime gameplay/timer state and resets the `UGameInstance`; caller-owned backing arrays remain owned by the application.

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
prepare_render callback
(CPU/RAM only: culling, layout, effects, RenderPlan)
        |
        v
ng_wait_vblank()
        |
        v
audio transport
        |
        v
application render callback
(post-VBlank VRAM commit)
        |
        v
render_phase callback
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

demo_loop_prepare_render
  -> unsigned_game_instance_prepare_render

demo_loop_render
  -> unsigned_game_instance_render (commit)
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

Level rendering is split between `unsigned_game_instance_prepare_render()` during active display and the commit performed by `unsigned_game_instance_render()` immediately after VBlank. The prepare call is a frame precondition for the commit; `unsigned_game_instance_render()` does not recompute a missing plan.

## 5. Level lifecycle

`ULevelDefinition` (`engine/level/level_definition.h`) describes stable content. `ULevel` (`engine/level/level_runtime.h`) contains mutable state.

### Loading

`unsigned_level_load()`:

1. unloads the previous level;
2. binds the new definition and application context;
3. resets TLSS, background state and the Beat'Em Up camera;
4. installs declared background layers;
5. calls the optional `load` hook;
6. instantiates declared spawns;
7. updates the background once from the initial camera position;
8. builds persistent static collisions;
9. calls the optional `enter` hook.

Level content/capacity relationships are authored contracts. `unsigned_level_load()` is a direct replace operation rather than a fallible transaction API.

### Unloading

`unsigned_level_unload()` calls:

1. `exit`;
2. `unload`;
3. cleanup of the level's mutable state.

### Level tick

The actual order in `engine/level/level.c` is:

1. begin the TLSS frame;
2. tick actors + player input;
3. classify and tick NPC AI;
4. tick active abilities;
5. update the Beat'Em Up camera from active players;
6. build/order the shared actor view once for collision and rendering;
7. run collision detection, which probes first and only materializes dynamic collision state on query-producing frames;
8. call `resolve_hits` when detected hits exist;
9. tick effects;
10. tick cues;
11. update background scrolling from the camera position.

The engine detects interactions; the level definition decides their gameplay meaning through `resolve_hits`.

`resolve_hits` consumes the frame-local `UCollisionHitContainer` already produced in `level->collision.hits`. Gameplay code must not rebuild hitboxes/hurtboxes in world space to perform a second detection pass: geometry belongs to the collision pipeline, while damage, guard, knockdown and combo rules belong to the game.
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
           height + character shadow
             |
             +--> UPlayer
             |      input + ability bindings
             |
             +--> UNpc
                    state graph + activity + TLSS AI

UProjectile -> references a UActor + trajectory/lifetime
```

`UActorContainer` is a non-owning view over active actors gathered from specialized pools.

`UActor` intentionally has no concept of playable area or bounds. It stores a world position; `UCharacter` provides generic movement/orientation operations, but the decision to constrain that movement remains external to the actor. `UCharacter.height` is presentation elevation: `actor.position` stays on the ground plane for movement and depth ordering, while the body sprite is shifted vertically and `UCharacterShadow` shrinks the ground shadow. The renderer does not know this rule; it still handles the shadow as a generic `underlay`.

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
- `unsigned_character_move(character, direction, horizontal_speed, vertical_speed)` interprets each direction component only by sign, applies independent whole-pixel axis speeds, saturates world coordinates to `s16`, and updates facing from horizontal input;
- `unsigned_character_set_facing()` remains available when orientation must change without moving;
- `UMovementBounds` and `unsigned_physics_movement_constrain()` live in `engine/physics/movement.h` / `engine/physics/movement.c`;
- the owner of the movement explicitly decides whether bounds apply. A projectile or NPC can therefore remain free to leave the playable area.

The demo reuses this same engine primitive both for normal ground movement (`src/characters/player/demo/player_move.c`) and Arthur's air steering (`src/characters/player/arthur/arthur_jump.c`), then applies level-owned bounds separately. Movement mechanics stay in `UCharacter`, while playable-area policy stays outside `UActor`.

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
- the runtime owner decides whether it needs a clock. `ULevelManagerMode` makes this explicit: graph mode owns/ticks the clock, while direct mode leaves level replacement to the higher-level caller.

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

The level collision pipeline keeps static object collision registered from level load and avoids rebuilding dynamic state when no gameplay source can query it:

1. probe active actors cheaply for hitbox activity when no projectiles are active;
2. return through the idle fast path when neither dynamic nor persistent hitboxes can produce a query;
3. on query-producing frames, clear the old dynamic tail once and materialize each active player's/NPC's/non-static object's boxes in the same pool traversal;
4. build `UCollisionActorIndex` from the shared actor ordering prepared by the level;
5. resolve projectiles when present;
6. detect attacker/target pairs when hitboxes are present.

Collision storage sizes are composition contracts. The hot path therefore focuses on avoiding work instead of carrying defensive capacity branches.

Detected pairs are stored as frame-local `UCollisionHit` values in `level->collision.hits`. The `resolve_hits` callback consumes that container directly, applies damage/guard/reactions or deduplicates an activation, and does not repeat the geometric intersection that the engine has already resolved.

## 11. Display, renderer and system

The current structure explicitly separates three responsibilities.

### `engine/display`

Contains backend-independent presentation structures and behavior:

- camera;
- generic presentation effects (`UEffect`);
- sprite and render state;
- text;
- UI;
- viewport.


`UCamera` does not know about players or levels: it owns only its world-space position, previous position and bounds. `UViewport` remains the world/screen conversion boundary and centralizes visible-rectangle intersection tests. Beat'Em Up follow policy lives in `engine/level/level_camera.c`: it aggregates active players, applies the dead zone, optional backtracking, level limits and multiplayer screen constraints.

`UCameraBounds` can be overridden at runtime with `unsigned_level_camera_set_bounds()` and restored with `unsigned_level_camera_reset_bounds()`, providing the primitive needed for arena locks without adding event logic to the camera itself.

Generic UI lives in `engine/display/ui/`. Reusable widgets live in `engine/display/ui/widget/`, for example `engine/display/ui/widget/progress_bar.c`.

Visual effects are centralized in `engine/display/effect/`: `effect.[ch]` defines sampling/composition and `effects.[ch]` groups the built-in presets. The same `UEffect` can be owned by `UViewport.effect` or `USprite.effect`. An object has one local effect slot; the renderer then composes the viewport effect with the sprite effect. Built-in presets cover static transform, zoom, pseudo-3D turn, shake, bob, blink, shear and wave. Their configuration is non-owning and must outlive the binding. Gameplay configures an effect; it does not manipulate SCBs or Neo Geo shrink units.

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

The renderer/system boundary is split by responsibility: `engine/system/renderer_backend.h` owns the global begin/end hardware transaction, while `engine/system/sprite_backend.h`, `engine/system/background_backend.h` and `engine/system/palette_backend.h` expose domain-specific Neo Geo operations. Renderers include only the backend contract they need.

## 12. Backgrounds and sprites

`engine/level/background/background.c` now derives parallax from the camera's absolute X position; it no longer follows a player or another gameplay actor directly. `engine/renderer/background_renderer.c` then treats backgrounds as reusable strips of hardware sprites. The cache avoids rewriting the entire screen for a simple scroll.

`engine/renderer/sprite_renderer.c` uses sprite dirty state so only modified parts are pushed to the backend: graphics, position, scale/shrink, flips and layout. It composes the viewport effect with the sprite-local effect, applies pivots, and chooses between the compact hardware chain and per-column rendering when required.

`USpriteRenderState` mirrors this lifecycle explicitly: `layout` owns the assigned hardware range, `committed` mirrors the last state written to hardware, `previous` keeps the ownership snapshot needed across relocation, and `prepared` freezes CPU-side transform data between active-display preparation and VBlank commit. Dirty bits remain a compact top-level mask. This separation is structural only; it adds no runtime dispatch or allocation.

`UCharacterShadow` remains a character component, but its responsibility is now limited to converting `UCharacter.height` into a generic scale. It owns a `UTransformEffect` bound to `shadow.sprite.effect`; rendering math and Neo Geo encoding stay outside `engine/actor/character_shadow.c`.

Native sprite transforms exposed by the engine are translation, X/Y scale up to 100%, pivots, X/Y flips, visibility and per-column deformation. The `turn` preset creates a pseudo-3D rotation illusion with squash + flip. Neo Geo hardware cannot magnify a sprite beyond its source size or perform arbitrary rotation: true >100% zoom or free rotation requires pre-rendered frames/tiles.

The hardware-specific vertical-shrink caveat remains inside `engine/system/sprite_backend.c`. On Neo Geo, SCB3 keeps a fixed display window while the vertical zoom lookup can read SCB1 rows below the sprite's logical height. A `USpriteDefinition` can therefore opt into `clear_unused_rows` and provide a `transparent_tile`; the backend initializes unused SCB1 rows with that tile only when the hardware layout is rebuilt. This prevents stale VRAM data from resurfacing during shrink without adding work to every animation frame.

`engine/renderer/level_renderer.c` now uses a two-phase `URenderPlan`. Preparation performs no VRAM writes: it computes visibility, actor ranges, relocations, uniform effect transforms and background ring-buffer uploads. The post-VBlank commit executes the plan in priority order:

```text
ACTIVE DISPLAY
    unsigned_level_renderer_prepare()
        -> actor culling/layout/relocation detection
        -> sprite transform/effect preparation
        -> background upload/transform planning
        -> VRAM word estimate / relocation counters

VBLANK
    unsigned_renderer_backend_begin()
        CRITICAL -> stale actor-range clears + chain-boundary breaks
        HIGH     -> actor SCB1/SCB2/SCB3/SCB4 commits
        NORMAL   -> background column uploads + scrolling transforms
        DEFERRED -> reserved for future safely postponable work
    unsigned_renderer_backend_end()
```

`URenderFrameStats` exposes the estimated VRAM words per priority, actor relocation count, actor sprite-column count and background column uploads. The latest prepared frame is queried with `unsigned_level_renderer_stats()` on the owning `ULevelRenderer` (for a game instance, `&game->renderer`).

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

The progress bar can be bound to a `UGameplayAttribute`. The widget stores normalized progress in the 0..256 range so the renderer does not depend directly on gameplay state. While visible, the HUD observes Health and its bounds, compares them with its cache, and synchronizes the widget only when a displayed value changed. It never takes ownership of `UGameplayAttribute.on_change`, so UI cannot replace a gameplay callback or retain a subscription that must be detached after the player is destroyed.

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

The details of USER 1/2/3 lifecycle, GAME START COMPULSION, audio transport and credits are documented in `docs/Bios_en.md`.

### Demo flow

`src/game/demo_flow.c` remains an application rule. `UDemoScene` / `UDemoFlowContext.current` is the source of truth for the active demo scene; stage/scene code no longer rediscovers scene identity by comparing `ULevelDefinition *` pointers. Transitions are described through a rule table (timeout/event/completion owner) instead of being scattered across large `switch` statements. Stable dependencies are bound once through `UDemoFlowConfig`, and `UTimerPool` owns presentation durations/countdowns exposed to the HUD.

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
