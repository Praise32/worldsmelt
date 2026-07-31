#ifndef MELTING_RUN_RUN_SUSPEND_H
#define MELTING_RUN_RUN_SUSPEND_H

#include "core/game_types.h"

/* WP17 (DEC-050, docs/design/systems/save-and-meta-progression.md
 * "Sospensione della run e ripresa"): LA SOSPENSIONE di una run in corso.
 *
 * Il giocatore lascia in qualunque momento ("Sospendi e esci" di `PauseMenu`)
 * e riprende da "Continua" nel menu principale. Al rientro la STANZA CORRENTE
 * RIPARTE DALL'INGRESSO, coi nemici ripristinati -- non esiste uno snapshot di
 * meta' combattimento -- mentre il resto della run (piani gia' fatti, build,
 * oggetti, risorse, prove, tempo) riprende esattamente com'era.
 *
 * ============================================================
 * FORMATO -- un file di testo chiave=valore, stessa disciplina di
 * src/content/run_catalog.c (MAI cJSON nel gioco, AGENTS.md; tmp+rename
 * atomico; zero-default per la retrocompatibilita': una chiave assente vale
 * sempre il significato piu' innocuo). Percorso di default "suspend/current.txt",
 * accanto a "catalog/" e mai versionato (dati del giocatore).
 * ============================================================
 * La PRIMA riga e' "suspendSchema=1" ed e' il campo VERSIONE richiesto dal
 * documento: un file senza quella riga, o con un numero diverso, viene
 * IGNORATO per intero (voce "Continua" assente, nessun crash) -- lo stesso
 * pattern con cui RunCatalogAggregate salta un record di catalogo corrotto.
 *
 * COSA SI SALVA: seed di run, personaggio scelto (indice, piu' la definizione
 * COMPLETA quando e' quello generato per la run), piano e stanza correnti,
 * stato del giocatore (salute, Crust, risorse, statistiche di base, inventario
 * INTERO -- sorgente Lua compreso, cosi' un oggetto fuso o generato si
 * ricostruisce senza dipendere dal manifest), stanze visitate/ripulite/
 * premiate, arene accettate, segrete aperte, distruttibili distrutti, Innesti
 * lasciati a terra, la puntata della Pourhouse (firma di run compresa), le
 * prove della run col loro stato, il numero di fusioni, il tempo di run, i
 * contatori di correzione di fortuna e i flag di archetipo generato.
 *
 * COSA NON SI SALVA (per decisione del documento): la posizione esatta del
 * giocatore nella stanza, i nemici vivi, i colpi/particelle, e qualunque
 * stato del Piano 0 (le simulazioni d'arena non sono una run).
 *
 * DETERMINISMO. Il mondo NON viene serializzato: si RICOSTRUISCE dal seed di
 * run (`GameResetRunWithSeed` -> `WorldStartFloor`), e lo stato salvato si
 * applica SOPRA. Perche' la mappa del piano corrente torni identica servono
 * DUE valori di RNG, non uno:
 *   - `rngFloorEntry` = `Game.floorEntryRng`, il valore di `game->rng`
 *     catturato da `WorldStartFloor` PRIMA di generare la mappa. Rimesso li'
 *     prima di rigenerare il piano, produce esattamente la stessa mappa
 *     (WorldGenerateFloorMap pesca da `game->rng`, che nel frattempo il
 *     combattimento ha fatto avanzare).
 *   - `rngNow` = `game->rng` al momento della sospensione. Rimesso DOPO la
 *     rigenerazione, fa proseguire la sequenza di gioco esattamente da dove
 *     era: la ripresa non introduce alcuna divergenza (DEFAULT PROPOSTO
 *     DALL'IMPLEMENTAZIONE, registrato in save-and-meta-progression.md e in
 *     governance/open-questions.md -- il documento non fissa questo punto).
 * ============================================================ */

/* Versione del formato. Un file con un valore diverso (o senza la riga) e'
   incompatibile: si ignora, mai si tenta una lettura parziale. */
#define RUN_SUSPEND_SCHEMA 1

/* Percorso di test: se non-NULL sostituisce "suspend" in scrittura, lettura e
   cancellazione. Stesso schema di RunCatalogSetTestPath (src/content/
   run_catalog.h): e' cio' che permette a --suspend-test di lavorare in una
   cartella temporanea senza mai toccare i dati del giocatore. NULL ripristina
   il default. */
void RunSuspendSetTestPath(const char *path);
const char *RunSuspendGetTestPath(void);

/* Scrive la sospensione della run in corso. Ritorna false -- e non lascia
   nessun file a meta', grazie al tmp+rename -- se 'game' non e' in una run
   vera (game->floor < 1: il Piano 0 non e' sospendibile, vedi il LIMITE
   DICHIARATO in docs/engineering/known-issues.md) o se il disco rifiuta la
   scrittura. Mai un blocco per il chiamante: un fallimento e' una riga di log
   su stderr, come in RunCatalogWriteRun. */
bool RunSuspendWrite(const Game *game);

/* Vero se esiste una sospensione VALIDA (file presente, schema compatibile,
   piano/stanza dentro i limiti, i due stati RNG non nulli). E' l'unica fonte
   della visibilita' della voce "Continua" nel menu principale: un file
   corrotto, troncato o di una versione diversa risponde falso -- voce assente,
   file ignorato, MAI un crash. Legge il file su disco: chiamarla nei punti in
   cui la sospensione puo' essere cambiata, MAI per-frame (stessa disciplina
   di RunCatalogAggregate). */
bool RunSuspendIsAvailable(void);

/* Ricostruisce in 'game' la run sospesa e la CONSUMA (il file viene cancellato
   in caso di successo: una sospensione si consuma, morire dopo la ripresa non
   deve permettere di ricaricare). Ritorna false, lasciando 'game' intatto, se
   non esiste una sospensione valida.
   Al ritorno 'game' e' in `PHASE_PLAY` nel piano/stanza salvati, col giocatore
   all'INGRESSO della stanza corrente e i nemici ri-materializzati: il chiamante
   deve solo entrare in `APP_GAMEPLAY`. */
bool RunSuspendResume(Game *game);

/* Cancella la sospensione senza leggerla: l'abbandono di una run (DEC-089,
   WP19) e "Nuova run" con una sospensione attiva la buttano via. Sicura anche
   se il file non esiste. */
void RunSuspendClear(void);

#endif
