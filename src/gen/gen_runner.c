#include "gen/gen_runner.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32

/* Su Windows la generazione resta esterna (.bat), come oggi: nessun fork
 * POSIX disponibile, quindi il modulo si riduce a uno stub che fallisce
 * sempre in modo pulito e non trascina header POSIX nella build Windows. */

bool GenRunnerStart(GenRunner *runner, const char *command, unsigned int seed,
                    double timeoutSec, const char *progressPath)
{
    (void)command; (void)seed; (void)timeoutSec; (void)progressPath;
    memset(runner, 0, sizeof(*runner));
    runner->state = GEN_RUNNER_FAILED;
    return false;   /* su Windows la generazione resta esterna (.bat), come oggi */
}

void GenRunnerUpdate(GenRunner *runner) { (void)runner; }
void GenRunnerCancel(GenRunner *runner) { (void)runner; }

#else

#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static double NowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

/* Rilegge il file di progresso scritto atomicamente dal generatore (temp file
 * + rename in Task 4): se manca ancora (il figlio non ha scritto nulla) non
 * e' un errore, si lascia lo stato precedente. */
static void ReadProgress(GenRunner *runner)
{
    FILE *f = fopen(runner->progressPath, "r");
    if (!f) return;
    char line[192];
    if (fgets(line, sizeof(line), f))
    {
        char phase[32] = { 0 };
        int percent = 0;
        char message[96] = { 0 };
        if (sscanf(line, "%31[^|]|%d|%95[^\n]", phase, &percent, message) >= 2)
        {
            snprintf(runner->progress.phase, sizeof(runner->progress.phase), "%s", phase);
            if (percent < 0) percent = 0;
            if (percent > 100) percent = 100;
            runner->progress.percent = percent;
            snprintf(runner->progress.message, sizeof(runner->progress.message), "%s", message);
        }
    }
    fclose(f);
}

bool GenRunnerStart(GenRunner *runner, const char *command, unsigned int seed,
                    double timeoutSec, const char *progressPath)
{
    memset(runner, 0, sizeof(*runner));
    snprintf(runner->progressPath, sizeof(runner->progressPath), "%s", progressPath);
    remove(progressPath);

    char seedText[32];
    snprintf(seedText, sizeof(seedText), "%u", seed);
    pid_t pid = fork();
    if (pid < 0)
    {
        runner->state = GEN_RUNNER_FAILED;
        return false;
    }
    if (pid == 0)
    {
        execl(command, command, "--seed", seedText, (char *)NULL);
        _exit(127);
    }
    runner->pid = (long)pid;
    runner->state = GEN_RUNNER_RUNNING;
    runner->startTime = NowSeconds();
    runner->timeoutSec = timeoutSec;
    snprintf(runner->progress.phase, sizeof(runner->progress.phase), "avvio");
    snprintf(runner->progress.message, sizeof(runner->progress.message), "avvio del generatore");
    return true;
}

void GenRunnerUpdate(GenRunner *runner)
{
    if (runner->state != GEN_RUNNER_RUNNING) return;
    ReadProgress(runner);
    int status = 0;
    pid_t done = waitpid((pid_t)runner->pid, &status, WNOHANG);
    if (done == (pid_t)runner->pid)
    {
        /* Rilettura finale: il figlio puo' aver scritto l'ultimo progresso
           (100%/"fine" o "errore") dopo la ReadProgress() qui sopra ma prima
           di uscire, altrimenti lo stato finale mostrerebbe una percentuale
           non aggiornata. */
        ReadProgress(runner);
        runner->state = (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            ? GEN_RUNNER_SUCCEEDED : GEN_RUNNER_FAILED;
        return;
    }
    if (NowSeconds() - runner->startTime > runner->timeoutSec)
    {
        GenRunnerCancel(runner);
        snprintf(runner->progress.message, sizeof(runner->progress.message), "tempo scaduto");
    }
}

void GenRunnerCancel(GenRunner *runner)
{
    if (runner->state != GEN_RUNNER_RUNNING) return;
    kill((pid_t)runner->pid, SIGTERM);
    for (int i = 0; i < 20; i++)
    {
        if (waitpid((pid_t)runner->pid, NULL, WNOHANG) == (pid_t)runner->pid)
        {
            runner->state = GEN_RUNNER_FAILED;
            return;
        }
        struct timespec ts = { 0, 50L*1000L*1000L };
        nanosleep(&ts, NULL);
    }
    kill((pid_t)runner->pid, SIGKILL);
    waitpid((pid_t)runner->pid, NULL, 0);
    runner->state = GEN_RUNNER_FAILED;
}

#endif
