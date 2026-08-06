#ifndef MELTING_GEN_VISUALSPEC_H
#define MELTING_GEN_VISUALSPEC_H

/* gen_visualspec.{h,c}: R1 (mandato del proprietario 06/08, bake-off S1-S4 vs
   architettura di prompting). Stesso schema di gen_attacks.c (prompt ->
   modello -> valida -> ritenta, sessione GIA' aperta dal chiamante, nessun
   flag nuovo per il modello), ma il dominio e' un terzo: non uno script Lua
   di un attore della demo, un OGGETTO JSON appaiato -- uno SPEC strutturato
   (vincolato da grammatica GBNF) e un FREE_PROMPT libero (senza grammatica),
   stesso soggetto, due architetture di prompting a confronto. Il confine con
   l'altro cantiere (harness runtime, immagini vere) e' il file scritto qui:
   <outDir>/visualspecs/batch.json, contratto CONGELATO dall'orchestratore:

   { "version":1, "seed":N, "model":"...", "requests":[
       { "id":"<domain>_<nn>", "domain":"character|enemy|weapon|item|boss_part",
         "spec":{ "category":"<=domain>", "subtype":"1-4 parole",
                   "body_plan":"1-4 parole", "materials":["2-4 voci"],
                   "distinctive_feature":"frase breve",
                   "size_class":"small|medium|large" },
         "free_prompt":"prompt SD COMPLETO in inglese, <=330 caratteri" } ] }

   10 richieste per dominio di default (5 domini, 50 totali): per OGNI
   richiesta il modello produce SIA lo spec SIA il free_prompt -- due
   generazioni indipendenti sullo STESSO soggetto (il free_prompt riceve
   subtype/materials appena generati per lo spec come brief, cosi' la coppia
   parla davvero della stessa cosa), perche' la coppia stessa e' il punto del
   confronto lato harness, non un dettaglio di implementazione qui.

   COSA VUOL DIRE "COMPLETO" (precisazione del contratto, correzione del
   06/08 -- prima versione bocciata proprio qui): il free_prompt e' il prompt
   INTERO che l'harness manda a sd-cli, vista e sfondo compresi. Nel braccio
   "spec" a valle esiste un template deterministico (scripts/
   visualspec_template.py) che aggiunge vista top-down three-quarter,
   soggetto singolo e sfondo grigio piatto; nel braccio "free" a valle NON
   esiste NIENTE -- e' l'architettura "il gioco chiede a Gemma un prompt e lo
   passa a SD cosi' com'e'". Un free_prompt di solo soggetto renderebbe il
   confronto una misura di "con template tecnico vs senza", non delle due
   architetture: il postprocesso (rimozione sfondo per flood-fill + rimappa
   di palette, scripts/teacher_bench_post.py) e' lo STESSO per i due bracci e
   pretende lo sfondo grigio piatto, quindi chiederlo a uno solo dei due
   predetermina il verdetto. Percio' ValidateFreePrompt ESIGE (non "suggerisce
   nel prompt di sistema": vedi la lezione sulle parole di sistema piu' sotto)
   che il testo dichiari vista, soggetto singolo e sfondo. Cio' che resta
   diverso fra i due bracci -- ed e' la variabile vera dell'esperimento -- e'
   CHI compone la frase: un template fisso a partire da sei campi, oppure il
   modello in prosa libera.

   Due responsabilita' pubbliche: GenVisualSpecGenerateBatch (il ciclo
   completo domini x indici -> batch.json) e GenVisualSpecPromptBudgetCheck
   (la guardia byte-budget del prompt, senza modello, come per le altre due
   famiglie di prompt del tool). */

#include "melting_gen.h"

#include <stdbool.h>
#include <stddef.h>

/* I cinque domini del contratto, in quest'ordine fisso (e' anche l'ordine in
   cui il batch li genera: character, poi enemy, poi weapon, poi item, poi
   boss_part -- l'harness non dipende dall'ordine, ma un ordine fisso rende
   deterministico anche l'ID "<domain>_<nn>", non solo il contenuto). */
#define GEN_VISUALSPEC_DOMAIN_COUNT 5
extern const char *GEN_VISUALSPEC_DOMAINS[GEN_VISUALSPEC_DOMAIN_COUNT];

/* N per dominio: <=0 = "non specificato", ricade sul default 10; >20 =
   clampato a 20. Non c'e' un minimo diverso da "qualunque valore positivo" --
   anche N=1 e' un giro legittimo (il giro reale minimo del task brief usa
   N=2). Questo e' il contratto della FUNZIONE, per un chiamante
   programmatico: la CLI (--visualspecs N, main.c) NON ci arriva mai con <=0
   perche' valida prima e rifiuta con un errore esplicito. La distinzione
   conta: il ramo --visualspecs e' un'uscita anticipata, e un valore non
   numerico accettato in silenzio come "0 = default" farebbe cadere il
   processo nella generazione di una run COMPLETA, che cancella e riscrive
   generated/scripts (bug reale della prima versione). Per lo stesso motivo
   la CLI RIFIUTA anche il fuori-scala verso l'alto invece di clampare in
   silenzio: chi scrive "--visualspecs 40" ha chiesto una cosa precisa, e
   riceverne un'altra senza che nessuno lo dica e' peggio di un errore. Il
   clamp qui sotto resta per il chiamante programmatico. */
#define GEN_VISUALSPEC_PER_DOMAIN_DEFAULT 10
#define GEN_VISUALSPEC_PER_DOMAIN_MAX 20

/* Capienza dei buffer d'errore: stesso ordine di grandezza di
   GEN_ATTACK_ERR_CAP (gen_attacks.h) per lo stesso motivo -- l'errore non e'
   solo per il log, e' l'UNICO testo che il ritento rimanda al modello (vedi
   GenVisualSpecGenerateBatch), e deve poter portare sia il messaggio di
   validazione sia, per l'anti-fotocopia, il subtype gia' accettato con cui
   collide. */
#define GEN_VISUALSPEC_ERR_CAP 384

/* n_predict per le due generazioni indipendenti di UNA richiesta. Lo spec
   (job 1, grammatica GBNF) e' un oggetto JSON piccolo -- sei campi, il piu'
   lungo dei quali e' una frase di 120 caratteri -- quindi basta molto meno
   di GEN_ATTACK_N_PREDICT (768, gen_attacks.h: quello e' un intero on_tick
   con macchina a stati). Il free_prompt (job 2, senza grammatica) e' un
   singolo paragrafo (vedi GEN_VISUALSPEC_FREE_PROMPT_MAX_CHARS sotto per il
   tetto in caratteri): un margine largo per un testo cosi' corto, mai
   stretto quanto quello dello spec strutturato. */
#define GEN_VISUALSPEC_SPEC_N_PREDICT 220
#define GEN_VISUALSPEC_FREE_N_PREDICT 160

/* Tentativi TOTALI per stadio (tentativo iniziale piu' i ritenti, con
   l'errore di validazione dell'ultimo rimandato al modello nel prompt del
   successivo). Lo stadio (a) sta sui 3 di GEN_ATTACK_MAX_ATTEMPTS/
   GEN_LUA_MAX_ATTEMPTS: la grammatica gli garantisce gia' la forma, gli
   restano da rispettare due regole di contenuto (nomenclatura, anti-
   fotocopia) e infatti sbaglia quasi solo su quella anti-fotocopia, al
   primo colpo.
   Lo stadio (b) ne ha 5, e non e' una toppa sulla difficolta': e' l'unico
   punto del tool dove il modello deve azzeccare TRE requisiti ortogonali
   nello stesso testo (vista, soggetto singolo, sfondo) piu' lunghezza,
   charset e vocabolario, senza grammatica che lo aiuti. Misurato sul giro
   vero del 06/08: ~50% di primi tentativi respinti, quasi sempre risolti al
   secondo -- con 3 tentativi restavano comunque 2 richieste perse su 10, e
   una richiesta persa qui non e' un'immagine in meno, e' una COPPIA in meno
   (lo spec valido viene buttato con lei) e un id in meno per le coppie di
   fusione dell'harness a valle. */
#define GEN_VISUALSPEC_MAX_ATTEMPTS 3
#define GEN_VISUALSPEC_FREE_MAX_ATTEMPTS 5

/* Temperatura di ENTRAMBI gli stadi (mandato R1): qui conta la varieta' fra
   le 10 richieste di uno stesso dominio, non la fedeltà a un formato --
   diversa dal 0.6 "conta solo la correttezza" di GEN_ATTACK_TEMP
   (gen_attacks.h), perche' li' il compito e' un on_tick sintatticamente
   fragile, qui sono sei campi corti e un paragrafo, molto piu' tolleranti. */
#define GEN_VISUALSPEC_TEMP 0.8f

/* Tetto del free_prompt. Un conteggio in BYTE, non in caratteri Unicode --
   il prompt e' vincolato ASCII puro (DEC-052, stesso principio del resto di
   questo tool), quindi le due misure coincidono per costruzione.

   330 e non i 280 del mandato: DEVIAZIONE DELIBERATA, misurata, dallo stesso
   argomento che ha imposto il free_prompt completo (vedi la testata). I 280
   erano il tetto giusto per un free_prompt di SOLO SOGGETTO; ora che il
   testo deve portare anche vista, soggetto singolo e sfondo (~65 caratteri
   di contenuto obbligatorio), lo stesso numero diventa un handicap
   sistematico per un braccio solo: i prompt che il template compone per il
   braccio "spec" misurano 293-326 caratteri sul batch vero del 06/08, senza
   nessun tetto, mentre il braccio libero doveva stare sotto 280. Togliere un
   confondimento e lasciarne un altro sulla stessa variabile non serve a
   niente. Il costo del vecchio numero era misurabile: 7 tentativi su 33
   respinti per lunghezza e 3 richieste su 10 perse del tutto.
   330 e' il tetto dell'intervallo dell'altro braccio, non un numero comodo. */
#define GEN_VISUALSPEC_FREE_PROMPT_MAX_CHARS 330

/* Derivazione IDENTICA a GEN_LUA_PROMPT_BYTE_CEILING (gen_lua.h) e a
   GEN_ATTACK_PROMPT_BYTE_CEILING (gen_attacks.h) -- stessa formula, stesso
   *16/5 volutamente generoso sul rapporto byte/token misurato. Il n_predict
   sottratto e' quello dello stadio piu' AFFAMATO dei due (lo spec, 220):
   fra due stadi che condividono lo stesso system prompt il tetto giusto e'
   il piu' stretto, non uno per stadio -- due ceiling diversi si
   sfaserebbero al primo che qualcuno tocca. */
#define GEN_VISUALSPEC_PROMPT_BYTE_CEILING (((GEN_LLM_SESSION_N_CTX) - (GEN_VISUALSPEC_SPEC_N_PREDICT)) * 16 / 5)

/* Il ciclo domini x indici -> batch.json. 'sess' e' GIA' aperta dal
   chiamante (main.c, riusa --model/--ngl/--seed/--out gia' esistenti:
   nessun flag nuovo per il modello, stesso principio di GenAttackGenerate).
   'perDomain' e' il valore grezzo di --visualspecs N (clamp/default
   applicati QUI, vedi le due costanti sopra). 'modelLabel' e' gia' risolto
   dal chiamante (stesso "local:<basename>" che main.c compone per
   run->source/provenance -- questo modulo non conosce il percorso
   completo del modello, solo l'etichetta da scrivere nel campo "model").

   Per ciascuna delle perDomain*5 richieste (seed derivato seed+i*97, 'i'
   indice GLOBALE 0-based sull'intero batch, stesso schema di seedI in
   GenAttackGenerate): prima lo SPEC (grammatica GBNF, temp alta per la
   varieta', fino a GEN_VISUALSPEC_MAX_ATTEMPTS tentativi; il gate della
   nomenclatura di gioco respinge campo per campo, e il gate anti-fotocopia
   confronta il subtype
   normalizzato -- minuscolo, spazi collassati -- contro ogni subtype GIA'
   accettato nel batch, in QUALUNQUE dominio, per confronto substring in
   entrambe le direzioni), poi, solo se lo spec valida, il FREE_PROMPT
   (nessuna grammatica, stesso subtype/materials appena accettati passati
   come brief, fino a GEN_VISUALSPEC_FREE_MAX_ATTEMPTS tentativi; valida
   lunghezza, ASCII puro, l'assenza delle parole trigger delle config del
   bake-off -- le aggiunge la pipeline immagine, il modello non deve
   conoscerle -- l'assenza della nomenclatura di gioco e la presenza dei tre
   elementi che rendono il prompt COMPLETO: vista, soggetto singolo, sfondo).
   Una richiesta il cui SPEC valida ma il cui FREE_PROMPT fallisce dopo i
   tentativi viene scartata INTERA (nessuna voce parziale in requests[]: il
   contratto vuole SEMPRE la coppia, mai uno spec orfano) -- ma il suo
   subtype resta comunque "speso" per l'anti-fotocopia delle richieste
   successive, perche' il modello lo ha comunque gia' proposto in questa
   sessione.

   Scrittura ATOMICA di un unico file (tmp+rename, GenPublishFile), a
   differenza del lotto per-file di GenAttackGenerate: il contratto e' UN
   batch.json con l'intero array 'requests', non file indipendenti — quindi
   qui non ha senso pubblicare parzialmente durante il giro, solo alla fine.
   Se NESSUNA richiesta valida entro tutto il batch, il file non viene
   scritto affatto (mai un batch.json con "requests":[] sul disco: e' lo
   stesso principio "carta assente, mai una carta vuota" di
   GenWriteCharacterProposal, DEC-037).

   Logga per ogni tentativo (stadio, esito) su STDOUT, come GenAttackGenerate.
   Ritorna il numero di richieste scritte con successo nel batch (0 = niente
   scritto: il chiamante ne fa l'exit code, vedi main.c). */
int GenVisualSpecGenerateBatch(GenLlmSession *sess, const char *promptsDir, const char *outDir,
                                int perDomain, unsigned int seed, const char *modelLabel);

/* Compone i prompt PIU' GRANDI che questo modulo possa mandare al modello
   (entrambi gli stadi, dominio col nome piu' lungo, campi dello spec ai
   tetti che visualspec.gbnf stesso ammette, blocco di ritento pieno) e
   fallisce se superano GEN_VISUALSPEC_PROMPT_BYTE_CEILING. Nessun modello
   coinvolto: e' la stessa guardia di GenLuaPromptBudgetCheck/
   GenAttackPromptBudgetCheck, agganciata allo stesso --prompt-budget-check
   in main.c cosi' scripts/test-gen.sh la esercita gratis. Senza, un prompt
   che sfora n_ctx si manifesta come tre tentativi identici con
   "generation failed (decode or token limit)" e la richiesta sparisce in
   silenzio dal batch. 'err' (puo' essere NULL/errSize 0) riceve il motivo. */
bool GenVisualSpecPromptBudgetCheck(const char *promptsDir, char *err, size_t errSize);

#endif
