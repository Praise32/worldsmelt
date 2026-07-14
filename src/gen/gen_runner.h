#ifndef MELTING_RUN_GEN_RUNNER_H
#define MELTING_RUN_GEN_RUNNER_H

#include "core/game_types.h"

/* Ciclo di vita del processo esterno del generatore (melting-gen o un finto
 * sostituto per i test). Nessuna logica di gioco: solo avvio, sondaggio del
 * progresso, rilevazione della fine e cancellazione. */

typedef enum GenRunnerState {
    GEN_RUNNER_IDLE,
    GEN_RUNNER_RUNNING,
    GEN_RUNNER_SUCCEEDED,
    GEN_RUNNER_FAILED
} GenRunnerState;

typedef struct GenRunner {
    GenRunnerState state;
    GenProgress progress;
    long pid;
    double startTime;
    double timeoutSec;
    char progressPath[256];
} GenRunner;

bool GenRunnerStart(GenRunner *runner, const char *command, unsigned int seed,
                    double timeoutSec, const char *progressPath);

/* Come GenRunnerStart, ma con argomenti IN PIU' dopo "--seed <seed>" (array
 * terminato da NULL; NULL = nessuno, cioe' esattamente GenRunnerStart). Serve
 * allo step B2: il processo di RIPRESA in sottofondo va avviato con
 * "--from-json ... --resume --lua-first ...", non con le sole opzioni di
 * default. Resta qui, e non in app.c, perche' e' questo modulo a possedere
 * fork/exec: app.c non deve costruire una argv da solo. */
bool GenRunnerStartWithArgs(GenRunner *runner, const char *command, unsigned int seed,
                            double timeoutSec, const char *progressPath,
                            const char *const *extraArgs);
void GenRunnerUpdate(GenRunner *runner);
void GenRunnerCancel(GenRunner *runner);

#endif
