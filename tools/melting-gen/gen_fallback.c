#include "melting_gen.h"

#include <stdio.h>
#include <string.h>

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

        for (int j = 0; j < GEN_ITEMS; j++)
        {
            GenItem *item = &floor->items[j];
            const char *trait = GEN_TRAITS[GenRngRange(&rng, 0, 8)];
            snprintf(item->name, sizeof(item->name), "%s %s", itemNames[GenRngRange(&rng, 0, 7)], trait);
            snprintf(item->slot, sizeof(item->slot), "%s", GEN_SLOTS[GenRngRange(&rng, 0, 5)]);
            snprintf(item->traits[0], sizeof(item->traits[0]), "%s", trait);
            item->traitCount = 1;
            GenHsvToHex((h + 80 + j*53)%360, 0.75, 0.92, item->color);
            FallbackScriptForTrait(trait, &rng, item);
        }
    }
}
