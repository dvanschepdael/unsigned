# Getting Started with Unsigned

This guide is intended for a developer who knows the basics of C but is new to the engine and/or the Neo Geo. The goal is not to memorize every file: the important part is to understand **who owns the data**, **who decides behavior**, **who decides rendering**, and **who actually writes to the hardware**.

## 1. Five landmarks to know

### `UGameInstance`: the engine root

`UGameInstance` (`engine/game/game.h`) groups the generic subsystems of a game session: input, timers, gameplay, actor pools, level, renderer, level manager, viewport and audio.

`unsigned_game_instance_init()` receives already allocated buffers through `UGameInstanceStorage`. Insufficient capacity should therefore be fixed at startup rather than hidden behind dynamic allocation during gameplay.

### `ULevelDefinition`: declared content

`ULevelDefinition` (`engine/level/level_definition.h`) contains stable elements:

- backgrounds;
- Beat'Em Up camera policy;
- NPC/object spawns;
- `load`, `enter`, `exit`, `unload` callbacks;
- actor ordering;
- gameplay hit resolution;
- backdrop color.

The definition must remain valid for as long as the level uses it.

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

The `ULevelDefinition` / `ULevel` split prevents content data from being mixed with runtime state.

### `UActor` / `UCharacter`: world and combat

`UActor` represents presence in the world: position, sprite and collision.

`UCharacter` adds:

- attributes;
- abilities;
- tags;
- orientation;
- presentation height above the ground plane;
- an optional ground shadow whose shrink follows that height.

`UPlayer` and `UNpc` then wrap a `UCharacter` with their respective control logic.

### `engine/system`: the Neo Geo boundary

BIOS, VRAM, FIX, video, platform input, audio transport and Neo Geo storage details are grouped under `engine/system/`.

Level or character code must not bypass this layer to write directly to hardware registers.

## 2. Reading the directory tree without getting lost

Start with this map:

```text
engine/
  core/       generic building blocks
  actor/      entities and pools
  gameplay/   attributes/tags/abilities/effects/cues
  level/      level lifecycle and orchestration
  physics/    geometric collision, movement constraints, trajectories
  collision/  gameplay collision
  display/    presentation data, effects, sprites, UI, viewport
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

Simple rule: `engine/` must remain reusable; `src/` may know the exact needs of the game.

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

The BIOS remains authoritative over some system transitions. Do not reason as if this were a fully autonomous PC-style main loop.

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
Beat'Em Up camera
    |
collision registration + hit/projectile detection
    |
level resolve_hits
    |
effects
    |
cues
    |
background <- camera.x
```

This order is a behavioral contract. Moving a stage can change gameplay even if the code still compiles.

Example: `resolve_hits` runs after abilities but before end-of-frame effects/cues.

## 5. Understanding fixed memory

The engine uses fixed-capacity pools. A `UPoolInstance` notably contains a `generation`.

Why? A callback can:

1. release a slot;
2. immediately reuse it;
3. leave the same physical pointer visible to the caller.

The generation distinguishes the old logical instance from the new one.

### Practical rule

Do not keep a pointer to a slot while assuming it remains the same logical object. Check the subsystem contract and, when available, the `generation` mechanism.

## 6. Where should code be added?

### A new generic character mechanic

Look first at:

- `engine/actor/character.h`;
- `engine/gameplay/`;
- optionally `engine/physics/` or `engine/collision/`.

If the mechanic only applies to one game character, put it under `src/characters/` instead.

### A new player action

1. define or reuse a `UGameplayAbility`;
2. define its tags and conditions;
3. create a `UGameplayAbilityBinding`;
4. choose the `UInputTrigger`;
5. choose `U_INPUT_MATCH_ALL` or `U_INPUT_MATCH_ANY` for the button mask;
6. define how the ability ends or is cancelled;
7. test activation failure when the pool is full.

For continuous direction input, prefer one `ANY` binding covering the D-pad, then read `UPlayer.input_state` / `unsigned_input_direction()`. Do not reserve one separate ability per direction when the logical action is unique.

Useful files:

- `engine/actor/player.c`;
- `engine/input/input.c`;
- `engine/gameplay/ability.c`;
- `engine/gameplay/ability_pool.c`;
- `engine/gameplay/gameplay_pool.c`;
- examples under `src/characters/player/demo/` and `src/characters/player/arthur/`.

For a reaction that replaces all current actions of a character, use the owner-level pool API (`release_owner` / `replace_owner`) instead of inspecting `UAbilityPool` internal arrays directly.

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

Do not add bounds to `UActor`: the actor stores a position, while level/gameplay code decides whether that position must be constrained.

### A new attribute

Attributes are defined in `engine/gameplay/attribute.h`.

For an attribute specific to the demo player character, see `src/characters/player/demo/player_health.c`.

An `on_change` callback can synchronize UI, as the HUD does for player health. To apply a delta, prefer `unsigned_gameplay_attribute_add_current_value()` instead of reimplementing clamp logic locally.

### A new NPC

1. define its content/spawn;
2. provide its `UCharacter`;
3. configure its state graph;
4. call `unsigned_npc_init()`;
5. let `level_ai.c` and TLSS manage its cadence according to activity.

`UStateGraph` does not contain elapsed time. If an owner needs `U_TRANSITION_ON_TIMEOUT` transitions, it must explicitly own a `UStateGraphClock` and use `unsigned_state_graph_clock_tick()`. Do not reintroduce a time counter into the generic graph structure.

Useful files:

- `engine/actor/npc.h`;
- `engine/actor/npc_ai.c`;
- `engine/level/level_ai.c`;
- `engine/core/tlss/tlss.h`.

### A new level

Concrete content belongs under `src/levels/<level_name>/`.

1. declare a `ULevelDefinition`;
2. define backgrounds and spawns with a sufficient lifetime;
3. use `load` for setup that cannot be expressed as data;
4. use `enter` / `exit` for state changes around the active level;
5. use `unload` to undo work performed by `load`;
6. expose the definition to `src/game/demo_scenes.c` or the relevant flow.

Loading is transactional: an error triggers rollback of content that has already been installed.

## 7. Display, renderer and system: do not mix them up

This separation is essential in the current structure.

### `engine/display`

Describes backend-independent concepts:

- generic effects (`UEffect`);
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

Performs operations that truly depend on the Neo Geo:

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

### Applying an effect to any `USprite`

Presentation effects live in `engine/display/effect/` and use the same `UEffect` abstraction as the viewport. Include `display/effect/effect.h` for the generic mechanism, then include only the concrete effect header you use (`transform.h`, `zoom.h`, `turn.h`, `wave.h`, `bob.h`, `shake.h`, `shear.h` or `blink.h`). Every sprite owns `sprite.effect`; the effect keeps advancing even when sprite animation is stopped.

Centered pulse-zoom example:

```c
static const UZoomEffect pulse = {
    .min_scale_x = 160u,
    .min_scale_y = 160u,
    .max_scale_x = U_EFFECT_SCALE_ONE,
    .max_scale_y = U_EFFECT_SCALE_ONE,
    .pivot_x = U_EFFECT_PIVOT_CENTER,
    .pivot_y = U_EFFECT_PIVOT_CENTER,
};

unsigned_effect_set_zoom(&actor.sprite.effect, &pulse, 2u);
```

`UEffect` does not copy `pulse`: the configuration must remain valid while the effect is active. A `static const` configuration is appropriate for immutable presets; for gameplay-driven values, store the configuration in the runtime object that owns those values. `unsigned_effect_clear()` detaches the effect.

For a gameplay-driven transform, use `UTransformEffect`, then edit its offset, scale, flips or visibility without touching the renderer. `UCharacterShadow` follows exactly this model: it maps character height to scale and leaves `sprite_renderer` + `sprite_backend` to do the rest.

Available presets: `transform`, `zoom`, `turn`, `shake`, `bob`, `blink`, `shear`, `wave`. For game-specific effects, `unsigned_effect_set_advanced()` accepts a uniform or per-column callback while reusing the same pipeline. Sprite effects are presentation-only: they do not move `actor.position` or transform hitboxes/hurtboxes. `turn` is a pseudo-3D rotation made from shrink + flip. Neo Geo hardware provides neither free rotation nor magnification above 100%; use pre-rendered graphics for those cases.

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
2. binds it to a non-owning health `UGameplayAttribute`;
3. compares Health, MinHealth and MaxHealth with its cache while rendering;
4. calls `unsigned_ui_progress_bar_sync()` only when a displayed value changed;
5. renders the `UUIScreen` through `UUIRenderer` + `UNeoGeoUIRenderer`.

The HUD does not replace `UGameplayAttribute.on_change`. The character owner therefore keeps control of gameplay callbacks. The bound attribute and its bounds must remain alive while the HUD is visible.

For menus, use `unsigned_ui_input_from_controller()` to convert the controller snapshot into standard `UUIInput` instead of duplicating direction/A/B mapping in each screen.

## 9. Collisions: two layers to distinguish

`engine/physics` knows about boxes and collision layers. It does not know what an attack is.

`engine/collision` adds:

- hitbox/hurtbox;
- actors;
- projectile;
- hit detection.

`engine/level/level_collision.c` then orchestrates all of this for the active level.

Static collisions are built at load time. Dynamic collisions are cleared and registered again every frame.

The `ULevelDefinition.resolve_hits` callback must read already detected pairs through `unsigned_level_collision_hits()`. It then applies game rules (damage, guard, reactions, deduplication), but does not recompute hitbox/hurtbox intersections itself.

If registration buffers are saturated, the collision frame is invalidated instead of being partially computed.

## 10. State graph: adding time without polluting the graph

`UStateGraph` owns the current logical state, not a frame counter. `UStateGraphNode.duration_frames` is definition data; `UStateGraphClock` is the optional temporal runtime.

The pattern is:

```text
unsigned_state_graph_init(...)
unsigned_state_graph_clock_reset(&clock, &graph)

each frame that requires timeouts:
    unsigned_state_graph_clock_tick(&clock, &graph)
```

If no timeout is required, simply call `unsigned_state_graph_tick()` and do not store a clock. `ULevelManager` demonstrates both modes: graph-driven with a clock, or direct control without a graph.

## 11. TLSS: temporally reduced simulation

TLSS lives in `engine/core/tlss/`.

It can spread work over 1/2/4/8/16 frames. Two uses are currently configurable in `UGameInstanceConfig`:

- AI;
- collision.

Off-screen or dormant NPCs can therefore cost less without reducing the global frame rate.

Important: collision has an immediate-resolution mechanism for a hitbox that has just become active so a reduced TLSS cadence does not lose its first attack instant.

## 12. Neo Geo: what you should not simulate yourself

On MVS/AES, the BIOS owns part of the lifecycle.

The system runtime notably manages:

- USER requests;
- ATTRACT/TITLE/GAME/GAME_OVER phases;
- `PLAYER_START`;
- player session;
- credits;
- GAME START COMPULSION;
- associated audio handoff.

Before changing this domain, read `docs/Bios_en.md`, then the relevant public files under `engine/system/`.

Do not simply convert a START button read from generic input into a decision to begin an MVS session: the BIOS `PLAYER_START` remains the authoritative source.

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

In `demo_flow.c`, scene transitions are described by a rule table. `UTimerPool` remains responsible for presentation durations/countdowns. Do not confuse this application flow with `UStateGraphClock`, which is only used to time a generic `UStateGraph`.

## 14. Recommended modification workflow

For a contribution:

1. find the subsystem's public `.h` API;
2. read the corresponding implementation;
3. read at least one test under `test/`;
4. identify fixed capacities and pointer lifetimes;
5. make the smallest change;
6. add or adapt a host test;
7. then verify in MAME/hardware if the change depends on the Neo Geo.

When a comment is necessary, document the invariant, lifetime, performance reason or BIOS constraint. Avoid paraphrasing the code.
