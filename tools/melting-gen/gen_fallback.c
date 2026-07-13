#include "melting_gen.h"

#include <stdio.h>
#include <string.h>

/* Oggetto stat-up del piano (fase 3, ricompensa del boss): stesso stile
   procedurale degli oggetti attivi sopra (nome tema+trait, slot/colore
   dall'rng), ma SENZA alcuna operazione mini-VM (opCount resta 0: un
   oggetto stat-up non ha comportamento, solo statistiche via on_evaluate/il
   ripiego C keyed-off-trait, vedi src/script/script_items.c). Il trait
   scelto qui non pilota nessuna azione di gioco: serve solo come "etichetta"
   per il nome e per il ripiego C se lo script Lua del modello fallisce o non
   c'e' (vedi ScriptItemsApplyStatUpFallback). */
static void FallbackBossItem(unsigned int *rng, GenItem *item, int h, int floorIdx)
{
    static const char *bossNames[] = { "Nucleo", "Reliquia", "Sigillo", "Totem", "Cristallo", "Anima", "Cuore", "Emblema" };
    const char *trait = GEN_TRAITS[GenRngRange(rng, 0, 8)];
    snprintf(item->name, sizeof(item->name), "%s %s", bossNames[GenRngRange(rng, 0, 7)], trait);
    snprintf(item->slot, sizeof(item->slot), "%s", GEN_SLOTS[GenRngRange(rng, 0, 5)]);
    snprintf(item->traits[0], sizeof(item->traits[0]), "%s", trait);
    item->traitCount = 1;
    GenHsvToHex((h + 260 + floorIdx*37)%360, 0.70, 0.95, item->color);
    snprintf(item->kind, sizeof(item->kind), "statup");
    item->opCount = 0;
    item->lua[0] = '\0';
}

static void FallbackScriptForTrait(const char *trait, unsigned int *rng, GenItem *item)
{
    GenScriptOp *op = &item->ops[0];
    item->opCount = 1;
    const GenTraitRule *rule = GenTraitRuleFor(trait);
    if (rule)
    {
        snprintf(op->trigger, sizeof(op->trigger), "%s", rule->trigger);
        snprintf(op->op, sizeof(op->op), "%s", rule->op);
        op->a = rule->a;
        op->b = rule->b;
        snprintf(op->trait, sizeof(op->trait), "%s", rule->trait);
        return;
    }
    snprintf(op->trigger, sizeof(op->trigger), "%s", GenRngRange(rng, 0, 1) ? "on_hit" : "on_fire");
    snprintf(op->op, sizeof(op->op), "projectile");
    op->a = 1;
    op->b = 300;
    snprintf(op->trait, sizeof(op->trait), "none");
}

void GenFallbackRun(GenRun *run, unsigned int seed)
{
    static const char *themeWords[] = { "Cantina", "Biblioteca", "Acquario", "Fucina", "Cattedrale", "Laboratorio", "Teatro" };
    static const char *weirdWords[] = { "Neon", "Muffita", "Lunare", "Radioattiva", "di Zucchero", "Elettrica", "di Carta" };
    static const char *styles[]     = { "pixel semplice", "toon scuro", "arcade secco", "inchiostro piatto", "low-fi fantasy" };
    static const char *itemNames[]  = { "Corona", "Occhiali", "Guanto", "Mantello", "Medaglia", "Cappello", "Aureola", "Spada" };

    memset(run, 0, sizeof(*run));
    unsigned int rng = seed ? seed : 0xA341316Cu;
    snprintf(run->source, sizeof(run->source), "fallback");
    run->seed = seed;

    for (int f = 0; f < GEN_FLOORS; f++)
    {
        GenFloor *floor = &run->floors[f];
        int h = GenRngRange(&rng, 0, 359);

        /* Ordine di estrazione dall'RNG identico a fallbackRun() in run_content.mjs:
           in JS il ciclo sugli item viene eseguito PRIMA di costruire l'oggetto
           floor (theme/style sono valutati solo nel floors.push() successivo),
           quindi qui gli item vanno generati subito dopo `h` e prima di
           theme/style. I colori (bg/floor/wall/accent/.../item.color) non
           consumano RNG: dipendono solo da h e j, quindi la loro posizione nel
           codice non altera lo stream e possono restare dopo per leggibilita'. */
        for (int j = 0; j < GEN_ITEMS; j++)
        {
            GenItem *item = &floor->items[j];
            const char *trait = GEN_TRAITS[GenRngRange(&rng, 0, 8)];
            snprintf(item->name, sizeof(item->name), "%s %s", itemNames[GenRngRange(&rng, 0, 7)], trait);
            snprintf(item->slot, sizeof(item->slot), "%s", GEN_SLOTS[GenRngRange(&rng, 0, 5)]);
            snprintf(item->traits[0], sizeof(item->traits[0]), "%s", trait);
            item->traitCount = 1;
            GenHsvToHex((h + 80 + j*53)%360, 0.75, 0.92, item->color);
            snprintf(item->kind, sizeof(item->kind), "active");
            FallbackScriptForTrait(trait, &rng, item);
        }

        /* Oggetto stat-up del piano (fase 3): subito dopo i 3 attivi, stesso
           motivo di ordine RNG del commento sopra (documentare deliberatamente
           dove consuma lo stream, non lasciarlo implicito). E' un NUOVO punto
           di consumo rispetto a prima di questa fase: il golden file di
           regressione (tests/melting-gen/golden-fallback-seed12345.txt) e'
           stato rigenerato di conseguenza, vedi scripts/test-gen.sh. */
        FallbackBossItem(&rng, &floor->bossItem, h, f);

        snprintf(floor->theme, sizeof(floor->theme), "%s %s",
                 themeWords[GenRngRange(&rng, 0, 6)], weirdWords[GenRngRange(&rng, 0, 6)]);
        snprintf(floor->style, sizeof(floor->style), "%s", styles[GenRngRange(&rng, 0, 4)]);
        if (f == GEN_FLOORS - 1) snprintf(floor->boss, sizeof(floor->boss), "Ultimo Custode");
        else snprintf(floor->boss, sizeof(floor->boss), "Custode %d", f + 1);
        GenHsvToHex(h, 0.32, 0.12, floor->bg);
        GenHsvToHex((h + 20)%360, 0.38, 0.22, floor->floorColor);
        GenHsvToHex((h + 52)%360, 0.55, 0.45, floor->wall);
        GenHsvToHex((h + 100)%360, 0.62, 0.86, floor->accent);
        GenHsvToHex((h + 172)%360, 0.70, 0.94, floor->accent2);
        GenHsvToHex((h + 235)%360, 0.58, 0.82, floor->enemy);
        GenHsvToHex((h + 300)%360, 0.75, 0.88, floor->bossColor);
    }
}
