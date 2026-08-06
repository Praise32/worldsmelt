/* Vedi gen_visualspec.h per il contratto completo (batch.json) e la
   panoramica del ciclo. Questo file non linka nulla in piu' di main.c
   (cJSON gia' linkato da melting-gen, AGENTS.md): a differenza di
   gen_attacks.c non c'e' nessuna sandbox da compilare qui dentro, lo "spec"
   e' JSON puro e il "free_prompt" e' testo libero, nessuno dei due passa
   mai da uno script Lua. */

#include "gen_visualspec.h"

#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *GEN_VISUALSPEC_DOMAINS[GEN_VISUALSPEC_DOMAIN_COUNT] = {
    "character", "enemy", "weapon", "item", "boss_part"
};

/* Percorso FISSO della grammatica, come GEN_PROPOSE_GRAMMAR_PATH/
   GEN_CHARACTER_GRAMMAR_PATH in main.c: nessun test ha bisogno di puntarla
   altrove, quindi non e' un --grammar-visualspec configurabile, solo un
   letterale accanto a run.gbnf/character.gbnf/propose.gbnf. */
#define GEN_VISUALSPEC_GRAMMAR_PATH "tools/melting-gen/visualspec.gbnf"

/* Buffer del testo grezzo generato dal modello, prima di ogni estrazione.
   ~16 byte/token di margine (lo stesso rapporto di GEN_ATTACK_RAW_CAP in
   gen_attacks.c), abbondante per un JSON di sei campi o un paragrafo. */
#define GEN_VISUALSPEC_SPEC_RAW_CAP 4096
#define GEN_VISUALSPEC_FREE_RAW_CAP 3072

/* Capienza dei campi di UNA richiesta (con margine sopra i tetti imposti
   dalla grammatica/dal contratto: vedi visualspec.gbnf per i tetti veri --
   subtype/body_plan 40 caratteri, ogni materiale 24, distinctive_feature
   120). id: dominio piu' lungo "boss_part" (9) + "_" + due cifre. */
#define GEN_VISUALSPEC_ID_CAP 24
#define GEN_VISUALSPEC_WORDS_CAP 64
#define GEN_VISUALSPEC_MATERIAL_CAP 32
#define GEN_VISUALSPEC_MATERIAL_COUNT_MAX 4
#define GEN_VISUALSPEC_FEATURE_CAP 160
#define GEN_VISUALSPEC_SIZECLASS_CAP 12
/* Margine sopra GEN_VISUALSPEC_FREE_PROMPT_MAX_CHARS: il buffer deve poter
   ospitare ANCHE un tentativo troppo lungo prima che ValidateFreePrompt lo
   respinga -- rifiutarlo per overflow di snprintf invece che per la vera
   regola sulla lunghezza confonderebbe l'errore rimandato al modello. */
#define GEN_VISUALSPEC_FREE_PROMPT_CAP (GEN_VISUALSPEC_FREE_PROMPT_MAX_CHARS + 64)

/* Blocco "cosa devi fare adesso" del prompt utente, il piu' grande dei due
   stadi (quello del free_prompt, che ci infila anche il brief dello spec
   appena accettato). DIMENSIONATO DALLE COSTANTI, non a occhio: ai limiti
   che visualspec.gbnf stesso ammette (subtype e body_plan 40 caratteri, 4
   materiali da 24, distinctive_feature 120) il vecchio buffer da 700 byte
   troncava per davvero, e la coda tagliata era proprio "...no labels, no
   preamble", cioe' l'istruzione che evita il preambolo che poi fa fallire
   la validazione. I 1024 byte di margine coprono il testo fisso del blocco
   (~800 caratteri) con spazio per riscriverlo senza rifare i conti. */
#define GEN_VISUALSPEC_STAGE_BLOCK_CAP (1024 + GEN_VISUALSPEC_WORDS_CAP*2 + \
                                        GEN_VISUALSPEC_MATERIAL_COUNT_MAX*(GEN_VISUALSPEC_MATERIAL_CAP + 2) + \
                                        GEN_VISUALSPEC_FEATURE_CAP)

/* Forma normalizzata di un subtype (vedi NormalizeSubtype sotto): minuscolo,
   spazi/trattini/apostrofi collassati, tutto il resto scartato. 64 e'
   generoso sopra i 40 caratteri grezzi massimi di 'words' in
   visualspec.gbnf (la normalizzazione puo' solo accorciare, mai allungare). */
#define GEN_VISUALSPEC_SUBTYPE_NORM_CAP 64

/* Una richiesta gia' validata (entrambi gli stadi), pronta per finire in
   requests[]. Campo per campo lo stesso schema del contratto batch.json
   (vedi gen_visualspec.h) -- 'category' e' ridondante con 'domain' per
   costruzione (la grammatica lo garantisce), tenuto comunque come campo suo
   perche' e' cosi' che il writer lo scrive nel JSON di output. */
typedef struct GenVisualSpecItem {
    char id[GEN_VISUALSPEC_ID_CAP];
    char domain[16];
    char category[16];
    char subtype[GEN_VISUALSPEC_WORDS_CAP];
    char bodyPlan[GEN_VISUALSPEC_WORDS_CAP];
    char materials[GEN_VISUALSPEC_MATERIAL_COUNT_MAX][GEN_VISUALSPEC_MATERIAL_CAP];
    int materialCount;
    char distinctiveFeature[GEN_VISUALSPEC_FEATURE_CAP];
    char sizeClass[GEN_VISUALSPEC_SIZECLASS_CAP];
    char freePrompt[GEN_VISUALSPEC_FREE_PROMPT_CAP];
} GenVisualSpecItem;

/* Tetto del batch intero: GEN_VISUALSPEC_DOMAIN_COUNT domini x il massimo di
   --visualspecs N (20) = 100 richieste. Dimensiona sia l'accumulo delle
   richieste scritte sia la lista dei subtype gia' accettati (anti-
   fotocopia, vedi sotto): la seconda deve coprire OGNI spec accettato,
   anche quello di una richiesta poi scartata per free_prompt fallito (vedi
   il commento in gen_visualspec.h, "il subtype resta comunque speso"). */
#define GEN_VISUALSPEC_BATCH_CAP (GEN_VISUALSPEC_DOMAIN_COUNT * GEN_VISUALSPEC_PER_DOMAIN_MAX)

static void TrimWhitespace(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\n' || s[len-1] == '\r' || s[len-1] == '\t')) s[--len] = '\0';
    size_t start = 0;
    while (s[start] == ' ' || s[start] == '\n' || s[start] == '\r' || s[start] == '\t') start++;
    if (start > 0) memmove(s, s + start, len - start + 1);
}

/* ============================================================
   Confronto senza distinzione fra maiuscole/minuscole scritto a mano
   (stesso motivo di SameTextIgnoreCase in gen_validate.c: strcasestr non e'
   POSIX, richiederebbe _GNU_SOURCE, non vale una dipendenza in piu' per
   dieci righe).
   ============================================================ */
static bool ContainsIgnoreCase(const char *haystack, const char *needle)
{
    if (!haystack || !needle || !needle[0]) return false;
    size_t needleLen = strlen(needle);
    for (const char *h = haystack; *h; h++)
    {
        size_t i = 0;
        while (i < needleLen && h[i])
        {
            char ch = (h[i] >= 'A' && h[i] <= 'Z') ? (char)(h[i] - 'A' + 'a') : h[i];
            char cn = (needle[i] >= 'A' && needle[i] <= 'Z') ? (char)(needle[i] - 'A' + 'a') : needle[i];
            if (ch != cn) break;
            i++;
        }
        if (i == needleLen) return true;
    }
    return false;
}

/* Come ContainsIgnoreCase, ma solo su PAROLA INTERA: "ember glow" non deve
   scattare su "embers", ne' "graft" dentro "grafted". Confine = qualunque
   cosa non sia lettera/cifra (l'apostrofo NON e' un confine: "flux's" resta
   un'occorrenza di "flux"). Serve solo al gate della nomenclatura sotto --
   le parole trigger delle LoRA restano su ContainsIgnoreCase, li' un
   sottotoken tipo "pixelart" dentro una parola composta e' comunque un
   problema. */
static bool ContainsWordIgnoreCase(const char *haystack, const char *needle)
{
    if (!haystack || !needle || !needle[0]) return false;
    size_t needleLen = strlen(needle);
    for (const char *h = haystack; *h; h++)
    {
        if (h > haystack)
        {
            char prev = h[-1];
            if ((prev >= 'a' && prev <= 'z') || (prev >= 'A' && prev <= 'Z') || (prev >= '0' && prev <= '9')) continue;
        }
        size_t i = 0;
        while (i < needleLen && h[i])
        {
            char ch = (h[i] >= 'A' && h[i] <= 'Z') ? (char)(h[i] - 'A' + 'a') : h[i];
            char cn = (needle[i] >= 'A' && needle[i] <= 'Z') ? (char)(needle[i] - 'A' + 'a') : needle[i];
            if (ch != cn) break;
            i++;
        }
        if (i != needleLen) continue;
        char next = h[needleLen];
        if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9')) continue;
        return true;
    }
    return false;
}

/* Nomenclatura di interfaccia di Worldsmelt (DEC-072, fonte unica
   docs/design/governance/glossary.md): DEC-073a vieta che entri nei prompt
   di generazione dei contenuti dei World -- "quei prompt descrivono funzione
   e tema, non il vocabolario di fonderia dell'interfaccia". Fino al 06/08
   era solo una riga del prompt di sistema, e il modello la ignorava (2/10
   subtype del batch vero contenevano "flux"): un divieto che il C non
   controlla non e' un divieto, e' un augurio -- stessa lezione di
   size_class/materials, che infatti si validano da sempre.

   PLURALI, non singolari, dove il termine di gioco e' plurale (Embers,
   Ingots, Relics, Blast Charges, Cast Keys): "ember" al singolare e'
   vocabolario Fucina esplicitamente CHIESTO dal prompt di sistema (bronzo/
   brace/cenere/ardesia/patina) e compariva 12 volte nel batch vero, mentre
   "Embers" e' la valuta meta. Il confine di parola di
   ContainsWordIgnoreCase e' quindi la meta' del lavoro: senza, bandire
   "embers" bandirebbe anche "ember".

   "Heat" e "Crust" (in-game: energia e salute temporanea) restano in
   elenco anche se sono inglese comune da fonderia: sono nomenclatura come
   gli altri, il costo misurato e' basso (3 occorrenze su 20 testi del batch
   vero) e il prompt di sistema suggerisce esplicitamente i sinonimi da
   usare al loro posto ("glow"/"warmth", "shell"/"scab"). */
static const char *GEN_VISUALSPEC_SYSTEM_WORDS[] = {
    "smelting", "smeltery", "smelt", "flux", "ingots", "embers", "heat", "crust",
    "blast charges", "blast charge", "cast keys", "cast key", "graft", "grafts",
    "tempered", "relics", "signature shot", "discovery card", "daily smelt",
};
#define GEN_VISUALSPEC_SYSTEM_WORD_COUNT (sizeof(GEN_VISUALSPEC_SYSTEM_WORDS)/sizeof(GEN_VISUALSPEC_SYSTEM_WORDS[0]))

/* La prima parola di nomenclatura presente in 'text', NULL se nessuna. */
static const char *FindSystemWord(const char *text)
{
    for (size_t i = 0; i < GEN_VISUALSPEC_SYSTEM_WORD_COUNT; i++)
    {
        if (ContainsWordIgnoreCase(text, GEN_VISUALSPEC_SYSTEM_WORDS[i])) return GEN_VISUALSPEC_SYSTEM_WORDS[i];
    }
    return NULL;
}

/* ============================================================
   Stadio (a): SPEC via grammatica.
   ============================================================ */

/* Legge visualspec.gbnf UNA VOLTA e sostituisce {DOMAIN} col dominio della
   richiesta corrente (vedi il commento in cima al file .gbnf per il
   perche': "category" non e' mai una scelta del modello). Buffer malloc,
   NULL su fallimento (file mancante). */
static char *GenVisualSpecBuildGrammar(const char *domain)
{
    char *tpl = GenReadFile(GEN_VISUALSPEC_GRAMMAR_PATH);
    if (!tpl) return NULL;
    char *out = GenReplaceAll(tpl, "{DOMAIN}", domain);
    free(tpl);
    return out;
}

/* Parte utente del prompt (kind/stage/retry sostituiti in
   prompts/visualspec_user.txt): condivisa dai due stadi, 'stageBlock' e'
   composto dal chiamante (BuildVisualSpecSpecPrompt/BuildVisualSpecFreePrompt
   sotto) perche' i due compiti non condividono nessuna frase in comune oltre
   all'intestazione dominio/seed. Stesso schema di BuildAttackUserFinal in
   gen_attacks.c per {RETRY_BLOCK}: 'prevError' vuoto/NULL = nessuna riga di
   ritento, non vuoto = l'errore dell'ultimo tentativo accodato con
   l'istruzione di correggerlo. */
static char *BuildVisualSpecUserFinal(const char *promptsDir, const char *domain, unsigned int seed,
                                       const char *stageBlock, const char *prevError)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/visualspec_user.txt", promptsDir);
    char *tpl = GenReadFile(path);
    if (!tpl) return NULL;

    char *step1 = GenReplaceAll(tpl, "{DOMAIN}", domain);
    free(tpl);
    if (!step1) return NULL;

    char seedText[32];
    snprintf(seedText, sizeof(seedText), "%u", seed);
    char *step2 = GenReplaceAll(step1, "{SEED}", seedText);
    free(step1);
    if (!step2) return NULL;

    char *step3 = GenReplaceAll(step2, "{STAGE_BLOCK}", stageBlock ? stageBlock : "");
    free(step2);
    if (!step3) return NULL;

    /* {RETRY_BLOCK}: stessa scelta di {BRIEF_LINE} in gen_attacks.c -- riga
       omessa (stringa vuota) quando non c'e' un errore precedente, mai un
       placeholder lasciato a meta' frase. */
    char retryBlock[GEN_VISUALSPEC_ERR_CAP + 160];
    if (prevError && prevError[0])
    {
        snprintf(retryBlock, sizeof(retryBlock),
                 "Your previous answer was rejected: %s\nAnswer again from scratch, avoiding that exact mistake.",
                 prevError);
    }
    else retryBlock[0] = '\0';
    char *userFinal = GenReplaceAll(step3, "{RETRY_BLOCK}", retryBlock);
    free(step3);
    return userFinal;
}

static char *BuildVisualSpecSpecPrompt(const char *promptsDir, const char *domain, unsigned int seed,
                                        const char *prevError)
{
    char sysPath[512];
    snprintf(sysPath, sizeof(sysPath), "%s/visualspec_system.txt", promptsDir);
    char *sys = GenReadFile(sysPath);
    if (!sys) return NULL;

    /* Stesso buffer dello stadio (b) anche se qui il testo e' molto piu'
       corto: una sola costante da controllare quando si riscrive un blocco. */
    char stageBlock[GEN_VISUALSPEC_STAGE_BLOCK_CAP];
    snprintf(stageBlock, sizeof(stageBlock),
        "Invent one new %s concept for this request and write its VisualSpec as the JSON "
        "object described above (job 1). Reply with the JSON object only, nothing before "
        "or after it.", domain);

    char *userFinal = BuildVisualSpecUserFinal(promptsDir, domain, seed, stageBlock, prevError);
    if (!userFinal) { free(sys); return NULL; }

    char *prompt = GenChatMlWrap(sys, userFinal);
    free(sys);
    free(userFinal);
    return prompt;
}

/* Normalizza un subtype per il confronto anti-fotocopia: minuscolo, via
   spazi/trattini/apostrofi ripetuti o di contorno, collassati a UNO spazio
   fra le parole rimaste. "Armored Beetle" e "armored  beetle-" normalizzano
   identici, che e' esattamente la forma di "stesso concetto riscritto" da
   riconoscere -- stesso spirito di GenAttackNormalize in gen_attacks.c, ma
   per una singola frase corta invece che per codice Lua multi-riga. */
static void NormalizeSubtype(const char *src, char *out, size_t outCap)
{
    size_t w = 0;
    bool pendingSpace = false;
    for (const char *p = src; *p && w + 1 < outCap; p++)
    {
        char c = *p;
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c == ' ' || c == '-' || c == '\'')
        {
            if (w > 0) pendingSpace = true;
            continue;
        }
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) continue;
        if (pendingSpace) { out[w++] = ' '; pendingSpace = false; }
        out[w++] = c;
    }
    out[w] = '\0';
}

/* Sotto questa lunghezza normalizzata un subtype non prova niente da solo
   ("orb", "claw"): confrontarlo per substring produrrebbe falsi positivi
   contro qualunque subtype piu' lungo che lo contiene per caso (es. "claw"
   dentro "clawed armored beetle"). Stesso ruolo di GEN_ATTACK_COPY_MIN_LINE
   in gen_attacks.c. */
#define GEN_VISUALSPEC_DUP_MIN_LEN 5

/* true se 'normCandidate' e' "lo stesso concetto" di uno fra i primi
   'acceptedCount' elementi di 'accepted' (entrambi gia' normalizzati):
   substring in ENTRAMBE le direzioni, cosi' ne' "iron blade" ne' "blade"
   sfuggono al confronto quando l'altro e' gia' nel batch. 'matchOut' (puo'
   essere NULL) riceve l'elemento con cui collide, per un errore rimandabile
   al modello. */
static bool SubtypeIsDuplicate(const char *normCandidate, char accepted[][GEN_VISUALSPEC_SUBTYPE_NORM_CAP],
                                int acceptedCount, char *matchOut, size_t matchOutCap)
{
    size_t candLen = strlen(normCandidate);
    if (candLen < GEN_VISUALSPEC_DUP_MIN_LEN) return false;
    for (int i = 0; i < acceptedCount; i++)
    {
        if (strlen(accepted[i]) < GEN_VISUALSPEC_DUP_MIN_LEN) continue;
        if (strstr(accepted[i], normCandidate) || strstr(normCandidate, accepted[i]))
        {
            if (matchOut) snprintf(matchOut, matchOutCap, "%s", accepted[i]);
            return true;
        }
    }
    return false;
}

/* Estrae e valida i sei campi dello spec dal JSON grezzo (gia' garantito
   sintatticamente valido dalla grammatica, ma "fidati e verifica" come ogni
   altro punto di questo tool -- vedi il parsing difensivo del colpo firmato
   in RunProposeCharacter, main.c). Riempie 'out' e 'outNorm' (il subtype
   normalizzato, da registrare nell'elenco degli accettati SOLO se questa
   funzione ritorna true) solo su successo. L'UNICO controllo che puo' far
   fallire uno spec altrimenti ben formato e' l'anti-fotocopia contro
   'accepted'. */
static bool ParseAndValidateSpec(const char *rawJson, const char *domain,
                                  char accepted[][GEN_VISUALSPEC_SUBTYPE_NORM_CAP], int acceptedCount,
                                  GenVisualSpecItem *out, char *outNorm, size_t outNormCap,
                                  char *err, size_t errSize)
{
    cJSON *root = cJSON_Parse(rawJson);
    if (!root)
    {
        if (err) snprintf(err, errSize, "output is not valid JSON");
        return false;
    }

    cJSON *jCategory = cJSON_GetObjectItemCaseSensitive(root, "category");
    cJSON *jSubtype = cJSON_GetObjectItemCaseSensitive(root, "subtype");
    cJSON *jBodyPlan = cJSON_GetObjectItemCaseSensitive(root, "body_plan");
    cJSON *jMaterials = cJSON_GetObjectItemCaseSensitive(root, "materials");
    cJSON *jFeature = cJSON_GetObjectItemCaseSensitive(root, "distinctive_feature");
    cJSON *jSize = cJSON_GetObjectItemCaseSensitive(root, "size_class");

    bool shapeOk = cJSON_IsString(jCategory) && jCategory->valuestring && jCategory->valuestring[0] &&
                   cJSON_IsString(jSubtype) && jSubtype->valuestring && jSubtype->valuestring[0] &&
                   cJSON_IsString(jBodyPlan) && jBodyPlan->valuestring && jBodyPlan->valuestring[0] &&
                   cJSON_IsArray(jMaterials) &&
                   cJSON_IsString(jFeature) && jFeature->valuestring && jFeature->valuestring[0] &&
                   cJSON_IsString(jSize) && jSize->valuestring && jSize->valuestring[0];
    if (!shapeOk)
    {
        cJSON_Delete(root);
        if (err) snprintf(err, errSize, "the JSON object is missing a required field or has the wrong type");
        return false;
    }

    /* category==domain per costruzione (visualspec.gbnf): un mismatch qui
       e' un bug futuro nella grammatica, mai una scelta legittima del
       modello, ma si respinge comunque invece di riscrivere in silenzio --
       stesso principio del colpo firmato in RunProposeCharacter. */
    if (strcmp(jCategory->valuestring, domain) != 0)
    {
        cJSON_Delete(root);
        if (err) snprintf(err, errSize, "category must be exactly \"%s\" for this request", domain);
        return false;
    }

    int materialCount = cJSON_GetArraySize(jMaterials);
    if (materialCount < 2 || materialCount > GEN_VISUALSPEC_MATERIAL_COUNT_MAX)
    {
        cJSON_Delete(root);
        if (err) snprintf(err, errSize, "materials must have 2 to %d entries, got %d",
                           GEN_VISUALSPEC_MATERIAL_COUNT_MAX, materialCount);
        return false;
    }
    for (int i = 0; i < materialCount; i++)
    {
        cJSON *m = cJSON_GetArrayItem(jMaterials, i);
        if (!cJSON_IsString(m) || !m->valuestring || !m->valuestring[0])
        {
            cJSON_Delete(root);
            if (err) snprintf(err, errSize, "materials[%d] is not a non-empty string", i);
            return false;
        }
    }

    if (strcmp(jSize->valuestring, "small") != 0 && strcmp(jSize->valuestring, "medium") != 0 &&
        strcmp(jSize->valuestring, "large") != 0)
    {
        cJSON_Delete(root);
        if (err) snprintf(err, errSize, "size_class must be one of small, medium, large");
        return false;
    }

    /* Nomenclatura di interfaccia (DEC-073a, vedi FindSystemWord sopra):
       campo per campo, cosi' l'errore rimandato al modello dice QUALE campo
       riscrivere e non solo che "da qualche parte" c'e' una parola vietata.
       Come ogni altro 'return false' che nomina un valore dell'albero, lo
       snprintf viene PRIMA di cJSON_Delete. */
    {
        const char *fieldNames[3] = { "subtype", "body_plan", "distinctive_feature" };
        const char *fieldValues[3] = { jSubtype->valuestring, jBodyPlan->valuestring, jFeature->valuestring };
        for (int i = 0; i < 3; i++)
        {
            const char *bad = FindSystemWord(fieldValues[i]);
            if (bad)
            {
                if (err) snprintf(err, errSize,
                    "%s contains \"%s\", one of this game's own interface words: describe the object itself "
                    "with ordinary English instead", fieldNames[i], bad);
                cJSON_Delete(root);
                return false;
            }
        }
        for (int i = 0; i < materialCount; i++)
        {
            const char *value = cJSON_GetArrayItem(jMaterials, i)->valuestring;
            const char *bad = FindSystemWord(value);
            if (bad)
            {
                if (err) snprintf(err, errSize,
                    "materials[%d] contains \"%s\", one of this game's own interface words: name a real "
                    "material instead", i, bad);
                cJSON_Delete(root);
                return false;
            }
        }
    }

    /* Anti-fotocopia (vedi il commento sopra SubtypeIsDuplicate): l'ULTIMO
       controllo, dopo che la forma e' gia' garantita valida -- stesso ordine
       di GenAttackGenerate (validazione PRIMA, gate anti-copia DOPO). */
    char norm[GEN_VISUALSPEC_SUBTYPE_NORM_CAP];
    NormalizeSubtype(jSubtype->valuestring, norm, sizeof(norm));
    char matchNorm[GEN_VISUALSPEC_SUBTYPE_NORM_CAP];
    if (SubtypeIsDuplicate(norm, accepted, acceptedCount, matchNorm, sizeof(matchNorm)))
    {
        /* snprintf PRIMA di cJSON_Delete: jSubtype->valuestring vive dentro
           l'albero di 'root', leggerlo DOPO averlo distrutto e' un
           use-after-free -- e' il bug REALE che il giro vero del 06/08 ha
           mostrato (l'errore rimandato al modello diceva
           'subtype "(null)" e' troppo simile...', memoria gia' liberata
           che per puro caso stampava una stringa nulla invece di andare in
           segfault). Ogni altro 'return false' qui sopra usa solo interi o
           stringhe FUORI dall'albero (domain, materialCount, i) e non ha
           questo problema. */
        if (err) snprintf(err, errSize,
            "subtype \"%s\" is too close to \"%s\", already used earlier in this batch: "
            "invent a genuinely different concept", jSubtype->valuestring, matchNorm);
        cJSON_Delete(root);
        return false;
    }

    snprintf(out->category, sizeof(out->category), "%s", jCategory->valuestring);
    snprintf(out->subtype, sizeof(out->subtype), "%s", jSubtype->valuestring);
    snprintf(out->bodyPlan, sizeof(out->bodyPlan), "%s", jBodyPlan->valuestring);
    out->materialCount = materialCount;
    for (int i = 0; i < materialCount; i++)
    {
        snprintf(out->materials[i], sizeof(out->materials[i]), "%s", cJSON_GetArrayItem(jMaterials, i)->valuestring);
    }
    snprintf(out->distinctiveFeature, sizeof(out->distinctiveFeature), "%s", jFeature->valuestring);
    snprintf(out->sizeClass, sizeof(out->sizeClass), "%s", jSize->valuestring);
    snprintf(outNorm, outNormCap, "%s", norm);

    cJSON_Delete(root);
    return true;
}

/* ============================================================
   Stadio (b): FREE_PROMPT senza grammatica.
   ============================================================ */

/* Parole trigger delle configurazioni immagine del bake-off (vedi
   scripts/teacher-bench.sh, CONFIG_ROW: S1/S2/F1..F9/A2..A3 -- ogni riga che
   usa una LoRA pixel-art aggiunge uno di questi token al prompt PRIMA di
   passarlo a sd-cli): il trigger lo mette l'harness, e lo mette a ENTRAMBI
   i bracci allo stesso modo -- e' l'unico pezzo di testo che i due bracci
   condividono per forza, perche' dipende dalla config e non
   dall'architettura di prompting. Il modello non deve conoscerlo: se lo
   scrivesse anche lui, quel braccio avrebbe il trigger due volte. Confronto senza
   distinzione fra maiuscole/minuscole via ContainsIgnoreCase -- "pixel"
   nudo NON e' in elenco di proposito: e' un termine troppo comune in una
   descrizione di un asset di un gioco pixel-art per bandirlo senza
   respingere anche frasi legittime, mentre le forme sotto sono token
   distintivi di UNA LoRA specifica, mai linguaggio naturale. */
static const char *GEN_VISUALSPEC_BANNED_WORDS[] = {
    "pixel_art", "pixel art", "pixelsprite", "pixelart", "basepixel",
    "8bitdiffuser", "rpgicondiff", "8-bit", "8bit",
};
#define GEN_VISUALSPEC_BANNED_WORD_COUNT (sizeof(GEN_VISUALSPEC_BANNED_WORDS)/sizeof(GEN_VISUALSPEC_BANNED_WORDS[0]))

/* I tre ingredienti che il free_prompt DEVE dichiarare per essere il prompt
   COMPLETO che il contratto promette (vedi gen_visualspec.h): la vista, il
   soggetto singolo, lo sfondo. Non sono un'imposizione stilistica -- sono
   esattamente cio' che scripts/visualspec_template.py mette nel braccio
   "spec" e che a valle del braccio "free" non mette NESSUNO, ed e' cio' che
   il postprocesso comune ai due bracci (flood-fill dello sfondo + rimappa di
   palette) pretende per funzionare. Ogni riga elenca piu' forme accettate
   dello stesso concetto, con la piu' comune per prima: al modello si chiede
   di DIRE la cosa, non di dirla con le parole del template (sarebbe di nuovo
   il template, solo scritto da lui). */
static const char *GEN_VISUALSPEC_VIEW_MARKERS[] = {
    "top-down", "top down", "topdown", "overhead", "top view", "birds-eye", "bird's-eye", "bird's eye",
    /* Aggiunte dopo il giro vero del 06/08 (5 respinte su "manca la vista"):
       sono modi in cui il modello DICE la stessa inquadratura, non gradi di
       tolleranza sul requisito. Restano fuori "three-quarter" da solo (e'
       un'inquadratura diversa senza il dall'alto) e "side view"/"front view"
       (sono proprio l'inquadratura sbagliata). */
    "from above", "seen from above", "viewed from above", "aerial", "high angle",
};
static const char *GEN_VISUALSPEC_SINGLE_MARKERS[] = {
    "single", "isolated", "alone", "solitary", "on its own", "one subject",
    /* "lone" e' la forma che il modello usa piu' spesso di tutte ("a lone
       bronze spike maul"): senza, si respingevano prompt che dicevano
       ESATTAMENTE la cosa richiesta -- 3 respinte su 13 nel giro
       strumentato del 06/08. Copre anche "alone" per sottostringa, tenute
       entrambe perche' l'elenco si legge come un elenco di parole, non come
       un insieme di prefissi. */
    "lone",
};
static const char *GEN_VISUALSPEC_BACKGROUND_MARKERS[] = {
    "background", "backdrop",
};
#define GEN_VISUALSPEC_MARKER_COUNT(arr) (sizeof(arr)/sizeof((arr)[0]))

static bool ContainsAnyIgnoreCase(const char *text, const char **markers, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        if (ContainsIgnoreCase(text, markers[i])) return true;
    }
    return false;
}

/* Aperture chiacchierone tipiche ("Sure, here is the prompt:"): l'istruzione
   "reply with the prompt text only" c'e' gia' nel prompt, ma quando il
   modello la ignora il preambolo finirebbe DENTRO il prompt mandato a
   sd-cli, dove pesa token veri. Confronto sul solo INIZIO del testo (dopo
   trim), mai in mezzo: "here" a meta' frase e' inglese legittimo. */
static const char *GEN_VISUALSPEC_PREAMBLES[] = {
    "sure", "here is", "here's", "here you", "okay", "ok,", "certainly",
    "of course", "prompt:", "the prompt", "absolutely",
};

static bool StartsWithIgnoreCase(const char *text, const char *prefix)
{
    size_t i = 0;
    for (; prefix[i]; i++)
    {
        char ct = (text[i] >= 'A' && text[i] <= 'Z') ? (char)(text[i] - 'A' + 'a') : text[i];
        char cp = (prefix[i] >= 'A' && prefix[i] <= 'Z') ? (char)(prefix[i] - 'A' + 'a') : prefix[i];
        if (!text[i] || ct != cp) return false;
    }
    return true;
}

/* Ripieghi da tipografia a ASCII applicati PRIMA della validazione: e' la
   stessa divisione "estrazione tollerante, validazione severa" di
   ExtractLuaCode in gen_attacks.c. Gemma in prosa scrive virgolette curve e
   trattini lunghi quasi sempre -- misurato sul giro vero del 06/08: 10/10
   primi tentativi respinti dalla sola regola ASCII, 23 chiamate al modello
   per 10 prompt. Nessuno di questi caratteri cambia il SIGNIFICATO del
   prompt, quindi respingerli e' costo puro: si ripiegano e si va avanti.
   Ogni ripiego non allunga mai la stringa (U+2026 e' 3 byte UTF-8 e "..."
   e' 3 byte ASCII), quindi la riscrittura in loco e' sicura. */
typedef struct GenVisualSpecFold { const char *from; const char *to; } GenVisualSpecFold;
static const GenVisualSpecFold GEN_VISUALSPEC_FOLDS[] = {
    { "\xE2\x80\x98", "'" },    /* U+2018 virgoletta singola aperta */
    { "\xE2\x80\x99", "'" },    /* U+2019 virgoletta singola chiusa (anche apostrofo) */
    { "\xE2\x80\x9C", "\"" },   /* U+201C virgolette doppie aperte */
    { "\xE2\x80\x9D", "\"" },   /* U+201D virgolette doppie chiuse */
    { "\xE2\x80\x93", "-" },    /* U+2013 trattino medio */
    { "\xE2\x80\x94", "-" },    /* U+2014 trattino lungo */
    { "\xE2\x80\xA6", "..." },  /* U+2026 puntini di sospensione */
    { "\xC2\xA0",     " " },    /* U+00A0 spazio unificatore */
};
#define GEN_VISUALSPEC_FOLD_COUNT (sizeof(GEN_VISUALSPEC_FOLDS)/sizeof(GEN_VISUALSPEC_FOLDS[0]))

/* Ripieghi tipografici + ogni spazio bianco (compresi a capo e tabulazioni)
   collassato a UNO spazio singolo: il free_prompt e' un paragrafo su una
   riga, e un a capo in mezzo e' formattazione, non contenuto. Il preambolo
   chiacchierone NON viene tolto qui: quello e' testo vero che il modello ha
   scritto di sua iniziativa, e va respinto con un errore che glielo dica
   (ValidateFreePrompt), non nascosto sotto il tappeto. */
static void FoldToAscii(char *s)
{
    size_t w = 0;
    bool pendingSpace = false;
    for (size_t r = 0; s[r]; )
    {
        const char *replacement = NULL;
        size_t consumed = 1;
        for (size_t f = 0; f < GEN_VISUALSPEC_FOLD_COUNT; f++)
        {
            size_t fromLen = strlen(GEN_VISUALSPEC_FOLDS[f].from);
            if (strncmp(s + r, GEN_VISUALSPEC_FOLDS[f].from, fromLen) == 0)
            {
                replacement = GEN_VISUALSPEC_FOLDS[f].to;
                consumed = fromLen;
                break;
            }
        }
        const char *chunk = replacement ? replacement : NULL;
        char single[2];
        if (!chunk) { single[0] = s[r]; single[1] = '\0'; chunk = single; }
        r += consumed;

        for (const char *c = chunk; *c; c++)
        {
            if (*c == ' ' || *c == '\n' || *c == '\r' || *c == '\t')
            {
                if (w > 0) pendingSpace = true;
                continue;
            }
            if (pendingSpace) { s[w++] = ' '; pendingSpace = false; }
            s[w++] = *c;
        }
    }
    s[w] = '\0';
}

/* Marcatori di fine turno che il modello scrive come TESTO NORMALE. Il ciclo
   di campionamento (GenLlmSampleLoop, gen_llm.c) si ferma su
   llama_vocab_is_eog, ma il prompt di questo tool e' ChatML
   (GenChatMlWrap: "<|im_end|>") mentre il marcatore VERO di Gemma e'
   "<end_of_turn>": "<|im_end|>" non e' un token speciale per questo
   vocabolario, e' una manciata di token ordinari, quindi is_eog non scatta
   mai e la stringa finisce nell'output. Misurato sul giro vero del 06/08:
   4 free_prompt su 7 la portavano in coda -- e da li' sarebbe finita dritta
   nel prompt di sd-cli, dove pesa token veri e sporca solo il braccio
   "free" (il braccio "spec" e' vincolato dalla grammatica e non puo'
   scriverla). Lo stadio (a) e gli altri generatori del tool non se ne
   accorgono per costruzione: hanno una grammatica, o estraggono fra ``` */
static const char *GEN_VISUALSPEC_TURN_MARKERS[] = {
    "<|im_end|>", "<|im_start|>", "<end_of_turn>", "<start_of_turn>", "<eos>",
};

/* Estrazione tollerante: blocco ```...``` se il modello lo aggiunge comunque,
   taglio al primo marcatore di fine turno, trim, una coppia di virgolette
   dritte in testa/coda, ripieghi tipografici (FoldToAscii sopra). Tutto cio'
   che resta lo giudica ValidateFreePrompt, col ritento che rimanda l'errore
   al modello. */
static void ExtractFreePrompt(const char *raw, char *out, size_t outCap)
{
    const char *body = raw;
    size_t len = strlen(raw);

    const char *fence = strstr(raw, "```");
    if (fence)
    {
        body = fence + 3;
        while (*body == '\n' || *body == '\r') body++;
        const char *closeFence = strstr(body, "```");
        len = closeFence ? (size_t)(closeFence - body) : strlen(body);
    }
    else len = strlen(body);

    for (size_t i = 0; i < sizeof(GEN_VISUALSPEC_TURN_MARKERS)/sizeof(GEN_VISUALSPEC_TURN_MARKERS[0]); i++)
    {
        const char *marker = strstr(body, GEN_VISUALSPEC_TURN_MARKERS[i]);
        if (marker && (size_t)(marker - body) < len) len = (size_t)(marker - body);
    }

    if (len >= outCap) len = outCap - 1;
    memcpy(out, body, len);
    out[len] = '\0';
    TrimWhitespace(out);

    size_t n = strlen(out);
    if (n >= 2 && out[0] == '"' && out[n-1] == '"')
    {
        memmove(out, out + 1, n - 2);
        out[n - 2] = '\0';
        TrimWhitespace(out);
    }
    FoldToAscii(out);
}

/* Nomina il carattere non-ASCII sopravvissuto ai ripieghi: senza il codice
   vero il ritento non e' azionabile (il modello non sa QUALE carattere
   togliere, e infatti riproponeva lo stesso errore due volte di fila).
   Decodifica il code point UTF-8 quando la sequenza e' ben formata,
   altrimenti riporta il byte grezzo -- un output non-UTF-8 e' comunque un
   dato utile, non un caso da nascondere. */
static void DescribeNonAscii(const char *text, char *out, size_t outCap)
{
    const unsigned char *p = (const unsigned char *)text;
    for (size_t i = 0; p[i]; i++)
    {
        unsigned char b = p[i];
        if (b >= 0x20 && b <= 0x7E) continue;
        if (b < 0x20)
        {
            snprintf(out, outCap, "a control character (byte 0x%02X)", b);
            return;
        }
        unsigned int cp = b;
        int extra = 0;
        if ((b & 0xE0) == 0xC0) { cp = b & 0x1Fu; extra = 1; }
        else if ((b & 0xF0) == 0xE0) { cp = b & 0x0Fu; extra = 2; }
        else if ((b & 0xF8) == 0xF0) { cp = b & 0x07u; extra = 3; }
        bool wellFormed = extra > 0;
        for (int k = 1; k <= extra && wellFormed; k++)
        {
            if ((p[i+k] & 0xC0) != 0x80) wellFormed = false;
            else cp = (cp << 6) | (p[i+k] & 0x3Fu);
        }
        if (wellFormed) snprintf(out, outCap, "U+%04X", cp);
        else snprintf(out, outCap, "the raw byte 0x%02X", b);
        return;
    }
    snprintf(out, outCap, "an unknown character");
}

static bool ValidateFreePrompt(const char *text, char *err, size_t errSize)
{
    if (!text || !text[0])
    {
        if (err) snprintf(err, errSize, "empty prompt");
        return false;
    }
    for (size_t i = 0; i < sizeof(GEN_VISUALSPEC_PREAMBLES)/sizeof(GEN_VISUALSPEC_PREAMBLES[0]); i++)
    {
        if (StartsWithIgnoreCase(text, GEN_VISUALSPEC_PREAMBLES[i]))
        {
            if (err) snprintf(err, errSize,
                "the answer starts with \"%s\": drop the introduction, the whole reply must be the prompt "
                "itself and it must start with the camera view",
                GEN_VISUALSPEC_PREAMBLES[i]);
            return false;
        }
    }
    for (const unsigned char *p = (const unsigned char *)text; *p; p++)
    {
        if (*p < 0x20 || *p > 0x7E)
        {
            char what[48];
            DescribeNonAscii(text, what, sizeof(what));
            if (err) snprintf(err, errSize,
                "the prompt contains %s, which is not plain ASCII: rewrite that word using only straight "
                "quotes ' and \", a plain hyphen -, and unaccented letters",
                what);
            return false;
        }
    }
    size_t len = strlen(text);
    if (len > (size_t)GEN_VISUALSPEC_FREE_PROMPT_MAX_CHARS)
    {
        if (err) snprintf(err, errSize, "prompt is %zu characters, over the %d limit: cut it down to one shorter paragraph",
                           len, GEN_VISUALSPEC_FREE_PROMPT_MAX_CHARS);
        return false;
    }
    for (size_t i = 0; i < GEN_VISUALSPEC_BANNED_WORD_COUNT; i++)
    {
        if (ContainsIgnoreCase(text, GEN_VISUALSPEC_BANNED_WORDS[i]))
        {
            if (err) snprintf(err, errSize,
                "prompt must not mention \"%s\" or any other pixel-art tool/technique keyword: "
                "the image pipeline adds those itself",
                GEN_VISUALSPEC_BANNED_WORDS[i]);
            return false;
        }
    }
    const char *sysWord = FindSystemWord(text);
    if (sysWord)
    {
        if (err) snprintf(err, errSize,
            "prompt contains \"%s\", one of this game's own interface words: describe the object itself "
            "with ordinary English instead", sysWord);
        return false;
    }
    /* Completezza (vedi GEN_VISUALSPEC_VIEW_MARKERS sopra e la testata di
       gen_visualspec.h). I tre elementi si controllano insieme e mancano
       insieme nell'errore, non uno per volta: coi tre controlli separati che
       tornavano al primo fallimento il ritento giocava a colpisci-la-talpa
       -- misurato sul giro vero del 06/08, il modello aggiustava la vista e
       nel farlo perdeva lo sfondo, bruciando due tentativi per due difetti
       che avrebbe potuto correggere insieme. */
    const char *missing[3];
    int missingCount = 0;
    if (!ContainsAnyIgnoreCase(text, GEN_VISUALSPEC_VIEW_MARKERS, GEN_VISUALSPEC_MARKER_COUNT(GEN_VISUALSPEC_VIEW_MARKERS)))
    {
        /* Il difetto piu' frequente in assoluto e' UNO solo e sempre lo
           stesso: il modello scrive "three-quarter view" (o peggio
           "low-angle three-quarter view") e lascia cadere il "top-down",
           cioe' descrive una TELECAMERA DIVERSA. Vale un messaggio suo:
           quello generico lo faceva ricadere nello stesso errore al ritento. */
        if (ContainsIgnoreCase(text, "three-quarter") || ContainsIgnoreCase(text, "three quarter"))
        {
            missing[missingCount++] = "the words that put the camera ABOVE the subject (you wrote "
                                      "\"three-quarter\" but never that it is seen from above: write "
                                      "top-down three-quarter)";
        }
        else missing[missingCount++] = "the camera view (a top-down three-quarter view)";
    }
    if (!ContainsAnyIgnoreCase(text, GEN_VISUALSPEC_SINGLE_MARKERS, GEN_VISUALSPEC_MARKER_COUNT(GEN_VISUALSPEC_SINGLE_MARKERS)))
    {
        missing[missingCount++] = "that a single subject is alone in frame";
    }
    if (!ContainsAnyIgnoreCase(text, GEN_VISUALSPEC_BACKGROUND_MARKERS, GEN_VISUALSPEC_MARKER_COUNT(GEN_VISUALSPEC_BACKGROUND_MARKERS)))
    {
        missing[missingCount++] = "a flat solid neutral gray background";
    }
    if (missingCount > 0)
    {
        if (err)
        {
            size_t used = (size_t)snprintf(err, errSize, "the prompt is missing: ");
            for (int i = 0; i < missingCount && used < errSize; i++)
            {
                int n = snprintf(err + used, errSize - used, "%s%s", i > 0 ? "; " : "", missing[i]);
                if (n < 0) break;
                used += (size_t)n;
            }
            if (used < errSize)
            {
                snprintf(err + used, errSize - used,
                    ". Nothing downstream adds these -- add them all without dropping what the prompt already says.");
            }
        }
        return false;
    }
    return true;
}

/* Unisce item->materials[0..materialCount) in "a, b, c" per il brief dello
   stadio (b): stessa lista che lo stadio (a) ha appena accettato, mai
   riformulata. */
static void JoinMaterials(const GenVisualSpecItem *item, char *out, size_t outCap)
{
    out[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < item->materialCount && used < outCap; i++)
    {
        int n = snprintf(out + used, outCap - used, "%s%s", i > 0 ? ", " : "", item->materials[i]);
        if (n < 0) break;
        used += (size_t)n;
    }
}

static char *BuildVisualSpecFreePrompt(const char *promptsDir, const char *domain, unsigned int seed,
                                        const GenVisualSpecItem *item, const char *prevError)
{
    char sysPath[512];
    snprintf(sysPath, sizeof(sysPath), "%s/visualspec_system.txt", promptsDir);
    char *sys = GenReadFile(sysPath);
    if (!sys) return NULL;

    char materialsJoined[GEN_VISUALSPEC_MATERIAL_COUNT_MAX*(GEN_VISUALSPEC_MATERIAL_CAP + 2)];
    JoinMaterials(item, materialsJoined, sizeof(materialsJoined));

    char stageBlock[GEN_VISUALSPEC_STAGE_BLOCK_CAP];
    snprintf(stageBlock, sizeof(stageBlock),
        "Brief from job 1, the SAME %s you already invented (do not restate these field "
        "names, just use the ideas): subtype \"%s\"; body plan \"%s\"; materials %s; "
        "distinctive feature \"%s\". Now write job 2: the COMPLETE prompt that will be sent "
        "to the image model exactly as you write it, with nothing added before or after. In "
        /* Le cinque richieste sono formulate SOLO in positivo. Il primo giro
           reale del 06/08 le accompagnava con l'esempio contrario ("never a
           low-angle or side three-quarter view"): il modello ha cominciato a
           scrivere "low-angle view" nell'80% dei tentativi respinti -- gli
           si era messa in bocca la frase che si voleva evitare. In un prompt
           una parola nominata e' una parola suggerita, anche dopo un
           "never". */
        "your own words it must state all five of these: that the camera is directly ABOVE "
        "the subject, looking down on it in a top-down three-quarter view; what this %s is "
        "and what it is made of; its one distinctive detail; that a single subject is alone "
        "in frame; and that the background is a flat solid neutral gray. English, plain "
        "prose, one paragraph, at most %d characters total. Reply with "
        "the prompt text only: no quotes, no labels, no preamble.",
        domain, item->subtype, item->bodyPlan, materialsJoined, item->distinctiveFeature,
        domain, GEN_VISUALSPEC_FREE_PROMPT_MAX_CHARS);

    char *userFinal = BuildVisualSpecUserFinal(promptsDir, domain, seed, stageBlock, prevError);
    if (!userFinal) { free(sys); return NULL; }

    char *prompt = GenChatMlWrap(sys, userFinal);
    free(sys);
    free(userFinal);
    return prompt;
}

/* ============================================================
   Guardia byte-budget del prompt (vedi gen_visualspec.h).
   ============================================================ */

bool GenVisualSpecPromptBudgetCheck(const char *promptsDir, char *err, size_t errSize)
{
    if (err && errSize) err[0] = '\0';

    /* Caso peggiore, tutto insieme e tutto verificabile a occhio:
       - il dominio col nome piu' lungo fra i cinque (lo si cerca invece di
         scriverlo a mano: GEN_VISUALSPEC_DOMAINS puo' cambiare);
       - i campi dello spec ai tetti VERI di visualspec.gbnf (40/40/24x4/120),
         non a un esempio "rappresentativo" come per il brief libero di
         GenAttackPromptBudgetCheck -- qui il tetto c'e' e lo impone la
         grammatica, quindi si misura quello;
       - il blocco di ritento pieno (GEN_VISUALSPEC_ERR_CAP di caratteri
         qualunque): il ramo che consuma piu' contesto e' sempre il ritento,
         mai il primo tentativo. */
    const char *worstDomain = GEN_VISUALSPEC_DOMAINS[0];
    for (int d = 1; d < GEN_VISUALSPEC_DOMAIN_COUNT; d++)
    {
        if (strlen(GEN_VISUALSPEC_DOMAINS[d]) > strlen(worstDomain)) worstDomain = GEN_VISUALSPEC_DOMAINS[d];
    }

    char worstError[GEN_VISUALSPEC_ERR_CAP];
    memset(worstError, 'x', sizeof(worstError) - 1);
    worstError[sizeof(worstError) - 1] = '\0';

    GenVisualSpecItem worstItem;
    memset(&worstItem, 0, sizeof(worstItem));
    memset(worstItem.subtype, 'x', 40);
    memset(worstItem.bodyPlan, 'x', 40);
    worstItem.materialCount = GEN_VISUALSPEC_MATERIAL_COUNT_MAX;
    for (int m = 0; m < GEN_VISUALSPEC_MATERIAL_COUNT_MAX; m++) memset(worstItem.materials[m], 'x', 24);
    memset(worstItem.distinctiveFeature, 'x', 120);

    size_t worstBytes = 0;
    char *specPrompt = BuildVisualSpecSpecPrompt(promptsDir, worstDomain, 4294967295u, worstError);
    if (!specPrompt)
    {
        if (err) snprintf(err, errSize, "prompt visualspec (spec) non costruibile (file mancanti in %s?)", promptsDir);
        return false;
    }
    worstBytes = strlen(specPrompt);
    free(specPrompt);

    char *freePrompt = BuildVisualSpecFreePrompt(promptsDir, worstDomain, 4294967295u, &worstItem, worstError);
    if (!freePrompt)
    {
        if (err) snprintf(err, errSize, "prompt visualspec (free) non costruibile (file mancanti in %s?)", promptsDir);
        return false;
    }
    size_t freeBytes = strlen(freePrompt);
    free(freePrompt);
    if (freeBytes > worstBytes) worstBytes = freeBytes;

    if (worstBytes > (size_t)GEN_VISUALSPEC_PROMPT_BYTE_CEILING)
    {
        if (err) snprintf(err, errSize,
            "prompt visualspec = %zu byte, oltre il ceiling di %d (vedi GEN_VISUALSPEC_PROMPT_BYTE_CEILING in gen_visualspec.h)",
            worstBytes, GEN_VISUALSPEC_PROMPT_BYTE_CEILING);
        return false;
    }
    return true;
}

/* ============================================================
   Scrittura del batch (tmp+rename, GenPublishFile).
   ============================================================ */

static bool WriteVisualSpecBatch(const GenVisualSpecItem *items, int count, unsigned int seed,
                                  const char *modelLabel, const char *outDir)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return false;
    cJSON_AddNumberToObject(root, "version", 1);
    cJSON_AddNumberToObject(root, "seed", seed);
    cJSON_AddStringToObject(root, "model", modelLabel ? modelLabel : "unknown");

    cJSON *requests = cJSON_AddArrayToObject(root, "requests");
    for (int i = 0; i < count; i++)
    {
        const GenVisualSpecItem *it = &items[i];
        cJSON *req = cJSON_CreateObject();
        cJSON_AddStringToObject(req, "id", it->id);
        cJSON_AddStringToObject(req, "domain", it->domain);

        cJSON *spec = cJSON_AddObjectToObject(req, "spec");
        cJSON_AddStringToObject(spec, "category", it->category);
        cJSON_AddStringToObject(spec, "subtype", it->subtype);
        cJSON_AddStringToObject(spec, "body_plan", it->bodyPlan);
        cJSON *materials = cJSON_AddArrayToObject(spec, "materials");
        for (int m = 0; m < it->materialCount; m++)
        {
            cJSON_AddItemToArray(materials, cJSON_CreateString(it->materials[m]));
        }
        cJSON_AddStringToObject(spec, "distinctive_feature", it->distinctiveFeature);
        cJSON_AddStringToObject(spec, "size_class", it->sizeClass);

        cJSON_AddStringToObject(req, "free_prompt", it->freePrompt);
        cJSON_AddItemToArray(requests, req);
    }

    char *text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!text) return false;

    char dirPath[512];
    snprintf(dirPath, sizeof(dirPath), "%s/visualspecs", outDir);
    if (GenEnsureDir(dirPath) != 0) { cJSON_free(text); return false; }

    char finalPath[560], tmpPath[560];
    snprintf(finalPath, sizeof(finalPath), "%s/batch.json", dirPath);
    snprintf(tmpPath, sizeof(tmpPath), "%s/batch.json.tmp", dirPath);

    FILE *f = fopen(tmpPath, "w");
    if (!f) { cJSON_free(text); return false; }
    fputs(text, f);
    cJSON_free(text);

    return GenPublishFile(f, tmpPath, finalPath) == 0;
}

/* ============================================================
   Ciclo completo (vedi gen_visualspec.h per il contratto).
   ============================================================ */

int GenVisualSpecGenerateBatch(GenLlmSession *sess, const char *promptsDir, const char *outDir,
                                int perDomain, unsigned int seed, const char *modelLabel)
{
    if (!sess) return 0;
    if (perDomain <= 0) perDomain = GEN_VISUALSPEC_PER_DOMAIN_DEFAULT;
    if (perDomain > GEN_VISUALSPEC_PER_DOMAIN_MAX) perDomain = GEN_VISUALSPEC_PER_DOMAIN_MAX;

    /* Buffer statici: stesso trattamento di 'static char raw[...]' in
       gen_attacks.c/main.c per gli output del modello -- fuori dallo stack
       di questa funzione, che a sua volta e' ricorsiva su niente ma tiene
       comunque due array grossi (items/acceptedSubtypes) che non hanno
       senso sullo stack di un chiamante ricorsivo altrove nel processo. */
    static GenVisualSpecItem items[GEN_VISUALSPEC_BATCH_CAP];
    static char acceptedSubtypesNorm[GEN_VISUALSPEC_BATCH_CAP][GEN_VISUALSPEC_SUBTYPE_NORM_CAP];
    int itemCount = 0;
    int acceptedCount = 0;
    int globalIndex = 0;   /* per il seed derivato seed+globalIndex*97, vedi gen_visualspec.h */

    for (int d = 0; d < GEN_VISUALSPEC_DOMAIN_COUNT; d++)
    {
        const char *domain = GEN_VISUALSPEC_DOMAINS[d];
        char *grammarText = GenVisualSpecBuildGrammar(domain);
        if (!grammarText)
        {
            GenLogLine("visualspecs: %s: grammatica non costruibile (visualspec.gbnf mancante?), dominio saltato", domain);
            globalIndex += perDomain;
            continue;
        }

        for (int idx = 0; idx < perDomain; idx++, globalIndex++)
        {
            unsigned int seedI = seed + (unsigned int)globalIndex*97u;
            char id[GEN_VISUALSPEC_ID_CAP];
            snprintf(id, sizeof(id), "%s_%02d", domain, idx + 1);

            GenVisualSpecItem item;
            memset(&item, 0, sizeof(item));
            snprintf(item.id, sizeof(item.id), "%s", id);
            snprintf(item.domain, sizeof(item.domain), "%s", domain);

            /* ---- Stadio (a): SPEC ---- */
            char specErr[GEN_VISUALSPEC_ERR_CAP];
            specErr[0] = '\0';
            char specNorm[GEN_VISUALSPEC_SUBTYPE_NORM_CAP];
            specNorm[0] = '\0';
            bool specOk = false;

            for (int attempt = 0; attempt < GEN_VISUALSPEC_MAX_ATTEMPTS && !specOk; attempt++)
            {
                char *prompt = BuildVisualSpecSpecPrompt(promptsDir, domain, seedI, attempt > 0 ? specErr : NULL);
                if (!prompt)
                {
                    snprintf(specErr, sizeof(specErr), "spec prompt not buildable (missing files in %s?)", promptsDir);
                    printf("melting-gen: visualspecs %s spec attempt %d/%d: %s\n",
                           id, attempt + 1, GEN_VISUALSPEC_MAX_ATTEMPTS, specErr);
                    break;
                }

                static char raw[GEN_VISUALSPEC_SPEC_RAW_CAP];
                unsigned int callSeed = seedI + (unsigned int)attempt*17u + 1u;
                int rc = GenLlmComplete(sess, prompt, grammarText, GEN_VISUALSPEC_SPEC_N_PREDICT, GEN_VISUALSPEC_TEMP,
                                         callSeed, NULL, NULL, 0, 0, raw, sizeof(raw), NULL);
                free(prompt);
                if (rc != 0)
                {
                    snprintf(specErr, sizeof(specErr), "generation failed (decode or token limit)");
                    printf("melting-gen: visualspecs %s spec attempt %d/%d: %s\n",
                           id, attempt + 1, GEN_VISUALSPEC_MAX_ATTEMPTS, specErr);
                    continue;
                }

                bool ok = ParseAndValidateSpec(raw, domain, acceptedSubtypesNorm, acceptedCount,
                                                &item, specNorm, sizeof(specNorm), specErr, sizeof(specErr));
                printf("melting-gen: visualspecs %s spec attempt %d/%d: %s\n",
                       id, attempt + 1, GEN_VISUALSPEC_MAX_ATTEMPTS, ok ? "OK" : specErr);
                if (ok) specOk = true;
            }

            if (!specOk)
            {
                GenLogLine("visualspecs: %s spec failed after %d attempts: %s", id, GEN_VISUALSPEC_MAX_ATTEMPTS, specErr);
                continue;
            }

            /* Il subtype e' "speso" appena lo spec valida, ANCHE se il
               free_prompt sotto fallisse (vedi gen_visualspec.h): il
               modello lo ha comunque gia' proposto in questa sessione. */
            if (acceptedCount < GEN_VISUALSPEC_BATCH_CAP)
            {
                snprintf(acceptedSubtypesNorm[acceptedCount], GEN_VISUALSPEC_SUBTYPE_NORM_CAP, "%s", specNorm);
                acceptedCount++;
            }

            /* ---- Stadio (b): FREE_PROMPT ---- */
            char freeErr[GEN_VISUALSPEC_ERR_CAP];
            freeErr[0] = '\0';
            bool freeOk = false;

            for (int attempt = 0; attempt < GEN_VISUALSPEC_FREE_MAX_ATTEMPTS && !freeOk; attempt++)
            {
                char *prompt = BuildVisualSpecFreePrompt(promptsDir, domain, seedI, &item, attempt > 0 ? freeErr : NULL);
                if (!prompt)
                {
                    snprintf(freeErr, sizeof(freeErr), "free prompt not buildable (missing files in %s?)", promptsDir);
                    printf("melting-gen: visualspecs %s free attempt %d/%d: %s\n",
                           id, attempt + 1, GEN_VISUALSPEC_FREE_MAX_ATTEMPTS, freeErr);
                    break;
                }

                static char raw[GEN_VISUALSPEC_FREE_RAW_CAP];
                unsigned int callSeed = seedI + (unsigned int)attempt*17u + 5003u;
                int rc = GenLlmComplete(sess, prompt, NULL, GEN_VISUALSPEC_FREE_N_PREDICT, GEN_VISUALSPEC_TEMP,
                                         callSeed, NULL, NULL, 0, 0, raw, sizeof(raw), NULL);
                free(prompt);
                if (rc != 0)
                {
                    snprintf(freeErr, sizeof(freeErr), "generation failed (decode or token limit)");
                    printf("melting-gen: visualspecs %s free attempt %d/%d: %s\n",
                           id, attempt + 1, GEN_VISUALSPEC_FREE_MAX_ATTEMPTS, freeErr);
                    continue;
                }

                char extracted[GEN_VISUALSPEC_FREE_PROMPT_CAP];
                ExtractFreePrompt(raw, extracted, sizeof(extracted));
                bool ok = ValidateFreePrompt(extracted, freeErr, sizeof(freeErr));
                printf("melting-gen: visualspecs %s free attempt %d/%d: %s\n",
                       id, attempt + 1, GEN_VISUALSPEC_FREE_MAX_ATTEMPTS, ok ? "OK" : freeErr);
                if (ok)
                {
                    snprintf(item.freePrompt, sizeof(item.freePrompt), "%s", extracted);
                    freeOk = true;
                }
            }

            if (!freeOk)
            {
                GenLogLine("visualspecs: %s free_prompt failed after %d attempts, request discarded: %s",
                           id, GEN_VISUALSPEC_FREE_MAX_ATTEMPTS, freeErr);
                continue;
            }

            if (itemCount < GEN_VISUALSPEC_BATCH_CAP) items[itemCount++] = item;
            GenLogLine("visualspecs: %s ok (subtype=\"%s\")", id, item.subtype);
        }

        free(grammarText);
    }

    if (itemCount == 0)
    {
        GenLogLine("visualspecs: no valid request in the whole batch, batch.json not written");
        return 0;
    }

    if (!WriteVisualSpecBatch(items, itemCount, seed, modelLabel, outDir))
    {
        GenLogLine("visualspecs: %d requests validated but batch.json write failed", itemCount);
        return 0;
    }

    GenLogLine("visualspecs: batch.json written with %d/%d requests", itemCount, GEN_VISUALSPEC_DOMAIN_COUNT*perDomain);
    return itemCount;
}
