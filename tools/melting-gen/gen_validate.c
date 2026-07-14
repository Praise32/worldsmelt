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
static void NormalizeShot(const cJSON *rawFloor, const GenFloor *fbFloor, GenFloor *floor)
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

    /* Quale dei tre oggetti attivi lo conferisce (1..3). Fuori range o assente ->
       il primo: sempre un oggetto vero, mai un indice che non esiste. */
    int ok = 0;
    int idx = (int)JsonNumber(rawFloor, "shotItem", (double)fbFloor->shotItem, &ok);
    if (idx < 1 || idx > GEN_ITEMS) idx = 1;
    floor->shotItem = idx;
}

void GenNormalizeRun(const struct cJSON *rawRoot, unsigned int seed, GenRun *out)
{
    GenRun fb;
    GenFallbackRun(&fb, seed);
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
        NormalizeShot(rawFloor, fbFloor, floor);

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
            NormalizeScript(rawItem ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawItem, "script") : NULL, fbItem, item);
            /* "kind" non fa parte della grammatica JSON (run.gbnf): i tre
               oggetti di items[] sono SEMPRE attivi, deciso qui in C, mai dal
               modello (vedi il commento su GenItem.kind in melting_gen.h). */
            snprintf(item->kind, sizeof(item->kind), "active");
            /* Rarita' (fase 3b): stesso trattamento di "kind" sopra, e per lo
               stesso motivo -- il pool (quindi la rarita') e' una decisione
               di design/bilanciamento, non creativita' del modello: non fa
               parte della grammatica JSON, si prende SEMPRE dal fallback
               procedurale (gia' tirato dalla tabella di pesi tesoro/negozio
               in GenFallbackRun, che GenNormalizeRun richiama sempre come
               'fb' qui sopra), mai dal JSON grezzo. */
            snprintf(item->rarity, sizeof(item->rarity), "%s", fbItem->rarity);
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
}
