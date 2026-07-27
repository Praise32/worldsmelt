#include "melting_gen.h"

#include "cJSON.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Stesse regole di llm/run_content.mjs (SCRIPT_BOUNDS, OP_TRAITS, SCRIPT_TRAIT_PRIORITY).
 *
 * Divergenze note e volute rispetto a normalizeRun() in llm/run_content.mjs,
 * per risparmiare a chi legge un'indagine inutile:
 *  - I trait vengono deduplicati e ordinati per priorita' gia' in fase di
 *    raccolta qui in C (NormalizeTraits), mentre in Node succede dentro lo
 *    script di normalizzazione: equivalente ai fini del gioco, perche' il
 *    gioco memorizza i trait come bitmask (l'ordine di inserimento non
 *    sopravvive comunque).
 *  - Le stringhe numeriche tipo "a": "3" vengono rifiutate qui (JsonNumber
 *    accetta solo cJSON_IsNumber) invece che convertite, a differenza del
 *    ramo Node che le forza a numero: il fallback per campo copre il caso.
 *  - I nomi vengono troncati alla dimensione del campo C (name[48], vedi
 *    melting_gen.h) invece di essere rifiutati o troncati diversamente lato
 *    Node.
 */

static const char *SCRIPT_TRAIT_PRIORITY[9] = {
    "split", "bounce", "rapid", "homing", "pierce", "explode", "slow", "giant", "vamp"
};

typedef struct OpBounds {
    const char *op;
    double aMin, aMax, bMin, bMax;
    int roundA, roundB;
} OpBounds;

static const OpBounds OP_BOUNDS[4] = {
    { "burst",       1,  6, 0.05, 1.20, 1, 0 },
    { "projectile",  1,  6, 120,  720,  1, 0 },
    { "area",       18, 96, 0.05, 1.15, 0, 0 },
    { "heal",        0, 60, 1,    2,    1, 1 },
};

static const OpBounds *BoundsFor(const char *op)
{
    for (int i = 0; i < 4; i++) if (strcmp(OP_BOUNDS[i].op, op) == 0) return &OP_BOUNDS[i];
    return &OP_BOUNDS[1];
}

static int OpAllowsTrait(const char *op, const char *trait)
{
    static const char *burst[]      = { "split", "bounce", "rapid", "homing", "pierce", NULL };
    static const char *projectile[] = { "homing", "pierce", "bounce", "rapid", NULL };
    static const char *area[]       = { "explode", "slow", "giant", NULL };
    static const char *heal[]       = { "vamp", NULL };
    const char **list = NULL;
    if (strcmp(op, "burst") == 0) list = burst;
    else if (strcmp(op, "projectile") == 0) list = projectile;
    else if (strcmp(op, "area") == 0) list = area;
    else if (strcmp(op, "heal") == 0) list = heal;
    if (!list || !trait) return 0;
    for (int i = 0; list[i]; i++) if (strcmp(list[i], trait) == 0) return 1;
    return 0;
}

static int TraitIndex(const char *trait)
{
    for (int i = 0; i < 9; i++) if (trait && strcmp(GEN_TRAITS[i], trait) == 0) return i;
    return -1;
}

static int PriorityIndex(const char *trait)
{
    for (int i = 0; i < 9; i++) if (trait && strcmp(SCRIPT_TRAIT_PRIORITY[i], trait) == 0) return i;
    return 9;
}

static const char *JsonString(const cJSON *obj, const char *key)
{
    if (!obj) return NULL;
    const cJSON *v = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    return (cJSON_IsString(v) && v->valuestring) ? v->valuestring : NULL;
}

static double JsonNumber(const cJSON *obj, const char *key, double fallback, int *ok)
{
    *ok = 0;
    if (!obj) return fallback;
    const cJSON *v = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!cJSON_IsNumber(v)) return fallback;
    *ok = 1;
    return v->valuedouble;
}

static void CopyText(char *dst, size_t dstSize, const char *src, const char *fallback)
{
    const char *use = fallback;
    if (src)
    {
        while (*src == ' ' || *src == '\t') src++;
        if (*src) use = src;
    }
    snprintf(dst, dstSize, "%s", use ? use : "");
    size_t len = strlen(dst);
    while (len > 0 && (dst[len - 1] == ' ' || dst[len - 1] == '\t')) dst[--len] = '\0';
}

static int IsHexColor(const char *text)
{
    if (!text || text[0] != '#' || strlen(text) != 7) return 0;
    for (int i = 1; i < 7; i++)
    {
        char c = text[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return 0;
    }
    return 1;
}

static void CopyColor(char *dst, size_t dstSize, const char *src, const char *fallback)
{
    snprintf(dst, dstSize, "%s", IsHexColor(src) ? src : fallback);
}

static double ClampD(double v, double min, double max)
{
    return v < min ? min : (v > max ? max : v);
}

static void NormalizeTraits(const cJSON *rawTraits, const GenItem *fbItem, GenItem *out)
{
    out->traitCount = 0;
    if (cJSON_IsArray((cJSON *)rawTraits))
    {
        const cJSON *el = NULL;
        cJSON_ArrayForEach(el, (cJSON *)rawTraits)
        {
            if (out->traitCount >= 2) break;
            if (!cJSON_IsString(el) || TraitIndex(el->valuestring) < 0) continue;
            int dup = 0;
            for (int i = 0; i < out->traitCount; i++)
            {
                if (strcmp(out->traits[i], el->valuestring) == 0) dup = 1;
            }
            if (!dup) snprintf(out->traits[out->traitCount++], sizeof(out->traits[0]), "%s", el->valuestring);
        }
    }
    if (out->traitCount == 0)
    {
        for (int i = 0; i < fbItem->traitCount && i < 2; i++)
        {
            snprintf(out->traits[out->traitCount++], sizeof(out->traits[0]), "%s", fbItem->traits[i]);
        }
    }
    if (out->traitCount == 2 && PriorityIndex(out->traits[0]) > PriorityIndex(out->traits[1]))
    {
        char tmp[10];
        memcpy(tmp, out->traits[0], sizeof(tmp));
        memcpy(out->traits[0], out->traits[1], sizeof(tmp));
        memcpy(out->traits[1], tmp, sizeof(tmp));
    }
}

static const char *PickScriptTrait(const char *op, const GenItem *item, const char *want)
{
    if (want && OpAllowsTrait(op, want))
    {
        for (int i = 0; i < item->traitCount; i++)
        {
            if (strcmp(item->traits[i], want) == 0) return want;
        }
    }
    for (int i = 0; i < item->traitCount; i++)
    {
        if (OpAllowsTrait(op, item->traits[i])) return item->traits[i];
    }
    return "none";
}

static const char *PreferredOpForTraits(const GenItem *item)
{
    const GenTraitRule *rule = item->traitCount > 0 ? GenTraitRuleFor(item->traits[0]) : NULL;
    return rule ? rule->op : "projectile";
}

/* DEC-162 (docs/design/systems/synergies.md#budget-di-potenza-del-risultato) e i
 * campi 'a'/'b' della mini-VM: NON si allarga niente qui. Il budget di POTENZA
 * dedicato al risultato di una sinergia e' garanzia a runtime
 * (ScriptItemsClampSynergyResultDelta, src/script/script_items.c), che agisce
 * sulle statistiche del giocatore risultanti, non sui singoli campi scritti qui.
 * Un allargamento gemello di OP_BOUNDS in QUESTO file (tentato in una revisione
 * precedente per il caso di un singolo oggetto che dichiara gia' da solo una
 * coppia di trait canonica) e' stato tolto perche' dimostrabilmente INERTE su
 * due fronti: il motore clampa comunque 'a'/'b' alle bande base in esecuzione
 * (src/gameplay/script_vm.c, stessi numeri di OP_BOUNDS sotto), e una sinergia
 * non si accende MAI da un oggetto solo (SignalPresent/excludeItem in
 * src/gameplay/synergies.c: "una sinergia e' fra DUE oggetti diversi"), quindi
 * quel caso non produce nemmeno il risultato di cui allargava il budget.
 * Il controllo DEC-162 che gira davvero a tempo di generazione e' un altro, ed
 * e' in NormalizeShot sotto: il RISULTATO di una sinergia che il tipo di colpo
 * generato dichiara da solo (chain/pierce) deve stare nel budget di
 * LEGGIBILITA', che per le sinergie non si allarga -- l'unico pezzo del
 * risultato che nessuna garanzia a runtime copre. */

static void NormalizeScriptOp(const cJSON *rawOp, const GenItem *item, GenScriptOp *out)
{
    const char *rawTrigger = JsonString(rawOp, "trigger");
    const char *rawKind = JsonString(rawOp, "op");
    const char *trigger = (rawTrigger && strcmp(rawTrigger, "on_fire") == 0) ? "on_fire" : "on_hit";
    const char *kind = "projectile";
    if (rawKind && (strcmp(rawKind, "burst") == 0 || strcmp(rawKind, "projectile") == 0 ||
                    strcmp(rawKind, "area") == 0 || strcmp(rawKind, "heal") == 0)) kind = rawKind;

    if (strcmp(trigger, "on_fire") == 0) kind = "burst";
    else if (strcmp(kind, "burst") == 0) kind = "projectile";

    int anyCompatible = 0;
    for (int i = 0; i < item->traitCount; i++)
    {
        if (OpAllowsTrait(kind, item->traits[i])) anyCompatible = 1;
    }
    if (!anyCompatible) kind = PreferredOpForTraits(item);

    const char *trait = PickScriptTrait(kind, item, JsonString(rawOp, "trait"));
    const GenTraitRule *rule = GenTraitRuleFor(trait);
    double defA = rule ? rule->a : 1;
    double defB = rule ? rule->b : (strcmp(kind, "projectile") == 0 ? 280 : 0.35);
    const OpBounds *bounds = BoundsFor(kind);

    int okA = 0, okB = 0;
    double a = JsonNumber(rawOp, "a", defA, &okA);
    double b = JsonNumber(rawOp, "b", defB, &okB);
    a = ClampD(okA ? a : defA, bounds->aMin, bounds->aMax);
    b = ClampD(okB ? b : defB, bounds->bMin, bounds->bMax);
    if (bounds->roundA) a = floor(a + 0.5);
    if (bounds->roundB) b = floor(b + 0.5);

    snprintf(out->trigger, sizeof(out->trigger), "%s", strcmp(kind, "burst") == 0 ? "on_fire" : "on_hit");
    snprintf(out->op, sizeof(out->op), "%s", kind);
    out->a = a;
    out->b = b;
    snprintf(out->trait, sizeof(out->trait), "%s", trait);
}

static void NormalizeScript(const cJSON *rawScript, const GenItem *fbItem, GenItem *item)
{
    item->opCount = 0;
    if (cJSON_IsArray((cJSON *)rawScript))
    {
        const cJSON *el = NULL;
        cJSON_ArrayForEach(el, (cJSON *)rawScript)
        {
            if (item->opCount >= GEN_MAX_OPS) break;
            if (!cJSON_IsObject(el)) continue;
            NormalizeScriptOp(el, item, &item->ops[item->opCount]);
            item->opCount++;
        }
    }
    for (int t = 0; t < item->traitCount && item->opCount < GEN_MAX_OPS; t++)
    {
        int covered = 0;
        for (int s = 0; s < item->opCount; s++)
        {
            if (strcmp(item->ops[s].trait, item->traits[t]) == 0) covered = 1;
        }
        const GenTraitRule *rule = GenTraitRuleFor(item->traits[t]);
        if (covered || !rule) continue;
        GenScriptOp *op = &item->ops[item->opCount++];
        snprintf(op->trigger, sizeof(op->trigger), "%s", rule->trigger);
        snprintf(op->op, sizeof(op->op), "%s", rule->op);
        op->a = rule->a;
        op->b = rule->b;
        snprintf(op->trait, sizeof(op->trait), "%s", rule->trait);
    }
    if (item->opCount == 0)
    {
        memcpy(item->ops, fbItem->ops, sizeof(item->ops));
        item->opCount = fbItem->opCount;
    }
}

/* Tipo di colpo del piano (step C): l'UNICO campo nuovo di questa fase che il
   MODELLO scrive davvero (kind/rarity restano decisioni di bilanciamento prese in
   C, vedi sotto). Qui si fa quello che questo file fa per ogni altro campo --
   ripiego per-campo su quello procedurale, mai un errore fatale -- piu' una cosa
   in piu' che gli altri campi non hanno: ShotTypeBalance (core/shot_type.c), che
   riporta il tipo dentro la banda di potenza qualunque numero il modello abbia
   scritto. E' la ragione per cui si puo' lasciare che sia un 7B a inventare i
   modi di sparare: la creativita' e' sua, l'equilibrio e' del C. */
static void NormalizeShot(const cJSON *rawFloor, const GenFloor *fbFloor, GenFloor *floor, int floorNum)
{
    const cJSON *rawShot = rawFloor ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawFloor, "shot") : NULL;

    if (!cJSON_IsObject((cJSON *)rawShot))
    {
        floor->shot = fbFloor->shot;   /* nessun tipo di colpo nel JSON: quello procedurale, mai un piano senza */
    }
    else
    {
        const GenFloor *fb = fbFloor;
        ShotTypeDef type;
        memset(&type, 0, sizeof(type));
        type.active = true;
        CopyText(type.name, sizeof(type.name), JsonString(rawShot, "name"), fb->shot.name);

        /* Una forma sconosciuta NON ricade su SHOT_FORM_ORB (che e' il colpo
           base: un "tipo di colpo nuovo" che si disegna come quello di sempre non
           e' un tipo nuovo), ma sulla forma del ripiego procedurale. */
        const char *formText = JsonString(rawShot, "form");
        ShotForm form = ShotFormFromText(formText);
        if (!formText || (form == SHOT_FORM_ORB && strcmp(formText, "orb") != 0)) form = fb->shot.form;
        type.form = form;

        int ok = 0;
        type.speedMul    = (float)JsonNumber(rawShot, "speed",   fb->shot.speedMul,  &ok);
        type.damageMul   = (float)JsonNumber(rawShot, "damage",  fb->shot.damageMul, &ok);
        type.radiusMul   = (float)JsonNumber(rawShot, "size",    fb->shot.radiusMul, &ok);
        type.lifeMul     = (float)JsonNumber(rawShot, "life",    fb->shot.lifeMul,   &ok);
        type.pierceBonus = (int)JsonNumber(rawShot, "pierce",  0.0, &ok);
        type.chain       = (int)JsonNumber(rawShot, "chain",   0.0, &ok);
        type.pellets     = (int)JsonNumber(rawShot, "pellets", 1.0, &ok);

        ShotTypeBalance(&type);   /* clampa + riporta in banda di potenza: nessun dud, nessun tipo rotto */
        floor->shot = type;
    }

    /* Proxy di leggibilita' (DEC-146, core/shot_type.h): verificato SEMPRE, sia il
       ramo del modello sia quello procedurale sopra. DEC-146 e' esplicito sulla
       conseguenza di uno sforamento: "un contenuto che la supera NON PASSA LA
       VALIDAZIONE e segue la normale catena di fallback" -- non una riparazione
       sul posto. Riparare sul posto (tagliare pellets poi radiusMul) uscirebbe
       dalla banda di potenza gia' garantita da ShotTypeBalance qui sopra SENZA
       rigirare quel bilanciamento, con l'effetto di far divergere il manifest
       (che dichiarerebbe il tipo riparato) da quello che gioco vedrebbe: qui si
       segue invece alla lettera la catena di fallback prevista, come per ogni
       altro campo di questo file -- l'intero tipo di colpo, non solo il campo
       fuori banda, ricade su quello procedurale del piano (fbFloor->shot, gia'
       verificato dentro budget per costruzione, vedi il commento in
       GenFallbackRun). Il ramo procedurale sopra (floor->shot = fbFloor->shot)
       supera sempre questo controllo per lo stesso motivo, quindi il ramo "sia
       il ramo del modello sia quello procedurale" resta vero senza bisogno di un
       caso speciale qui. */
    if (!ShotTypeReadabilityOk(&floor->shot))
    {
        GenLogLine("piano %d: il tipo di colpo generato (\"%s\", %.1f%% di leggibilita' stimata) supera la soglia DEC-146 -> sostituito con quello procedurale",
                   floorNum, floor->shot.name, (double)ShotTypeReadabilityPercent(&floor->shot));
        floor->shot = fbFloor->shot;
    }

    /* Budget del RISULTATO di una sinergia DICHIARATA dal contenuto (DEC-162,
       docs/design/systems/synergies.md#budget-di-potenza-del-risultato). Il
       controllo vero e la sua motivazione stanno in core/shot_type.h (blocco
       SHOT_TYPE_SYNERGY_RESULT_EXTRA_PELLETS); qui c'e' la CONSEGUENZA, che e'
       la stessa del controllo DEC-146 sopra e per lo stesso motivo: si scarta
       l'intero tipo di colpo e si ricade su quello procedurale del piano, mai
       una riparazione sul posto. In due parole: un tipo di colpo che dichiara
       chain/pierce dichiara meta' di una coppia canonica, e cio' che il
       giocatore vedra' a schermo quando la coppia si accende non e' il tipo di
       colpo ma il RISULTATO (piu' pallettoni, sommati a runtime senza alcun
       tetto di leggibilita': combat.c taglia solo a 5). Il risultato sta sotto
       la STESSA soglia del singolo, non una piu' larga: DEC-162 alza il budget
       di POTENZA (garantito a runtime da ScriptItemsClampSynergyResultDelta),
       mai quello di leggibilita'.
       Il ripiego non puo' rimbalzare su se' stesso: i tre tipi procedurali
       (ShotTypeExample) stanno abbondantemente sotto la soglia anche col
       risultato stimato -- verificato da TestSynergyResultReadabilityBudget
       (test AW di src/tests/script_items_tests.c, 'make test-script'), che
       stampa i tre valori e fallisce se un domani un esempio o la formula li
       portasse oltre. */
    if (!ShotTypeSynergyResultReadabilityOk(&floor->shot))
    {
        GenLogLine("piano %d: il tipo di colpo generato (\"%s\") dichiara una sinergia (chain/pierce) il cui RISULTATO stimato e' %.1f%% > soglia DEC-146 -> budget del risultato DEC-162 sforato, sostituito con quello procedurale",
                   floorNum, floor->shot.name, (double)ShotTypeSynergyResultReadabilityPercent(&floor->shot));
        floor->shot = fbFloor->shot;
    }

    /* Quale dei tre oggetti attivi lo conferisce (1..3). Fuori range o assente ->
       il primo: sempre un oggetto vero, mai un indice che non esiste. */
    int ok = 0;
    int idx = (int)JsonNumber(rawFloor, "shotItem", (double)fbFloor->shotItem, &ok);
    if (idx < 1 || idx > GEN_ITEMS) idx = 1;
    /* Task "4 categorie" (bloccante round 0): uno stat-up non porta MAI il tipo
       di colpo del piano (items-pools-and-rarity.md: "senza comportamento
       nuovo"), stessa regola gia' applicata al bossItem (scripts/test-gen.sh).
       'kind' non e' ancora stato normalizzato per gli item di QUESTO piano a
       questo punto della funzione (il ciclo sugli item gira dopo, in
       GenNormalizeRun) ma e' comunque leggibile con certezza da fbFloor: kind
       si prende SEMPRE dal precalcolo di copertura del fallback (mai dal
       JSON), quindi fbFloor->items[i].kind e' gia' il valore finale che
       floor->items[i].kind avra' fra poco. Se la posizione scelta (dal modello
       o dal ripiego) risolve a statup, si ricade sulla prima posizione
       non-statup del piano -- lo stesso identico rimedio di GenFallbackRun
       sopra, mai una posizione inventata. */
    if (strcmp(fbFloor->items[idx - 1].kind, "statup") == 0)
    {
        int fallbackIdx = idx;
        for (int j = 0; j < GEN_ITEMS; j++)
        {
            if (strcmp(fbFloor->items[j].kind, "statup") != 0) { fallbackIdx = j + 1; break; }
        }
        if (fallbackIdx != idx)
        {
            GenLogLine("piano %d: shotItem=%d e' uno stat-up (mai il tipo di colpo) -> spostato su item%d",
                       floorNum, idx, fallbackIdx);
        }
        idx = fallbackIdx;
    }
    floor->shotItem = idx;
}

/* Confronto senza distinzione fra maiuscole e minuscole, scritto a mano invece di
   usare strcasecmp: quest'ultima e' POSIX, e melting-gen compila con -std=c99, dove
   non e' dichiarata senza feature-test macro. Non vale una dipendenza in piu' per
   dieci righe. */
static int SameTextIgnoreCase(const char *a, const char *b)
{
    if (!a || !b) return 0;
    for (; *a && *b; a++, b++)
    {
        char ca = (*a >= 'A' && *a <= 'Z') ? (char)(*a - 'A' + 'a') : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (char)(*b - 'A' + 'a') : *b;
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

/* La rete anti-fotocopia (step C review, bug REALE osservato con una generazione
   vera al seed 20260714: il 7B ha prodotto CINQUE PIANI IDENTICI -- stesso tema,
   stesso boss, stessi oggetti, stesso tipo di colpo).
 *
 * La causa immediata era la finestra della penalita' sulle ripetizioni, piu' corta
 * di un piano di JSON (vedi GEN_PENALTY_LAST_N in melting_gen.h): allargarla ha
 * migliorato molto le cose, ma non le puo' GARANTIRE -- il campionamento resta
 * campionamento, e su cinque piani il quinto puo' sempre ricopiare il primo, che e'
 * ormai fuori da qualunque finestra ragionevole.
 *
 * Quindi si fa qui la stessa cosa che si fa per i tipi di colpo (ShotTypeBalance,
 * core/shot_type.c) e per il "mai un dud" degli oggetti: IL MODELLO E' LIBERO, IL C
 * GARANTISCE IL MINIMO. Un piano che ricopia il tema di un piano precedente non e'
 * contenuto: e' un buco. Lo si sostituisce in blocco col piano PROCEDURALE dello
 * stesso indice (tema, stile, boss, colori, oggetti, tipo di colpo -- tutto
 * coerente fra loro, perche' viene tutto dallo stesso GenFallbackRun), che e'
 * esattamente cio' che il gioco userebbe se il modello non ci fosse affatto. Un
 * piano procedurale e' meno ispirato di uno inventato bene, ma e' infinitamente
 * meglio di una fotocopia: la run resta VARIA, che e' la promessa minima di un
 * generatore di contenuti.
 *
 * Secondo giro, piu' fine: due piani per il resto diversi potrebbero comunque dare
 * lo stesso NOME al proprio tipo di colpo (e' il campo piu' corto, quindi il piu'
 * facile da ripetere). In quel caso non si butta l'intero piano: si sostituisce il
 * solo tipo di colpo con quello procedurale. */
static void DedupeFloors(GenRun *out, const GenRun *fb)
{
    for (int f = 1; f < GEN_FLOORS; f++)
    {
        for (int g = 0; g < f; g++)
        {
            if (!SameTextIgnoreCase(out->floors[f].theme, out->floors[g].theme)) continue;
            GenLogLine("anti-fotocopia: il piano %d ripete il tema del piano %d (\"%s\") -> sostituito col piano procedurale",
                       f + 1, g + 1, out->floors[f].theme);
            out->floors[f] = fb->floors[f];
            break;
        }
    }

    for (int f = 1; f < GEN_FLOORS; f++)
    {
        for (int g = 0; g < f; g++)
        {
            if (!out->floors[f].shot.active || !out->floors[g].shot.active) continue;
            if (!SameTextIgnoreCase(out->floors[f].shot.name, out->floors[g].shot.name)) continue;
            GenLogLine("anti-fotocopia: il tipo di colpo del piano %d ripete quello del piano %d (\"%s\") -> sostituito con quello procedurale",
                       f + 1, g + 1, out->floors[f].shot.name);
            out->floors[f].shot = fb->floors[f].shot;
            break;
        }
    }
}

/* Un tipo di nemico dal JSON (fase 3b). Stessa filosofia del tipo di colpo: il
   modello e' libero sui nomi, sulle forme e sui numeri; il C garantisce solo che il
   risultato sia bilanciato (EnemyTypeBalance) -- e' l'unica ragione per cui si puo'
   lasciare a un 7B l'invenzione dei nemici. Un campo mancante o sbagliato ricade su
   quello del ripiego procedurale, mai su un errore. */
static void NormalizeEnemyType(const cJSON *rawFoe, const EnemyTypeDef *fb, bool isBoss, EnemyTypeDef *out)
{
    if (!cJSON_IsObject((cJSON *)rawFoe))
    {
        *out = *fb;              /* nessun nemico nel JSON: quello procedurale, mai un piano senza */
        out->boss = isBoss;
        EnemyTypeBalance(out);
        return;
    }

    EnemyTypeDef type;
    memset(&type, 0, sizeof(type));
    type.active = true;
    type.boss = isBoss;
    CopyText(type.name, sizeof(type.name), JsonString(rawFoe, "name"), fb->name);

    /* Una forma/movimento/fuoco sconosciuti NON ricadono sul valore 0 (che e' il
       nemico piu' banale: un blob che insegue e non spara -- un "nemico nuovo" cosi'
       non e' un nemico nuovo), ma su quelli del ripiego procedurale. */
    const char *formText = JsonString(rawFoe, "form");
    type.form = formText ? EnemyFormFromText(formText) : fb->form;
    if (formText && type.form == ENEMY_FORM_BLOB && strcmp(formText, "blob") != 0) type.form = fb->form;
    const char *moveText = JsonString(rawFoe, "move");
    type.move = moveText ? EnemyMoveFromText(moveText) : fb->move;
    if (moveText && type.move == ENEMY_MOVE_CHASE && strcmp(moveText, "chase") != 0) type.move = fb->move;
    const char *fireText = JsonString(rawFoe, "fire");
    type.fire = fireText ? EnemyFireFromText(fireText) : fb->fire;
    if (fireText && type.fire == ENEMY_FIRE_NONE && strcmp(fireText, "none") != 0) type.fire = fb->fire;

    int ok = 0;
    type.hpMul    = (float)JsonNumber(rawFoe, "hp",    fb->hpMul,    &ok);
    type.speedMul = (float)JsonNumber(rawFoe, "speed", fb->speedMul, &ok);
    type.sizeMul  = (float)JsonNumber(rawFoe, "size",  fb->sizeMul,  &ok);
    type.fireRate = (float)JsonNumber(rawFoe, "rate",  fb->fireRate, &ok);
    type.pellets  = (int)JsonNumber(rawFoe, "pellets", (double)(fb->pellets > 0 ? fb->pellets : 1), &ok);

    EnemyTypeBalance(&type);
    *out = type;
}

/* Il layout della stanza dal JSON (fase 3c). Stessa filosofia: il modello e' libero
   su nome/forma/densita', il C garantisce solo che siano validi (RoomLayoutClamp);
   la giocabilita' vera la garantisce RoomLayoutBuild lato gioco. */
static void NormalizeRoomLayout(const cJSON *rawRoom, const RoomLayoutDef *fb, RoomLayoutDef *out)
{
    if (!cJSON_IsObject((cJSON *)rawRoom)) { *out = *fb; RoomLayoutClamp(out); return; }

    RoomLayoutDef def;
    memset(&def, 0, sizeof(def));
    def.active = true;
    CopyText(def.name, sizeof(def.name), JsonString(rawRoom, "name"), fb->name);

    const char *formText = JsonString(rawRoom, "form");
    def.form = formText ? RoomFormFromText(formText) : fb->form;
    /* "open" col nome resta un layout attivo a forma OPEN: il writer lo scrivera'
       comunque (il gioco lo trattera' come stanza vuota, vedi ReadRoomLayout).
       Una forma sconosciuta ricade sul ripiego, non su OPEN (una stanza "nuova" che
       si disegna vuota non e' una stanza nuova). */
    if (formText && def.form == ROOM_LAYOUT_OPEN && strcmp(formText, "open") != 0) def.form = fb->form;

    int ok = 0;
    def.density = (float)JsonNumber(rawRoom, "density", fb->density, &ok);

    RoomLayoutClamp(&def);
    *out = def;
}

void GenNormalizeRun(const struct cJSON *rawRoot, unsigned int seed, const GenChosenTheme *chosen, GenRun *out)
{
    GenRun fb;
    GenFallbackRun(&fb, seed, chosen);
    memset(out, 0, sizeof(*out));
    snprintf(out->source, sizeof(out->source), "local");
    out->seed = seed;

    const cJSON *floors = rawRoot ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawRoot, "floors") : NULL;
    for (int f = 0; f < GEN_FLOORS; f++)
    {
        const cJSON *rawFloor = cJSON_IsArray((cJSON *)floors) ? cJSON_GetArrayItem((cJSON *)floors, f) : NULL;
        const GenFloor *fbFloor = &fb.floors[f];
        GenFloor *floor = &out->floors[f];
        CopyText(floor->theme, sizeof(floor->theme), JsonString(rawFloor, "theme"), fbFloor->theme);
        CopyText(floor->style, sizeof(floor->style), JsonString(rawFloor, "style"), fbFloor->style);
        CopyText(floor->boss, sizeof(floor->boss), JsonString(rawFloor, "boss"), fbFloor->boss);
        CopyColor(floor->bg, sizeof(floor->bg), JsonString(rawFloor, "bg"), fbFloor->bg);
        CopyColor(floor->floorColor, sizeof(floor->floorColor), JsonString(rawFloor, "floor"), fbFloor->floorColor);
        CopyColor(floor->wall, sizeof(floor->wall), JsonString(rawFloor, "wall"), fbFloor->wall);
        CopyColor(floor->accent, sizeof(floor->accent), JsonString(rawFloor, "accent"), fbFloor->accent);
        CopyColor(floor->accent2, sizeof(floor->accent2), JsonString(rawFloor, "accent2"), fbFloor->accent2);
        CopyColor(floor->enemy, sizeof(floor->enemy), JsonString(rawFloor, "enemy"), fbFloor->enemy);
        CopyColor(floor->bossColor, sizeof(floor->bossColor), JsonString(rawFloor, "bossColor"), fbFloor->bossColor);
        NormalizeShot(rawFloor, fbFloor, floor, f + 1);

        /* Tipi di nemico del piano (fase 3b): due normali + il boss. */
        const cJSON *rawFoes = rawFloor ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawFloor, "enemies") : NULL;
        for (int i = 0; i < 2; i++)
        {
            const cJSON *rawFoe = cJSON_IsArray((cJSON *)rawFoes) ? cJSON_GetArrayItem((cJSON *)rawFoes, i) : NULL;
            NormalizeEnemyType(rawFoe, &fbFloor->enemies[i], false, &floor->enemies[i]);
        }
        NormalizeEnemyType(rawFloor ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawFloor, "bossType") : NULL,
                            &fbFloor->bossType, true, &floor->bossType);
        NormalizeRoomLayout(rawFloor ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawFloor, "room") : NULL,
                            &fbFloor->roomLayout, &floor->roomLayout);

        const cJSON *rawItems = rawFloor ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawFloor, "items") : NULL;
        for (int i = 0; i < GEN_ITEMS; i++)
        {
            const cJSON *rawItem = cJSON_IsArray((cJSON *)rawItems) ? cJSON_GetArrayItem((cJSON *)rawItems, i) : NULL;
            const GenItem *fbItem = &fbFloor->items[i];
            GenItem *item = &floor->items[i];
            CopyText(item->name, sizeof(item->name), JsonString(rawItem, "name"), fbItem->name);
            const char *slot = JsonString(rawItem, "slot");
            int slotOk = 0;
            for (int s = 0; s < 6; s++)
            {
                if (slot && strcmp(GEN_SLOTS[s], slot) == 0) slotOk = 1;
            }
            snprintf(item->slot, sizeof(item->slot), "%s", slotOk ? slot : fbItem->slot);
            NormalizeTraits(rawItem ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawItem, "traits") : NULL, fbItem, item);
            CopyColor(item->color, sizeof(item->color), JsonString(rawItem, "color"), fbItem->color);

            /* "kind"/"rarity"/cariche-cooldown non fanno parte della
               grammatica JSON (run.gbnf): sono decisioni di
               bilanciamento prese SEMPRE in C, mai dal modello (vedi il
               commento su GenItem.kind in melting_gen.h). Si prendono SEMPRE
               dal fallback procedurale (gia' calcolato dal precalcolo di
               copertura dentro GenFallbackRun, che GenNormalizeRun richiama
               sempre come 'fb' qui sopra), mai dal JSON grezzo -- stessa
               garanzia del mix delle 4 categorie e del floor di rarita'
               (DEC-144) qualunque cosa il modello abbia scritto. */
            snprintf(item->kind, sizeof(item->kind), "%s", fbItem->kind);
            snprintf(item->rarity, sizeof(item->rarity), "%s", fbItem->rarity);
            item->charges = fbItem->charges;
            item->cooldown = fbItem->cooldown;

            /* Normalizzato SEMPRE, ANCHE per kind=="statup": il modello ha
               scritto un "script" per questo slot come per ogni altro (non
               sa che finira' stat-up, kind si decide dopo, ne' fa parte
               della grammatica). Un GenItem con opCount==0 produrrebbe
               "script":[] nel JSON di debug (--emit-llm-json, RunToJson in
               gen_manifest.c), che run.gbnf rifiuta (la regola 'item' vuole
               1-3 op): tenerlo popolato qui evita quel problema senza
               bisogno di casi speciali nel writer di debug. "Uno stat-up non
               ha comportamento mini-VM" resta vero nel MANIFEST vero
               (WriteManifest decide SOLO li' se scrivere la riga
               ".script=", in base al kind, non a opCount) -- stessa
               contenzione del bossItem, che invece non ha proprio uno
               "script" da normalizzare (non fa parte della grammatica per
               quel campo). */
            NormalizeScript(rawItem ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawItem, "script") : NULL, fbItem, item);
            /* Difetto di contenuto (task "4 categorie"): un attivo senza
               cariche ne' cooldown ricade sul cooldown di riserva del
               motore, mai su un oggetto "active" inutilizzabile. */
            GenValidateItemRecharge(item);
        }

        /* Oggetto stat-up del piano (fase 3): non fa parte della grammatica
           JSON che il modello scrive (niente "bossItem" in run.gbnf/
           system.txt), quindi non c'e' nulla da leggere da rawFloor qui.
           Si prende SEMPRE il bossItem procedurale derivato dal seed della
           run (fbFloor, gia' calcolato sopra da GenFallbackRun): stessa
           qualita' di ogni altro contenuto di ripiego, deterministico e mai
           un doppione degli oggetti attivi. Il suo comportamento Lua (se un
           modello e' disponibile) viene scritto A PARTE da
           GenLuaGenerateForRun con un prompt dedicato (vedi gen_lua.c),
           DOPO questa normalizzazione: qui il campo 'lua' resta quindi
           quello del fallback (vuoto, vedi GenFallbackRun/FallbackBossItem). */
        floor->bossItem = fbFloor->bossItem;
    }

    /* Garanzia del motore (stessa filosofia di ShotTypeBalance per i tipi di
       colpo, DEC-052/M3): quando il giocatore ha scelto un tema (--theme-file),
       DEC-005/M5 dice "Floor 1 opens in this exact world" -- alla lettera, non
       "un tema simile". Il modello a volte scrive un nome leggermente diverso
       da quello scelto (osservato: "Glass Cathedral" al posto di "Foundry of
       Glass"), rendendo il check M5 di scripts/test-llm.sh non deterministico.
       Si forza QUI, DOPO che il piano 1 e' stato normalizzato dal JSON grezzo
       (quindi vale per entrambi i chiamanti di GenNormalizeRun: i due tentativi
       LLM di RunJsonAttempts e --from-json/--resume in main.c) e PRIMA della
       rete anti-fotocopia sotto, cosi' un'eventuale collisione col piano 1
       forzato viene gia' presa in carico da DedupeFloors. Il ramo --fallback
       puro (GenFallbackRun chiamato direttamente, mai attraverso questa
       funzione) non e' toccato: scrive gia' chosen->name da solo. Il valore
       scartato del modello non si perde in silenzio: una riga in
       logs/melting-gen.log (GenLogLine, stesso canale di sviluppo usato da
       DedupeFloors qui sotto), mai player-facing. I piani 2-5 restano quelli
       del modello (l'aderenza "stesso mondo che evolve" e' backlog separato,
       non questa garanzia). */
    if (chosen && chosen->name[0])
    {
        if (!SameTextIgnoreCase(out->floors[0].theme, chosen->name))
        {
            GenLogLine("piano 1: il modello ha scritto il tema \"%s\", forzato al tema scelto \"%s\"",
                       out->floors[0].theme, chosen->name);
        }
        snprintf(out->floors[0].theme, sizeof(out->floors[0].theme), "%s", chosen->name);
    }

    /* Ultima cosa, quando tutti i piani sono normalizzati (e il piano 1 e'
       stato eventualmente forzato sopra): la rete anti-fotocopia (vedi
       DedupeFloors sopra). Va per forza QUI e non dentro il ciclo: per sapere
       se un piano ripete un altro bisogna averli tutti. */
    DedupeFloors(out, &fb);
}
