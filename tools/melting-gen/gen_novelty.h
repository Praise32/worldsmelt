#ifndef GEN_NOVELTY_H
#define GEN_NOVELTY_H

#include <stddef.h>

struct GenRun;

/* Tetto dell'elenco "da evitare" (~50 token nel prompt a ~2 token/parola in
   media sulle parole inglesi in gioco, DEC-052): budget contenuto apposta,
   lo stesso spirito del blocco ispirazioni di gen_inspire.c (~150 token). */
#define GEN_NOVELTY_MAX_AVOID_WORDS 24

/* Dimensione MINIMA del buffer da passare a GenNoveltyAvoidList perche'
   l'elenco non venga mai troncato a meta' parola nel caso peggiore: fino a
   GEN_NOVELTY_MAX_AVOID_WORDS parole di al massimo 31 caratteri (gli slot a
   32 byte di gen_novelty.c le tagliano li'), separate da ", " (2 byte),
   piu' il terminatore. Il tetto reale (31 + 23*(2+31) = 790 byte) sta sotto
   i 24*34 = 816 di questa formula: il margine copre gli slot pieni fino
   all'ultimo byte. Il chiamante che dimensiona il buffer "a spanne" (es.
   MAX*32) TRONCHEREBBE l'ultima parola a meta' e spedirebbe un token
   spazzatura nel prompt LLM -- usare SEMPRE questa costante. */
#define GEN_NOVELTY_AVOID_BUF_SIZE (GEN_NOVELTY_MAX_AVOID_WORDS * 34 + 1)

/* Ledger di novita' fra RUN (piano strategico, sezione "check contro le
   ultime ~20 run"): i semi d'ispirazione e gli esempi rotanti (gen_inspire.c)
   risolvono la varieta' DENTRO una run, ma non sanno nulla di quello che il
   modello ha gia' scritto IERI -- su decine di run il vocabolario puo'
   comunque riconvergere. La contromossa e' ancora una leva sul modello, non
   una censura: si registra il vocabolario-contenuto di ogni run riuscita in
   logs/novelty-ledger.txt (un file di testo, una riga per run, append-only,
   stesso spirito "singleton di processo aperto pigramente" di gen_corpus.c
   ma qui il file e' condiviso fra TUTTE le run passate, non uno per
   processo: e' la memoria che serve, non la telemetria di UNA generazione) e
   si inietta nel prompt l'elenco delle parole che stanno convergendo, perche'
   il modello le eviti da solo (vedi {EVITA} in gen_llm.c/prompts/user.txt).

   Formato di una riga del ledger (testo semplice, non JSON: lo legge solo
   questo modulo, e un formato piatto e' piu' facile da ispezionare a mano):
     seed=<seed> words=<parola1> <parola2> ... <parolaN>
   'words' sono le parole-contenuto UNICHE di quella run (stopword inglesi e
   parole < 3 caratteri gia' scartate in fase di scrittura, stessa lista di
   scripts/gen_metrics.py:STOPWORDS -- le due liste vanno tenute
   sincronizzate a mano, non esiste un punto unico di verita' condiviso fra
   Python e C in questo repo), separate da un singolo spazio: split "su
   spazi", mai virgole, per restare simmetrico con come si leggono. */

/* Appende una riga al ledger con le parole-contenuto uniche estratte da
   temi, nomi dei colpi, nemici (compreso il nome del boss E del suo tipo
   nemico), stanze e oggetti ATTIVI di 'run' -- MAI dal bossItem, che e'
   sempre procedurale/deterministico sul seed (mai scritto dal modello, vedi
   il commento su GenFloor.bossItem in melting_gen.h) e non deve inquinare la
   misura della creativita' del modello.

   Va chiamata SOLO dal chiamante che sa per certo che QUESTA run e' uscita
   da una generazione del modello riuscita (run->source "local:...", mai
   "fallback" ne' "resume"/"from-json"): un ripiego procedurale userebbe
   sempre lo stesso vocabolario di gen_fallback.c, e la ripresa (--resume)
   non inventa nulla di nuovo, ricopia solo la run gia' scritta dal primo
   processo -- in entrambi i casi la riga avvelenerebbe la lista "da evitare"
   con parole che il modello non ha mai scelto.

   Rispetta MELTING_GEN_NO_CORPUS=1 (stessa classe di effetto persistente di
   gen_corpus.c: le suite di test lanciano melting-gen decine di volte e non
   devono riempire il ledger VERO del giocatore di run finte). Nessun errore
   riportato: un ledger che non si riesce a scrivere non deve far fallire una
   generazione altrimenti riuscita, e' solo un mancato apprendimento. */
void GenNoveltyAppend(const struct GenRun *run);

/* Legge le ULTIME 20 righe del ledger (file assente = *buf vuoto, MAI un
   errore: e' lo stato normale della primissima run mai generata) e scrive in
   'buf' l'elenco, separato da ", ", delle parole che compaiono in ALMENO 2
   di quelle 20 run (una parola vista una volta sola e' varieta', non
   convergenza), ordinate per numero di run decrescente e TAGLIATE a
   GEN_NOVELTY_MAX_AVOID_WORDS parole (~50 token nel prompt: budget
   contenuto, lo stesso spirito di GenInspireBuild). 'buf' e' SOLO l'elenco
   (mai la frase introduttiva "Parole gia' viste..."): quella la compone
   gen_llm.c, che decide anche di ometterla del tutto quando l'elenco e'
   vuoto (niente blocco monco nel prompt). Se 'buf' non basta per l'elenco
   intero OMETTE l'ultima parola che non ci sta TUTTA, mai troncandola a
   meta' (una parola tagliata sarebbe un token spazzatura nel prompt): stesso
   spirito "niente blocco monco", esteso alla singola parola. Dimensionare
   'buf' con GEN_NOVELTY_AVOID_BUF_SIZE per non perderne nessuna. */
void GenNoveltyAvoidList(char *buf, size_t size);

#endif
