#ifndef MELTING_RUN_GAME_TESTS_H
#define MELTING_RUN_GAME_TESTS_H

#include "core/game_types.h"

#include <stddef.h>

bool GamePortalRespawnTest(Game *game);

/* W8: il pacchetto artistico originale (assets/art/) -- parser dei tre sapori
   di manifest (spritesheet, tileset, font bitmap) comprese le estensioni,
   manifest rotti, animatore deterministico, cache e voci negative, risoluzione
   a priorita' degli image-id, e degrado quando un asset manca o e' corrotto.
   Le fixture sono in una cartella temporanea, MAI in assets/art/. Gira dopo
   InitWindow perche' gli scenari di caricamento creano texture vere; la parte
   di parsing e di animazione e' pura e non ne avrebbe bisogno. Vedi
   src/tests/art_atlas_tests.c. */
bool GameArtAtlasTest(Game *game);

/* M1a: la macchina a stati canonica dei 9 stati (ui/navigation-map.md).
   Come GamePortalRespawnTest, gira dopo InitWindow ma chiama UpdateApp
   (src/app/app_internal.h) direttamente con AppInput sintetici -- mai
   IsKeyPressed. Vedi src/tests/game_tests.c per l'elenco degli scenari. */
bool GameStatesTest(Game *game);

/* M1b: la sala d'attesa giocabile del Piano 0 (systems/floor-zero.md,
   ui/generation-status.md). Come GameStatesTest, chiama UpdateApp
   direttamente con AppInput sintetici (mai IsKeyPressed) e gira dopo
   InitWindow; usa pero' un AppGen con generazione ABILITATA e
   tests/fake-gen.sh come comando (stesso finto generatore di
   GenRunnerSelfTest sotto), per esercitare davvero la pipeline in
   sottofondo -- non solo il caso "gen disabilitata" gia' coperto da
   GameStatesTest. Vedi src/tests/game_tests.c per i quattro scenari. */
bool GameFloorZeroTest(Game *game);

/* DEC-170 (taglie multiple stile Isaac + telecamera; DEC-009 per il minimo
   garantito). Per 24 seed fissi e per ogni piano 1..5 verifica (a) ogni stanza
   e' una delle cinque classi e non scende sotto il minimo garantito (una
   cella), (b) nessuna sovrapposizione -- ogni cella esistente appartiene a
   esattamente una stanza, e la forma a L ha davvero tre celle contigue di un
   blocco 2x2 -- e tutte e cinque le classi compaiono davvero, (c) il numero di
   CELLE del piano cade nella banda attesa e varia fra piani/seed, (d)
   determinismo (stesso seed => stessa mappa, stesse forme, stesse porte), (e)
   ogni transizione di porta atterra dentro una CELLA OCCUPATA della stanza di
   arrivo, (f) esiste esattamente una stanza boss (2x2 quasi sempre) e una di
   partenza, (g) RoomLayoutBuild alla taglia minima con ogni forma/densita'
   massima resta giocabile e non collassa silenziosamente, (h) la telecamera:
   1x1 ferma, mai un'inquadratura fuori dai bordi, inseguimento monotono e
   agganciato (funzioni pure di world/room_camera.h), (i) le porte esistono
   ESATTAMENTE fra celle adiacenti di stanze diverse, (j) connettivita': dalla
   partenza si raggiunge ogni stanza, (k) l'angolo mancante di una forma a L e'
   solido davvero (il giocatore ci viene respinto da un passo di simulazione
   vero) e la telecamera vi si clampa sulla cella corrente.
   Come GamePortalRespawnTest, gira dopo InitWindow ma non serve la finestra
   per davvero (nessun rendering): 'game' non viene letto, ogni piano si
   rigenera su un Game locale pulito. */
bool GameRoomsTest(Game *game);

/* SOLO manuale (mai in make test): entra nel Piano 0 con gen disabilitata
   (uscita aperta subito) e scatta logs/worldsmelt-floorzero-screen.png,
   stessa tradizione degli altri *ScreenshotTest di questo file. */
bool GameFloorZeroScreenshotTest(Game *game);

/* M4, SOLO manuale: identico a GameFloorZeroScreenshotTest sopra (stesso
   ingresso nel Piano 0, gen disabilitata, uscita gia' aperta), ma chiamato
   da app.c mentre la finestra e' a dimensione del MONITOR (avvio fullscreen
   vero, non la finestra grande di test) -- serve a guardare il layout
   adattivo (spec M4) su una risoluzione reale, non sulla finestra compatta
   di test. Scrive logs/worldsmelt-fullscreen-screen.png. */
bool GameFullscreenScreenshotTest(Game *game);

/* DEC-137, SOLO manuale (--overlay-screenshot-test): scatto dell'HUD in overlay
   sulla game view a tutto schermo, alla risoluzione del monitor (come
   GameFullscreenScreenshotTest), ma in APP_GAMEPLAY con una scena ricca. Scrive
   logs/worldsmelt-overlay-<W>x<H>.png (il nome porta la risoluzione). Serve al
   giudizio di gusto del proprietario, non e' in make test. */
bool GameOverlayScreenshotTest(Game *game);

/* W8: uno scatto per ciascuna delle nove schermate canoniche, con gli asset
   artistici veri (tileset della stanza, HUD in pixel art, cornici 9-patch, font
   da 5 pixel). Scrive logs/worldsmelt-w8-<schermata>.png. Serve al giudizio di
   gusto sul reskin e a vedere in fila se una schermata e' rimasta indietro;
   come gli altri screenshot test NON e' in make test (nessun assert puo' dire
   "questa schermata e' bella"). Vedi src/tests/game_tests.c. */
bool GameArtScreensScreenshotTest(Game *game);

bool GameScriptSandboxTest(Game *game);
bool GameManifestTest(Game *game);
bool GameAtlasFallbackTest(Game *game);

/* Personaggio a strati (src/render/item_layers.h): BuildItemLayers su un
   mix di oggetti costruito a mano (un layer per slot + uno slot in
   overflow), poi lo stesso mix disegnato per davvero con RendererDrawApp.
   Vedi src/tests/game_tests.c per i dettagli. */
bool GameLayerTest(Game *game);

/* Fase 3b VISIVA (src/render/rarity_style.h): screenshot di verifica con un
   oggetto per ciascuna delle quattro rarita', sia equipaggiato (pannello
   "OGGETTI PRESI") sia a terra (pickup col suo anello colorato). Vedi
   src/tests/game_tests.c per i dettagli. Scrive
   logs/melting-run-rarity-screen.png, percorso separato sia da
   logs/melting-run-screen.png (--screenshot-test) sia da
   logs/melting-run-layers-screen.png (--layer-test). */
bool GameRarityScreenshotTest(Game *game);

/* Step C VISIVO (src/core/shot_type.h): screenshot di verifica con un colpo per
   ciascuna delle cinque FORME di resa (piu' un colpo nemico, che resta sempre una
   palla) e l'oggetto che conferisce il tipo di colpo in mano al giocatore. Vedi
   src/tests/game_tests.c per i dettagli. Scrive
   logs/melting-run-shotforms-screen.png, percorso separato da tutti gli altri
   screenshot di test. */
bool GameShotFormsScreenshotTest(Game *game);

/* DEC-170, SOLO manuale (--room-shapes-screenshot-test, mai in make test):
   cerca fra i seed un piano che contenga ciascuna taglia maggiore, ci entra
   col giocatore in un punto scelto e scatta logs/worldsmelt-room-*.png (2x2 al
   centro e nell'angolo dove la telecamera sbatte contro il clamp, 1x2 contro
   il bordo, forma a L in due celle diverse: le due inquadrature fra cui
   interpola). Serve al giudizio di gusto sulla telecamera, che nessun assert
   puo' dare: --rooms-test verifica la geometria, non come si vede. */
bool GameRoomShapesScreenshotTest(Game *game);

bool GenRunnerSelfTest(void);

/* Suite di test della sandbox Lua (src/script/script_sandbox.c): un test
   per ciascuna fuga elencata nella spec, vedi src/tests/script_sandbox_tests.c.
   Non richiede una finestra raylib (a differenza dei test sopra), quindi
   src/app/app.c le richiama PRIMA di InitWindow, come per GenRunnerSelfTest. */
bool ScriptSandboxSelfTest(void);

/* Carica ed esegue un piccolo script deterministico (pairs() su chiavi
   stringa + la RNG del gioco) con il seed dato, e scrive il suo output
   osservabile in 'out'. Usata da --script-determinism-test: la prova di
   determinismo vera confronta l'output di DUE PROCESSI separati con lo
   stesso seed (scripts/test-script.sh), non solo due chiamate nello stesso
   processo. */
bool ScriptSandboxDeterminismProbe(unsigned int seed, char *out, size_t outSize);

/* Suite di test dell'API di gioco a handle e delle callback degli oggetti
   (fase 3a-L2, src/script/script_api.c e src/script/script_items.c): vedi
   src/tests/script_items_tests.c. Come ScriptSandboxSelfTest, non richiede
   una finestra raylib (nessuna delle funzioni esercitate tocca GLFW/OpenGL),
   quindi src/app/app.c la richiama PRIMA di InitWindow. */
bool ScriptItemsSelfTest(void);

/* M6b-2 (DEC-037): suite di test del runtime del trait del personaggio
   generato (src/script/script_character.c): vedi
   src/tests/script_character_tests.c. Come ScriptItemsSelfTest, non
   richiede una finestra raylib, quindi src/app/app.c la richiama PRIMA di
   InitWindow. */
bool ScriptCharacterSelfTest(void);

/* M7 (DEC-015/041/045/069, substrato del catalogo persistente): come
   GameStatesTest, gira DOPO InitWindow (chiama UpdateApp con AppInput
   sintetici) ma ogni scenario costruisce il proprio Game locale invece di
   riusare quello passato (isolamento totale dallo stato che AppRun ha gia'
   caricato da generated/, mai deterministico fra checkout diversi). Vedi
   src/tests/catalog_tests.c per gli otto scenari coperti (guardie di
   esclusione, scrittura vera nei tre esiti, progressivo, atomicita'). */
bool GameCatalogTest(Game *game);

/* M8 (DEC-045, vista Catalogo v1): come GameCatalogTest, gira DOPO InitWindow
   (esercita davvero UpdateApp/RendererDrawApp con AppInput sintetici) e usa
   la STESSA pulizia snapshot-based di catalog/ per non lasciare residui.
   Vedi src/tests/catalog_tests.c per i due scenari (catalogo vuoto, catalogo
   popolato con aggregazione/navigazione/file corrotto). */
bool GameCatalogScreenTest(Game *game);

/* SOLO manuale (mai in make test, stessa tradizione di
   GameFloorZeroScreenshotTest): scrive un catalogo sintetico popolato, apre
   la vista e scatta logs/worldsmelt-catalog-screen.png, poi ripulisce i
   propri file (stessa pulizia snapshot-based degli altri test del
   catalogo). Vedi src/tests/catalog_tests.c. */
bool GameCatalogScreenshotTest(Game *game);

/* DEC-141: l'RNG di gameplay ('game->rng') derivato dal seed di run invece
   che dall'orologio (GameResetRunWithSeed, src/game/game.c). Come
   GameRoomsTest, gira dopo InitWindow ma non serve la finestra per davvero;
   a differenza di GameRoomsTest usa 'game' (quello gia' pronto passato da
   AppRun) per davvero, perche' GameResetRunWithSeed chiama AssetsLoad. Due
   reset con lo stesso seed devono produrre la stessa stanza di
   combattimento (nemici identici dopo un passo di GameUpdate vero), seed
   diversi devono produrne una diversa. Vedi src/tests/game_tests.c. */
bool GameRngSeedTest(Game *game);

/* DEC-144 + DEC-145 (docs/design/systems/items-pools-and-rarity.md):
   estrazione dai pool con pesi di rarita' DEC-019, garanzia di copertura del
   pool curato minimo (DEC-144) e correzione di fortuna con soglia N ridotta
   dalla Fortuna (DEC-145). Come GameRngSeedTest, gira dopo InitWindow (usa
   'game' per davvero: GameResetRunWithSeed chiama AssetsLoad) ma non
   richiede la finestra per disegnare nulla. Vedi src/tests/game_tests.c per
   gli scenari (garanzia numerica sul pool di 20 e sul pool di 15 dell'intera
   run di ripiego, soglia N, correzione avversariale, DISTRIBUZIONE MARGINALE
   dell'estrazione rispetto ai pesi DEC-019, statistica su molti semi che la
   Fortuna abbrevia le sequenze sfortunate, copertura del contenuto di
   ripiego sull'intera run, determinismo). */
bool GameItemPoolTest(Game *game);

/* DEC-167 (docs/design/systems/rewards-and-economy.md): la valuta principale
   arriva da QUALUNQUE stanza completata secondo la propria condizione --
   combattimento ripulito, boss sconfitto, tesoro aperto, negozio visitato.
   Come GameRngSeedTest, gira dopo InitWindow e usa 'game' per davvero
   (GameResetRunWithSeed chiama AssetsLoad), entrando direttamente nelle
   stanze invece di navigare per porte. Vedi src/tests/game_tests.c per i
   quattro scenari (piu' le guardie anti-doppio-pagamento). */
bool GameEconomyTest(Game *game);

/* LA FUSIONE (docs/design/systems/item-fusion.md; DEC-022/023/101/102/143/
   162/171). Come GameEconomyTest, gira dopo InitWindow e usa 'game' per
   davvero (GameResetRunWithSeed chiama AssetsLoad) ma non disegna nulla.
   Sette blocchi, vedi src/tests/game_tests.c: determinismo dal seed,
   categoria della sorgente dominante e tie-break (DEC-143), budget di
   potenza dedicato e tetto di leggibilita' invariato (DEC-162/DEC-146),
   consumo di oggetti e catalizzatore con rifiuti a effetto nullo, immagine
   curata mai ripetuta nella run (DEC-171), ri-fusione di un fuso (DEC-102),
   e il flusso completo dentro BuildScreen attraverso UpdateApp con AppInput
   sintetici. */
bool GameFusionTest(Game *game);

/* SOLO manuale (mai in make test): BuildScreen con la fascia FUSIONE viva --
   sorgenti selezionate, catalizzatore, esito con lo sprite curato -- in
   logs/worldsmelt-fusion-screen.png. Vedi src/tests/game_tests.c. */
bool GameFusionScreenshotTest(Game *game);

/* DEC-065/131/152/159/169 (ui/hud.md, ui/pause-menu.md,
   ui/results-and-leaderboards.md, systems/floor-zero.md): coda delle card di
   scoperta scartata a morte/cambio stanza con il Catalogo intatto (DEC-152),
   HudCombatShouldDraw per stato/prova del Piano 0 (DEC-169), causa della
   sconfitta popolata da un PHASE_GAME_OVER sintetico (DEC-159). Come
   GameEconomyTest, gira dopo InitWindow e usa 'game' per davvero
   (GameResetRunWithSeed chiama AssetsLoad) ma non disegna nulla. Vedi
   src/tests/discovery_tests.c. */
bool GameDiscoveryTest(Game *game);

/* DEC-172 (docs/design/content/audio-and-feedback.md): mappatura pura
   AppMode/piano/stanza-boss -> traccia (AudioTrackForState, nessun bisogno
   di device), ciclo di vita init/shutdown ripetuto senza crash (headless
   compreso: nessun dispositivo audio reale sotto Xvfb), e chiamata di ogni
   AudioPlaySfx/AudioSyncMusic sia PRIMA che DOPO AudioInit senza mai
   andare in crash (il fallback e' lo stesso ramo di codice in entrambi i
   casi, vedi audio.c). Come GameEconomyTest, gira dopo InitWindow ma non
   disegna nulla. Vedi src/tests/audio_tests.c. */
bool GameAudioTest(Game *game);

/* W5b (DEC-153): il formato + loader del pool curato (content/
   curated_catalog.h), il layer di indirezione immagini (content/
   curated_image_map.h + CuratedImagesFindById in content/curated_images.h)
   e la precedenza "curated-content -> fallback deterministico" dentro
   RunContentLoad (content/run_content.c). Non tocca mai assets/
   curated-content/ reale: ogni fixture vive in una cartella temporanea
   propria, isolata via CuratedCatalogSetTestDir (stesso schema di
   RunCatalogSetTestPath in content/run_catalog.h). Non disegna nulla e non
   ha bisogno di GameResetRun/AssetsLoad: 'game' resta per uniformita' con
   ogni altro --xxx-test. Vedi src/tests/curated_content_tests.c. */
bool GameCuratedContentTest(Game *game);

#endif
