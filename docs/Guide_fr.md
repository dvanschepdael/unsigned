# Débuter dans Unsigned

Ce guide s'adresse à un développeur qui connaît les bases du C et découvre le moteur et/ou la Neo Geo. La lecture s'organise autour de quatre questions structurantes : **qui possède les données**, **qui décide du comportement**, **qui décide du rendu** et **qui écrit réellement sur le matériel**. Ces questions permettent de retrouver rapidement la responsabilité d'un sous-système sans mémoriser chaque fichier.

## 1. Les cinq repères à connaître

### `UGameInstance` : la racine du moteur

`UGameInstance` (`engine/game/game.h`) regroupe les sous-systèmes génériques d'une partie : input, timers, gameplay, pools d'acteurs, niveau, level renderer, level manager, viewport et audio.

`unsigned_game_instance_init()` reçoit des buffers déjà alloués via `UGameInstanceStorage`. Les capacités se dimensionnent au démarrage, ce qui maintient une utilisation mémoire déterministe pendant la partie.

### `ULevelDefinition` : le contenu déclaré

`ULevelDefinition` (`engine/level/level_definition.h`) contient les éléments stables :

- backgrounds ;
- spawns NPC/objets ;
- callbacks `load`, `enter`, `exit`, `unload` ;
- ordre des acteurs ;
- résolution métier des hits ;
- couleur de backdrop.

La définition reste valide pendant toute la durée d'utilisation du niveau.

### `ULevel` : l'état mutable

`ULevel` (`engine/level/level_runtime.h`) contient ce qui change pendant le jeu :

- acteurs actifs ;
- pools associés ;
- TLSS ;
- collisions ;
- background ;
- caméra ;
- `UGameplayRuntime` ;
- définition active et contexte applicatif.

La séparation `ULevelDefinition` / `ULevel` distingue clairement les données de contenu de l'état d'exécution.

### `UActor` / `UCharacter` : monde et combat

`UActor` contient la présence dans le monde : position, sprite et collision.

`UCharacter` ajoute :

- attributes ;
- abilities ;
- tags ;
- orientation.

`UPlayer` et `UNpc` enveloppent ensuite un `UCharacter` avec leur logique de contrôle respective.

### `engine/system` : la frontière Neo Geo

Les détails de BIOS, VRAM, FIX, vidéo, input plateforme, transport audio et stockage Neo Geo sont regroupés dans `engine/system/`.

Le code de niveau et de personnage utilise les APIs de cette couche pour toute opération liée aux registres matériels, au BIOS ou à la VRAM.

## 2. Lire rapidement l'arborescence

Commencer par cette carte :

```text
engine/
  core/       briques génériques
  actor/      entités et pools
  gameplay/   attributes/tags/abilities/effects/cues
  level/      cycle de vie et orchestration d'un niveau
  physics/    collision géométrique, contraintes de mouvement, trajectoires
  collision/  collision gameplay
  display/    données de présentation, UI, viewport
  renderer/   politique/cache de rendu
  system/     Neo Geo/BIOS/backends matériels
  audio/      logique audio
  save/       sauvegarde générique
  input/      input générique
  game/       UGameInstance

src/
  game/       composition et scénario de la démo
  characters/ personnages concrets
  levels/     niveaux concrets
  menu/       menus
  hud/        HUD
  localizations/
  audio/
```

Règle simple : `engine/` contient les mécanismes réutilisables ; `src/` décrit les besoins précis du jeu et compose ces mécanismes.

## 3. Du BIOS à une frame de jeu

Le chemin principal est :

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
        |      +--> demo scenario / stage / menu
        |
        +--> demo_loop_render()
        |      +--> unsigned_game_instance_render()
        |      +--> overlays / menu / HUD
        |
        +--> ng_wait_vblank()
        +--> transport audio
```

Le BIOS reste autoritaire sur certaines transitions système. La boucle de jeu se lit donc comme une coopération entre la boucle applicative et le cycle de vie BIOS, plutôt que comme une boucle principale entièrement autonome.

## 4. Lire une frame de niveau

Dans `engine/level/level.c`, la simulation suit cet ordre :

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

Cet ordre est un contrat comportemental. Modifier la position d'une étape peut modifier le gameplay, même lorsque le code compile toujours.

Exemple : `resolve_hits` intervient après les abilities mais avant les effects/cues de fin de frame.

## 5. Comprendre la mémoire fixe

Le moteur utilise des pools à capacité fixe. Un `UPoolInstance` possède notamment une `generation`.

Pourquoi ? Un callback peut :

1. libérer un slot ;
2. le réutiliser immédiatement ;
3. laisser le même pointeur physique visible au code appelant.

La génération permet de distinguer l'ancienne instance de la nouvelle.

### Règle pratique

Pour conserver une référence vers un slot, s'appuyer sur le contrat du sous-système et, lorsqu'il existe, sur le mécanisme `generation`. La génération identifie l'instance logique même lorsqu'une adresse mémoire est réutilisée.

## 6. Où ajouter du code ?

### Une nouvelle mécanique générique de personnage

Regarder d'abord :

- `engine/actor/character.h` ;
- `engine/gameplay/` ;
- éventuellement `engine/physics/` ou `engine/collision/`.

Une mécanique propre à un seul personnage du jeu trouve naturellement sa place sous `src/characters/`.

### Une nouvelle action du joueur

1. définir ou réutiliser un `UGameplayAbility` ;
2. définir ses tags et conditions ;
3. créer un `UGameplayAbilityBinding` ;
4. choisir le `UInputTrigger` ;
5. choisir `U_INPUT_MATCH_ALL` ou `U_INPUT_MATCH_ANY` pour le masque de boutons ;
6. prévoir la fin ou l'annulation de l'ability ;
7. tester l'échec d'activation lorsque le pool est plein.

Pour une direction continue, un seul binding `ANY` peut couvrir le D-pad ; l'ability lit ensuite `UPlayer.input_state` / `unsigned_input_direction()`. Cette organisation conserve une seule ability pour une action logique unique, y compris en diagonale.

Fichiers utiles :

- `engine/actor/player.c` ;
- `engine/input/input.c` ;
- `engine/gameplay/ability.c` ;
- `engine/gameplay/ability_pool.c` ;
- `engine/gameplay/gameplay_pool.c` ;
- exemples dans `src/characters/player/demo/` et `src/characters/player/arthur/`.

Pour une réaction qui remplace toutes les actions courantes d'un personnage, l'API owner-level du pool (`release_owner` / `replace_owner`) centralise le remplacement et encapsule les tableaux internes de `UAbilityPool`.

### Déplacer un personnage et limiter sa zone

Séparer les trois responsabilités :

```text
input direction  ->  character movement  ->  optional world constraint
engine/input         engine/actor            engine/physics
```

- `unsigned_input_direction()` produit un `Vec2` directionnel ;
- `unsigned_character_move()` modifie la position ;
- `unsigned_character_set_facing()` gère l'orientation ;
- `unsigned_physics_movement_constrain()` applique éventuellement un `UMovementBounds`.

`UActor` stocke la position monde. Le niveau ou le gameplay choisit ensuite les `UMovementBounds` et applique la contrainte lorsque la zone jouable le demande.

### Un nouvel attribute

Les attributes sont définis dans `engine/gameplay/attribute.h`.

Pour un attribute propre au personnage de démonstration, regarder `src/characters/player/demo/player_health.c`.

Un callback `on_change` peut servir à synchroniser une UI, comme le fait le HUD pour la vie du joueur. Pour appliquer un delta, `unsigned_gameplay_attribute_add_current_value()` fournit directement les règles de saturation, clamp et notification communes.

### Un nouveau NPC

1. définir son contenu/spawn ;
2. fournir son `UCharacter` ;
3. configurer son state graph ;
4. appeler `unsigned_npc_init()` ;
5. laisser `level_ai.c` et TLSS gérer sa cadence selon son activité.

`UStateGraph` porte l'état logique du graph. Lorsqu'un propriétaire utilise des transitions `U_TRANSITION_ON_TIMEOUT`, il possède explicitement un `UStateGraphClock` et appelle `unsigned_state_graph_clock_tick()`. Le compteur temporel reste ainsi dans le composant temporel dédié.

Fichiers utiles :

- `engine/actor/npc.h` ;
- `engine/actor/npc_ai.c` ;
- `engine/level/level_ai.c` ;
- `engine/core/tlss/tlss.h`.

### Un nouveau niveau

Le contenu concret va dans `src/levels/<nom_du_niveau>/`.

1. déclarer un `ULevelDefinition` ;
2. définir les backgrounds et spawns avec une durée de vie suffisante ;
3. utiliser `load` pour le setup impératif complémentaire aux données ;
4. utiliser `enter` / `exit` pour le changement d'état autour du niveau actif ;
5. utiliser `unload` pour annuler le travail de `load` ;
6. exposer la définition à `src/game/demo_scenes.c` ou au scénario concerné.

Le chargement est transactionnel : une erreur déclenche le rollback du contenu déjà installé.

## 7. Display, renderer et system : trois responsabilités complémentaires

Cette séparation est essentielle dans la structure actuelle. `engine/renderer` désigne le sous-système qui porte la politique de rendu ; les types comme `UUIRenderer` et `UNeoGeoUIRenderer` sont les objets et adaptateurs utilisés dans cette chaîne de rendu.

### `engine/display`

Décrit des concepts indépendants du backend :

- sprites ;
- texte ;
- viewport ;
- éléments UI ;
- widgets ;
- layout/screen/page.

### `engine/renderer`

Décide comment rendre ces concepts :

- visibilité ;
- ordre ;
- allocation de sprites ;
- dirty state ;
- cache de background ;
- adaptation UI Neo Geo.

### `engine/system`

Effectue les opérations réellement dépendantes de la Neo Geo :

- VRAM ;
- FIX ;
- palette ;
- vidéo ;
- BIOS ;
- input plateforme ;
- transport audio ;
- sauvegarde backend.

### Exemple

Pour afficher un acteur :

```text
UActor / USprite
    -> engine/renderer/actor_renderer.c
    -> engine/renderer/sprite_renderer.c
    -> engine/system/renderer_backend.h
    -> engine/system/sprite_backend.c
    -> VRAM / SCB
```

Pour afficher un widget :

```text
UUIProgressBar
    -> UUIRenderer
    -> engine/renderer/ui_renderer.c
    -> engine/system/fix.c
    -> FIX layer
```

## 8. Ajouter ou modifier une UI

L'UI générique est sous `engine/display/ui/`.

Les widgets sont sous `engine/display/ui/widget/` :

- button ;
- image ;
- label ;
- panel ;
- progress bar ;
- selector ;
- blink label.

Pour une interface de jeu concrète, utiliser `src/menu/` ou `src/hud/`.

### Exemple : HealthBar actuelle

`src/hud/demo_hud.c` :

1. crée une `UUIProgressBar` ;
2. la lie à un `UGameplayAttribute` de vie ;
3. branche `on_change` sur l'attribute et ses bornes ;
4. appelle `unsigned_ui_progress_bar_sync()` quand la valeur change ;
5. rend le `UUIScreen` via `UUIRenderer` + `UNeoGeoUIRenderer`.

Le widget stocke une progression normalisée 0..256. Le renderer réutilise directement cette valeur normalisée à chaque frame. Le HUD actuel possède volontairement les slots `on_change` pendant le binding ; cette politique reste dans `src/hud/`.

Pour les menus, `unsigned_ui_input_from_controller()` convertit le snapshot contrôleur en `UUIInput` standard et centralise le mapping direction/A/B pour l'ensemble des écrans.

## 9. Collisions : deux couches complémentaires

`engine/physics` manipule les boîtes, les couches de collision et les primitives géométriques. La notion d'attaque apparaît dans `engine/collision`, qui ajoute :

- hitbox/hurtbox ;
- acteurs ;
- projectile ;
- hit detection.

`engine/level/level_collision.c` orchestre ensuite l'ensemble pour le niveau actif.

Les collisions statiques sont construites au chargement. Les collisions dynamiques sont effacées puis enregistrées à nouveau chaque frame.

Le callback `ULevelDefinition.resolve_hits` parcourt les couples déjà détectés via `unsigned_level_collision_hits()`, puis applique les règles de jeu : dégâts, garde, réactions et déduplication. L'intersection géométrique reste la responsabilité du pipeline collision.

Si les buffers de registration sont saturés, la frame de collision est invalidée plutôt que partiellement calculée.

## 10. State graph : séparer logique et temps

`UStateGraph` possède l'état logique courant. `UStateGraphNode.duration_frames` est une donnée de définition ; `UStateGraphClock` porte l'état temporel optionnel.

Le pattern est :

```text
unsigned_state_graph_init(...)
unsigned_state_graph_clock_reset(&clock, &graph)

chaque frame nécessitant les timeouts :
    unsigned_state_graph_clock_tick(&clock, &graph)
```

Sans timeout, `unsigned_state_graph_tick()` suffit et le propriétaire peut fonctionner sans clock. `ULevelManager` illustre les deux modes : graph-driven avec clock, ou contrôle direct sans graph.

## 11. TLSS : simulation temporellement réduite

TLSS est dans `engine/core/tlss/`.

Il peut étaler le travail sur 1/2/4/8/16 frames. Deux usages sont actuellement configurables dans `UGameInstanceConfig` :

- AI ;
- collision.

Les NPC hors écran ou dormants peuvent donc coûter moins cher tout en conservant le framerate global.

La collision dispose d'un mécanisme de résolution immédiate pour une hitbox qui vient de devenir active. Ce mécanisme préserve son premier instant d'attaque même avec une cadence TLSS réduite.

## 12. Neo Geo : responsabilités du BIOS et du runtime système

Sur MVS/AES, le BIOS possède une partie du cycle de vie.

Le runtime système gère notamment :

- USER requests ;
- phases ATTRACT/TITLE/GAME/GAME_OVER ;
- `PLAYER_START` ;
- session joueurs ;
- crédits ;
- GAME START COMPULSION ;
- handoff audio associé.

Pour modifier ce domaine, commencer par `engine/system/BIOS_WORKFLOW.md`, puis consulter les fichiers publics concernés dans `engine/system/`.

Une pression sur START issue de l'input générique devient une demande applicative ; l'acceptation effective d'un joueur MVS reste pilotée par le `PLAYER_START` du BIOS, qui constitue la source autoritaire.

## 13. Lire le projet de démonstration

Pour comprendre comment toutes les couches sont assemblées, suivre cet ordre :

1. `main.c` ;
2. `src/game/demo_game.c` ;
3. `src/game/demo_loop.c` ;
4. `src/game/demo_flow.c` ;
5. `src/game/demo_scenes.c` ;
6. un niveau sous `src/levels/` ;
7. le joueur sous `src/characters/player/demo/` ;
8. `src/hud/demo_hud.c` et `src/menu/main/demo_menu.c`.

Cette lecture montre la frontière entre le moteur réutilisable et le contenu du jeu plus clairement qu'une lecture fichier par fichier de tout `engine/`.

Dans `demo_flow.c`, les transitions de scène sont décrites par une table de règles. Le `UTimerPool` reste responsable des durées/countdowns de présentation. `demo_flow.c` orchestre le scénario concret de la démo, tandis que `UStateGraphClock` chronomètre un `UStateGraph` générique : les deux composants répondent à des responsabilités distinctes.

## 14. Méthode de modification recommandée

Pour une contribution :

1. trouver l'API publique `.h` du sous-système ;
2. lire l'implémentation correspondante ;
3. lire au moins un test dans `test/` ;
4. identifier les capacités fixes et la durée de vie des pointeurs ;
5. faire le changement minimal ;
6. ajouter ou adapter un test hôte ;
7. vérifier ensuite MAME/hardware si le changement dépend de la Neo Geo.

Quand un commentaire est nécessaire, documenter l'invariant, la durée de vie, la raison de performance ou la contrainte BIOS. Privilégier l'information qui complète le code : intention, contrat ou justification technique.