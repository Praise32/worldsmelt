#include "melting_gen.h"

#include <stdio.h>
#include <string.h>

/* M5 (DEC-005): spostati a scope file (erano locali a GenFallbackRun) per
 * essere condivisi con GenFallbackThemeProposals sotto -- un solo pool di
 * nomi procedurali, mai due liste da tenere allineate a mano. */
static const char *themeWords[] = { "Cellar", "Library", "Aquarium", "Forge", "Cathedral", "Laboratory", "Theater" };
static const char *weirdWords[] = { "of Neon", "of Mold", "of the Moon", "of Radiation", "of Sugar", "of Electricity", "of Paper" };

/* M5, requisito 5: suffissi di stadio per i piani 2-5 quando GenFallbackRun
 * riceve un tema scelto (34 voci, content designer, logs/m5-content-notes.md
 * (c)-1). Pensati per essere concatenati con una virgola dopo il nome del
 * mondo ("%s, %s"), mai con uno spazio nudo -- il nome e' gia' "place + of
 * quality", un secondo "of X" incollato senza virgola sarebbe agrammaticale.
 * Deliberatamente ortogonali alla lista QUALITA di gen_inspire.c (nessuna
 * voce identica): non impoveriscono quel pool. Tutte sotto i 34 caratteri. */
static const char *stageSuffixes[] = {
    "cracking at every seam",
    "half swallowed by its own weather",
    "boiling from within",
    "crawling with rust",
    "unraveling at the edges",
    "gone feral overnight",
    "buckling under its own weight",
    "seized by a slow frost",
    "choking on its own smoke",
    "flooding without end",
    "past its last warning",
    "eaten hollow from the inside",
    "screaming in its sleep",
    "gnawed down to the frame",
    "spilling its own foundations",
    "twisted a quarter turn from normal",
    "infested and restless",
    "burning at the edges now",
    "sinking one room at a time",
    "wrapped in a cold that will not lift",
    "shedding its own walls",
    "curdled and watchful",
    "starting to rot from the rafters down",
    "waking up wrong",
    "overrun and unbothered by it",
    "splitting wide open",
    "drowning in its own tide",
    "gone quiet in a bad way",
    "consumed a little more each hour",
    "buried under its own ruin",
    "grown teeth it did not have before",
    "turning on everyone still inside it",
    "coming apart at its own hinges",
    "past the point of fixing",
};

/* M5, requisito 2: i 32 blurb curati per GenFallbackThemeProposals sotto
 * (content designer, logs/m5-content-notes.md (c)-2) -- generici/atmosferici
 * apposta per accompagnare QUALSIASI combinazione themeWords x weirdWords,
 * mai legati a un sostantivo di luogo specifico. */
static const char *fallbackThemeBlurbs[] = {
    "It looks abandoned, until something in the dark decides to notice you.",
    "Every hallway loops back to somewhere it should not.",
    "Something here remembers you, and it is not friendly.",
    "The air hums like it is counting down to something.",
    "Nothing moves until you stop looking directly at it.",
    "It was built for a purpose nobody living remembers anymore.",
    "The deeper you go, the less it agrees to make sense.",
    "Quiet in a way that feels like it is holding its breath.",
    "Every surface is warm, as if something underneath is still awake.",
    "It welcomes you in, then quietly locks the way back.",
    "The light here plays tricks that outlast the blinking.",
    "Something enormous used to live here, and left in a hurry.",
    "The walls keep a rhythm that is almost, but not quite, a heartbeat.",
    "It feels recently abandoned, though the dust says otherwise.",
    "The further in you walk, the older the air gets.",
    "Every echo answers a question you never asked out loud.",
    "It is beautiful in the specific way ruins are beautiful.",
    "Something here is still working, long after it should have stopped.",
    "The place keeps rearranging itself when you are not watching.",
    "It smells like a celebration that ended very badly.",
    "The shadows are a beat behind everything that casts them.",
    "It has the hush of a room right before something happens.",
    "Every door was left open on purpose, which is the unsettling part.",
    "Something patient has been waiting here for a very long time.",
    "The temperature drops exactly where the map says nothing is wrong.",
    "It looks safe from a distance, and less safe with every step closer.",
    "The floor remembers footsteps that are not yours.",
    "It hums with a machinery nobody switched on.",
    "Everything here is arranged like a trap that has not sprung yet.",
    "The silence has a texture, and it is getting thicker.",
    "It feels like walking into the middle of somebody else's story.",
    "Something keeps almost catching up with you, and never quite does.",
};

/* M5, requisito 5: 4 indici DISTINTI in stageSuffixes per i piani 2-5, un
 * RNG DEDICATO (mai 'rng' del chiamante, vedi il commento su GenFallbackRun)
 * cosi' che il percorso 'chosen == NULL' resti byte-per-byte identico a
 * prima -- il golden file di regressione non deve cambiare. Rejection
 * sampling: 34 voci >> 4 servite, termina in poche iterazioni per costruzione. */
static void PickDistinctStageSuffixes(unsigned int seed, int outIdx[4])
{
    unsigned int stageRng = seed ^ 0x57A6E5EEu;
    int total = (int)(sizeof(stageSuffixes)/sizeof(stageSuffixes[0]));
    int picked = 0;
    while (picked < 4)
    {
        int candidate = GenRngRange(&stageRng, 0, total - 1);
        int dup = 0;
        for (int i = 0; i < picked; i++) if (outIdx[i] == candidate) { dup = 1; break; }
        if (!dup) outIdx[picked++] = candidate;
    }
}

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

void GenFallbackRun(GenRun *run, unsigned int seed, const GenChosenTheme *chosen)
{
    static const char *styles[]     = { "simple pixel", "dark toon", "stark arcade", "flat ink", "low-fi fantasy" };
    static const char *itemNames[]  = { "Crown", "Goggles", "Glove", "Cloak", "Medal", "Hat", "Halo", "Sword" };

    memset(run, 0, sizeof(*run));
    unsigned int rng = seed ? seed : 0xA341316Cu;
    snprintf(run->source, sizeof(run->source), "fallback");
    run->seed = seed;

    /* M5, requisito 5/7: gli indici di stadio si tirano UNA VOLTA, PRIMA del
       ciclo, da un RNG dedicato che non condivide stato con 'rng' -- cosi'
       ogni estrazione dentro il ciclo sotto (item/nemici/colpo/stanza) resta
       ESATTAMENTE la stessa sequenza, chosen NULL o no: il golden file di
       regressione non cambia mai per questo ramo. */
    int chosenActive = (chosen != NULL && chosen->name[0] != '\0');
    int stageIdx[4] = { 0, 0, 0, 0 };
    if (chosenActive) PickDistinctStageSuffixes(seed, stageIdx);

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

        /* M5, requisito 5: col tema scelto, il piano 1 e' il tema alla
           lettera; i piani 2-5 sono lo STESSO nome ("place + quality", in
           qualunque forma l'abbia scritta il modello/il giocatore) piu' una
           virgola e uno stageSuffix DISTINTO -- mai un secondo " of X"
           incollato senza separatore (agrammaticale su un nome gia' "of X",
           vedi il commento su stageSuffixes). snprintf tronca in sicurezza
           se la somma sfora i 64 byte del campo: nessun overflow, solo un
           suffisso eventualmente accorciato. */
        if (chosenActive)
        {
            if (f == 0) snprintf(floor->theme, sizeof(floor->theme), "%s", chosen->name);
            else snprintf(floor->theme, sizeof(floor->theme), "%s, %s", chosen->name, stageSuffixes[stageIdx[f - 1]]);
        }
        else
        {
            snprintf(floor->theme, sizeof(floor->theme), "%s %s",
                     themeWords[GenRngRange(&rng, 0, 6)], weirdWords[GenRngRange(&rng, 0, 6)]);
        }
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

void GenFallbackThemeProposals(unsigned int seed, int count, GenThemeProposal out[GEN_THEME_PROPOSALS])
{
    if (count < 1) count = 1;
    if (count > GEN_THEME_PROPOSALS) count = GEN_THEME_PROPOSALS;

    /* RNG dedicato (salt diverso da PickDistinctStageSuffixes sopra: le due
       funzioni non devono MAI condividere stream, girano in processi/momenti
       diversi -- questa da --propose-themes, quella dentro una run). */
    unsigned int rng = (seed ? seed : 0xA341316Cu) ^ 0x7080E5A1u;
    int nameTotal = (int)(sizeof(themeWords)/sizeof(themeWords[0]))
                   * (int)(sizeof(weirdWords)/sizeof(weirdWords[0]));
    int blurbTotal = (int)(sizeof(fallbackThemeBlurbs)/sizeof(fallbackThemeBlurbs[0]));

    int nameIdx[GEN_THEME_PROPOSALS];
    int blurbIdx[GEN_THEME_PROPOSALS];
    for (int i = 0; i < count; i++)
    {
        int candidate, dup;
        do
        {
            candidate = GenRngRange(&rng, 0, nameTotal - 1);
            dup = 0;
            for (int j = 0; j < i; j++) if (nameIdx[j] == candidate) { dup = 1; break; }
        } while (dup);
        nameIdx[i] = candidate;

        do
        {
            candidate = GenRngRange(&rng, 0, blurbTotal - 1);
            dup = 0;
            for (int j = 0; j < i; j++) if (blurbIdx[j] == candidate) { dup = 1; break; }
        } while (dup);
        blurbIdx[i] = candidate;
    }

    int weirdCount = (int)(sizeof(weirdWords)/sizeof(weirdWords[0]));
    for (int i = 0; i < count; i++)
    {
        int w1 = nameIdx[i] / weirdCount;
        int w2 = nameIdx[i] % weirdCount;
        snprintf(out[i].name, sizeof(out[i].name), "%s %s", themeWords[w1], weirdWords[w2]);
        snprintf(out[i].blurb, sizeof(out[i].blurb), "%s", fallbackThemeBlurbs[blurbIdx[i]]);
    }
}
