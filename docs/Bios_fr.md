# Intégration du BIOS Neo Geo

Unsigned centralise les règles de cycle de vie BIOS dans `engine/system/`. Le code du jeu travaille avec des phases runtime et un état joueur typé ; il ne lit pas directement la RAM BIOS et ne recrée pas les événements BIOS à partir de l'input contrôleur.

## Frontière du cycle de vie

| Requête BIOS | Entrée ngdevkit | Comportement runtime |
| --- | --- | --- |
| USER 0 | `rom_mvs_startup_init` | géré par le code de démarrage ngdevkit |
| USER 1 | `main()` | eye catcher optionnel ; aucune initialisation complète du jeu |
| USER 2 | `main()` | initialisation, ATTRACT, GAME/GAME_OVER éventuel, shutdown |
| USER 3 | `main_mvs_title()` | initialisation, TITLE, GAME/GAME_OVER éventuel, shutdown |

`BIOS_USER_REQUEST` est interprété via le type interne `UNeoGeoBiosRequest` partagé par les couches runtime/callback BIOS. `UNeoGeoPhase` est un concept exposé au jeu : une même requête USER peut exécuter plusieurs phases.

## Callbacks BIOS

`PLAYER_START` et `COIN_SOUND` sont des événements BIOS autoritaires. Unsigned ne les synthétise ni depuis les changements contrôleur ni depuis `BIOS_COMPULSION_TIMER`.

Un binding runtime possède trois états internes :

- `UNBOUND` : aucun runtime USER 2/3 actif ; les requêtes START sont rejetées ;
- `INITIALIZING` : `initialize()` est en cours ; les requêtes START sont rejetées ;
- `READY` : l'initialisation est terminée ; `accept_start` peut filtrer les requêtes START.

Cette séparation empêche un callback VBlank/BIOS d'entrer en GAME avec un contexte applicatif partiellement initialisé.

### Handshake PLAYER_START

`neo_geo_bios_process_start()` réalise le côté cartouche du handshake :

1. lire les bits de requête P1..P4 dans `BIOS_START_FLAG` ;
2. retirer les joueurs déjà en `PLAYING` ;
3. rejeter la requête entière tant que le runtime n'est pas `READY` ;
4. laisser `accept_start` retirer d'autres bits demandés ;
5. réécrire uniquement les bits acceptés dans `BIOS_START_FLAG` ;
6. préparer `BIOS_CREDIT_DEC1..4` pour les joueurs acceptés ;
7. passer les joueurs acceptés en `PLAYING` ;
8. positionner `BIOS_USER_MODE = GAME` lorsqu'au moins un joueur a été accepté.

Le BIOS possède le décrément réel des crédits MVS après le retour du callback.

## État des joueurs

Unsigned utilise directement les valeurs BIOS `PLAYER_MOD` :

| État | Signification |
| --- | --- |
| `NEVER_PLAYED` | le slot n'a pas encore participé |
| `PLAYING` | gameplay actif |
| `CONTINUE` | attente d'un continue ; garde la session GAME active |
| `GAME_OVER` | état terminal pour ce joueur |

Le code du jeu change l'état joueur via des transitions contrôlées :

- `unsigned_neo_geo_player_begin_continue()` : `PLAYING -> CONTINUE` ;
- `unsigned_neo_geo_player_game_over()` : `PLAYING/CONTINUE -> GAME_OVER` ;
- un `PLAYER_START` BIOS accepté ultérieurement fait `CONTINUE -> PLAYING`.

`unsigned_neo_geo_request_game_over()` applique la transition terminale à tous les joueurs participants. `unsigned_neo_geo_end_session()` ferme en plus la session runtime locale.

## Crédits MVS

Le runtime générique modélise uniquement les compteurs de crédits P1/P2 standards.

- le Japon utilise le pool partagé de crédits P1 ;
- les MVS US et Europe utilisent des pools séparés P1/P2 ;
- l'ownership des crédits P3/P4 n'est pas deviné, car le routage 4 joueurs dépend de l'extension BIOS/système.

`engine/system/credits.c` lit les compteurs de crédits pour l'UI et écrit `BIOS_CREDIT_DEC1..4`. Il ne décrémente jamais directement les compteurs de crédits en backup RAM.

`unsigned_neo_geo_set_game_start_compulsion(bool enabled)` est l'exception explicite pour la configuration cabinet. Sur MVS, la fonction déverrouille la backup RAM BIOS, écrit le réglage persistant GAME START COMPULSION puis reverrouille la RAM. `false` sélectionne `WITHOUT` ; `true` active le forced start. Sur AES, l'appel est sans effet. Comme ce réglage est persistant et global au cabinet, le jeu ne doit le modifier que volontairement.

Les helpers cabinet associés sont :

- `unsigned_neo_geo_game_start_compulsion_enabled()` lit le réglage forced-start courant ;
- `unsigned_neo_geo_game_start_compulsion_seconds()` expose le timer BIOS BCD en secondes normales ;
- `unsigned_neo_geo_set_game_start_compulsion_seconds()` stocke une valeur de 0 à 99 secondes en BCD BIOS ;
- `unsigned_neo_geo_demo_sound_enabled()` indique si le son de démo cabinet est actif (`true` sur AES).

Le helper demo-sound ne remplace pas une logique de soft DIP propre au jeu. Les setters ci-dessus modifient la backup RAM BIOS persistante du cabinet et doivent donc être utilisés volontairement.

## Commandes son

Le driver son réserve les commandes `0..3` :

| Commande | Usage |
| --- | --- |
| 0 | aucune commande |
| 1 | préparation d'un changement de ROM |
| 2 | eye catcher ngdevkit |
| 3 | reset driver/YM2610 |
| 4+ | commandes du jeu |

L'audio générique accepte uniquement les commandes `>= U_AUDIO_COMMAND_FIRST_GAME`. Le code de cycle de vie plateforme atteint les commandes réservées uniquement via des helpers backend Neo Geo comme `neo_geo_audio_reset()`.

Le registre 68k `REG_SOUND` est un latch d'un octet ; le gameplay ne l'écrit donc jamais directement. Les commandes audio normales entrent dans la queue de transport Neo Geo et au plus une commande est écrite immédiatement après chaque VBlank terminé. Un `COIN_SOUND` qui ne peut pas provoquer de handoff de ROM BIOS reste envoyé immédiatement et réserve le prochain slot normal de transport. Le driver nullsound copie déjà les commandes jeu acceptées dans sa propre FIFO côté Z80.

GAME START COMPULSION sur MVS suit un autre chemin. Le cycle forced-start SNK demande USER 3/Title et la carte peut sélectionner temporairement les ROM FIX/SM1 de la carte mère. La commande nullsound 1 est conçue pour ce changement de ROM : elle arrête le son, reset le YM2610 et attend en RAM Z80. La même sélection matérielle change aussi la source FIX, ce qui explique qu'un émulateur puisse montrer un flash bref exactement lorsque le premier sample de crédit lancé trop tôt est interrompu.

Unsigned traite donc le coin comme un événement, et non comme un playback qui devrait survivre au changement de ROM :

1. pendant USER 2/ATTRACT avec GAME START COMPULSION, le callback vérifie les signaux de transition BIOS (`BIOS_USER_REQUEST`, état de compulsion et crédits courants). Seul un coin qui conduit réellement au TITLE forced-start est différé ; un dépôt partiel qui n'a pas encore créé de crédit reste immédiat. Un événement différé incrémente un petit token de handoff dans le bloc backup cartouche (`.bss.bram`), que ngdevkit ne vide pas pendant la réinitialisation du runtime C de USER 3 ;
2. USER 3 démarre après que ngdevkit a resélectionné `CRTFIX` et la M1 cartouche ;
3. USER 3 envoie la commande son BIOS 3, nécessaire pour initialiser le driver nullsound M1 nouvellement sélectionné ;
4. le token persistant est consommé immédiatement et armé comme état de transport volatile ;
5. au premier VBlank terminé, la commande coin est envoyée avant l'audio TITLE/GAME normal. Cette frontière de frame est une frontière de protocole après la commande 3, pas un délai arbitraire.

Lorsque GAME START COMPULSION vaut `WITHOUT`, USER 3 ne fait pas partie de ce handoff forced-start du premier crédit ; le chemin `COIN_SOUND` immédiat normal reste donc inchangé.

`UAudioManager` conserve localement une commande résolue lorsque la queue plateforme est pleine et la réessaie plus tard ; la back-pressure du transport ne supprime donc pas silencieusement une transition musicale ou une commande SFX.

USER 2 et USER 3 reset tous deux le driver son après la sélection `CRTFIX`/M1. USER 2 nettoie aussi tout état de handoff périmé. USER 3 rejoue ensuite tout événement coin différé spécifiquement pour la transition ROM forced-start.

## Responsabilités internes

| Fichier | Responsabilité |
| --- | --- |
| `engine/system/runtime.c` | dispatch BIOS USER et séquencement ATTRACT/TITLE/GAME/GAME_OVER |
| `engine/system/bios_callbacks.c` | callbacks ABI cartouche et état de binding runtime |
| `engine/system/session.c` | accès `PLAYER_MOD`, transitions joueur et durée de vie de session |
| `engine/system/credits.c` | coût de start et routage standard des crédits MVS P1/P2 |
| `engine/system/input.c` | snapshots contrôleur BIOS -> `UInputManager` |
| `engine/system/audio_backend.c` | transport sérialisé `REG_SOUND`, priorité coin BIOS et commandes plateforme réservées |
| `engine/system/save_backend.c` | opérations BIOS memory-card / backup-RAM |
| `engine/system/renderer_backend.h` + backends sprite/background/palette | transaction de rendu globale + accès matériels Neo Geo VRAM/palette/sprites |

Les headers internes suivent les mêmes frontières : `engine/system/bios_callbacks_internal.h`, `engine/system/bios_state_internal.h`, `engine/system/session_internal.h` et `engine/system/credits_internal.h`.

## Compatibilité ngdevkit

Le build est fixé sur la version de package ngdevkit `0.5` via `NGDEVKIT_REQUIRED_VERSION` dans le Makefile. Lors d'un changement de version ngdevkit, revérifier au minimum :

- le dispatch USER dans `<ngdevkit>/runtime/ngdevkit-crt0.S` ;
- les wrappers `PLAYER_START` et `COIN_SOUND` ;
- `<ngdevkit>/include/ngdevkit/bios-ram.h` ;
- les exemples officiels de gestion des crédits et de memory card.

Les builds source/nightly de ngdevkit peuvent partager la même version de package ; un build de release doit aussi enregistrer la révision git ngdevkit exacte utilisée par la toolchain.
