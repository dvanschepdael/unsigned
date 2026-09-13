# Débuter dans Unsigned

Ce guide vise un développeur qui connaît les bases du C mais découvre le moteur et/ou la Neo Geo. L'objectif n'est pas de mémoriser chaque fichier : il faut surtout comprendre **qui possède les données**, **qui décide du comportement**, **qui décide du rendu** et **qui écrit réellement sur le matériel**.

## 1. Les cinq repères à connaître

### `UGameInstance` : la racine du moteur

`UGameInstance` (`engine/game/game.h`) regroupe les sous-systèmes génériques d'une partie : input, timers, gameplay, pools d'acteurs, niveau, renderer, level manager, viewport et audio.

`unsigned_game_instance_init()` reçoit des buffers déjà alloués via `UGameInstanceStorage`. Une capacité insuffisante doit donc être corrigée au démarrage plutôt que masquée par une allocation dynamique en pleine partie.

### `ULevelDefinition` : le contenu déclaré

`ULevelDefinition` (`engine/level/level_definition.h`) contient les éléments stables :

- backgrounds ;
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
- orientation.

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
  physics/    collision géométrique bas niveau
  collision/  collision gameplay
  display/    données de présentation, UI, viewport
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
        +--> demo_loop_render()
        |      +--> unsigned_game_instance_render()
        |      +--> overlays / menu / HUD
        |
        +--> ng_wait_vblank()
        +--> transport audio
```

Le BIOS reste autoritaire sur certaines transitions système. Il ne faut donc pas raisonner comme sur une boucle PC totalement autonome.

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
5. prévoir la fin ou l'annulation de l'ability ;
6. tester l'échec d'activation lorsque le pool est plein.

Fichiers utiles :

- `engine/actor/player.c` ;
- `engine/gameplay/ability.c` ;
- `engine/gameplay/ability_pool.c` ;
- `engine/gameplay/gameplay_pool.c` ;
- exemples dans `src/characters/player/demo/`.

### Un nouvel attribute

Les attributes sont définis dans `engine/gameplay/attribute.h`.

Pour un attribute propre au personnage de démonstration, regarder `src/characters/player/demo/player_health.c`.

Un callback `on_update` peut servir à synchroniser une UI, comme le fait le HUD pour la vie du joueur.

### Un nouveau NPC

1. définir son contenu/spawn ;
2. fournir son `UCharacter` ;
3. configurer son state graph ;
4. appeler `unsigned_npc_init()` ;
5. laisser `level_ai.c` et TLSS gérer sa cadence selon son activité.

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

Le chargement est transactionnel : une erreur déclenche le rollback du contenu déjà installé.

## 7. Display, renderer et system : ne pas les confondre

Cette séparation est essentielle dans la structure actuelle.

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
3. branche `on_update` sur l'attribute et ses bornes ;
4. appelle `unsigned_ui_progress_bar_sync()` quand la valeur change ;
5. rend le `UUIScreen` via `UUIRenderer` + `UNeoGeoUIRenderer`.

Le widget stocke une progression normalisée 0..256. Le renderer n'a donc pas besoin de refaire le calcul complet de l'attribute à chaque frame.

## 9. Collisions : deux couches à distinguer

`engine/physics` connaît des boîtes et des couches de collision. Il ne sait pas ce qu'est une attaque.

`engine/collision` ajoute :

- hitbox/hurtbox ;
- acteurs ;
- projectile ;
- hit detection.

`engine/level/level_collision.c` orchestre ensuite tout cela pour le niveau actif.

Les collisions statiques sont construites au chargement. Les collisions dynamiques sont effacées puis enregistrées à nouveau chaque frame.

Si les buffers de registration sont saturés, la frame de collision est invalidée plutôt que partiellement calculée.

## 10. TLSS : simulation temporellement réduite

TLSS est dans `engine/core/tlss/`.

Il peut étaler le travail sur 1/2/4/8/16 frames. Deux usages sont actuellement configurables dans `UGameInstanceConfig` :

- AI ;
- collision.

Les NPC hors écran ou dormants peuvent donc coûter moins cher sans réduire le framerate global.

Important : la collision dispose d'un mécanisme de résolution immédiate pour une hitbox qui vient de devenir active, afin qu'une cadence TLSS réduite ne fasse pas perdre son premier instant d'attaque.

## 11. Neo Geo : ce qu'il ne faut pas simuler soi-même

Sur MVS/AES, le BIOS possède une partie du cycle de vie.

Le runtime système gère notamment :

- USER requests ;
- phases ATTRACT/TITLE/GAME/GAME_OVER ;
- `PLAYER_START` ;
- session joueurs ;
- crédits ;
- GAME START COMPULSION ;
- handoff audio associé.

Avant de modifier ce domaine, lire `engine/system/BIOS_WORKFLOW.md` puis les fichiers publics concernés dans `engine/system/`.

Ne transformez pas simplement un bouton START lu dans l'input générique en décision de démarrer une session MVS : `PLAYER_START` BIOS reste la source autoritaire.

## 12. Lire le projet de démonstration

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

## 13. Méthode de modification recommandée

Pour une contribution :

1. trouver l'API publique `.h` du sous-système ;
2. lire l'implémentation correspondante ;
3. lire au moins un test dans `test/` ;
4. identifier les capacités fixes et la durée de vie des pointeurs ;
5. faire le changement minimal ;
6. ajouter ou adapter un test hôte ;
7. vérifier ensuite MAME/hardware si le changement dépend de la Neo Geo.

Quand un commentaire est nécessaire, documenter l'invariant, la durée de vie, la raison de performance ou la contrainte BIOS. Éviter de paraphraser le code.
