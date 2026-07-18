#ifndef MELTING_RUN_GAME_TESTS_H
#define MELTING_RUN_GAME_TESTS_H

#include "core/game_types.h"

#include <stddef.h>

bool GamePortalRespawnTest(Game *game);

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

/* SOLO manuale (mai in make test): entra nel Piano 0 con gen disabilitata
   (uscita aperta subito) e scatta logs/worldsmelt-floorzero-screen.png,
   stessa tradizione degli altri *ScreenshotTest di questo file. */
bool GameFloorZeroScreenshotTest(Game *game);

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

/* Piano strategico 16/07/2026, sezione tier: AppReadBenchmarkPreset
   (src/app/app.h) e' solo I/O di file (nessun Game, nessuna finestra),
   quindi questo test gira prima di InitWindow come i tre sopra. Vedi
   src/tests/game_tests.c per i dettagli degli scenari coperti. */
bool AppBenchmarkPresetSelfTest(void);

#endif
