# Intégration du BIOS Neo Geo

Version anglaise : [`bios_en.md`](bios_en.md)

Unsigned regroupe le cycle de vie BIOS Neo Geo sous `src/system/` et expose les APIs plateforme publiques correspondantes sous `include/system/`. Le code de jeu travaille avec des phases de runtime et des états typés de joueur/session, tandis que l’accès à la RAM BIOS, les callbacks, le routage des crédits, le transport audio et les réglages persistants de la borne restent dans la couche système.

## Frontière du cycle de vie

| Requête BIOS | Entrée ngdevkit | Comportement Unsigned |
| --- | --- | --- |
| USER 0 | `rom_mvs_startup_init` | géré par le code de démarrage ngdevkit |
| USER 1 | `main()` | eye catcher optionnel ; pas d’initialisation du runtime USER 2/3 |
| USER 2 | `main()` | initialisation, ATTRACT, GAME/GAME_OVER éventuel, shutdown |
| USER 3 | `main_mvs_title()` | initialisation, TITLE, GAME/GAME_OVER, shutdown |

`BIOS_USER_REQUEST` est interprété via le type interne `UNeoGeoBiosRequest` déclaré dans `include/system/bios_state_internal.h`. `UNeoGeoPhase`, déclaré dans `include/system/runtime.h`, représente les phases visibles par l’application. Une même requête USER du BIOS peut donc contenir plusieurs phases Unsigned.

`src/system/runtime.c` traduit ces entrées BIOS en séquence de runtime. La VBlank reste la frontière de synchronisation : ngdevkit exécute `SYSTEM_IO`, met à jour l’état contrôleur maintenu par le BIOS et distribue les callbacks BIOS, puis Unsigned consomme cet état pendant la frame suivante.

## Callbacks BIOS

`PLAYER_START` et `COIN_SOUND` sont des callbacks BIOS autoritaires. Leurs implémentations côté cartouche se trouvent dans `src/system/bios_callbacks.c`.

Le binding du runtime possède trois états internes :

- `UNBOUND` : aucun runtime applicatif USER 2/3 actif ;
- `INITIALIZING` : `initialize()` est en cours ;
- `READY` : l’initialisation est terminée et les requêtes `PLAYER_START` peuvent être filtrées via `accept_start`.

Cette machine d’état évite qu’un `PLAYER_START` fasse entrer le jeu dans GAME avec un contexte applicatif partiellement initialisé.

### Handshake `PLAYER_START`

`neo_geo_bios_process_start()` réalise la partie cartouche du handshake :

1. lire les bits de requête P1..P4 depuis `BIOS_START_FLAG` ;
2. retirer les joueurs déjà en `PLAYING` ;
3. accepter les requêtes uniquement lorsque le binding du runtime est `READY` ;
4. transmettre les bits candidats à `accept_start` lorsque l’application fournit ce callback ;
5. réécrire le masque accepté dans `BIOS_START_FLAG` ;
6. préparer `BIOS_CREDIT_DEC1..4` pour les joueurs acceptés ;
7. passer les joueurs acceptés en `PLAYING` ;
8. positionner `BIOS_USER_MODE = GAME` dès qu’au moins un joueur est accepté.

Le BIOS effectue réellement la décrémentation des crédits MVS après le retour du callback cartouche.

## État des joueurs et de la session

Unsigned utilise directement les valeurs BIOS `PLAYER_MOD` via `src/system/session.c` :

| État | Signification |
| --- | --- |
| `NEVER_PLAYED` | le slot n’a pas encore participé |
| `PLAYING` | joueur actif en gameplay |
| `CONTINUE` | en attente d’un continue ; participe encore à la session GAME |
| `GAME_OVER` | état terminal pour ce joueur |

Les transitions publiques sont exposées par `include/system/session.h` :

- `unsigned_neo_geo_player_begin_continue()` : `PLAYING -> CONTINUE` ;
- `unsigned_neo_geo_player_game_over()` : `PLAYING/CONTINUE -> GAME_OVER` ;
- un `PLAYER_START` BIOS accepté ultérieurement repasse le slot concerné en `PLAYING` ;
- `unsigned_neo_geo_request_game_over()` applique la transition terminale à tous les joueurs participants ;
- `unsigned_neo_geo_end_session()` ferme également la session runtime locale.

GAME reste actif tant qu’au moins un joueur est en `PLAYING` ou `CONTINUE`. GAME_OVER commence lorsque les joueurs participants ont quitté ces deux états, sauf si la session locale a déjà été explicitement terminée.

## Crédits MVS

Le runtime générique modélise les compteurs de crédits standard P1/P2.

- le Japon utilise le pool de crédits partagé de P1 ;
- les régions US et Europe utilisent des pools séparés P1/P2 ;
- la propriété des crédits P3/P4 est laissée aux extensions BIOS/système spécifiques au lieu d’être déduite par l’API générique.

`src/system/credits.c` lit les compteurs BIOS pour les prompts/UI et prépare `BIOS_CREDIT_DEC1..4` pour les starts acceptés. Il ne décrémente pas directement les compteurs de crédits BIOS.

Les helpers publics de `include/system/credits.h` exposent notamment le partage des crédits, les valeurs de crédits par joueur et la sélection standard des prompts INSERT COIN / PRESS START.

## Réglages de borne MVS

`src/system/settings.c` modélise les réglages de borne actuellement nécessaires à Unsigned.

`unsigned_neo_geo_set_game_start_compulsion(bool enabled)` modifie le réglage persistant GAME START COMPULSION dans la backup RAM BIOS MVS. Le mode AES reste inchangé.

Les helpers associés sont :

- `unsigned_neo_geo_game_start_compulsion_enabled()` ;
- `unsigned_neo_geo_game_start_compulsion_seconds()` ;
- `unsigned_neo_geo_set_game_start_compulsion_seconds()` ;
- `unsigned_neo_geo_demo_sound_enabled()`.

Le délai de compulsion est exposé en secondes normales alors que la valeur BIOS est stockée en BCD. Les valeurs supérieures à 99 secondes sont clampées à 99. Ces APIs modifient des réglages persistants à l’échelle de la borne ; le code applicatif les traite donc comme des opérations de configuration plutôt que comme un état de gameplay de session.

## Frontière des commandes audio

Le BIOS Neo Geo réserve les commandes audio `0..3` :

| Commande | Signification dans nullsound |
| --- | --- |
| 0 | inutilisée / aucune commande |
| 1 | préparation au changement de ROM et attente en RAM Z80 |
| 2 | reset du driver et lancement de l’eye catcher fourni par la ROM du jeu |
| 3 | initialisation/reset du driver audio |
| 4+ | commandes définies par le jeu |

Unsigned reflète cette frontière avec `U_AUDIO_COMMAND_FIRST_GAME = 4` dans `include/audio/audio_types.h`.

Le registre 68k `REG_SOUND` est un latch d’un octet, pas une file. `src/system/audio_backend.c` possède donc l’écriture matérielle et sérialise les commandes de jeu normales via une petite file de transport. `neo_geo_audio_transport_tick()` envoie au maximum une commande en attente après une VBlank terminée.

Un `COIN_SOUND` BIOS normal est écrit immédiatement et réserve le slot de transport normal suivant. Cela laisse au chemin NMI du Z80 une frontière de transport complète avant qu’une autre commande de jeu puisse remplacer la valeur du latch.

`UAudioManager` gère également la back-pressure du backend. Si `unsigned_audio_backend_send()` ne peut pas accepter une commande musique/SFX déjà résolue, `src/audio/audio.c` la conserve dans `pending_backend_command` et la réessaie lors d’un tick audio ultérieur.

## GAME START COMPULSION et handoff audio du premier crédit

GAME START COMPULSION peut faire passer le MVS de USER 2/ATTRACT à USER 3/TITLE. Pendant ce cycle, la carte peut temporairement sélectionner les ROMs FIX/SM1 de la carte mère. La commande 1 de nullsound existe précisément pour ce changement de ROM : elle arrête le son, reset le YM2610 et attend depuis la RAM Z80 jusqu’à ce que la ROM audio de la cartouche redevienne active. ngdevkit restaure ensuite le FIX/M1 cartouche avant d’appeler `main_mvs_title()`.

Jouer immédiatement l’échantillon du premier crédit avant ce handoff peut donc démarrer un son que la commande 1 coupe volontairement. Unsigned préserve alors **l’événement de crédit** à travers la réinitialisation du runtime C entre USER 2 et USER 3, puis le rejoue lorsque le driver audio cartouche a redémarré.

### Détection pendant USER 2

`neo_geo_bios_forced_start_handoff_pending()` intervient uniquement lorsque toutes les conditions suivantes sont réunies :

- la requête active est USER 2 / DEMO ;
- le système est MVS ;
- `BIOS_USER_MODE` est encore DEMO ;
- GAME START COMPULSION est activé.

Dans ce contexte, l’implémentation considère les éléments suivants comme indicateurs d’une transition forced-start :

- `BIOS_USER_REQUEST` demande déjà TITLE ;
- l’état du timer de compulsion BIOS indique une fenêtre de transition active/terminée ;
- un crédit standard P1/P2 est disponible alors que USER 2 est encore en cours.

Une insertion partielle qui n’a pas encore produit de crédit utilisable et n’a activé aucun de ces indicateurs reste sur le chemin `COIN_SOUND` immédiat.

### Jeton persistant de handoff

Lorsque le handoff forced-start est détecté, `neo_geo_audio_defer_coin_sound()` incrémente un petit jeton persistant déclaré avec l’attribut ngdevkit `_backup_ram`. La structure stockée par `src/system/audio_backend.c` contient :

- une valeur magique ;
- `pending_coin_count` ;
- un compteur inversé servant de contrôle de validité.

L’attribut `_backup_ram` place ce jeton dans `.bss.bram`. L’initialisation du runtime C de USER 3 par ngdevkit efface la `.bss` normale, tandis que ce bloc backup cartouche reste disponible pour le court handoff USER 2 -> USER 3.

### Séquence de redémarrage USER 3

La séquence actuelle est :

1. USER 2 reçoit le callback BIOS `COIN_SOUND` ;
2. une transition forced-start est détectée et l’événement de crédit est persisté au lieu d’être joué ;
3. le cycle BIOS/ngdevkit entre dans USER 3 et restaure le FIX/M1 cartouche ;
4. `src/system/runtime.c` initialise le transport Neo Geo et envoie la commande audio 3 via `neo_geo_audio_reset()` ;
5. `neo_geo_audio_resume_deferred_coin_sounds()` consomme immédiatement le jeton persistant et arme l’état de lecture volatile ;
6. après la première VBlank complète, `neo_geo_audio_transport_tick()` envoie la commande du crédit différé avant l’audio TITLE/GAME normal.

La frontière VBlank sert ici de frontière de protocole après la commande 3 ; l’implémentation n’utilise ni sleep arbitraire ni boucle d’attente temporisée.

USER 2 efface un éventuel état différé obsolète lorsqu’un nouveau runtime attract démarre. USER 3 consomme le jeton avant d’armer la lecture afin qu’un reset ultérieur ne puisse pas rejouer le même événement.

Lorsque GAME START COMPULSION est désactivé, ce chemin spécifique USER 2 -> USER 3 du premier crédit ne s’applique pas et le transport immédiat normal de `COIN_SOUND` reste utilisé.

## Responsabilités internes

| Fichier | Responsabilité |
| --- | --- |
| `src/system/runtime.c` | dispatch des USER BIOS et séquencement ATTRACT/TITLE/GAME/GAME_OVER |
| `src/system/bios_callbacks.c` | implémentation des callbacks cartouche et état du binding runtime |
| `src/system/session.c` | accès à `PLAYER_MOD`, transitions joueurs et durée de vie de session |
| `src/system/credits.c` | routage standard P1/P2 et préparation de la décrémentation au start |
| `src/system/settings.c` | réglages persistants de borne MVS utilisés par Unsigned |
| `src/system/input.c` | snapshots contrôleurs BIOS vers `UInputManager` |
| `src/system/audio_backend.c` | transport sérialisé de `REG_SOUND`, priorité du son de crédit BIOS, reset du driver et handoff forced-start |
| `src/system/save_backend.c` | opérations backend carte mémoire / backup RAM |
| `src/system/*_backend.c`, `src/system/fix.c`, `src/system/video.c` | accès matériels de rendu/vidéo Neo Geo |

Les contrats internes associés se trouvent sous `include/system/`, notamment `bios_callbacks_internal.h`, `bios_state_internal.h`, `session_internal.h`, `credits_internal.h` et `audio_backend_internal.h`.

## Compatibilité ngdevkit

Unsigned dépend du comportement de cycle de vie et d’ABI ngdevkit autour de :

- `runtime/ngdevkit-crt0.S` pour le dispatch USER et la réinitialisation du runtime C ;
- les wrappers `PLAYER_START` et `COIN_SOUND` ;
- `include/ngdevkit/bios-ram.h` et les définitions de backup RAM BIOS ;
- `_backup_ram` / `.bss.bram` ;
- les commandes BIOS nullsound 1, 2 et 3.

Le dépôt Unsigned contient actuellement `unsigned.mk`, mais pas la configuration complète de toolchain ngdevkit du projet parent. Ce document ne revendique donc pas de version précise du package ou d’un commit ngdevkit. Une release doit enregistrer la révision exacte de ngdevkit/toolchain utilisée par le projet parent et revalider ces points d’intégration lorsque cette dépendance change.
