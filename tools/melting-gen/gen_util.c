#include "melting_gen.h"

#include <dirent.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

double GenNowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

char *GenReplaceAll(const char *src, const char *find, const char *repl)
{
    if (!src) return NULL;
    if (!find || !find[0]) return strdup(src);
    const char *safeRepl = repl ? repl : "";
    size_t findLen = strlen(find);
    size_t replLen = strlen(safeRepl);

    size_t count = 0;
    for (const char *p = src; (p = strstr(p, find)) != NULL; p += findLen) count++;

    /* Se repl e' piu' corto di find l'output e' SOLO piu' corto di src:
       strlen(src)+1 resta una stima generosa (mai insufficiente) e non vale
       la pena calcolare la dimensione esatta in quel ramo. */
    size_t extra = (replLen > findLen) ? count*(replLen - findLen) : 0;
    char *out = malloc(strlen(src) + extra + 1);
    if (!out) return NULL;

    char *w = out;
    const char *r = src;
    const char *hit;
    while ((hit = strstr(r, find)) != NULL)
    {
        size_t chunk = (size_t)(hit - r);
        memcpy(w, r, chunk); w += chunk;
        memcpy(w, safeRepl, replLen); w += replLen;
        r = hit + findLen;
    }
    strcpy(w, r);
    return out;
}

char *GenChatMlWrap(const char *sys, const char *user)
{
    if (!sys || !user) return NULL;
    size_t total = strlen(sys) + strlen(user) + 128;
    char *prompt = malloc(total);
    if (!prompt) return NULL;
    snprintf(prompt, total,
             "<|im_start|>system\n%s<|im_end|>\n<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n",
             sys, user);
    return prompt;
}

unsigned int GenRngNext(unsigned int *state)
{
    unsigned int s = *state ? *state : 0xA341316Cu;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    if (!s) s = 0xA341316Cu;
    *state = s;
    return s;
}

int GenRngRange(unsigned int *state, int min, int max)
{
    if (max <= min) return min;
    return min + (int)(GenRngNext(state) % (unsigned int)(max - min + 1));
}

void GenHsvToHex(double h, double s, double v, char out[8])
{
    h = fmod(fmod(h, 360.0) + 360.0, 360.0);
    double c = v*s;
    double m = fmod(h/60.0, 2.0) - 1.0;
    double x = c*(1.0 - (m < 0 ? -m : m));
    double base = v - c;
    double r = 0, g = 0, b = 0;
    if (h < 60)       { r = c; g = x; }
    else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; }
    else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x; b = c; }
    else              { r = c; b = x; }
    snprintf(out, 8, "#%02x%02x%02x",
             (int)lround((r + base)*255.0),
             (int)lround((g + base)*255.0),
             (int)lround((b + base)*255.0));
}

int GenEnsureDir(const char *path)
{
    if (mkdir(path, 0755) == 0) return 0;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) ? 0 : -1;
}

char *GenReadFile(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[got] = '\0';
    return buf;
}

int GenFileExists(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

void GenProgressWrite(const char *outDir, const char *phase, int percent, const char *message)
{
    /* outDir NULL = "non scrivere" (--bench, tools/melting-gen/main.c: non deve
     * toccare generated/, nemmeno tramite il progress callback di caricamento
     * del modello). Ogni altro chiamante passa sempre un outDir vero, quindi
     * questa guardia non cambia nulla per loro. */
    if (!outDir) return;
    char tmp[512], fin[512];
    snprintf(fin, sizeof(fin), "%s/gen_progress.txt", outDir);
    snprintf(tmp, sizeof(tmp), "%s/gen_progress.tmp", outDir);
    FILE *f = fopen(tmp, "w");
    if (!f) return;
    fprintf(f, "%s|%d|%s\n", phase, percent, message);
    fclose(f);
    rename(tmp, fin);
}

/* Rimuove gli script Lua di una generazione PRECEDENTE (step B2, correzione da
 * review). Va chiamata all'inizio di ogni generazione NON di ripresa.
 *
 * Il bug che chiude: i file si chiamano <outDir>/scripts/floorN_itemM.lua --
 * un nome indicizzato SOLO da piano e oggetto, che non porta con se' nessuna
 * identita' della run (ne' il seed, ne' il nome dell'oggetto). Nessuno li ha mai
 * cancellati (nemmeno `make clean`, che tocca solo bin/). Finche' erano scritti e
 * riletti dalla stessa generazione non faceva differenza; da quando esiste la
 * RIPRESA in sottofondo (GenLuaLoadExisting), invece, il secondo processo li
 * ADOTTA: se sul disco erano rimasti gli script di una run precedente, la run
 * NUOVA se li prendeva -- il comportamento inventato per "Guanto di Chiodi" di
 * ieri finiva addosso a "Aureola Gelida" di oggi, con un nome, un tema e dei trait
 * che non c'entrano nulla. Silenzioso: lo script e' valido, semplicemente non e'
 * il suo.
 *
 * Non e' un errore se la cartella non esiste o se un file non si lascia
 * cancellare: e' pulizia opportunistica, non una precondizione. */
void GenRemoveOldScripts(const char *outDir)
{
    if (!outDir) return;
    for (int f = 1; f <= GEN_FLOORS; f++)
    {
        char path[512];
        for (int i = 1; i <= GEN_ITEMS; i++)
        {
            snprintf(path, sizeof(path), "%s/scripts/floor%d_item%d.lua", outDir, f, i);
            remove(path);
        }
        snprintf(path, sizeof(path), "%s/scripts/floor%d_bossItem.lua", outDir, f);
        remove(path);
    }
}

int GenPublishFile(FILE *f, const char *tmpPath, const char *finalPath)
{
    /* ferror() va controllato PRIMA di fclose(): dopo la chiusura lo stream
       non e' piu' valido. Un disco pieno fa fallire le fprintf/fwrite senza
       che il chiamante lo controlli riga per riga: e' qui che l'errore viene
       intercettato prima che il file troncato prenda il posto di quello
       valido. */
    int hadError = ferror(f);
    int closeFailed = (fclose(f) != 0);
    if (hadError || closeFailed)
    {
        remove(tmpPath);
        return -1;
    }
    if (rename(tmpPath, finalPath) != 0)
    {
        remove(tmpPath);
        return -1;
    }
    return 0;
}

void GenLogLine(const char *fmt, ...)
{
    GenEnsureDir("logs");
    FILE *f = fopen("logs/melting-gen.log", "a");
    if (!f) return;
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmv);
    fprintf(f, "[%s] ", stamp);
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fputc('\n', f);
    fclose(f);
}

/* Tabelle condivise: stesse regole di TRAIT_SCRIPT_RULES in llm/run_content.mjs. */
const char *GEN_SLOTS[6] = { "hat", "eyes", "hand", "back", "body", "aura" };
const char *GEN_TRAITS[9] = {
    "bounce", "homing", "explode", "split", "pierce", "rapid", "giant", "slow", "vamp"
};
/* Vedi il commento su GenItem.kind in melting_gen.h: mai scelto dal modello,
 * sempre assegnato in C. bossItem resta sempre "statup" (invariato); i 15
 * oggetti normali di items[] (5 piani x 3) ricevono un MIX delle 4 categorie
 * (GenKindMinimumCounts sotto), non piu' sempre "active". */
const char *GEN_KINDS[GEN_KIND_COUNT] = { "passive", "active", "graft", "statup" };

/* Mix di categorie per i 15 oggetti normali (items[]) di un'intera run
 * (task "melting-gen emette e valida le 4 categorie"): DEFAULT PROPOSTO
 * DALL'IMPLEMENTAZIONE (stile DEC-019), da confermare col playtest -- vedi
 * la domanda aperta annotata durante questa fase. Stesso ordine di
 * GEN_KINDS sopra: passivo maggioritario (nessun limite di slot, e' stato
 * l'unico contenuto reale finora), attivo e Innesto piu' rari (hanno slot
 * limitati: trovarne uno e' un evento, non la norma), stat-up presente ma
 * minoritario nel pool normale (DEC-035, "gli stat-up compaiono anche nei
 * pool normali, non solo come ricompensa boss" -- il boss ne ha comunque
 * SEMPRE uno garantito a parte, bossItem, invariato). */
static const int GEN_KIND_WEIGHTS_NORMAL[GEN_KIND_COUNT] = { 50, 20, 15, 15 };

static const GenTraitRule GEN_TRAIT_RULES[9] = {
    { "bounce",  "on_fire", "burst",       2, 0.25 },
    { "homing",  "on_hit",  "projectile",  2, 260  },
    { "explode", "on_hit",  "area",       58, 0.48 },
    { "split",   "on_fire", "burst",       3, 0.36 },
    { "pierce",  "on_hit",  "projectile",  1, 420  },
    { "rapid",   "on_fire", "burst",       2, 0.16 },
    { "giant",   "on_hit",  "area",       44, 0.34 },
    { "slow",    "on_hit",  "area",       54, 0.22 },
    { "vamp",    "on_hit",  "heal",       18, 1    },
};

const GenTraitRule *GenTraitRuleFor(const char *trait)
{
    for (int i = 0; i < 9; i++)
    {
        if (trait && strcmp(GEN_TRAIT_RULES[i].trait, trait) == 0) return &GEN_TRAIT_RULES[i];
    }
    return NULL;
}

/* ============================================================
   Rarita' (fase 3b, docs/engineering/specs/2026-07-13-pools-rarity-design.md,
   sezioni 1-3). Tavole dichiarative -- MODIFICA QUI per bilanciare/espandere.
   ============================================================ */

/* Ordine canonico: deve restare sincronizzato a mano con l'enum Rarity in
   core/game_types.h (RARITY_COMMON=0 .. RARITY_LEGENDARY=3) e con
   RarityFromText in src/content/run_content.c, che traduce questi stessi
   quattro testi nel verso opposto lato gioco. */
const char *GEN_RARITIES[4] = { "common", "uncommon", "rare", "legendary" };

/* Pesi di drop per pool (design sezione 3): un intero per cella, MODIFICA
   QUI per ribilanciare la frequenza -- non serve che sommino a 100, solo
   proporzioni fra loro (GenRollRarity sotto normalizza sul totale della
   riga). Tesoro/negozio pescano dalla prima riga (mista, common piu'
   frequente), il boss SEMPRE dalla seconda (zero peso su comune/non-comune:
   "il boss da' sempre roba buona", mai un premio deludente). */
static const int GEN_RARITY_WEIGHTS_TREASURE_SHOP[4] = { 55, 30, 12, 3 };
static const int GEN_RARITY_WEIGHTS_BOSS[4]          = {  0,  0, 70, 30 };

/* Frasi di intensita' per il prompt Lua per-oggetto (design sezione 2:
   "la rarita' entra nel prompt come intensita'"), in inglese (DEC-052,
   generazione IA inglese-first) -- MODIFICA QUI per cambiare quanto il
   prompt spinge verso numeri piccoli o grandi.
   Stesso ordine di GEN_RARITIES sopra. Iniettate da gen_lua.c
   (BuildLuaPrompt) al posto del placeholder {ITEM_RARITY} nei template
   prompts/lua_user.txt e prompts/lua_statup_user.txt.

   VOLUTAMENTE brevi (poche parole, non frasi): il prompt Lua di un oggetto
   e' gia' vicino al tetto n_ctx=4096 della sessione (lua_system.txt, il
   cheat-sheet condiviso, e' gia' lungo da solo, vedi GEN_LLM_SESSION_N_CTX
   in gen_llm.c) -- una frase intera qui per ognuno dei 20 oggetti di una
   run ha fatto sforare il budget e mandato in fallback OGNI oggetto
   (trovato girando davvero MODEL=...7b... make test-llm durante lo
   sviluppo di questa fase, vedi il report di fase). */
const char *GEN_RARITY_PROMPT_HINTS[4] = {
    "common (small numbers)",
    "uncommon (moderate numbers)",
    "rare (high numbers)",
    "legendary (big numbers)",
};

int GenRollRarity(unsigned int *rng, int isBoss)
{
    const int *w = isBoss ? GEN_RARITY_WEIGHTS_BOSS : GEN_RARITY_WEIGHTS_TREASURE_SHOP;
    int total = w[0] + w[1] + w[2] + w[3];
    int roll = GenRngRange(rng, 0, total - 1);
    int acc = 0;
    for (int i = 0; i < 4; i++)
    {
        acc += w[i];
        if (roll < acc) return i;
    }
    return 3;   /* difesa: mai raggiunto se i pesi sommano davvero a 'total' */
}

int GenRarityIndexFromText(const char *text)
{
    for (int i = 0; i < 4; i++)
    {
        if (text && strcmp(GEN_RARITIES[i], text) == 0) return i;
    }
    return -1;
}

/* ============================================================
   Garanzia di copertura DEC-144-style, generalizzata a 4 categorie
   qualunque (rarita' O kind: vedi il commento in melting_gen.h). Algoritmo
   duplicato AD HOC da ItemPoolMinimumCounts (src/gameplay/item_pool.c),
   stessa struttura, stesso ordine di operazioni: arrotondamento per difetto
   per proporzione, poi garanzia di almeno 1 per ogni peso > 0 (residuo
   preso prima dall'arrotondamento poi dalle categorie piu' comuni, indice
   piu' basso prima), totale sempre 'poolSize'. Vedi item_pool.h per il
   perche' non si linka quel modulo da qui (raylib via game_types.h).
   ============================================================ */

void GenPoolMinimumCounts(int poolSize, const int weights[GEN_KIND_COUNT], int outCounts[GEN_KIND_COUNT])
{
    if (!weights || !outCounts) return;
    if (poolSize < 0) poolSize = 0;

    int total = 0;
    for (int i = 0; i < GEN_KIND_COUNT; i++) if (weights[i] > 0) total += weights[i];

    int counts[GEN_KIND_COUNT] = { 0, 0, 0, 0 };
    if (total <= 0)
    {
        counts[0] = poolSize;   /* tabella senza pesi utili: tutto sulla prima categoria, ripiego sicuro */
        for (int i = 0; i < GEN_KIND_COUNT; i++) outCounts[i] = counts[i];
        return;
    }

    int sum = 0;
    for (int i = 0; i < GEN_KIND_COUNT; i++)
    {
        if (weights[i] > 0) counts[i] = (poolSize*weights[i])/total;
        sum += counts[i];
    }
    int leftover = poolSize - sum;

    /* Garanzia: ogni categoria a peso > 0 compare almeno una volta, presa
       dal residuo, a partire dall'ULTIMA (indice piu' alto, di norma la piu'
       rara nelle tabelle di questo file) verso la prima. */
    for (int i = GEN_KIND_COUNT - 1; i >= 0 && leftover > 0; i--)
    {
        if (weights[i] > 0 && counts[i] == 0) { counts[i] = 1; leftover--; }
    }
    /* Se il residuo non bastava, si toglie alle categorie piu' comuni
       (indice piu' basso) ancora capienti. */
    for (int i = 0; i < GEN_KIND_COUNT; i++)
    {
        if (weights[i] > 0 && counts[i] == 0)
        {
            int donor = 0;
            while (donor < GEN_KIND_COUNT && (donor == i || counts[donor] <= 0)) donor++;
            if (donor < GEN_KIND_COUNT) { counts[donor]--; counts[i] = 1; }
        }
    }
    /* Residuo ancora positivo (poolSize non proporzionale ai pesi): alla
       prima categoria a peso > 0. */
    if (leftover > 0)
    {
        int i = 0;
        while (i < GEN_KIND_COUNT - 1 && weights[i] <= 0) i++;
        counts[i] += leftover;
    }

    for (int i = 0; i < GEN_KIND_COUNT; i++) outCounts[i] = counts[i];
}

void GenRarityMinimumCounts(int poolSize, int isBoss, int outCounts[4])
{
    const int *w = isBoss ? GEN_RARITY_WEIGHTS_BOSS : GEN_RARITY_WEIGHTS_TREASURE_SHOP;
    GenPoolMinimumCounts(poolSize, w, outCounts);
}

void GenKindMinimumCounts(int poolSize, int outCounts[GEN_KIND_COUNT])
{
    GenPoolMinimumCounts(poolSize, GEN_KIND_WEIGHTS_NORMAL, outCounts);
}

void GenShuffleInts(unsigned int *rng, int *arr, int n)
{
    if (!rng || !arr) return;
    for (int i = n - 1; i > 0; i--)
    {
        int j = GenRngRange(rng, 0, i);
        int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

void GenValidateItemRecharge(GenItem *item)
{
    if (!item) return;
    if (strcmp(item->kind, "active") != 0) return;
    if (item->charges > 0 || item->cooldown > 0.0f) return;   /* gia' conforme */

    GenLogLine("validate: \"%s\" e' kind=active senza cariche ne' cooldown (difetto di contenuto) "
               "-> ripiego sul cooldown di riserva del motore (%.1fs, GEN_ACTIVE_DEFAULT_COOLDOWN)",
               item->name, (double)GEN_ACTIVE_DEFAULT_COOLDOWN);
    item->cooldown = GEN_ACTIVE_DEFAULT_COOLDOWN;
}

/* ============================================================
   FNV-1a 64 bit (RunBundle v1): vedi il commento su GEN_FNV1A64_OFFSET in
   melting_gen.h -- NON e' un hash di sicurezza, solo una checksum veloce per
   sapere "con quali prompt e' nata questa run".
   ============================================================ */

unsigned long long GenFnv1a64(unsigned long long hash, const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < len; i++)
    {
        hash ^= (unsigned long long)p[i];
        hash *= 1099511628211ULL;   /* prima FNV-1a a 64 bit */
    }
    return hash;
}

/* Comparatore per qsort: ordine alfabetico semplice (strcmp), le voci sono
   array a lunghezza fissa (vedi GenPromptsFnv sotto). */
static int CompareFilenames(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

/* 'dir'/'name' sono di lunghezza arbitraria (promptsDir arriva da --prompts
   sulla riga di comando): un buffer a dimensione dichiarata ("%s/%s" dentro
   char[512]) farebbe scattare -Wformat-truncation, e stavolta NON sarebbe un
   falso positivo (vedi il commento analogo in gen_manifest.c per il caso
   opposto, dove lo e'): gcc non puo' sapere che promptsDir resta corto in
   pratica. Un buffer malloc a dimensione ESATTA elimina il warning invece di
   sopprimerlo, ed e' corretto per qualunque lunghezza in ingresso. NULL su
   fallimento di allocazione. */
static char *JoinPath(const char *dir, const char *name)
{
    size_t dirLen = strlen(dir);
    size_t nameLen = strlen(name);
    char *path = malloc(dirLen + 1 + nameLen + 1);
    if (!path) return NULL;
    memcpy(path, dir, dirLen);
    path[dirLen] = '/';
    memcpy(path + dirLen + 1, name, nameLen);
    path[dirLen + 1 + nameLen] = '\0';
    return path;
}

/* Limite di file dentro promptsDir: la cartella vera (tools/melting-gen/prompts/)
   ne ha 5 (system.txt, user.txt, lua_system.txt, lua_user.txt,
   lua_statup_user.txt). 128 e' un tetto largo e fisso, coerente con lo stile
   del resto di questo file (buffer a dimensione dichiarata) -- file oltre il
   tetto vengono ignorati in silenzio, non e' pensato per cartelle enormi. I
   NOMI restano in buffer a dimensione dichiarata (256, ampiamente sufficiente
   per un nome di file): solo il PATH completo (dir+nome, vedi JoinPath sopra)
   ha lunghezza davvero arbitraria. */
#define GEN_PROMPTS_FNV_MAX_FILES 128

int GenPromptsFnv(const char *promptsDir, unsigned long long *out)
{
    if (!promptsDir || !out) return -1;
    DIR *dir = opendir(promptsDir);
    if (!dir) return -1;

    /* Nomi soltanto (mai il percorso intero): qsort li ordina, poi si
       ricostruisce il path file per file nel giro di lettura sotto. */
    char names[GEN_PROMPTS_FNV_MAX_FILES][256];
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < GEN_PROMPTS_FNV_MAX_FILES)
    {
        if (entry->d_name[0] == '.') continue;   /* salta "." ".." e i nascosti */
        char *path = JoinPath(promptsDir, entry->d_name);
        if (!path) { closedir(dir); return -1; }
        struct stat st;
        int isRegular = (stat(path, &st) == 0 && S_ISREG(st.st_mode));
        free(path);
        if (!isRegular) continue;
        snprintf(names[count], sizeof(names[count]), "%s", entry->d_name);
        count++;
    }
    closedir(dir);
    if (count == 0) return -1;

    qsort(names, (size_t)count, sizeof(names[0]), CompareFilenames);

    unsigned long long hash = GEN_FNV1A64_OFFSET;
    for (int i = 0; i < count; i++)
    {
        char *path = JoinPath(promptsDir, names[i]);
        if (!path) return -1;
        char *text = GenReadFile(path);
        free(path);
        if (!text) return -1;
        hash = GenFnv1a64(hash, text, strlen(text));
        free(text);
    }
    *out = hash;
    return 0;
}

/* M5 (DEC-005): vedi il commento di dichiarazione in melting_gen.h. Formato
 * atteso "<name> -- <blurb>" (stesso separatore " -- " di provenance.txt
 * chosenTheme= e del file scritto dal gioco, src/app/app.c
 * AppWriteChosenThemeFile). Nessuna virgoletta/backslash da spezzare (il
 * testo viene da propose.gbnf, charset ASCII senza quei caratteri, vedi
 * GenThemeProposal): uno strstr sul separatore letterale basta. */
bool GenLoadChosenTheme(const char *path, GenChosenTheme *out)
{
    if (!path || !out) return false;
    char *text = GenReadFile(path);
    if (!text) return false;

    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) text[--len] = '\0';

    const char *sep = strstr(text, " -- ");
    if (!sep || len == 0)
    {
        free(text);
        return false;
    }

    memset(out, 0, sizeof(*out));
    size_t nameLen = (size_t)(sep - text);
    if (nameLen >= sizeof(out->name)) nameLen = sizeof(out->name) - 1;
    memcpy(out->name, text, nameLen);
    out->name[nameLen] = '\0';

    snprintf(out->blurb, sizeof(out->blurb), "%s", sep + 4);
    snprintf(out->raw, sizeof(out->raw), "%s", text);
    free(text);
    return out->name[0] != '\0';
}
