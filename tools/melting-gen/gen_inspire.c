#include "gen_inspire.h"

#include "melting_gen.h"

#include <stdio.h>
#include <string.h>

/* Le liste sono materia prima, non un catalogo: il prompt dice al modello di
   combinarle e trasformarle, mai di copiarle. Le "qualita'" sono locuzioni
   invariabili (of X, in Y, without Z): si attaccano a qualunque luogo senza
   ambiguita' sintattica ("lighthouse of ash", "library in ruins") -- in
   inglese l'accordo di genere non esiste, ma la forma prepositiva resta piu'
   facile da comporre per un 7B che un aggettivo libero.
   Contenuto in inglese (DEC-052): ASCII puro per costruzione (nessuna parola
   qui usa lettere accentate), coerente con la grammatica namechar di
   run.gbnf. */

static const char *LUOGHI[] = {
    "greenhouse", "foundry", "ossuary", "lighthouse", "library", "aqueduct",
    "mine", "cathedral", "swamp", "glacier", "volcano", "mill",
    "observatory", "theater", "garden", "crypt", "lagoon", "canyon",
    "beehive", "shipwreck", "market", "labyrinth", "mushroom grove", "saltworks",
    "vineyard", "tower", "well", "press house", "arsenal", "planetarium",
    "catacomb", "cave", "orchard", "morgue", "monastery", "aquarium",
    "bazaar", "shipyard", "museum", "menagerie", "printworks", "distillery",
    "bell tower", "tailor shop", "granary", "furnace", "pier", "amphitheater",
};

static const char *QUALITA[] = {
    "of glass", "of ash", "on fire", "adrift", "bottomless",
    "at midnight", "in ruins", "of mirrors", "under salt", "in quarantine",
    "out of orbit", "of velvet", "upside down", "of sand",
    "among the clouds", "of obsidian", "in eternal autumn", "of neon",
    "wind-up", "of glowing mold", "without gravity", "of paper",
    "in miniature", "of coral", "in reverse", "of amber", "in hibernation",
    "of rust", "of tides", "of porcelain", "in festival", "of bone",
    "vacuum-sealed", "of lanterns", "in eclipse", "of cobwebs",
    "clockwork", "of honey", "in flood", "of tar",
};

static const char *MATERIE[] = {
    "glass", "spores", "lightning", "sand", "ink", "bones", "petals",
    "coins", "ice", "lava", "steam", "nails", "stars", "roots",
    "shards", "bubbles", "dice", "needles", "feathers", "chains", "sparks",
    "jellyfish", "crystals", "seeds", "spinning tops", "blades", "droplets", "prisms",
    "comets", "thorns", "gears", "ashes", "pearls", "bolts",
    "vials", "mirror shards",
};

static const char *BESTIARIO[] = {
    "moths", "crabs", "puppets", "gargoyles", "snails", "wasps",
    "hollow armor", "statues", "mushrooms", "serpents", "ravens", "spiders",
    "golems", "specters", "fish", "hedgehogs", "moles", "beetles",
    "living lanterns", "books", "roots", "living crystals", "swarms",
    "jellyfish", "salamanders", "mimics", "bells", "dolls", "turtles",
    "smoke sentinels",
};

static const char *VINCOLI[] = {
    "at least one floor is a place of water",
    "a boss is tiny but lethal",
    "a shot type punishes standing still",
    "two floors are the same place before and after a disaster",
    "a floor is absurdly cheerful, and unsettling because of it",
    "a theme grows out of an ancient trade",
    "a floor lives in perpetual night",
    "a shot type is made of something edible",
    "an enemy is an animated household object",
    "a floor sits inside something living and enormous",
    "a boss has a gentle name and terrible manners",
    "a floor is vertical: everything goes down or everything goes up",
    "a shot type comes back somehow",
    "a floor's enemies visibly work together",
    "a floor is the exact reverse of another",
    "a theme is a commonplace turned inside out",
};

/* Esempi rotanti per il SYSTEM prompt (vedi gen_inspire.h). A differenza
   delle liste sopra qui il contenuto deve restare JSON valido dentro le
   bande numeriche del prompt: non e' materia prima da combinare, e' un
   pezzo intero gia' pronto, quindi il pool e' scritto a mano invece che
   assemblato da liste di parole. Include gli esempi storici tradotti in
   inglese (Sacred Colonnade, Runaway Slag, Armored Sentinel, Rusty Nails,
   Lunar Ray, Salt Surge): restano UNA possibilita' fra tante, non piu'
   l'unica. Nomi in inglese (DEC-052), ASCII puro per costruzione, e nessuno
   ripreso dalle liste d'ispirazione. */

typedef struct {
    const char *name;
    const char *form;      /* open|pillars|corridor|arena|scatter */
    double density;        /* 0.2-1 */
} RoomExample;

/* form: blob|spiky|armored|floater -- move: chase|kite|orbit|zigzag|charge
   -- fire: none|single|spread|ring -- hp/speed/size: 0.5-3 -- rate: 0-2.5
   -- pellets: 1-8 (solo spread/ring, altrimenti 1 come da regola). */
typedef struct {
    const char *name;
    const char *form;
    const char *move;
    const char *fire;
    double hp, speed, size, rate;
    int pellets;
} EnemyExample;

/* form: orb|spike|beam|arc|blade -- speed/damage/size/life: 0.5-2 --
   pierce/chain: 0-3 -- pellets: 1-3. Ogni voce e' un COMPROMESSO (mai tutto
   alto), come richiesto dal prompt. */
typedef struct {
    const char *name;
    const char *form;
    double speed, damage, size, life;
    int pierce, chain, pellets;
} ShotExample;

static const RoomExample ROOM_EXAMPLES[] = {
    { "Sacred Colonnade",    "pillars",  0.6 },
    { "Hall of Mirrors",     "scatter",  0.5 },
    { "Bridge of Sighs",     "corridor", 0.3 },
    { "Lions' Den",          "arena",    0.7 },
    { "Ceremonial Void",     "open",     0.2 },
    { "Grove of Columns",    "pillars",  0.8 },
    { "Flooded Tunnel",      "corridor", 0.5 },
    { "Wrecked Depot",       "scatter",  0.9 },
    { "Rotunda of Judgment", "arena",    0.4 },
    { "Deserted Plaza",      "open",     0.3 },
};

/* Nemici "da mischia": move chase/charge/zigzag, fire none/single. */
static const EnemyExample ENEMY_EXAMPLES_MELEE[] = {
    { "Runaway Slag",       "blob",    "zigzag", "none",   0.8, 1.4, 0.9, 0,   1 },
    { "Brass Grunt",        "armored", "charge", "none",   2.2, 0.5, 1.6, 0,   1 },
    { "Wasp Swarm",         "spiky",   "zigzag", "single", 0.6, 1.8, 0.6, 1.2, 1 },
    { "Armored Mole",       "armored", "chase",  "none",   2.5, 0.7, 1.4, 0,   1 },
    { "Scorching Shade",    "blob",    "chase",  "none",   1.0, 1.6, 1.0, 0,   1 },
    { "Barbed Coil",        "spiky",   "charge", "single", 1.2, 1.0, 0.8, 0.6, 1 },
    { "Raging Shell",       "armored", "zigzag", "none",   2.0, 0.8, 1.5, 0,   1 },
    { "Ash Moth",           "floater", "chase",  "none",   0.7, 1.3, 0.7, 0,   1 },
};

/* Nemici "a distanza": move kite/orbit, fire spread/ring/single. */
static const EnemyExample ENEMY_EXAMPLES_RANGED[] = {
    { "Armored Sentinel",   "armored", "kite",  "spread", 1.8, 0.6, 1.3, 0.8, 3 },
    { "Drifting Jellyfish", "floater", "orbit", "ring",   0.9, 0.9, 1.1, 0.5, 6 },
    { "Glass Archer",       "spiky",   "kite",  "single", 0.7, 1.1, 0.7, 1.5, 1 },
    { "Wandering Crown",    "floater", "orbit", "spread", 1.1, 0.8, 0.9, 0.9, 4 },
    { "Distant Guardian",   "armored", "kite",  "single", 2.0, 0.5, 1.5, 0.7, 1 },
    { "Stinging Swarm",     "blob",    "orbit", "ring",   0.8, 1.0, 0.8, 1.0, 5 },
    { "Salt Sniper",        "spiky",   "kite",  "spread", 0.9, 0.9, 0.9, 1.1, 3 },
    { "Feather Vortex",     "floater", "orbit", "ring",   1.0, 1.2, 1.0, 0.6, 8 },
};

static const ShotExample SHOT_EXAMPLES[] = {
    { "Rusty Nails",     "spike", 1.4, 0.7, 0.6, 1.0, 1, 0, 1 },
    { "Lunar Ray",       "beam",  1.9, 0.6, 0.5, 1.6, 2, 0, 1 },
    { "Salt Surge",      "arc",   0.8, 0.9, 1.2, 0.9, 0, 2, 1 },
    { "Honey Sphere",    "orb",   0.7, 1.3, 1.4, 1.1, 0, 0, 1 },
    { "Wandering Blade", "blade", 1.0, 1.1, 0.9, 1.3, 1, 0, 1 },
    { "Hail Dart",       "spike", 1.6, 0.6, 0.5, 0.8, 0, 0, 3 },
    { "Ember Whip",      "arc",   1.1, 0.8, 0.8, 1.0, 0, 3, 1 },
    { "Pearl Swarm",     "orb",   0.9, 0.6, 0.7, 1.0, 0, 0, 3 },
    { "Glass Drill",     "spike", 0.6, 1.8, 0.6, 0.7, 2, 0, 1 },
    { "Mist Scythe",     "blade", 1.3, 0.7, 1.1, 0.9, 1, 0, 1 },
    { "Steam Column",    "beam",  0.5, 1.2, 1.5, 1.4, 1, 0, 1 },
};

#define COUNT(a) ((int)(sizeof(a)/sizeof((a)[0])))

/* Estrae 'take' voci DISTINTE da list e le concatena separate da virgole.
   Il costo del rifiuto dei duplicati e' irrisorio (take << n). */
static void PickInto(unsigned int *rng, const char **list, int n, int take,
                      char *out, size_t size)
{
    int picked[8] = { 0 };
    size_t used = 0;
    out[0] = '\0';
    if (take > 8) take = 8;
    for (int i = 0; i < take; i++)
    {
        int idx, clash;
        do
        {
            idx = GenRngRange(rng, 0, n - 1);
            clash = 0;
            for (int j = 0; j < i; j++) if (picked[j] == idx) clash = 1;
        } while (clash);
        picked[i] = idx;
        used += (size_t)snprintf(out + used, size > used ? size - used : 0,
                                 "%s%s", i ? ", " : "", list[idx]);
        if (used >= size) break;
    }
}

char *GenInspireBuild(unsigned int seed, char *buf, size_t size)
{
    /* Stato RNG dedicato e separato da quello del fallback: le ispirazioni
       non devono cambiare cio' che il procedurale produrrebbe a parita' di
       seed (il golden test del determinismo resta valido). */
    unsigned int rng = seed*2654435761u + 97u;
    if (rng == 0) rng = 1;

    char luoghi[256], qualita[256], materie[256], bestie[256];
    PickInto(&rng, LUOGHI, COUNT(LUOGHI), 5, luoghi, sizeof(luoghi));
    PickInto(&rng, QUALITA, COUNT(QUALITA), 5, qualita, sizeof(qualita));
    PickInto(&rng, MATERIE, COUNT(MATERIE), 5, materie, sizeof(materie));
    PickInto(&rng, BESTIARIO, COUNT(BESTIARIO), 4, bestie, sizeof(bestie));
    const char *vincolo = VINCOLI[GenRngRange(&rng, 0, COUNT(VINCOLI) - 1)];

    snprintf(buf, size,
             "Inspirations for THIS run: raw material, not obligations. Combine\n"
             "them, transform them, fix the grammar; do not copy them verbatim and\n"
             "don't feel obliged to use them all. Invent everything else from scratch.\n"
             "- places: %s\n"
             "- qualities of the places: %s\n"
             "- shot materials: %s\n"
             "- bestiary sparks: %s\n"
             "- creative constraint for this run: %s.",
             luoghi, qualita, materie, bestie, vincolo);
    return buf;
}

/* Estrae 'take' indici DISTINTI da [0, n). Come PickInto ma restituisce gli
   indici invece di concatenare stringhe: qui ogni voce e' una struct
   intera, non una parola sola. */
static void PickIndices(unsigned int *rng, int n, int take, int *outIdx)
{
    if (take > 8) take = 8;
    for (int i = 0; i < take; i++)
    {
        int idx, clash;
        do
        {
            idx = GenRngRange(rng, 0, n - 1);
            clash = 0;
            for (int j = 0; j < i; j++) if (outIdx[j] == idx) clash = 1;
        } while (clash);
        outIdx[i] = idx;
    }
}

/* Hash minimale (variante FNV-1a) sulla stringa 'kind', solo per decorrelare
   lo stream RNG delle tre chiamate (room/enemies/shots) fatte con lo STESSO
   seed da GenLlmBuildJsonPrompt: senza questo le tre estrazioni partirebbero
   dallo stesso punto dello stream su pool diversi, il che e' innocuo per il
   contenuto ma renderebbe le tre scelte correlate fra loro senza motivo. */
static unsigned int KindHash(const char *kind)
{
    unsigned int h = 2166136261u;
    for (const unsigned char *p = (const unsigned char *)kind; p && *p; p++)
    {
        h ^= *p;
        h *= 16777619u;
    }
    return h;
}

char *GenInspireExamples(unsigned int seed, const char *kind, char *buf, size_t size)
{
    /* Costante MOLTIPLICATIVA diversa da quella di GenInspireBuild (2654435761
       contro 2246822519): due stream RNG separati sullo stesso seed, cosi' il
       campionamento degli esempi non si correla con quello delle ispirazioni
       (stesso motivo del commento sopra: nessun legame voluto fra le due
       estrazioni, solo lo stesso seed di run come sorgente comune). */
    unsigned int rng = seed*2246822519u + 3266489917u + KindHash(kind);
    if (rng == 0) rng = 1;

    /* Un kind sconosciuto e' un refuso di un call-site futuro: meglio un
       buffer vuoto (il placeholder sparisce e il conteggio 1+2+3 dei test
       lo denuncia subito) che ricadere in silenzio su un pool sbagliato. */
    if (!kind || (strcmp(kind, "room") != 0 && strcmp(kind, "enemies") != 0 && strcmp(kind, "shots") != 0))
    {
        if (size) buf[0] = '\0';
        GenLogLine("inspire: kind sconosciuto '%s'", kind ? kind : "(null)");
        return buf;
    }

    if (strcmp(kind, "room") == 0)
    {
        const RoomExample *r = &ROOM_EXAMPLES[GenRngRange(&rng, 0, COUNT(ROOM_EXAMPLES) - 1)];
        snprintf(buf, size, "{\"name\":\"%s\",\"form\":\"%s\",\"density\":%g}",
                 r->name, r->form, r->density);
    }
    else if (strcmp(kind, "enemies") == 0)
    {
        /* Un esempio da ciascun pool (mischia + distanza): il contrasto di
           strategia e' garantito dalla costruzione dei due pool, non
           dall'estrazione -- vedi il commento sopra ENEMY_EXAMPLES_MELEE. */
        const EnemyExample *a =
            &ENEMY_EXAMPLES_MELEE[GenRngRange(&rng, 0, COUNT(ENEMY_EXAMPLES_MELEE) - 1)];
        const EnemyExample *b =
            &ENEMY_EXAMPLES_RANGED[GenRngRange(&rng, 0, COUNT(ENEMY_EXAMPLES_RANGED) - 1)];
        snprintf(buf, size,
                 "  {\"name\":\"%s\",\"form\":\"%s\",\"move\":\"%s\",\"fire\":\"%s\","
                 "\"hp\":%g,\"speed\":%g,\"size\":%g,\"rate\":%g,\"pellets\":%d}\n"
                 "  {\"name\":\"%s\",\"form\":\"%s\",\"move\":\"%s\",\"fire\":\"%s\","
                 "\"hp\":%g,\"speed\":%g,\"size\":%g,\"rate\":%g,\"pellets\":%d}",
                 a->name, a->form, a->move, a->fire, a->hp, a->speed, a->size, a->rate, a->pellets,
                 b->name, b->form, b->move, b->fire, b->hp, b->speed, b->size, b->rate, b->pellets);
    }
    else   /* "shots", garantito dal filtro in testa */
    {
        int idx[3];
        PickIndices(&rng, COUNT(SHOT_EXAMPLES), 3, idx);
        const ShotExample *s0 = &SHOT_EXAMPLES[idx[0]];
        const ShotExample *s1 = &SHOT_EXAMPLES[idx[1]];
        const ShotExample *s2 = &SHOT_EXAMPLES[idx[2]];
        snprintf(buf, size,
                 "  {\"name\":\"%s\",\"form\":\"%s\",\"speed\":%g,\"damage\":%g,"
                 "\"size\":%g,\"life\":%g,\"pierce\":%d,\"chain\":%d,\"pellets\":%d}\n"
                 "  {\"name\":\"%s\",\"form\":\"%s\",\"speed\":%g,\"damage\":%g,"
                 "\"size\":%g,\"life\":%g,\"pierce\":%d,\"chain\":%d,\"pellets\":%d}\n"
                 "  {\"name\":\"%s\",\"form\":\"%s\",\"speed\":%g,\"damage\":%g,"
                 "\"size\":%g,\"life\":%g,\"pierce\":%d,\"chain\":%d,\"pellets\":%d}",
                 s0->name, s0->form, s0->speed, s0->damage, s0->size, s0->life,
                 s0->pierce, s0->chain, s0->pellets,
                 s1->name, s1->form, s1->speed, s1->damage, s1->size, s1->life,
                 s1->pierce, s1->chain, s1->pellets,
                 s2->name, s2->form, s2->speed, s2->damage, s2->size, s2->life,
                 s2->pierce, s2->chain, s2->pellets);
    }
    return buf;
}
