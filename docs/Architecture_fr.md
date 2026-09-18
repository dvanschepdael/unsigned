# Architecture technique d'Unsigned

Ce document décrit l'organisation **actuelle** du moteur et du projet de démonstration. Le principe central est de garder les règles de jeu et la simulation séparées des détails Neo Geo afin de pouvoir tester la majorité du moteur sur hôte.

## 1. Sens des dépendances

La dépendance générale va du contenu du jeu vers les contrats du moteur, puis vers les backends matériels :

```text
src/  (jeu, personnages, niveaux, menus, HUD)
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

core = types et briques génériques partagées par les couches moteur
save = stockage persistant générique + backend plateforme
```

Les dépendances ne sont pas toutes strictement verticales, mais cette direction donne la règle à respecter :

- `src` décrit le jeu et compose le moteur ;
- `engine/display` décrit ce qui doit être présenté ;
- `engine/renderer` décide comment transformer cet état en opérations de rendu ;
- `engine/system` réalise les opérations spécifiques à la Neo Geo et au BIOS.

Le code de gameplay ne doit donc pas écrire directement dans la VRAM ou lire la RAM BIOS pour contourner les APIs du moteur.

## 2. Arborescence actuelle

### Moteur

```text
engine/
├── actor/       acteurs, personnages, joueurs, NPC, objets, projectiles, pools
├── audio/       événements, musique, résolution et commandes audio logiques
├── collision/   index d'acteurs, hit detection, projectiles, collision gameplay
├── core/        types, math, pools, state graph, timers, TLSS
├── display/     caméra, effets de présentation, sprites, texte, UI et viewport
├── game/        composition root UGameInstance
├── gameplay/    attributes, tags, abilities, effects, cues et runtime
├── input/       état d'entrée générique
├── level/       définition/runtime, caméra Beat'Em Up, spawns, AI, collisions, backgrounds
├── physics/     géométrie/collision bas niveau, contraintes de mouvement et trajectoires
├── renderer/    rendu acteurs/background/niveau/UI et cache de rendu
├── save/        records, blocks, sets et abstraction de stockage
└── system/      runtime BIOS et backends Neo Geo
```

Quelques sous-répertoires importants :

```text
engine/core/
├── math/
├── pool/
├── state/       state_graph + state_graph_clock
├── timer/
└── tlss/

engine/display/
├── camera/
├── effect/      effets génériques partagés par viewport et sprites
├── sprite/
├── text/
├── ui/
│   └── widget/
└── viewport/

engine/level/
└── background/
```

### Jeu de démonstration

```text
src/
├── audio/                  driver Z80 / son projet
├── characters/
│   ├── npc/
│   └── player/             runtime joueur de démo + personnages concrets (dont Arthur)
├── game/                   composition, flow, boucle et adaptation runtime Neo Geo
├── hud/                    HUD de jeu
├── levels/
│   ├── common/
│   ├── forest_edge/
│   ├── old_road/
│   └── ruin_gate/
├── localizations/          textes de la démo
└── menu/main/              menu principal / sélection
```

`main.c` reste volontairement mince : il délègue les entrées ngdevkit à `engine/system/runtime.c` en lui fournissant `DEMO_NEO_GEO_RUNTIME`.

## 3. Composition root : `UGameInstance`

`UGameInstance` dans `engine/game/game.h` regroupe les sous-systèmes génériques nécessaires à une partie :

- audio ;
- input ;
- timers ;
- runtime gameplay ;
- pools d'acteurs ;
- niveau ;
- level renderer ;
- level manager ;
- viewport.

`unsigned_game_instance_init()` suit essentiellement cet ordre :

1. valider la configuration et les capacités ;
2. initialiser le renderer ;
3. câbler les pools et le runtime gameplay ;
4. câbler le runtime du niveau et ses buffers de collision ;
5. initialiser l'input ;
6. initialiser l'audio et les timers ;
7. initialiser le viewport ;
8. démarrer le flux de niveau, soit par `ULevelGraph`, soit par un `initial_level` direct.

La mémoire à capacité variable est fournie par l'application via `UGameInstanceStorage`. Le moteur ne prend pas possession de ces tableaux et ne les libère pas.

En cas d'échec, `unsigned_game_instance_destroy()` remet l'instance dans un état vide tout en laissant les buffers à leur propriétaire.

## 4. Boucle d'une frame

### Couche Neo Geo

`engine/system/runtime.c` possède le cycle BIOS/USER et exécute une frame Neo Geo ainsi :

```text
SYSTEM_IO / callbacks BIOS pendant VBlank précédent
        |
        v
unsigned_neo_geo_input_poll()
        |
        v
callback application tick
        |
        v
callback prepare_render
(CPU/RAM uniquement : culling, layout, effets, RenderPlan)
        |
        v
ng_wait_vblank()
        |
        v
transport audio
        |
        v
callback application render
(commit VRAM post-VBlank)
        |
        v
callback render_phase
```

La boucle n'est donc pas un simple `while (1)` appartenant entièrement au jeu : le BIOS reste propriétaire d'une partie du cycle de vie.

### Couche application

Dans la démo, `src/game/demo_loop.c` orchestre :

```text
demo_loop_tick
  -> unsigned_game_instance_tick
  -> demo flow
  -> logique de scène/stage
  -> menu

demo_loop_prepare_render
  -> unsigned_game_instance_prepare_render

demo_loop_render
  -> unsigned_game_instance_render (commit)
  -> overlays de stage/scène
  -> menu
  -> HUD
```

### Couche moteur

`unsigned_game_instance_tick()` dans `engine/game/game.c` :

1. tick des timers ;
2. tick du level manager ;
3. si le niveau est actif : tick du niveau ;
4. tick du viewport ;
5. tick de l'audio.

Le rendu du niveau est désormais séparé en `unsigned_game_instance_prepare_render()` pendant l'affichage actif, puis un commit dans `unsigned_game_instance_render()` juste après le VBlank. Les anciens appelants peuvent encore appeler directement `unsigned_game_instance_render()` : il retombe alors sur prepare+commit.

## 5. Cycle de vie d'un niveau

`ULevelDefinition` (`engine/level/level_definition.h`) décrit le contenu stable. `ULevel` (`engine/level/level_runtime.h`) contient l'état mutable.

### Chargement

`unsigned_level_load()` :

1. décharge l'ancien niveau ;
2. conserve les buffers et dépendances runtime fournis au niveau ;
3. réinitialise TLSS, collisions et background ;
4. installe les couches de background déclarées ;
5. appelle `load` si présent ;
6. instancie les spawns déclarés ;
7. construit les collisions statiques ;
8. appelle `enter`.

Si une étape échoue après le début du `load`, le moteur appelle `unload` lorsque nécessaire, puis nettoie les pools, gameplay et collisions concernés.

### Déchargement

`unsigned_level_unload()` appelle :

1. `exit` ;
2. `unload` ;
3. le nettoyage de l'état mutable du niveau.

### Tick du niveau

L'ordre réel dans `engine/level/level.c` est :

1. début de frame TLSS ;
2. suppression des collisions dynamiques précédentes ;
3. tick acteurs + input player ;
4. classification et tick AI des NPC ;
5. tick des abilities actives ;
6. mise à jour de la caméra Beat'Em Up à partir des joueurs ;
7. enregistrement/détection des collisions ;
8. callback `resolve_hits` si des hits existent ;
9. tick des effects ;
10. tick des cues ;
11. mise à jour du background à partir de la position caméra.

Le moteur détecte les interactions ; la définition du niveau décide de leur signification métier via `resolve_hits`.

`resolve_hits` doit consommer les résultats déjà produits par le moteur via `unsigned_level_collision_hits()`. Le gameplay ne doit pas reconstruire les hitboxes/hurtboxes en world-space pour refaire une seconde détection : la géométrie appartient au pipeline collision, tandis que les dégâts, garde, knockdown ou règles de combo appartiennent au jeu.

## 6. Acteurs, personnages et pools

La hiérarchie est une **composition C explicite** :

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
                    state graph + activité + TLSS AI

UProjectile -> référence un UActor + trajectoire/lifetime
```

`UActorContainer` est une vue non propriétaire sur les acteurs actifs réunis depuis les pools spécialisés.

`UActor` ne possède volontairement aucune notion de zone jouable ou de bounds. Il stocke une position monde ; `UCharacter` fournit les opérations génériques de déplacement/orientation, mais la décision de contraindre ce déplacement reste extérieure à l'acteur. `UCharacter.height` est une élévation de présentation : `actor.position` reste sur le plan de sol pour le déplacement et l'ordre de profondeur, tandis que le sprite du corps se décale verticalement et que `UCharacterShadow` réduit l'ombre au sol. Le renderer ne connaît pas cette règle ; il continue de traiter l'ombre comme un simple `underlay`.

### Mouvement et contraintes spatiales

La séparation actuelle est :

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
       UMovementBounds choisi par le gameplay/niveau
```

- `unsigned_input_direction()` convertit le D-pad en axes `-1/0/1`; des directions opposées s'annulent ;
- `unsigned_character_set_facing()` et `unsigned_character_move()` gèrent le comportement générique d'un personnage ;
- `UMovementBounds` et `unsigned_physics_movement_constrain()` sont dans `engine/physics/movement.h` / `engine/physics/movement.c` ;
- le propriétaire du déplacement décide explicitement si ces bounds s'appliquent. Un projectile ou un NPC peut donc rester libre de sortir de la zone jouable.

Cette séparation évite de faire de `UActor` le propriétaire d'une politique de niveau.

Les pools utilisent des slots fixes. `UPoolInstance.generation` sert à détecter la réutilisation d'un même slot : une adresse identique ne signifie pas forcément qu'il s'agit encore de la même instance logique.

## 7. Gameplay : attributes, abilities, effects, cues et tags

Le runtime agrégé est `UGameplayRuntime` (`engine/gameplay/runtime.h`) :

```text
UGameplayRuntime
├── UAbilityPool
├── UEffectPool
└── UCuePool
```

Les principaux concepts sont :

- **Attribute** : valeur gameplay mutable, avec bornes/relations éventuelles et callback de mise à jour ;
- **Ability** : action active pouvant être déclenchée par le joueur ou le jeu ;
- **Effect** : modification instantanée, temporaire ou persistante ;
- **Cue** : événement gameplay temporisé ;
- **Tag** : identifiant à compteur de références utilisé pour accorder ou bloquer des états.

Les callbacks gameplay peuvent libérer ou réutiliser un slot pendant leur propre exécution. Les contrôles de génération et d'identité dans les pools évitent alors de poursuivre un travail sur une instance devenue obsolète.

### Input vers abilities

`UGameplayAbilityBinding` sépare le trigger (`DOWN`, `PRESSED`, `RELEASED`, `HOLD`) de la politique de matching :

- `U_INPUT_MATCH_ALL` : tous les boutons du masque doivent matcher ;
- `U_INPUT_MATCH_ANY` : au moins un bouton du masque suffit.

Le déplacement de la démo utilise ainsi un seul binding `ANY` couvrant le D-pad complet. Une diagonale n'instancie donc qu'une ability de déplacement au lieu d'une ability par direction.

`UPlayer.input_state` expose le snapshot courant aux abilities qui ont besoin d'un contrôle continu, par exemple le saut d'Arthur.

### Interruption/remplacement par owner

`UAbilityPool` fournit des opérations owner-level pour éviter qu'un personnage inspecte les tableaux internes du pool :

- `unsigned_gameplay_ability_pool_release_owner()` ;
- `unsigned_gameplay_ability_pool_replace_owner()`.

Le remplacement prévalide la capacité du pool et la capacité des tags pour l'état prospectif avant de retirer les abilities existantes. Les réactions comme Hurt/Knockdown/Caught peuvent donc interrompre un personnage sans coupler le code du personnage à la représentation interne du pool.

### Attributes

`unsigned_gameplay_attribute_set_current_value()` applique les bornes et ne notifie `on_change` que lorsque la valeur finale change réellement. `unsigned_gameplay_attribute_add_current_value()` ajoute un delta signé avec saturation `s16`, puis réutilise les mêmes règles de clamp/notification.

## 8. State graph : logique et temps séparés

`UStateGraph` (`engine/core/state/state_graph.h` / `engine/core/state/state_graph.c`) ne contient que l'état logique d'exécution : contexte, nœuds global/initial/courant, tasks et transitions. Il ne possède pas de compteur temporel.

Les nœuds peuvent déclarer `duration_frames`, mais le temps écoulé est possédé par le composant optionnel `UStateGraphClock` (`engine/core/state/state_graph_clock.h` / `engine/core/state/state_graph_clock.c`) :

```text
UStateGraphNode.duration_frames   définition stable
              |
              v
UStateGraph                    état logique courant
              ^
              |
UStateGraphClock               état temporel optionnel
  state
  elapsed_frames
```

`unsigned_state_graph_clock_tick()` exécute le graph, avance le compteur si le même état reste actif, puis signale l'expiration via `unsigned_state_graph_timeout()`. Une transition par task/event remet naturellement le clock à zéro pour le nouvel état.

Cette séparation a deux conséquences :

- un graph qui n'utilise pas de timeout n'a pas à stocker `elapsed_frames` ;
- le propriétaire du runtime choisit s'il a besoin d'un clock. `ULevelManager` en possède un en mode graph-driven ; le mode direct n'en dépend pas.

## 9. TLSS

TLSS est dans `engine/core/tlss/`.

Il fournit des cadences de simulation 1/2/4/8/16 frames et distribue les phases entre les entrées afin de ne pas réveiller toutes les tâches ralenties en même temps.

Le niveau contient une configuration séparée pour :

- l'AI ;
- la résolution de collision.

Les NPC peuvent être classés `ACTIVE`, `OFFSCREEN` ou `DORMANT`. `engine/level/level_ai.c` choisit leur activité selon le viewport, puis `engine/actor/npc_ai.c` applique la cadence TLSS correspondante.

La collision utilise également TLSS lors de la résolution des hits/projectiles. Une hitbox nouvellement active peut forcer une résolution immédiate afin de ne pas perdre sa première frame active.

## 10. Collision : physique et sens gameplay

### `engine/physics`

Cette couche gère les primitives physiques indépendantes du gameplay :

- géométrie et couches de boîtes ;
- `UMovementBounds` et contrainte explicite d'une `Vec2` ;
- trajectoires `x/depth/height` ;
- projection des trajectoires ;
- `unsigned_physics_trajectory_parabola_height()` pour la hauteur parabolique générique.

Elle ne connaît ni joueur, ni NPC, ni attaque. Une ability comme le saut d'Arthur choisit sa hauteur et ses règles de charge, puis réutilise la primitive de trajectoire du moteur.

### `engine/collision`

Cette couche ajoute le sens gameplay :

- transformation des hitboxes/hurtboxes d'acteurs ;
- index des acteurs ;
- détection des hits ;
- résolution des projectiles.

### `engine/level/level_collision.c`

Le niveau orchestre la frame :

1. conserve les collisions statiques construites au chargement ;
2. reconstruit les collisions dynamiques ;
3. resynchronise la vue globale des acteurs ;
4. trie les acteurs selon `actor_order` ;
5. construit `UCollisionActorIndex` ;
6. résout les projectiles ;
7. détecte les couples attaquant/cible.

Une registration incomplète invalide les résultats de collision de la frame. Ce comportement évite d'obtenir un résultat dépendant de l'ordre d'insertion lorsque les buffers fixes sont saturés.

Les couples détectés sont exposés comme `UCollisionHit` frame-local. `unsigned_level_collision_hits()` permet au callback `resolve_hits` de les parcourir. Le code métier peut ensuite appliquer dégâts/garde/réactions ou dédupliquer une activation, mais ne doit pas refaire l'intersection géométrique déjà décidée par l'engine.

## 11. Display, renderer et system

La nouvelle structure sépare explicitement trois responsabilités.

### `engine/display`

Contient les structures et comportements de présentation indépendants du backend :

- caméra ;
- effets de présentation génériques (`UEffect`) ;
- sprite et état de rendu ;
- texte ;
- UI ;
- viewport.


`UCamera` ne connaît ni les joueurs ni le niveau : elle ne gère que sa position world-space, sa position précédente et ses bounds. `UViewport` reste la frontière de conversion world/screen et centralise les tests d'intersection visibles. La politique Beat'Em Up est dans `engine/level/level_camera.c` : elle agrège les joueurs actifs, applique la dead zone, le backtracking optionnel, les limites du niveau et les contraintes multijoueur.

Les `UCameraBounds` peuvent être remplacés à runtime avec `unsigned_level_camera_set_bounds()` puis restaurés avec `unsigned_level_camera_reset_bounds()`, ce qui fournit la primitive nécessaire aux verrous d'arène sans ajouter de logique d'événement à la caméra.

La UI générique est dans `engine/display/ui/`. Les widgets réutilisables sont dans `engine/display/ui/widget/`, par exemple `progress_bar.c`.

Les effets visuels sont centralisés dans `engine/display/effect/` : `effect.[ch]` définit le mécanisme de sampling/composition et `effects.[ch]` regroupe les presets fournis. Le même `UEffect` peut être porté par `UViewport.effect` ou `USprite.effect`. Un objet ne possède qu'un slot d'effet local ; le renderer compose ensuite l'effet du viewport avec celui du sprite. Les presets couvrent transformation statique, zoom, pseudo-rotation/turn, shake, bob, blink, shear et wave. Leur configuration est non possédée et doit vivre aussi longtemps que le binding. Le code de gameplay configure un effet ; il ne manipule ni SCB ni unités de shrink Neo Geo.

### `engine/renderer`

Contient la politique de rendu :

- `actor_renderer` ;
- `sprite_renderer` ;
- `background_renderer` ;
- `level_renderer` ;
- `palette_renderer` ;
- `ui_renderer`.

Le renderer détermine visibilité, allocations de sprites, dirty flags, ring/cache de backgrounds et ordre d'émission des commandes.

`engine/renderer/ui_renderer.c` adapte actuellement le renderer UI générique au FIX Neo Geo, notamment pour labels, selectors et progress bars.

### `engine/system`

Contient les détails plateforme :

- runtime BIOS ;
- callbacks BIOS ;
- crédits/session/settings ;
- input Neo Geo ;
- FIX ;
- vidéo ;
- audio backend ;
- sprite/background/palette backends ;
- writer VRAM ;
- save backend.

`engine/system/renderer_backend.h` est la frontière appelée par les renderers pour demander des opérations matérielles sans exposer leurs détails d'encodage dans la logique de niveau.

## 12. Backgrounds et sprites

`engine/level/background/background.c` dérive désormais son parallax de la position X absolue de la caméra ; il ne suit plus directement un joueur ou un autre acteur gameplay. `engine/renderer/background_renderer.c` traite ensuite ces backgrounds comme des bandes de sprites matériels réutilisables. Le cache évite de réécrire l'ensemble de l'écran lors d'un simple scroll.

`engine/renderer/sprite_renderer.c` utilise l'état dirty du sprite pour ne pousser que les parties modifiées vers le backend : graphisme, position, scale/shrink, flips et layout. Il compose l'effet du viewport avec l'effet local du sprite, applique les pivots et choisit entre la chaîne matérielle compacte et un rendu par colonne lorsque l'effet le demande.

`UCharacterShadow` reste un composant de personnage, mais sa responsabilité est désormais limitée à convertir `UCharacter.height` en une échelle générique. Il possède un `UTransformEffect` branché sur `shadow.sprite.effect`; le calcul de rendu et l'encodage Neo Geo restent hors de `character_shadow.c`.

Les transformations natives exposées aux sprites sont la translation, le scale X/Y jusqu'à 100 %, les pivots, les flips X/Y, la visibilité et les déformations par colonne. Le preset `turn` produit une illusion de rotation 3D avec squash + flip. La Neo Geo ne sait pas agrandir un sprite au-delà de sa taille source ni effectuer une rotation arbitraire : un vrai zoom >100 % ou une rotation libre nécessite des frames/tiles pré-rendues.

Le détail matériel du shrink vertical reste dans `engine/system/sprite_backend.c`. Sur Neo Geo, SCB3 conserve une fenêtre d'affichage fixe pendant que le lookup de zoom vertical peut lire des lignes SCB1 situées sous la hauteur logique du sprite. Un `USpriteDefinition` peut donc déclarer `clear_unused_rows` avec un `transparent_tile` ; le backend initialise alors les lignes SCB1 inutilisées avec cette tile transparente uniquement lors d'une reconstruction du layout. Cette protection évite que d'anciennes données VRAM réapparaissent pendant un shrink sans ajouter de travail à chaque frame d'animation.

`engine/renderer/level_renderer.c` utilise maintenant un `URenderPlan` en deux phases. La préparation n'écrit jamais dans la VRAM : elle calcule la visibilité, les ranges acteurs, les relocations, les transforms/effets uniformes et les uploads du ring-buffer background. Le commit post-VBlank exécute ensuite le plan par priorité :

```text
AFFICHAGE ACTIF
    unsigned_level_renderer_prepare()
        -> culling/layout/détection des relocations acteurs
        -> préparation transforms/effets des sprites
        -> planification uploads/transforms background
        -> estimation des words VRAM / compteurs de relocation

VBLANK
    unsigned_renderer_backend_begin()
        CRITICAL -> clear des anciens ranges + coupure des chain boundaries
        HIGH     -> commit acteurs SCB1/SCB2/SCB3/SCB4
        NORMAL   -> uploads colonnes background + transforms de scrolling
        DEFERRED -> réservé aux travaux futurs réellement différables
    unsigned_renderer_backend_end()
```

`URenderFrameStats` expose l'estimation des words VRAM par priorité, le nombre de relocations acteurs, le nombre de colonnes sprites acteurs et les uploads de colonnes background. Ces statistiques sont accessibles avec `unsigned_game_instance_render_stats()`.

## 13. UI et HUD

L'UI suit elle aussi la séparation logique/backend :

```text
engine/display/ui/*
    éléments, layouts, screens, pages, widgets
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

Exemple actuel : `src/hud/demo_hud.c` construit un `UUIScreen`, une `UUIProgressBar`, un `UUIRenderer` et un `UNeoGeoUIRenderer`, puis rend l'ensemble sur le FIX layer.

La progress bar peut être liée à un `UGameplayAttribute`. Le widget conserve une progression normalisée 0..256 afin que le renderer ne dépende pas directement du gameplay. Le HUD observe Health et ses bornes pendant qu'il est visible, compare les valeurs à son cache puis synchronise le widget uniquement lorsqu'une valeur affichée change. Il ne prend pas possession de `UGameplayAttribute.on_change`, ce qui évite qu'une UI remplace un callback métier ou conserve un abonnement à détacher après la destruction du joueur.

Pour les menus, `unsigned_ui_input_from_controller()` transforme un `UInputState` en commandes UI standard (navigation, valeur, confirm, cancel). Le mapping de contrôleur est ainsi centralisé dans `engine/display/ui/input.h` / `engine/display/ui/input.c`, tandis que la signification d'un écran concret reste dans `src/menu/`.

## 14. Audio

`engine/audio` manipule les concepts logiques : catalogue, événements, musique, cooldowns et commandes.

`engine/system/audio_backend.c` réalise le transport Neo Geo vers le driver son. Les détails BIOS et les commandes réservées restent dans la couche système.

Le driver projet est actuellement dans `src/audio/sound_driver.s`.

## 15. Sauvegarde

`engine/save` sépare :

- `storage` : accès à un record physique versionné ;
- `block` : bloc applicatif dans un slot ;
- `set` : donnée répartie sur plusieurs slots consécutifs.

Le backend plateforme est dans `engine/system/save_backend.c`.

Les sets utilisent une génération cohérente entre blocs afin qu'une écriture interrompue ne soit pas prise pour une sauvegarde complète valide.

## 16. Runtime Neo Geo et BIOS

Le runtime est maintenant directement sous `engine/system/`.

Les principales phases applicatives sont :

- `ATTRACT` ;
- `TITLE` ;
- `GAME` ;
- `GAME_OVER`.

Elles sont construites au-dessus des USER requests BIOS. `PLAYER_START` reste autoritaire pour l'acceptation d'un joueur et la consommation des crédits.

Le détail du cycle USER 1/2/3, du GAME START COMPULSION, du transport audio et des crédits est documenté dans `docs/Bios_fr.md`.

### Flow de la démo

`src/game/demo_flow.c` reste une règle applicative. Il décrit les scènes via une table de règles (timeout/event/failure owner) au lieu de disperser ces transitions dans de gros `switch`. Il utilise `UTimerPool` pour les durées de présentation et pour exposer le compte à rebours au HUD.

Ce flow ne doit pas être confondu avec `UStateGraphClock` : le premier orchestre le scénario concret de la démo ; le second est un composant générique optionnel pour chronométrer un `UStateGraph`.

## 17. Frontière entre moteur et jeu

Une règle pratique permet de choisir où ajouter du code :

- comportement réutilisable sur plusieurs jeux : `engine/` ;
- contenu ou règle propre à Unsigned/démo : `src/` ;
- représentation logique d'un affichage : `engine/display/` ;
- politique de rendu : `engine/renderer/` ;
- accès BIOS/matériel : `engine/system/` ;
- scène ou niveau concret : `src/levels/` ou `src/game/` ;
- personnage concret : `src/characters/` ;
- interface concrète : `src/menu/` ou `src/hud/`.

Cette séparation est la base à préserver lors des prochaines évolutions de l'architecture.
