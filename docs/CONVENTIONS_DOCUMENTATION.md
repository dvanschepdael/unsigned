# Conventions de documentation et de commentaires

## Objectif

La documentation doit aider un développeur à comprendre les contrats, les invariants et les raisons d'architecture sans transformer le code en commentaire ligne par ligne.

Elle doit aussi rester synchronisée avec l'arborescence actuelle du projet : lorsqu'un module est déplacé ou qu'une responsabilité change de couche, les chemins et diagrammes de `docs/` doivent être modifiés dans le même changement.

## 1. Documenter l'intention, pas la syntaxe

Un commentaire est utile s'il répond à au moins une question non évidente :

- **Pourquoi** cet ordre, cette validation ou ce cache existe-t-il ?
- **Qui possède cette donnée** et combien de temps doit-elle rester valide ?
- **Quel invariant** doit être vrai avant/après l'appel ?
- **Que se passe-t-il en saturation**, overflow, rollback ou callback réentrant ?
- **Quelle contrainte Neo Geo/BIOS/ngdevkit** impose ce comportement ?
- **Quelle règle métier** est volontairement laissée à `src/` plutôt qu'au moteur ?
- **Pourquoi cette responsabilité appartient-elle à `actor`, `physics`, `display`, `renderer` ou `system` ?**

À éviter :

```c
/* Increment i. */
++i;

/* Set the player position. */
player->position = position;
```

Préférer :

```c
/* Keep the physical slot stable; generation distinguishes a reused handle. */
++instance->generation;
```

## 2. Responsabilités des couches à respecter dans les commentaires

La structure actuelle sépare plusieurs niveaux. La documentation ne doit pas recréer les anciens chemins ou brouiller ces responsabilités.

### `engine/display`

Décrit l'état logique de présentation et les widgets indépendants du backend.

Exemples :

- `engine/display/sprite/` ;
- `engine/display/ui/` ;
- `engine/display/viewport/`.

### `engine/renderer`

Décrit la politique de rendu et les caches : ordre, visibilité, allocation de sprites, dirty flags, ring de background, adaptation du renderer UI.

Exemples :

- `engine/renderer/level_renderer.c` ;
- `engine/renderer/sprite_renderer.c` ;
- `engine/renderer/ui_renderer.c`.

### `engine/actor` et `engine/physics`

`UCharacter` possède le comportement générique de mouvement/orientation via `unsigned_character_move()` / `unsigned_character_set_facing()`. `engine/physics` décrit les primitives spatiales indépendantes du gameplay : géométrie, contraintes explicites de mouvement et trajectoires. Une politique de zone jouable ne doit pas être stockée dans `UActor` ; le propriétaire du mouvement choisit les `UMovementBounds` à appliquer après le mouvement.

### `engine/core/state`

Sépare la logique d'état du temps d'exécution. `UStateGraph` décrit/exécute les états et transitions ; `UStateGraphClock` possède `elapsed_frames` uniquement pour les runtimes qui utilisent des timeouts. Ne pas déplacer un compteur temporel générique dans `UStateGraph`.

### `engine/collision` et `engine/level`

La collision moteur décide la géométrie et produit des `UCollisionHit`. Le callback `ULevelDefinition.resolve_hits` interprète ces résultats côté gameplay. Ne pas documenter ni réintroduire une seconde intersection hitbox/hurtbox dans un personnage après qu'un hit a déjà été produit par le pipeline.

### `engine/system`

Décrit les détails plateforme/Neo Geo : BIOS, VRAM, FIX, vidéo, input plateforme, audio et backends matériels.

Exemples :

- `engine/system/runtime.c` ;
- `engine/system/renderer_backend.h` pour la transaction globale begin/end ;
- `engine/system/sprite_backend.h`, `engine/system/background_backend.h` et `engine/system/palette_backend.h` pour les contrats matériels par domaine ;
- `engine/system/fix.c` ;
- `engine/system/vram_writer.c`.

### `src`

Contient le contenu et les décisions propres au jeu : personnages, niveaux, flow, menus et HUD.

Ne pas documenter une règle propre à la démo comme si elle constituait un invariant générique du moteur.

## 3. Headers publics

Utiliser Doxygen lorsqu'une API possède un contrat utile à exposer :

- préconditions ;
- propriété et durée de vie des pointeurs ;
- unités (`frames`, `pixels`, `tiles`, secondes, BCD, etc.) ;
- plage de valeurs ;
- comportement de retour/échec ;
- saturation ou overflow ;
- effets de bord ;
- différence entre contenu statique et runtime ;
- interaction avec des buffers caller-owned ;
- différence entre comportement générique et backend Neo Geo.

Un setter trivial n'a pas besoin d'un long bloc Doxygen si son nom et son type suffisent. Documenter davantage lorsqu'il déclenche un clamp, une invalidation de cache, une synchronisation, une écriture persistante ou une sémantique matérielle.

## 4. Fichiers `.c`

Les commentaires internes doivent expliquer les décisions locales difficiles à reconstruire :

- ordre de pipeline ;
- stratégie de cache/ring buffer ;
- protection contre handle périmé ;
- comportement en cas de capacité pleine ;
- callback pouvant libérer l'objet courant ;
- ordre d’un remplacement/teardown lorsque plusieurs sous-systèmes partagent un cycle de vie ;
- règle de scheduling TLSS ;
- workaround BIOS ;
- raison d'une écriture VRAM ou d'un handoff audio précis.

Éviter une phrase au-dessus de chaque fonction statique si le nom de la fonction décrit déjà correctement son rôle.

## 5. Commentaires spécifiques aux pools

Lorsqu'un code stocke un `UPoolInstance *`, documenter si nécessaire :

- qui possède le pool ;
- quand le slot peut être libéré ;
- si `generation` doit être vérifiée ;
- si un callback peut réutiliser le slot avant le retour.

Ne jamais laisser entendre qu'une adresse de slot constitue à elle seule une identité stable si la génération peut changer.

## 6. Commentaires spécifiques à TLSS

Lorsqu'un système est ralenti par TLSS, documenter :

- quelle cadence est appliquée ;
- quel slot/index fournit la phase ;
- si le delta correspond à plusieurs frames ;
- les cas qui forcent une exécution immédiate.

Exemple actuel : une hitbox nouvellement active peut forcer une résolution de collision immédiate pour éviter de perdre sa première frame active.

## 7. Documentation UI

Pour les widgets de `engine/display/ui/widget/`, documenter séparément :

- la donnée logique conservée par le widget ;
- les unités utilisées ;
- le lien éventuel avec un objet gameplay ;
- ce qui doit être synchronisé explicitement ;
- ce qui appartient au renderer/backend.

Exemple : `UUIProgressBar` stocke une progression normalisée 0..256 et peut être liée à un `UGameplayAttribute`. La synchronisation de l'attribute est explicite ; le rendu Neo Geo reste dans `engine/renderer/ui_renderer.c`.

## 8. Documentation BIOS / Neo Geo

Les commentaires de `engine/system/` doivent distinguer clairement :

- comportement imposé par le BIOS ;
- politique choisie par Unsigned ;
- détail du backend Neo Geo ;
- comportement uniquement observé dans la démo.

Pour le cycle de vie BIOS et les crédits, le document de référence interne est `docs/Bios_fr.md` / `docs/Bios_en.md`.

Ne pas recréer dans plusieurs fichiers une explication longue et divergente du même workflow ; utiliser un commentaire court et pointer vers le document de référence lorsque nécessaire.

## 9. Langue

Les symboles et commentaires source sont en anglais. Garder cette règle dans les `.c` et `.h` afin d'éviter un mélange de langues dans l'API.

Les documents d'onboarding dans `docs/` peuvent rester en français.

## 10. Chemins et noms de modules

Lorsqu'un chemin est écrit dans la documentation :

- utiliser le chemin réel depuis la racine du dépôt ;
- éviter les anciens chemins après un refactor ;
- préférer un dossier lorsqu'une explication concerne toute une couche ;
- utiliser un fichier précis lorsqu'un comportement est réellement implémenté à cet endroit.

Structure de référence actuelle :

```text
engine/actor/     état/comportement générique des acteurs et personnages
engine/physics/   primitives spatiales, movement bounds et trajectoires
engine/core/state logique StateGraph + runtime temporel optionnel StateGraphClock
engine/display/   état logique de présentation
engine/renderer/  politique de rendu
engine/system/    backends et runtime Neo Geo
src/game/         composition/flow de la démo
src/levels/       contenu des niveaux
src/characters/   contenu des personnages
src/menu/         UI de menu
src/hud/          HUD
```

## 11. Documentation d'architecture

`docs/Architecture_fr.md` / `docs/Architecture_en.md` doit décrire le comportement actuel, pas l'historique des refactors.

`docs/Guide_fr.md` / `docs/Guide_en.md` doit privilégier les chemins d'apprentissage stables : points d'entrée, frontières de responsabilités et exemples concrets.

Les détails très locaux doivent rester près du code ou dans un document spécialisé, comme `docs/Bios_fr.md` / `docs/Bios_en.md`.

## 12. Vérification avant commit

Avant de valider une documentation :

1. vérifier qu'elle décrit le comportement actuel ;
2. vérifier que tous les chemins cités existent encore ;
3. vérifier les unités, bornes et types dans les headers/implémentations ;
4. vérifier l'ordre réel des pipelines dans le code ;
5. vérifier qu'aucune garantie n'est promise sans implémentation ou test correspondant ;
6. vérifier les frontières de responsabilité (par exemple bounds hors de `UActor`, temps hors de `UStateGraph`) ;
7. supprimer les phrases génériques qui pourraient s'appliquer à n'importe quel moteur ;
8. mettre à jour la documentation dans le même changement qu'une modification de contrat ou d'arborescence.

## 13. Tests et documentation

Quand un document affirme un comportement testable, chercher d'abord un test existant dans `test/`.

Exemples de zones déjà couvertes par des tests dédiés :

- gameplay abilities/effects ;
- input/mouvement et contraintes spatiales ;
- state graph et clock de timeout ;
- niveaux ;
- collisions/hit detection ;
- TLSS ;
- renderer/sprites/backgrounds ;
- UI ;
- timer ;
- runtime/session/crédits Neo Geo ;
- sauvegarde.

La documentation ne remplace pas ces tests : elle explique le contrat et la raison, tandis que le test vérifie le comportement.
