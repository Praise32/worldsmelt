#ifndef MELTING_RUN_RUN_CATALOG_H
#define MELTING_RUN_RUN_CATALOG_H

#include "core/game_types.h"

/* M7 (DEC-015/041/045/069, substrato del catalogo persistente v1): UN file
 * di testo per run, in catalog/ alla radice (pattern logs/gen-corpus, mai
 * versionato: catalog/ e' in .gitignore, dati del giocatore). Formato manifest
 * chiave=valore (MAI cJSON nel gioco, AGENTS.md), tmp+rename atomico. Il
 * record e' AUTOSUFFICIENTE per decisione di formato esplicita
 * dell'orchestratore: porta la definizione COMPLETA di ogni contenuto
 * registrato (tutti i parametri, il sorgente Lua dove esiste) perche' una
 * futura riconvalida (DEC-069) deve poter rileggere SOLO questo file --
 * generated/ e' effimero, sovrascritto ad ogni run, e non puo' essere la
 * fonte delle Reliquie.
 *
 * v1 registra SOLO cio' che il giocatore ha DAVVERO incontrato in QUESTA run
 * (temi/mondi dei piani raggiunti, layout stanza, oggetti PRESI, tipi di
 * colpo adottati, nemici/boss DAVVERO spawnati, personaggio generato SE
 * scelto) e SOLO se la run ha contenuto DAVVERO generato (source=local:*,
 * riletto da generated/current_run.txt): una run interamente fallback non
 * scrive nulla (default v1 per la domanda aperta "fallback-usato conta?",
 * vedi systems/save-and-meta-progression.md). UI del Catalogo, museo, punti,
 * preferiti, riconvalida vera = gap di implementazione espliciti, fuori scope
 * qui. */

/* Le tre stringhe di esito che RunCatalogWriteRun scrive nella riga
 * "outcome=" -- testo, non un enum, cosi' un vecchio file di catalogo resta
 * leggibile anche se un futuro enum interno cambiasse ordine (stesso
 * principio di ogni altro campo testuale di questo modulo, coerente col
 * resto del progetto: RarityFromText, EnemyFormName, ecc.). */
#define RUN_CATALOG_OUTCOME_WIN "vittoria"
#define RUN_CATALOG_OUTCOME_LOSS "sconfitta"
#define RUN_CATALOG_OUTCOME_ABANDON "abbandono"

/* Scrive un record di catalogo per la run appena conclusa/abbandonata, se la
 * run ha davvero contenuto generato da registrare (vedi il commento sopra la
 * definizione in run_catalog.c per le guardie complete: manifest assente,
 * source=fallback, nessuna categoria con qualcosa da scrivere). Ritorna il
 * numero di record scritti in QUESTO file (0 se non si e' scritto nulla,
 * MAI un errore per il chiamante -- un fallimento di scrittura o lettura
 * resta silenzioso per il giocatore, solo una riga di log su stderr: vedi
 * AppWriteRunCatalog in src/app/app.c, l'UNICO chiamante di questa funzione,
 * per la garanzia "mai un blocco della transizione a RunResults").
 *
 * 'seed' e' quello dell'AppUi (la fonte di verita' della run, mai un altro
 * seed derivato); 'outcome' una delle tre costanti sopra. 'game' resta
 * const: questa funzione LEGGE lo stato di gioco e i file su disco che la
 * run ha gia' scritto (generated/current_run.txt, generated/chosen_theme.txt,
 * generated/scripts/character_trait.lua), non muta mai Game -- il chiamante
 * (AppWriteRunCatalog) e' il solo punto che scrive il conteggio di ritorno
 * dentro game->catalogRecordsWritten. */
int RunCatalogWriteRun(const Game *game, unsigned int seed, const char *outcome);

/* Inverte l'escaping multi-riga che RunCatalogWriteRun applica ai campi
 * sorgente Lua (un valore di catalogo e' sempre UNA riga sola: '\\' ->
 * "\\\\", newline -> il letterale "\n" a due caratteri, vedi il commento su
 * WriteEscapedValue in run_catalog.c). Esposta per due chiamanti: il test
 * dedicato (--catalog-test, src/tests/catalog_tests.c, per verificare il
 * round-trip del sorgente Lua) e una futura riconvalida (DEC-069) che dovra'
 * rileggere questi stessi campi. 'out' troncato in sicurezza su 'outSize'. */
void RunCatalogUnescapeText(const char *escaped, char *out, int outSize);

#endif
