#ifndef MELTING_RUN_GAME_TESTS_H
#define MELTING_RUN_GAME_TESTS_H

#include "core/game_types.h"

#include <stddef.h>

bool GamePortalRespawnTest(Game *game);
bool GameScriptSandboxTest(Game *game);
bool GameManifestTest(Game *game);
bool GameAtlasFallbackTest(Game *game);
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

#endif
