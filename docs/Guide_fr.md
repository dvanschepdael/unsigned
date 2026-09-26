# Débuter dans Unsigned

Ce guide vise un développeur qui connaît les bases du C mais découvre le moteur et/ou la Neo Geo. L'objectif n'est pas de mémoriser chaque fichier : il faut surtout comprendre **qui possède les données**, **qui décide du comportement**, **qui décide du rendu** et **qui écrit réellement sur le matériel**.

## 1. Les cinq repères à connaître

### `UGameInstance` : la racine du moteur

`UGameInstance` (`engine/game/game.h`) regroupe les sous-systèmes génériques d'une partie : input, timers, gameplay, pools d'acteurs, niveau, renderer, level manager, viewport et audio.

`unsigned_game_instance_init()` reçoit des buffers déjà alloués via `UGameInstanceStorage`. Les relations capacités/stockages sont des contrats de construction authored : dimensionnez-les avec les macros de composition du projet plutôt que d'ajouter une allocation dynamique ou une récupération défensive dans les hot paths.

### `ULevelDefinition` : le contenu déclaré

`ULevelDefinition` (`engine/level/level_definition.h`) contient les éléments stables :

- backgrounds ;
- politique caméra Beat'Em Up ;
- spawns NPC/objets ;
- callbacks `load`, `enter`, `exit`, `unload` ;
- ordre des acteurs ;
- résolution métier des hits ;
- couleur de backdrop.

La définition doit rester valide pendant l'utilisation du niveau.

### `ULevel` : l'état mutable

`ULevel` (`engine/level/level_runtime.h`) contient ce qui change pendant le jeu :

- acteurs actifs ;
- pools associés ;
- TLSS ;
- collisions ;
- background ;
- caméra ;
- gameplay runtime ;
- définition active et contexte applicatif.

La séparation `ULevelDefinition` / `ULevel` évite de mélanger données de contenu et runtime.

### `UActor` / `UCharacter` : monde et combat

`UActor` contient la présence dans le monde : position, sprite et collision.

`UCharacter` ajoute :

- attributes ;
- abilities ;
- tags ;
- déplacement générique sur le plan de sol et orientation ;
- hauteur de présentation au-dessus du plan de sol ;
- ombre au sol optionnelle, dont la réduction dépend de cette hauteur.

`UPlayer` et `UNpc` enveloppent ensuite un `UCharacter` avec leur logique de contrôle respective.

### `engine/system` : la frontière Neo Geo

Les détails de BIOS, VRAM, FIX, vidéo, input plateforme, transport audio et stockage Neo Geo sont regroupés dans `engine/system/`.

Le code de niveau ou de personnage ne doit pas contourner cette couche pour écrire directement dans les registres matériels.

## 2. Lire l'arborescence sans se perdre

Commencer par cette carte :

```text
engine/
  core/       briques génériques
  actor/      entités et pools
  gameplay/   attributes/tags/abilities/effects/cues
  level/      cycle de vie et orchestration d'un niveau
  physics/    collision géométrique, contraintes de mouvement, trajectoires
  collision/  collision gameplay
  display/    données de présentation, effets, sprites, UI, viewport
  renderer/   politique/cache de rendu
  system/     Neo Geo/BIOS/backends matériels
  audio/      logique audio
  save/       sauvegarde générique
  input/      input générique
  game/       UGameInstance

src/
  game/       composition et flow de la démo
  characters/ personnages concrets
  levels/     niveaux concrets
  menu/       menus
  hud/        HUD
  localizations/
  audio/
```

Règle simple : `engine/` doit rester réutilisable ; `src/` peut connaître les besoins précis du jeu.

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
        |      +--> demo flow / stage / menu
        |
        +--> demo_loop_prepare_render()
        |      +--> unsigned_game_instance_prepare_render()
        |
        +--> ng_wait_vblank()
        +--> transport audio
        |
        +--> demo_loop_commit_render()
               +--> unsigned_game_instance_commit_render()
               +--> overlays / menu / HUD
```

Le BIOS reste autoritaire sur certaines transitions système. Il ne faut donc pas raisonner comme sur une boucle PC totalement autonome.

## 4. Lire une frame de niveau

Dans `engine/level/level.c`, la simulation suit cet ordre :

```text
TLSS begin frame
    |
actors + player input
    |
NPC activity + AI
    |
abilities
    |
camera Beat'Em Up
    |
ordre partagé des acteurs
    |
probe collision / matérialisation conditionnelle + détection hits/projectiles
    |
level resolve_hits
    |
effects
    |
cues
    |
background <- camera.x
```

Cet ordre est un contrat comportemental. Déplacer une étape peut changer le gameplay, même si le code compile encore.

Exemple : `resolve_hits` intervient après les abilities mais avant les effects/cues de fin de frame.

## 5. Comprendre la mémoire fixe

Le moteur utilise des pools à capacité fixe. Un `UPoolInstance` possède notamment une `generation`.

Pourquoi ? Un callback peut :

1. libérer un slot ;
2. le réutiliser immédiatement ;
3. laisser le même pointeur physique visible au code appelant.

La génération permet de distinguer l'ancienne instance de la nouvelle.

### Règle pratique

Ne conservez pas un pointeur vers un slot en supposant qu'il restera le même objet logique. Vérifiez le contrat du sous-système et, lorsqu'il existe, le mécanisme `generation`.

## 6. Où ajouter du code ?

### Une nouvelle mécanique générique de personnage

Regarder d'abord :

- `engine/actor/character.h` ;
- `engine/gameplay/` ;
- éventuellement `engine/physics/` ou `engine/collision/`.

Si la mécanique ne sert qu'à un personnage du jeu, la mettre plutôt sous `src/characters/`.

### Une nouvelle action du joueur

1. définir ou réutiliser un `UGameplayAbility` ;
2. définir ses tags et conditions ;
3. créer un `UGameplayAbilityBinding` ;
4. choisir le `UInputTrigger` ;
5. choisir `U_INPUT_MATCH_ALL` ou `U_INPUT_MATCH_ANY` pour le masque de boutons ;
6. prévoir la fin ou l'annulation de l'ability ;
7. tester l'échec d'activation lorsque le pool est plein.

Pour une direction continue, préférer un seul binding `ANY` couvrant le D-pad, puis lire `UPlayer.input_state` / `unsigned_input_direction()`. Ne pas réserver une ability distincte pour chaque direction lorsque l'action logique est unique.

Fichiers utiles :

- `engine/actor/player.c` ;
- `engine/input/input.c` ;
- `engine/gameplay/ability.c` ;
- `engine/gameplay/ability_pool.c` ;
- `engine/gameplay/gameplay_pool.c` ;
- exemples dans `src/characters/player/demo/` et `src/characters/player/arthur/`.

Pour une réaction qui remplace toutes les actions courantes d'un personnage, utiliser l'API owner-level du pool (`release_owner` / `replace_owner`) au lieu d'inspecter directement les tableaux internes de `UAbilityPool`.

### Déplacer un personnage et limiter sa zone

Séparer les trois responsabilités :

```text
input direction  ->  character movement  ->  optional world constraint
engine/input         engine/actor            engine/physics
```

- `unsigned_input_direction()` produit un `Vec2` directionnel ;
- `unsigned_character_move(character, direction, horizontal_speed, vertical_speed)` déplace chaque axe selon le signe de `direction`, sature les coordonnées en `s16` et met à jour le facing horizontal ;
- `unsigned_character_set_facing()` sert lorsqu'il faut changer l'orientation sans déplacement ;
- `unsigned_physics_movement_constrain()` applique éventuellement un `UMovementBounds` après le mouvement.

La démo suit ce pipeline dans `src/characters/player/demo/player_move.c` et dans l'air steering d'Arthur (`src/characters/player/arthur/arthur_jump.c`) : les attributes de vitesse alimentent `unsigned_character_move()`, puis les bounds possédés par le niveau sont appliqués séparément. L'ancien helper de mouvement spécifique à la démo ne fait plus partie de l'architecture.

Ne pas ajouter de bounds dans `UActor` : l'acteur stocke une position, tandis que le niveau/gameplay décide si cette position doit être contrainte.

### Un nouvel attribute

Les attributes sont définis dans `engine/gameplay/attribute.h`.

Pour un attribute propre au personnage de démonstration, regarder `src/characters/player/demo/player_health.c`.

Un callback `on_change` peut servir à synchroniser une UI, comme le fait le HUD pour la vie du joueur. Pour appliquer un delta, préférer `unsigned_gameplay_attribute_add_current_value()` plutôt qu'un clamp recodé localement.

### Un nouveau NPC

1. définir son contenu/spawn ;
2. fournir son `UCharacter` ;
3. configurer son state graph ;
4. appeler `unsigned_npc_init()` ;
5. laisser `engine/level/level_ai.c` et TLSS gérer sa cadence selon son activité.

`UStateGraph` ne contient pas de temps écoulé. Si un propriétaire a besoin de transitions `U_TRANSITION_ON_TIMEOUT`, il doit posséder explicitement un `UStateGraphClock` et utiliser `unsigned_state_graph_clock_tick()`. Ne pas réintroduire un compteur temporel dans la structure de graph générique.

Fichiers utiles :

- `engine/actor/npc.h` ;
- `engine/actor/npc_ai.c` ;
- `engine/level/level_ai.c` ;
- `engine/core/tlss/tlss.h`.

### Un nouveau niveau

Le contenu concret va dans `src/levels/<nom_du_niveau>/`.

1. déclarer un `ULevelDefinition` ;
2. définir les backgrounds et spawns avec une durée de vie suffisante ;
3. utiliser `load` pour le setup qui ne peut pas être exprimé par les données ;
4. utiliser `enter` / `exit` pour le changement d'état autour du niveau actif ;
5. utiliser `unload` pour annuler le travail de `load` ;
6. exposer la définition à `src/game/demo_scenes.c` ou au flow concerné.

Le chargement est une opération directe de remplacement sur du contenu authored : l'ancien niveau est d'abord déchargé, puis backgrounds/hooks/spawns/collisions statiques sont installés dans un ordre fixe. Les capacités et la validité du contenu sont des contrats de construction, pas des cas de rollback runtime.

## 7. Display, renderer et system : ne pas les confondre

Cette séparation est essentielle dans la structure actuelle.

### `engine/display`

Décrit des concepts indépendants du backend :

- effets génériques (`UEffect`) ;
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
    -> engine/system/sprite_backend.h
    -> engine/system/sprite_backend.c
    -> VRAM / SCB

transaction de frame
    -> engine/system/renderer_backend.h
       (`unsigned_renderer_backend_begin/end`)
```

Pour afficher un widget :

```text
UUIProgressBar
    -> UUIRenderer
    -> engine/renderer/ui_renderer.c
    -> engine/system/fix.c
    -> FIX layer
```

### Appliquer un effet à n'importe quel `USprite`

Les effets de présentation sont dans `engine/display/effect/` et utilisent la même abstraction `UEffect` que le viewport. Le mécanisme générique se trouve dans `engine/display/effect/effect.h`. Avec `engine/` dans le chemin d'inclusion, le code C l'inclut ainsi :

```c
#include "display/effect/effect.h"
```

Les effets concrets sont dans le même dossier : `engine/display/effect/transform.h`, `engine/display/effect/zoom.h`, `engine/display/effect/turn.h`, `engine/display/effect/wave.h`, `engine/display/effect/bob.h`, `engine/display/effect/shake.h`, `engine/display/effect/shear.h` et `engine/display/effect/blink.h` ; n'inclure que l'effet utilisé. Un sprite possède directement `sprite.effect`; l'effet continue à avancer même si son animation est arrêtée.

Exemple de zoom pulsé centré :

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

`UEffect` ne copie pas `pulse` : la configuration doit donc rester valide tant que l'effet est actif. Une configuration `static const` convient aux presets immuables ; pour une valeur animée par le gameplay, stocker la configuration dans l'objet runtime qui la pilote. `unsigned_effect_clear()` détache l'effet.

Pour une transformation pilotée par le gameplay, utiliser `UTransformEffect` puis modifier son offset, son scale, ses flips ou sa visibilité sans toucher au renderer. `UCharacterShadow` suit exactement ce modèle : il traduit la hauteur du personnage en scale et laisse `sprite_renderer` + `sprite_backend` faire le reste.

Presets disponibles : `transform`, `zoom`, `turn`, `shake`, `bob`, `blink`, `shear`, `wave`. Pour un effet spécifique au jeu, `unsigned_effect_set_advanced()` permet de fournir un callback uniforme ou par colonne tout en réutilisant le même pipeline. Les effets de sprite sont purement visuels : ils ne déplacent pas `actor.position` et ne transforment pas les hitboxes/hurtboxes. `turn` est une rotation 3D simulée par réduction + flip. Le matériel Neo Geo ne fournit ni rotation libre ni agrandissement au-delà de 100 % ; pour ces cas, utiliser des graphismes pré-rendus.

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
2. la lie à un `UGameplayAttribute` de vie non possédé ;
3. compare Health, MinHealth et MaxHealth à son cache pendant le rendu ;
4. appelle `unsigned_ui_progress_bar_sync()` uniquement lorsqu'une valeur affichée a changé ;
5. rend le `UUIScreen` via `UUIRenderer` + `UNeoGeoUIRenderer`.

Le HUD ne remplace pas `UGameplayAttribute.on_change`. Le propriétaire du personnage reste donc responsable de ses callbacks métier. L'attribute lié et ses bornes doivent rester vivants tant que le HUD est visible.

Pour les menus, utiliser `unsigned_ui_input_from_controller()` pour convertir le snapshot contrôleur en `UUIInput` standard au lieu de dupliquer le mapping direction/A/B dans chaque écran.

`UUIBlinkLabel` s'initialise à partir d'un `UUIBlinkLabelConfig` nommé (`bounds`, `style`, texte, intervalle, largeur de clear et visibilité initiale) plutôt qu'avec une longue liste d'arguments positionnels. Utiliser cette forme de config pour composer les prompts clignotants.

## 9. Collisions : deux couches à distinguer

`engine/physics` connaît des boîtes et des couches de collision. Il ne sait pas ce qu'est une attaque.

`engine/collision` ajoute :

- hitbox/hurtbox ;
- acteurs ;
- projectile ;
- hit detection.

`engine/level/level_collision.c` orchestre ensuite tout cela pour le niveau actif.

Les collisions statiques des objets sont construites une seule fois au chargement. Les collisions dynamiques utilisent un fast path idle : si aucun projectile ni hitbox active ne peut produire de requête, le niveau se limite à un probe de l'activité d'attaque et évite de matérialiser toutes les hurtboxes. Sur les frames qui produisent une requête, l'ancien tail dynamique est effacé une fois puis les boîtes sont rafraîchies/enregistrées pendant la même traversée des pools.

Le callback `ULevelDefinition.resolve_hits` lit les couples déjà détectés dans le container frame-local `level->collision.hits`. Il applique ensuite les règles de jeu (dégâts, garde, réactions, déduplication), sans recalculer lui-même les intersections hitbox/hurtbox.

Les capacités des buffers de collision sont des contrats de composition dimensionnés avec la configuration du jeu ; le hot path de frame n'ajoute pas de branches de récupération pour des états de sous-capacité impossibles.

## 10. State graph : ajouter du temps sans polluer le graph

`UStateGraph` possède l'état logique courant, pas un compteur de frames. `UStateGraphNode.duration_frames` est une donnée de définition ; `UStateGraphClock` est le runtime temporel optionnel.

Le pattern est :

```text
unsigned_state_graph_init(...)
unsigned_state_graph_clock_reset(&clock, &graph)

chaque frame nécessitant les timeouts :
    unsigned_state_graph_clock_tick(&clock, &graph)
```

Si aucun timeout n'est nécessaire, appeler simplement `unsigned_state_graph_tick()` et ne pas stocker de clock. `ULevelManager` expose cette distinction explicitement via `ULevelManagerMode` : le mode graph possède/tick un `UStateGraphClock`, tandis que le mode direct laisse l'application de plus haut niveau remplacer les niveaux avec `unsigned_level_manager_set()`.

## 11. TLSS : simulation temporellement réduite

TLSS est dans `engine/core/tlss/`.

Il peut étaler le travail sur 1/2/4/8/16 frames. Deux usages sont actuellement configurables dans `UGameInstanceConfig` :

- AI ;
- collision.

Les NPC hors écran ou dormants peuvent donc coûter moins cher sans réduire le framerate global.

Important : la collision dispose d'un mécanisme de résolution immédiate pour une hitbox qui vient de devenir active, afin qu'une cadence TLSS réduite ne fasse pas perdre son premier instant d'attaque.

## 12. Neo Geo : ce qu'il ne faut pas simuler soi-même

Sur MVS/AES, le BIOS possède une partie du cycle de vie.

Le runtime système gère notamment :

- USER requests ;
- phases ATTRACT/TITLE/GAME/GAME_OVER ;
- `PLAYER_START` ;
- session joueurs ;
- crédits ;
- GAME START COMPULSION ;
- handoff audio associé.

Avant de modifier ce domaine, lire `docs/Bios_fr.md` puis les fichiers publics concernés dans `engine/system/`.

Ne transformez pas simplement un bouton START lu dans l'input générique en décision de démarrer une session MVS : `PLAYER_START` BIOS reste la source autoritaire.

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

Cette lecture montre la frontière entre le moteur réutilisable et le contenu du jeu mieux qu'une lecture fichier par fichier de tout `engine/`.

Dans `src/game/demo_flow.c`, les transitions de scène sont décrites par une table de règles. `UDemoFlowContext.current` (`UDemoScene`) est la source de vérité pour l'identité de la scène, et les dépendances stables sont liées une seule fois via `UDemoFlowConfig`. `UTimerPool` reste responsable des durées/countdowns de présentation. Ne pas confondre ce flow applicatif avec `UStateGraphClock`, qui ne sert qu'à chronométrer un `UStateGraph` générique.

## 14. Méthode de modification recommandée

Pour une contribution :

1. trouver l'API publique `.h` du sous-système ;
2. lire l'implémentation correspondante ;
3. lire au moins un test dans `test/` ;
4. identifier les capacités fixes et la durée de vie des pointeurs ;
5. faire le changement minimal ;
6. ajouter ou adapter un test hôte ;
7. vérifier ensuite MAME/hardware si le changement dépend de la Neo Geo.

Quand un commentaire est nécessaire, documenter l'invariant, la durée de vie, la raison de performance ou la contrainte BIOS. Éviter de paraphraser le code.
