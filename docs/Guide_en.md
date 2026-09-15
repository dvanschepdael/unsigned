# Getting Started with Unsigned

This guide is intended for developers who know the basics of C and are discovering the engine and/or the Neo Geo. The reading strategy is organized around four structuring questions: **who owns the data**, **who decides behavior**, **who decides rendering**, and **who actually writes to the hardware**. These questions make it possible to locate subsystem responsibilities quickly without memorizing every file.

## 1. Five landmarks to know

### `UGameInstance`: the engine root

`UGameInstance` (`engine/game/game.h`) groups the generic subsystems of a game session: input, timers, gameplay, actor pools, level, renderer, level manager, viewport and audio.

`unsigned_game_instance_init()` receives already allocated buffers through `UGameInstanceStorage`. Capacities are sized at startup, which keeps memory usage deterministic during gameplay.

### `ULevelDefinition`: declared content

`ULevelDefinition` (`engine/level/level_definition.h`) contains stable elements:

- backgrounds;
- NPC/object spawns;
- `load`, `enter`, `exit`, `unload` callbacks;
- actor ordering;
- gameplay hit resolution;
- backdrop color.

The definition remains valid for the full lifetime of the active level.

### `ULevel`: mutable state

`ULevel` (`engine/level/level_runtime.h`) contains what changes during gameplay:

- active actors;
- associated pools;
- TLSS;
- collisions;
- background;
- camera;
- gameplay runtime;
- active definition and application context.

The `ULevelDefinition` / `ULevel` split clearly separates content data from runtime state.

### `UActor` / `UCharacter`: world and combat

`UActor` represents presence in the world: position, sprite and collision.

`UCharacter` adds:

- attributes;
- abilities;
- tags;
- orientation.

`UPlayer` and `UNpc` then wrap a `UCharacter` with their respective control logic.

### `engine/system`: the Neo Geo boundary

BIOS, VRAM, FIX, video, platform input, audio transport and Neo Geo storage details are grouped under `engine/system/`.

Level and character code use this layer's APIs for operations involving hardware registers, BIOS services or VRAM.

## 2. Reading the directory tree quickly

Start with this map:

```text
engine/
  core/       generic building blocks
  actor/      entities and pools
  gameplay/   attributes/tags/abilities/effects/cues
  level/      level lifecycle and orchestration
  physics/    geometric collision, movement constraints, trajectories
  collision/  gameplay collision
  display/    presentation data, UI, viewport
  renderer/   rendering policy/cache
  system/     Neo Geo/BIOS/hardware backends
  audio/      audio logic
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

Simple rule: `engine/` contains reusable mechanisms; `src/` describes the game's specific needs and composes those mechanisms.

## 3. From BIOS to one game frame

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

The BIOS remains authoritative over some system transitions. The game loop is therefore best understood as cooperation between application runtime and the BIOS lifecycle, rather than as a fully autonomous main loop.

## 4. Reading one level frame

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

This order is a behavioral contract. Changing the position of a stage can change gameplay even when the code still compiles.

Example: `resolve_hits` runs after abilities but before end-of-frame effects/cues.

## 5. Understanding fixed memory

The engine uses fixed-capacity pools. A `UPoolInstance` notably contains a `generation`.

Why? A callback can:

1. release a slot;
2. immediately reuse it;
3. leave the same physical pointer visible to the caller.

The generation distinguishes the old logical instance from the new one.

### Practical rule

When keeping a reference to a slot, rely on the subsystem contract and, where available, the `generation` mechanism. Generation identifies the logical instance even when a memory address is reused.

## 6. Where should code be added?

### A new generic character mechanic

Look first at:

- `engine/actor/character.h`;
- `engine/gameplay/`;
- optionally `engine/physics/` or `engine/collision/`.

A mechanic specific to one game character naturally belongs under `src/characters/`.

### A new player action

1. define or reuse a `UGameplayAbility`;
2. define its tags and conditions;
3. create a `UGameplayAbilityBinding`;
4. choose the `UInputTrigger`;
5. choose `U_INPUT_MATCH_ALL` or `U_INPUT_MATCH_ANY` for the button mask;
6. define how the ability ends or is cancelled;
7. test activation failure when the pool is full.

For continuous direction input, one `ANY` binding can cover the D-pad; the ability then reads `UPlayer.input_state` / `unsigned_input_direction()`. This keeps one ability for one logical action, including diagonal movement.

Useful files:

- `engine/actor/player.c`;
- `engine/input/input.c`;
- `engine/gameplay/ability.c`;
- `engine/gameplay/ability_pool.c`;
- `engine/gameplay/gameplay_pool.c`;
- examples under `src/characters/player/demo/` and `src/characters/player/arthur/`.

For a reaction that replaces all current actions of a character, the owner-level pool API (`release_owner` / `replace_owner`) centralizes replacement and encapsulates `UAbilityPool` internal arrays.

### Moving a character and limiting its area

Separate the three responsibilities:

```text
input direction  ->  character movement  ->  optional world constraint
engine/input         engine/actor            engine/physics
```

- `unsigned_input_direction()` produces a directional `Vec2`;
- `unsigned_character_move()` changes position;
- `unsigned_character_set_facing()` handles orientation;
- `unsigned_physics_movement_constrain()` optionally applies `UMovementBounds`.

`UActor` stores the world position. Level or gameplay code then selects `UMovementBounds` and applies the constraint when the playable area requires it.

### A new attribute

Attributes are defined in `engine/gameplay/attribute.h`.

For an attribute specific to the demo player character, see `src/characters/player/demo/player_health.c`.

An `on_change` callback can synchronize UI, as the HUD does for player health. To apply a delta, `unsigned_gameplay_attribute_add_current_value()` directly provides the shared saturation, clamp and notification rules.

### A new NPC

1. define its content/spawn;
2. provide its `UCharacter`;
3. configure its state graph;
4. call `unsigned_npc_init()`;
5. let `level_ai.c` and TLSS manage its cadence according to activity.

`UStateGraph` carries the graph's logical state. When an owner uses `U_TRANSITION_ON_TIMEOUT` transitions, it explicitly owns a `UStateGraphClock` and calls `unsigned_state_graph_clock_tick()`. Temporal state therefore stays in the dedicated timing component.

Useful files:

- `engine/actor/npc.h`;
- `engine/actor/npc_ai.c`;
- `engine/level/level_ai.c`;
- `engine/core/tlss/tlss.h`.

### A new level

Concrete content belongs under `src/levels/<level_name>/`.

1. declare a `ULevelDefinition`;
2. define backgrounds and spawns with a sufficient lifetime;
3. use `load` for imperative setup that complements declarative data;
4. use `enter` / `exit` for state changes around the active level;
5. use `unload` to undo work performed by `load`;
6. expose the definition to `src/game/demo_scenes.c` or the relevant flow.

Loading is transactional: an error triggers rollback of content that has already been installed.

## 7. Display, renderer and system: three complementary responsibilities

This separation is essential in the current structure.

### `engine/display`

Describes backend-independent concepts:

- sprites;
- text;
- viewport;
- UI elements;
- widgets;
- layout/screen/page.

### `engine/renderer`

Decides how these concepts are rendered:

- visibility;
- ordering;
- sprite allocation;
- dirty state;
- background cache;
- Neo Geo UI adaptation.

### `engine/system`

Performs operations that depend directly on the Neo Geo:

- VRAM;
- FIX;
- palette;
- video;
- BIOS;
- platform input;
- audio transport;
- save backend.

### Example

To display an actor:

```text
UActor / USprite
    -> engine/renderer/actor_renderer.c
    -> engine/renderer/sprite_renderer.c
    -> engine/system/renderer_backend.h
    -> engine/system/sprite_backend.c
    -> VRAM / SCB
```

To display a widget:

```text
UUIProgressBar
    -> UUIRenderer
    -> engine/renderer/ui_renderer.c
    -> engine/system/fix.c
    -> FIX layer
```

## 8. Adding or changing UI

Generic UI lives under `engine/display/ui/`.

Widgets live under `engine/display/ui/widget/`:

- button;
- image;
- label;
- panel;
- progress bar;
- selector;
- blink label.

For a concrete game interface, use `src/menu/` or `src/hud/`.

### Example: current HealthBar

`src/hud/demo_hud.c`:

1. creates a `UUIProgressBar`;
2. binds it to a health `UGameplayAttribute`;
3. attaches `on_change` to the attribute and its bounds;
4. calls `unsigned_ui_progress_bar_sync()` when the value changes;
5. renders the `UUIScreen` through `UUIRenderer` + `UNeoGeoUIRenderer`.

The widget stores normalized progress in the 0..256 range. The renderer reuses this normalized value directly on each frame. The current HUD intentionally owns the `on_change` slots while bound; this policy remains in `src/hud/`.

For menus, `unsigned_ui_input_from_controller()` converts the controller snapshot into standard `UUIInput` and centralizes direction/A/B mapping for all screens.

## 9. Collisions: two complementary layers

`engine/physics` handles boxes, collision layers and geometric primitives. Attack semantics appear in `engine/collision`, which adds:

- hitbox/hurtbox;
- actors;
- projectile;
- hit detection.

`engine/level/level_collision.c` then orchestrates the complete pipeline for the active level.

Static collisions are built at load time. Dynamic collisions are cleared and registered again every frame.

The `ULevelDefinition.resolve_hits` callback iterates over already detected pairs through `unsigned_level_collision_hits()`, then applies game rules such as damage, guard, reactions and deduplication. Geometric intersection remains the responsibility of the collision pipeline.

If registration buffers are saturated, the collision frame is invalidated rather than partially computed.

## 10. State graph: separating logic and time

`UStateGraph` owns the current logical state. `UStateGraphNode.duration_frames` is definition data; `UStateGraphClock` carries the optional temporal runtime.

The pattern is:

```text
unsigned_state_graph_init(...)
unsigned_state_graph_clock_reset(&clock, &graph)

each frame that requires timeouts:
    unsigned_state_graph_clock_tick(&clock, &graph)
```

Without timeouts, `unsigned_state_graph_tick()` is sufficient and the owner can operate without a clock. `ULevelManager` demonstrates both modes: graph-driven with a clock, or direct control without a graph.

## 11. TLSS: temporally reduced simulation

TLSS lives in `engine/core/tlss/`.

It can spread work over 1/2/4/8/16 frames. Two uses are currently configurable in `UGameInstanceConfig`:

- AI;
- collision.

Off-screen or dormant NPCs can therefore cost less while preserving the global frame rate.

Collision includes an immediate-resolution mechanism for a hitbox that has just become active. This preserves its first attack instant even with a reduced TLSS cadence.

## 12. Neo Geo: BIOS and system-runtime responsibilities

On MVS/AES, the BIOS owns part of the lifecycle.

The system runtime notably manages:

- USER requests;
- ATTRACT/TITLE/GAME/GAME_OVER phases;
- `PLAYER_START`;
- player session;
- credits;
- GAME START COMPULSION;
- associated audio handoff.

When changing this domain, start with `engine/system/BIOS_WORKFLOW.md`, then read the relevant public files under `engine/system/`.

A START press coming from generic input becomes an application request; effective MVS player acceptance remains driven by the BIOS `PLAYER_START`, which is the authoritative source.

## 13. Reading the demo project

To understand how all layers are assembled, follow this order:

1. `main.c`;
2. `src/game/demo_game.c`;
3. `src/game/demo_loop.c`;
4. `src/game/demo_flow.c`;
5. `src/game/demo_scenes.c`;
6. one level under `src/levels/`;
7. the player under `src/characters/player/demo/`;
8. `src/hud/demo_hud.c` and `src/menu/main/demo_menu.c`.

This path shows the boundary between the reusable engine and game content more clearly than reading every file in `engine/` one by one.

In `demo_flow.c`, scene transitions are described by a rule table. `UTimerPool` remains responsible for presentation durations/countdowns. `demo_flow.c` orchestrates the concrete demo scenario, while `UStateGraphClock` times a generic `UStateGraph`: the two components serve distinct responsibilities.

## 14. Recommended modification workflow

For a contribution:

1. find the subsystem's public `.h` API;
2. read the corresponding implementation;
3. read at least one test under `test/`;
4. identify fixed capacities and pointer lifetimes;
5. make the smallest change;
6. add or adapt a host test;
7. then verify in MAME/hardware if the change depends on the Neo Geo.

When a comment is useful, document the invariant, lifetime, performance reason or BIOS constraint. Prefer information that complements the code: intent, contract or technical rationale.