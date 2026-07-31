#ifndef MELTING_RUN_PREFS_H
#define MELTING_RUN_PREFS_H

#include <stdbool.h>

/* DEC-189/190 (docs/design/governance/decision-log.md; docs/design/ui/
 * options-and-accessibility.md "Volumi"): LE PREFERENZE DEL GIOCATORE,
 * indipendenti da ogni run -- a differenza di catalog/ (meta-progressione
 * per run, src/content/run_catalog.c) e suspend/current.txt (la
 * sospensione della run IN CORSO, src/game/run_suspend.c), questo e' un
 * file solo per profilo che sopravvive a ogni run e a ogni avvio del gioco.
 * Vive in src/app perche' e' cio' che possiede i file: caricato/applicato e
 * salvato SOLO da AppRun/UpdateApp (src/app/app.c), mai da Game (il modulo
 * non tocca game_types.h/Game per nulla, a differenza di run_suspend.c) --
 * lo stesso principio di "chi possiede i file" che tiene AppInput/AppGen in
 * src/app/app_internal.h invece che nel core condiviso.
 *
 * Primo (e finora unico) contenuto: i tre volumi audio (DEC-190, canone).
 *
 * FORMATO -- stessa disciplina di run_catalog.c/run_suspend.c (AGENTS.md,
 * "MAI cJSON nel gioco"): testo chiave=valore, prima riga "prefsSchema=1"
 * (il campo VERSIONE richiesto da DEC-189), scrittura ATOMICA (tmp+rename,
 * pattern RunSuspendWrite). Percorso di default "prefs/settings.txt",
 * accanto a "catalog/" e "suspend/", mai versionato (dati del giocatore,
 * vedi .gitignore).
 *
 * DISCIPLINA ZERO-DEFAULT (DEC-189): un file assente, corrotto, troncato o
 * di uno schema diverso da 1 non produce MAI un crash -- PrefsLoad torna
 * semplicemente il default piu' innocuo (i tre volumi a 1.0, lo stesso
 * default con cui il modulo audio parte oggi anche senza alcun file, vedi
 * il commento su s_audio in src/audio/audio.c) e il file si riscrive al
 * prossimo salvataggio. La granularita' del fallback e' a due livelli: un
 * file assente/troncato/di schema estraneo vale il default PER INTERO; un
 * file di schema buono con un SINGOLO campo corrotto (testo non numerico,
 * nan/inf) recupera i campi sani e rimpiazza solo quello rotto col suo
 * default -- e' il fallback per-campo di ReadFloat, prefs.c. */

/* Versione del formato. Un file con un valore diverso (o senza la riga) e'
   incompatibile: si ignora per intero, mai una lettura parziale -- stesso
   contratto di RUN_SUSPEND_SCHEMA (src/game/run_suspend.h) e di
   RunCatalogWriteRun ("catalogSchema="). */
#define PLAYER_PREFS_SCHEMA 1

/* I tre volumi (DEC-190): stesso range [0,1] e stesso ordine canone
   (generale/musica/effetti, vedi AudioSetMasterVolume/... in audio.h) di
   ogni altro punto del motore che li tocca. */
typedef struct PlayerPrefs {
    float masterVolume;
    float musicVolume;
    float sfxVolume;
} PlayerPrefs;

/* Percorso di test: se non-NULL sostituisce "prefs" in lettura e scrittura.
   Stesso schema di RunSuspendSetTestPath (src/game/run_suspend.h) /
   RunCatalogSetTestPath (src/content/run_catalog.h): permette a
   --prefs-test di lavorare in una cartella temporanea senza mai toccare i
   dati veri del giocatore. NULL ripristina il default. */
void PrefsSetTestPath(const char *path);
const char *PrefsGetTestPath(void);

/* Riempie 'out' coi tre volumi salvati, o col default (1.0/1.0/1.0) se il
   file e' assente, illeggibile, di uno schema diverso da PLAYER_PREFS_SCHEMA
   o con un campo mancante. Ogni valore letto e' clampato in [0,1] -- un file
   manomesso a mano non deve poter produrre un volume fuori banda (stessa
   garanzia di AudioSetMasterVolume/..., audio.h). Non fallisce MAI: se 'out'
   e' NULL non fa nulla. */
void PrefsLoad(PlayerPrefs *out);

/* Scrive 'prefs' su disco con scrittura ATOMICA (tmp+rename, pattern
   RunSuspendWrite): crea la cartella "prefs/" se manca. I tre volumi si
   clampano in [0,1] anche qui, mai fidandosi di un chiamante che dimentichi
   di clampare prima di chiamare. Ritorna false -- mai un crash o un blocco
   per il chiamante -- se il disco rifiuta la scrittura (cartella non
   creabile, permessi, disco pieno): una riga di log su stderr, come
   RunSuspendWrite/RunCatalogWriteRun. */
bool PrefsSave(const PlayerPrefs *prefs);

#endif
