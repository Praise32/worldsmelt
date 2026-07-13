#ifndef MELTING_RUN_GAME_TESTS_H
#define MELTING_RUN_GAME_TESTS_H

#include "core/game_types.h"

#include <stddef.h>

bool GamePortalRespawnTest(Game *game);
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

#endif
