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
    static const char *bossNames[] = { "Core", "Relic", "Seal", "Totem", "Crystal", "Soul", "Heart", "Emblem" };
    const char *trait = GEN_TRAITS[GenRngRange(rng, 0, 8)];
    snprintf(item->name, sizeof(item->name), "%s %s", bossNames[GenRngRange(rng, 0, 7)], trait);
    snprintf(item->slot, sizeof(item->slot), "%s", GEN_SLOTS[GenRngRange(rng, 0, 5)]);
    snprintf(item->traits[0], sizeof(item->traits[0]), "%s", trait);
    item->traitCount = 1;
    /* Rarita' (fase 3b, design doc sezione 3): pool BOSS, isBoss=1 ->
       GEN_RARITY_WEIGHTS_BOSS (0/0/70/30 in gen_util.c) -- sempre raro o
       leggendario, mai comune/non-comune: "il boss da' sempre roba buona".
       NUOVO punto di consumo RNG rispetto alle fasi precedenti (motivo per
       cui il golden file di regressione e' stato rigenerato, vedi
       scripts/test-gen.sh). */
    snprintf(item->rarity, sizeof(item->rarity), "%s", GEN_RARITIES[GenRollRarity(rng, 1)]);
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
    static const char *themeWords[] = { "Cellar", "Library", "Aquarium", "Forge", "Cathedral", "Laboratory", "Theater" };
    static const char *weirdWords[] = { "of Neon", "of Mold", "of the Moon", "of Radiation", "of Sugar", "of Electricity", "of Paper" };
    static const char *styles[]     = { "simple pixel", "dark toon", "stark arcade", "flat ink", "low-fi fantasy" };
    static const char *itemNames[]  = { "Crown", "Goggles", "Glove", "Cloak", "Medal", "Hat", "Halo", "Sword" };

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
            /* Rarita' (fase 3b, design doc sezione 3): pool tesoro/negozio,
               isBoss=0 -> GEN_RARITY_WEIGHTS_TREASURE_SHOP (55/30/12/3 in
               gen_util.c), la mista. NUOVO punto di consumo RNG rispetto
               alle fasi precedenti (golden file rigenerato di conseguenza). */
            snprintf(item->rarity, sizeof(item->rarity), "%s", GEN_RARITIES[GenRollRarity(&rng, 0)]);
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

        /* Tipo di colpo del piano (step C): il ripiego procedurale per quando il
           modello non c'e' (--fallback, o una generazione fallita). Gli esempi
           veri vivono in src/core/shot_type.c (ShotTypeExample), condivisi con il
           ripiego del gioco (src/content/run_content.c): un solo elenco, mai due
           copie da tenere allineate. NON sono "i tipi di colpo del gioco" (il
           motore non ne ha: li inventa il modello, vedi il commento in cima a
           shot_type.h) -- sono contenuto di riserva, come i temi e gli oggetti
           procedurali qui sopra.
           ALTRO NUOVO PUNTO DI CONSUMO RNG rispetto alla fase precedente (due
           tiri: quale esempio, e quale dei tre oggetti lo porta): il golden file
           di regressione (tests/melting-gen/golden-fallback-seed12345.txt) e'
           stato rigenerato di conseguenza, vedi scripts/test-gen.sh. */
        ShotTypeExample(&floor->shot, GenRngRange(&rng, 0, SHOT_TYPE_EXAMPLE_COUNT - 1));
        floor->shotItem = GenRngRange(&rng, 1, GEN_ITEMS);

        /* Tipi di nemico del piano (fase 3b): stesso ruolo del ripiego dei tipi di
           colpo -- i nemici veri li inventa il modello, questi sono i tre storici
           (core/enemy_type.c, EnemyTypeExample), usati quando il modello non c'e'.
           NUOVI punti di consumo RNG: golden file rigenerato di conseguenza. */
        for (int i = 0; i < 2; i++)
        {
            EnemyTypeExample(&floor->enemies[i], GenRngRange(&rng, 0, ENEMY_TYPE_EXAMPLE_COUNT - 1));
        }
        EnemyTypeExampleBoss(&floor->bossType);

        /* Layout delle stanze del piano (fase 3c): ripiego procedurale (i layout
           veri li inventa il modello). NUOVO punto di consumo RNG: golden
           rigenerato. */
        RoomLayoutExample(&floor->roomLayout, GenRngRange(&rng, 0, ROOM_LAYOUT_EXAMPLE_COUNT - 1));

        snprintf(floor->theme, sizeof(floor->theme), "%s %s",
                 themeWords[GenRngRange(&rng, 0, 6)], weirdWords[GenRngRange(&rng, 0, 6)]);
        snprintf(floor->style, sizeof(floor->style), "%s", styles[GenRngRange(&rng, 0, 4)]);
        if (f == GEN_FLOORS - 1) snprintf(floor->boss, sizeof(floor->boss), "Final Guardian");
        else snprintf(floor->boss, sizeof(floor->boss), "Guardian %d", f + 1);
        GenHsvToHex(h, 0.32, 0.12, floor->bg);
        GenHsvToHex((h + 20)%360, 0.38, 0.22, floor->floorColor);
        GenHsvToHex((h + 52)%360, 0.55, 0.45, floor->wall);
        GenHsvToHex((h + 100)%360, 0.62, 0.86, floor->accent);
        GenHsvToHex((h + 172)%360, 0.70, 0.94, floor->accent2);
        GenHsvToHex((h + 235)%360, 0.58, 0.82, floor->enemy);
        GenHsvToHex((h + 300)%360, 0.75, 0.88, floor->bossColor);
    }
}
