#include "gen_novelty.h"

#include "melting_gen.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GEN_NOVELTY_LEDGER_PATH "logs/novelty-ledger.txt"
/* Quante run passate contano per la convergenza (piano strategico, "check
   contro le ultime ~20 run"). */
#define GEN_NOVELTY_LOOKBACK_RUNS 20
/* Tetto DIFENSIVO di parole-contenuto uniche per riga: una run (5 piani, un
   tema + un colpo + due nemici + un boss (nome+tipo) + una stanza + tre
   oggetti attivi, ciascuno di 1-3 parole) non ne produce mai piu' di una
   sessantina; il margine e' solo per non troncare in silenzio se un domani
   qualche campo crescesse. */
#define GEN_NOVELTY_MAX_WORDS_PER_RUN 64

/* Stessa lista di scripts/gen_metrics.py:ITALIAN_STOPWORDS -- le due liste
   vanno tenute sincronizzate a mano (vedi il commento in gen_novelty.h). */
static const char *STOPWORDS[] = {
    "di", "del", "della", "dei", "delle", "in", "a", "al", "alla", "la", "il",
    "le", "i", "lo", "gli", "un", "una", "e", "che", "con", "per", "da", "su",
    "tra", "fra",
};
#define STOPWORD_COUNT ((int)(sizeof(STOPWORDS)/sizeof(STOPWORDS[0])))

static int IsStopword(const char *w)
{
    for (int i = 0; i < STOPWORD_COUNT; i++) if (strcmp(w, STOPWORDS[i]) == 0) return 1;
    return 0;
}

/* Insieme di parole-contenuto UNICHE di UNA run: stesso spirito di PickInto
   in gen_inspire.c (dedup su un tetto piccolo), qui pero' costruito
   accumulando invece che campionando. */
typedef struct {
    char words[GEN_NOVELTY_MAX_WORDS_PER_RUN][32];
    int count;
} GenNoveltyWordSet;

static void WordSetAdd(GenNoveltyWordSet *ws, const char *word)
{
    if (word[0] == '\0') return;
    for (int i = 0; i < ws->count; i++) if (strcmp(ws->words[i], word) == 0) return;
    if (ws->count >= GEN_NOVELTY_MAX_WORDS_PER_RUN) return;   /* tetto difensivo, vedi sopra */
    snprintf(ws->words[ws->count], sizeof(ws->words[0]), "%s", word);
    ws->count++;
}

/* Spezza 'text' su SPAZI (mai su apostrofi o trattini: i nomi di questo
   generatore usano l'apostrofo al posto dell'accento, "d'ottone" resta una
   parola sola), abbassa a minuscolo (ASCII puro: system.txt chiede nomi
   "senza accenti", quindi tolower basta), scarta le stopword e le parole
   sotto le 3 lettere, e aggiunge il resto a 'ws' (gia' deduplicato). */
static void WordSetAddFromText(GenNoveltyWordSet *ws, const char *text)
{
    if (!text || !text[0]) return;
    char copy[128];
    snprintf(copy, sizeof(copy), "%s", text);
    char *tok = strtok(copy, " ");
    while (tok)
    {
        for (char *c = tok; *c; c++) *c = (char)tolower((unsigned char)*c);
        if (strlen(tok) >= 3 && !IsStopword(tok)) WordSetAdd(ws, tok);
        tok = strtok(NULL, " ");
    }
}

void GenNoveltyAppend(const struct GenRun *run)
{
    if (!run) return;
    /* Stessa guardia di gen_corpus.c: le suite di test lanciano melting-gen
       decine di volte, il ledger VERO del giocatore non deve vederle. */
    if (getenv("MELTING_GEN_NO_CORPUS")) return;

    GenNoveltyWordSet ws = { .count = 0 };
    for (int f = 0; f < GEN_FLOORS; f++)
    {
        const GenFloor *fl = &run->floors[f];
        WordSetAddFromText(&ws, fl->theme);
        WordSetAddFromText(&ws, fl->shot.name);
        WordSetAddFromText(&ws, fl->enemies[0].name);
        WordSetAddFromText(&ws, fl->enemies[1].name);
        WordSetAddFromText(&ws, fl->boss);
        WordSetAddFromText(&ws, fl->bossType.name);
        WordSetAddFromText(&ws, fl->roomLayout.name);
        /* Solo gli oggetti ATTIVI: bossItem e' sempre procedurale (vedi il
           commento in gen_novelty.h), mai contenuto del modello. */
        for (int i = 0; i < GEN_ITEMS; i++) WordSetAddFromText(&ws, fl->items[i].name);
    }

    char words[GEN_NOVELTY_MAX_WORDS_PER_RUN*32];
    words[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < ws.count; i++)
    {
        int n = snprintf(words + used, used < sizeof(words) ? sizeof(words) - used : 0,
                          "%s%s", i ? " " : "", ws.words[i]);
        if (n < 0) break;
        used += (size_t)n;
        if (used >= sizeof(words)) break;
    }

    GenEnsureDir("logs");
    FILE *f = fopen(GEN_NOVELTY_LEDGER_PATH, "a");
    if (!f) return;   /* un ledger non scrivibile non deve far fallire la generazione */
    fprintf(f, "seed=%u words=%s\n", run->seed, words);
    fclose(f);
}

/* Parole-contenuto di UNA riga del ledger gia' scritta -> conteggio di RUN
   diverse (fra quelle tenute) in cui compare, accumulato in 'seen'/'runCount'
   dal chiamante. Le parole dentro una riga sono gia' uniche per costruzione
   (GenNoveltyAppend/WordSetAdd sopra), quindi ogni token della riga vale
   "vista in questa run" una volta sola: non serve un secondo dedup qui. */
typedef struct {
    char word[GEN_NOVELTY_LOOKBACK_RUNS*GEN_NOVELTY_MAX_WORDS_PER_RUN][32];
    int runCount[GEN_NOVELTY_LOOKBACK_RUNS*GEN_NOVELTY_MAX_WORDS_PER_RUN];
    int total;
} GenNoveltyTally;

static void TallyLine(GenNoveltyTally *t, char *wordsField)
{
    char *tok = strtok(wordsField, " ");
    while (tok)
    {
        /* Il confronto avviene sulla FORMA TRONCATA (lo stesso cap di 31
           caratteri con cui la parola viene salvata in t->word): confrontare
           il token grezzo contro la copia troncata non farebbe MAI match per
           una parola oltre il cap, e quella parola non "convergerebbe" mai
           -- bug reale trovato dalla verifica adversariale con un ledger
           editato a mano. Il writer tronca gia' allo stesso cap, quindi per
           i ledger scritti dal tool le due forme coincidono. */
        char cut[sizeof(t->word[0])];
        snprintf(cut, sizeof(cut), "%s", tok);
        int found = -1;
        for (int i = 0; i < t->total; i++) if (strcmp(t->word[i], cut) == 0) { found = i; break; }
        if (found < 0 && t->total < (int)(sizeof(t->word)/sizeof(t->word[0])))
        {
            found = t->total++;
            snprintf(t->word[found], sizeof(t->word[0]), "%s", cut);
            t->runCount[found] = 0;
        }
        if (found >= 0) t->runCount[found]++;
        tok = strtok(NULL, " ");
    }
}

void GenNoveltyAvoidList(char *buf, size_t size)
{
    if (!buf || size == 0) return;
    buf[0] = '\0';

    char *text = GenReadFile(GEN_NOVELTY_LEDGER_PATH);
    if (!text) return;   /* file assente = elenco vuoto, MAI un errore */

    /* Anello delle ultime GEN_NOVELTY_LOOKBACK_RUNS righe NON VUOTE viste:
       si scandisce il file una volta sola (i ledger restano piccoli anche
       dopo anni di run, non serve leggerlo a ritroso). La posizione di
       scrittura e' 'lineCount % capacita'', quindi a fine scansione l'anello
       contiene esattamente le ultime 'capacita'' righe, ciascuna nella
       propria posizione modulo -- si puo' rileggerle nello stesso ordine
       (l'ordine non conta per un conteggio di frequenza, solo l'insieme). */
    char *lineStarts[GEN_NOVELTY_LOOKBACK_RUNS];
    int lineCount = 0;
    char *p = text;
    while (*p)
    {
        char *lineStart = p;
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';
        if (lineStart[0] != '\0')
        {
            lineStarts[lineCount % GEN_NOVELTY_LOOKBACK_RUNS] = lineStart;
            lineCount++;
        }
        p = nl ? nl + 1 : p + strlen(p);
    }

    int kept = lineCount < GEN_NOVELTY_LOOKBACK_RUNS ? lineCount : GEN_NOVELTY_LOOKBACK_RUNS;

    static GenNoveltyTally tally;   /* static: alcune decine di KB, non sullo stack */
    tally.total = 0;
    for (int i = 0; i < kept; i++)
    {
        char *line = lineStarts[i];
        char *wordsField = strstr(line, "words=");
        if (!wordsField) continue;
        TallyLine(&tally, wordsField + 6 /* strlen("words=") */);
    }
    free(text);

    /* Solo le parole viste in ALMENO 2 run diverse fra quelle tenute: una
       parola usata una volta sola e' varieta', non convergenza. */
    static int idx[GEN_NOVELTY_LOOKBACK_RUNS*GEN_NOVELTY_MAX_WORDS_PER_RUN];
    int n = 0;
    for (int i = 0; i < tally.total; i++) if (tally.runCount[i] >= 2) idx[n++] = i;

    /* Ordina per numero di run decrescente (selection sort: n resta piccolo
       anche con un ledger pieno -- al massimo le parole-contenuto UNICHE di
       20 run, e solo quelle ripetute sopravvivono al filtro sopra). */
    for (int i = 0; i < n; i++)
    {
        int best = i;
        for (int j = i + 1; j < n; j++) if (tally.runCount[idx[j]] > tally.runCount[idx[best]]) best = j;
        if (best != i) { int tmp = idx[i]; idx[i] = idx[best]; idx[best] = tmp; }
    }

    if (n > GEN_NOVELTY_MAX_AVOID_WORDS) n = GEN_NOVELTY_MAX_AVOID_WORDS;

    size_t used = 0;
    for (int i = 0; i < n; i++)
    {
        const char *sep = i ? ", " : "";
        /* Ometti la parola INTERA se non ci sta tutta (separatore + parola +
           terminatore): mai troncarla a meta', spedirebbe un token spazzatura
           nel prompt LLM ("...evitare: convergen" invece di "convergenza").
           Col buffer dimensionato a GEN_NOVELTY_AVOID_BUF_SIZE questo ramo non
           scatta mai; e' la rete di sicurezza per un chiamante che sottodimensiona. */
        size_t need = strlen(sep) + strlen(tally.word[idx[i]]);
        if (used + need + 1 > size) break;
        int w = snprintf(buf + used, size - used, "%s%s", sep, tally.word[idx[i]]);
        if (w < 0) break;
        used += (size_t)w;
    }
}
