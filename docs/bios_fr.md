# Intégration du BIOS Neo Geo

Unsigned conserve les règles du cycle de vie du BIOS dans `engine/system/neogeo`. Le code du jeu travaille avec des phases d’exécution et des états de joueur typés ; il ne lit pas directement la RAM du BIOS et ne recrée pas les événements BIOS à partir des entrées des contrôleurs.

## Frontière du cycle de vie

| Requête BIOS | Entrée ngdevkit | Comportement à l’exécution |
| --- | --- | --- |
| USER 0 | `rom_mvs_startup_init` | géré par le code de démarrage de ngdevkit |
| USER 1 | `main()` | eye catcher optionnel ; aucune initialisation du jeu |
| USER 2 | `main()` | initialisation, ATTRACT, GAME/GAME_OVER optionnels, arrêt |
| USER 3 | `main_mvs_title()` | initialisation, TITLE, GAME/GAME_OVER optionnels, arrêt |

`BIOS_USER_REQUEST` est interprété au moyen du type interne `UNeoGeoBiosRequest`, partagé par les couches runtime et callbacks Neo Geo. `UNeoGeoPhase` est un concept destiné au code du jeu : une seule requête USER peut exécuter plusieurs phases.

## Callbacks BIOS

`PLAYER_START` et `COIN_SOUND` sont des événements BIOS faisant autorité. Unsigned ne les synthétise pas à partir des changements d’état des contrôleurs ni de `BIOS_COMPULSION_TIMER`.

Une liaison runtime possède trois états internes :

- `UNBOUND` : aucun runtime USER 2/3 n’est actif ; les requêtes START sont refusées ;
- `INITIALIZING` : `initialize()` est en cours d’exécution ; les requêtes START sont refusées ;
- `READY` : l’initialisation est terminée ; les requêtes START peuvent être filtrées par `accept_start`.

Cela empêche un callback VBlank/BIOS de faire entrer l’application dans GAME alors que son contexte n’est encore que partiellement initialisé.

### Échange PLAYER_START

`neo_geo_bios_process_start()` réalise la partie cartouche de l’échange :

1. lire les bits de requête P1..P4 dans `BIOS_START_FLAG` ;
2. retirer les joueurs déjà en état `PLAYING` ;
3. refuser l’ensemble de la requête si le runtime n’est pas `READY` ;
4. laisser `accept_start` supprimer des bits de requête supplémentaires ;
5. réécrire dans `BIOS_START_FLAG` uniquement les bits acceptés ;
6. préparer `BIOS_CREDIT_DEC1..4` pour les joueurs acceptés ;
7. faire passer les joueurs acceptés à l’état `PLAYING` ;
8. définir `BIOS_USER_MODE = GAME` lorsqu’au moins un joueur a été accepté.

Le BIOS reste responsable du retrait effectif des crédits MVS après le retour du callback.

## État des joueurs

Unsigned utilise directement les valeurs BIOS de `PLAYER_MOD` :

| État | Signification |
| --- | --- |
| `NEVER_PLAYED` | le slot n’a encore jamais participé |
| `PLAYING` | joueur actif en partie |
| `CONTINUE` | en attente d’un continue ; maintient la session GAME active |
| `GAME_OVER` | état terminal pour ce joueur |

Le code du jeu modifie l’état des joueurs au moyen de transitions contrôlées :

- `unsigned_neo_geo_player_begin_continue()` : `PLAYING -> CONTINUE` ;
- `unsigned_neo_geo_player_game_over()` : `PLAYING/CONTINUE -> GAME_OVER` ;
- un `PLAYER_START` BIOS accepté ultérieurement fait passer `CONTINUE -> PLAYING`.

`unsigned_neo_geo_request_game_over()` applique la transition terminale à tous les joueurs ayant participé. `unsigned_neo_geo_end_session()` ferme en plus la session runtime locale.

## Crédits MVS

Le runtime générique modélise uniquement les compteurs de crédits standards P1/P2.

- au Japon, P1 utilise une réserve de crédits partagée ;
- sur les MVS US et Europe, P1 et P2 utilisent des réserves séparées ;
- la propriété des crédits P3/P4 n’est pas supposée, car leur routage à 4 joueurs dépend de l’extension BIOS/système utilisée.

`credits.c` ne fait que lire les compteurs de crédits pour l’interface utilisateur et écrire dans `BIOS_CREDIT_DEC1..4`. Il ne décrémente jamais directement les compteurs de crédits stockés en backup RAM.

`unsigned_neo_geo_set_game_start_compulsion(bool enabled)` constitue l’exception explicite destinée à la configuration de la borne. Sur MVS, cette fonction déverrouille la backup RAM du BIOS, écrit le réglage persistant GAME START COMPULSION, puis reverrouille la backup RAM. `false` sélectionne `WITHOUT` ; `true` active le démarrage forcé. Sur AES, la fonction ne fait rien. Comme ce réglage persistant concerne toute la borne, le jeu ne doit l’appeler que volontairement.

Les fonctions associées à la configuration de la borne sont :

- `unsigned_neo_geo_game_start_compulsion_enabled()` lit l’état actuel du démarrage forcé ;
- `unsigned_neo_geo_game_start_compulsion_seconds()` expose le timer BIOS en BCD sous forme de secondes normales ;
- `unsigned_neo_geo_set_game_start_compulsion_seconds()` enregistre une valeur de 0 à 99 secondes sous forme BCD dans le BIOS ;
- `unsigned_neo_geo_demo_sound_enabled()` indique si le son des démos est activé au niveau de la borne (`true` sur AES).

La fonction liée au son des démos ne remplace pas une éventuelle logique de soft DIP propre au jeu. Toutes les fonctions de modification ci-dessus changent la backup RAM persistante du BIOS de la borne et doivent donc être utilisées volontairement.

## Commandes audio

Le pilote audio réserve les commandes `0..3` :

| Commande | Utilisation |
| --- | --- |
| 0 | inutilisée / aucune commande |
| 1 | préparation au changement de ROM |
| 2 | eye catcher ngdevkit |
| 3 | réinitialisation du pilote/YM2610 |
| 4+ | commandes du jeu |

L’audio générique accepte uniquement les commandes `>= U_AUDIO_COMMAND_FIRST_GAME`. Le code de cycle de vie spécifique à la plateforme n’accède aux commandes réservées qu’au moyen de fonctions du backend Neo Geo telles que `neo_geo_audio_reset()`.

Le registre 68k `REG_SOUND` est un latch d’un octet. Unsigned n’autorise donc pas le gameplay à y écrire directement. Les commandes audio normales entrent dans la file de transport Neo Geo, et au maximum une commande est écrite immédiatement après chaque VBlank terminée. Un événement `COIN_SOUND` qui ne peut pas provoquer un transfert de ROM par le BIOS est malgré tout envoyé immédiatement et réserve le prochain créneau normal du transport. Le pilote nullsound copie déjà les commandes de jeu acceptées dans sa propre FIFO côté Z80.

Le comportement MVS GAME START COMPULSION est différent. Le cycle de vie du démarrage forcé de SNK demande USER 3/Title, et la carte peut sélectionner temporairement les ROM FIX/SM1 de la carte mère. La commande 1 de nullsound est précisément prévue pour ce changement de ROM : elle arrête le son, réinitialise le YM2610 et attend en RAM Z80. Cette même sélection matérielle change également la source FIX, ce qui explique pourquoi MAME peut afficher un bref flash visuel exactement au moment où un échantillon du premier crédit, joué trop tôt, est interrompu.

Unsigned traite donc l’insertion d’une pièce comme un événement, et non comme un son qui doit survivre au changement de ROM :

1. tant que USER 2/ATTRACT est actif avec GAME START COMPULSION, le callback vérifie les signaux de transition BIOS (`BIOS_USER_REQUEST`, l’état de compulsion et les crédits actuels). Seule une pièce qui provoque réellement l’entrée dans TITLE par démarrage forcé est différée ; une insertion partielle qui n’a pas encore produit de crédit reste immédiate. Un événement différé incrémente un petit jeton de transfert dans le bloc de backup de la cartouche (`.bss.bram`), que ngdevkit n’efface pas pendant l’initialisation du runtime C de USER 3 ;
2. USER 3 démarre après que ngdevkit a de nouveau sélectionné `CRTFIX` et le M1 de la cartouche ;
3. USER 3 envoie la commande audio BIOS 3, comme requis pour initialiser le nouveau pilote M1 nullsound sélectionné ;
4. le jeton persistant est consommé immédiatement et armé comme état de transport volatile ;
5. au premier VBlank terminé, la commande de pièce est envoyée avant l’audio normal de TITLE/GAME. Cette frontière de frame constitue une frontière de protocole après la commande 3, et non un délai arbitraire.

Lorsque GAME START COMPULSION est réglé sur `WITHOUT`, USER 3 ne fait pas partie de ce transfert de démarrage forcé lié au premier crédit ; le chemin `COIN_SOUND` immédiat normal reste donc inchangé.

`UAudioManager` conserve localement une commande déjà résolue lorsque la file de la plateforme est pleine et la réessaie ultérieurement. Ainsi, la contre-pression du transport ne supprime jamais silencieusement une transition musicale ou une commande d’effet sonore.

USER 2 et USER 3 réinitialisent tous deux le pilote audio après la sélection de `CRTFIX`/M1. USER 2 efface également tout état de transfert devenu obsolète. USER 3 rejoue ensuite tout événement de pièce différé spécifiquement pour la transition de ROM liée au démarrage forcé.

## Responsabilités internes

| Fichier | Responsabilité |
| --- | --- |
| `runtime.c` | distribution des requêtes BIOS USER et enchaînement ATTRACT/TITLE/GAME/GAME_OVER |
| `bios_callbacks.c` | callbacks ABI de la cartouche et état de liaison du runtime |
| `session.c` | accès à `PLAYER_MOD`, transitions des joueurs et durée de vie de la session |
| `credits.c` | coût de démarrage et routage standard des crédits MVS P1/P2 |
| `input.c` | snapshots des contrôleurs BIOS vers `UInputManager` |
| `audio_backend.c` | transport sérialisé de `REG_SOUND`, priorité du son de pièce BIOS et commandes audio réservées à la plateforme |
| `save_backend.c` | opérations BIOS de carte mémoire / backup RAM |
| backends du renderer | accès au matériel Neo Geo pour la VRAM, les palettes et les sprites |

Les headers internes suivent les mêmes frontières : `bios_callbacks_internal.h`, `bios_state_internal.h`, `session_internal.h` et `credits_internal.h`.

## Compatibilité ngdevkit

Le build est verrouillé sur la version de package ngdevkit `0.5` via `NGDEVKIT_REQUIRED_VERSION` dans le Makefile. Lors d’un changement de version de ngdevkit, il faut au minimum revérifier :

- le dispatch USER de `runtime/ngdevkit-crt0.S` ;
- les wrappers `PLAYER_START` et `COIN_SOUND` ;
- `include/ngdevkit/bios-ram.h` ;
- les exemples officiels de gestion des crédits et de carte mémoire.

Les builds source/nightly de ngdevkit peuvent partager le même numéro de version de package ; les builds de release doivent également enregistrer la révision git exacte de ngdevkit utilisée par la toolchain.
