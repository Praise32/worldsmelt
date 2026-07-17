#ifndef MELTING_RUN_APP_H
#define MELTING_RUN_APP_H

#include <stdbool.h>
#include <stddef.h>

int AppRun(int argc, char **argv);

/* Piano strategico 16/07/2026, sezione tier: decide se applicare DA SOLA il
 * preset --low-spec e quale messaggio (se c'e') mostrare in gioco, leggendo
 * 'path' (di norma "logs/benchmark.txt", scritto da scripts/benchmark.sh via
 * "make benchmark" -- vedi il formato in quello script). Override manuale:
 * se manualLowSpec o manualFullSpec sono true (l'utente ha passato
 * --low-spec o --full-spec a riga di comando) il file NON viene nemmeno
 * letto, il benchmark si ignora del tutto -- la scelta e' gia' fatta.
 *
 * *lowSpecOut riceve true SOLO quando il preset va applicato per via del
 * benchmark (tier=lowspec): il chiamante lo inizializza a manualLowSpec
 * PRIMA di chiamare, cosi' un --low-spec esplicito (gia' true) resta true
 * per costruzione anche se questa funzione ritorna subito (override, sopra).
 * msgOut (buffer di msgCap byte, msgCap>0) riceve il messaggio da passare a
 * GameSetMessage, o stringa vuota ("") se non c'e' nulla da dire (file
 * assente/illeggibile, benchSchema diverso da 1, o tier=full: nessuna
 * azione, e' il comportamento di sempre).
 *
 * Nessuna finestra ne' Game richiesti (solo I/O di file): testabile prima di
 * InitWindow, vedi AppBenchmarkPresetSelfTest in src/tests/game_tests.c. */
void AppReadBenchmarkPreset(const char *path, bool manualLowSpec, bool manualFullSpec,
                             bool *lowSpecOut, char *msgOut, size_t msgCap);

#endif
