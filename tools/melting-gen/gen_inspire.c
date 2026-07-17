#include "gen_inspire.h"

#include "melting_gen.h"

#include <stdio.h>
#include <string.h>

/* Le liste sono materia prima, non un catalogo: il prompt dice al modello di
   combinarle e trasformarle, mai di copiarle. Le "qualita'" sono locuzioni
   invariabili (di X, in Y, senza Z): si attaccano a qualunque luogo senza
   problemi di accordo di genere ("faro di cenere", "biblioteca in rovina"),
   che con aggettivi semplici il 7B sbaglierebbe una volta su tre.
   Convenzione dei prompt esistenti: niente lettere accentate (system.txt
   chiede nomi "senza accenti"), quindi apostrofo al posto dell'accento. */

static const char *LUOGHI[] = {
    "serra", "fonderia", "ossario", "faro", "biblioteca", "acquedotto",
    "miniera", "cattedrale", "palude", "ghiacciaio", "vulcano", "mulino",
    "osservatorio", "teatro", "giardino", "cripta", "laguna", "canyon",
    "alveare", "relitto", "mercato", "labirinto", "fungaia", "salina",
    "vigneto", "torre", "pozzo", "frantoio", "arsenale", "planetario",
    "catacomba", "grotta", "frutteto", "obitorio", "monastero", "acquario",
    "bazar", "cantiere", "museo", "serraglio", "stamperia", "distilleria",
    "campanile", "sartoria", "granaio", "fornace", "molo", "anfiteatro",
};

static const char *QUALITA[] = {
    "di vetro", "di cenere", "in fiamme", "alla deriva", "senza fondo",
    "di mezzanotte", "in rovina", "di specchi", "sotto sale", "in quarantena",
    "fuori orbita", "di velluto", "a testa in giu'", "di sabbia",
    "tra le nuvole", "di ossidiana", "in eterno autunno", "di neon",
    "a molla", "di muffa luminosa", "senza gravita'", "di carta",
    "in miniatura", "di corallo", "al contrario", "di ambra", "in letargo",
    "di ruggine", "a maree", "di porcellana", "in festa", "di ossa",
    "sotto vuoto", "di lanterne", "in eclissi", "di ragnatele",
    "a orologeria", "di miele", "in piena", "di pece",
};

static const char *MATERIE[] = {
    "vetro", "spore", "fulmini", "sabbia", "inchiostro", "ossa", "petali",
    "monete", "ghiaccio", "lava", "vapore", "chiodi", "stelle", "radici",
    "schegge", "bolle", "dadi", "aghi", "piume", "catene", "scintille",
    "meduse", "cristalli", "semi", "trottole", "lame", "gocce", "prismi",
    "comete", "spine", "ingranaggi", "ceneri", "perle", "saette",
    "ampolle", "frammenti di specchio",
};

static const char *BESTIARIO[] = {
    "falene", "granchi", "burattini", "gargolle", "lumache", "vespe",
    "armature vuote", "statue", "funghi", "serpi", "corvi", "ragni",
    "golem", "spettri", "pesci", "ricci", "talpe", "scarabei",
    "lanterne viventi", "libri", "radici", "cristalli viventi", "sciami",
    "meduse", "salamandre", "mimi", "campane", "bambole", "tartarughe",
    "sentinelle di fumo",
};

static const char *VINCOLI[] = {
    "almeno un piano e' un luogo d'acqua",
    "un boss e' minuscolo ma letale",
    "un tipo di colpo punisce chi sta fermo",
    "due piani sono lo stesso luogo prima e dopo un disastro",
    "un piano e' assurdamente allegro, e per questo inquietante",
    "un tema nasce da un mestiere antico",
    "un piano vive di notte perpetua",
    "un tipo di colpo e' fatto di qualcosa di commestibile",
    "un nemico e' un oggetto domestico animato",
    "un piano sta dentro qualcosa di vivo ed enorme",
    "un boss ha un nome gentile e maniere pessime",
    "un piano e' verticale: tutto scende o tutto sale",
    "un tipo di colpo torna indietro in qualche modo",
    "i nemici di un piano collaborano fra loro in modo visibile",
    "un piano e' il rovescio esatto di un altro",
    "un tema e' un luogo comune ribaltato",
};

/* Esempi rotanti per il SYSTEM prompt (vedi gen_inspire.h). A differenza
   delle liste sopra qui il contenuto deve restare JSON valido dentro le
   bande numeriche del prompt: non e' materia prima da combinare, e' un
   pezzo intero gia' pronto, quindi il pool e' scritto a mano invece che
   assemblato da liste di parole. Include gli esempi storici (Colonnato
   Sacro, Scoria Impazzita, Sentinella Corazzata, Chiodi Arrugginiti, Raggio
   Lunare, Scarica Salina): restano UNA possibilita' fra tante, non piu'
   l'unica. Nomi italiani, senza accenti (apostrofo al posto dell'accento,
   come sopra), e nessuno ripreso dalle liste d'ispirazione. */

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
    { "Colonnato Sacro",     "pillars",  0.6 },
    { "Sala degli Specchi",  "scatter",  0.5 },
    { "Ponte dei Sospiri",   "corridor", 0.3 },
    { "Fossa dei Leoni",     "arena",    0.7 },
    { "Vuoto Cerimoniale",   "open",     0.2 },
    { "Bosco di Colonne",    "pillars",  0.8 },
    { "Cunicolo Allagato",   "corridor", 0.5 },
    { "Deposito Sfasciato",  "scatter",  0.9 },
    { "Rotonda del Giudizio","arena",    0.4 },
    { "Piazza Deserta",      "open",     0.3 },
};

/* Nemici "da mischia": move chase/charge/zigzag, fire none/single. */
static const EnemyExample ENEMY_EXAMPLES_MELEE[] = {
    { "Scoria Impazzita",   "blob",    "zigzag", "none",   0.8, 1.4, 0.9, 0,   1 },
    { "Grugnito d'Ottone",  "armored", "charge", "none",   2.2, 0.5, 1.6, 0,   1 },
    { "Sciame di Vespe",    "spiky",   "zigzag", "single", 0.6, 1.8, 0.6, 1.2, 1 },
    { "Talpa Corazzata",    "armored", "chase",  "none",   2.5, 0.7, 1.4, 0,   1 },
    { "Ombra Rovente",      "blob",    "chase",  "none",   1.0, 1.6, 1.0, 0,   1 },
    { "Ricciolo Spinato",   "spiky",   "charge", "single", 1.2, 1.0, 0.8, 0.6, 1 },
    { "Guscio Rabbioso",    "armored", "zigzag", "none",   2.0, 0.8, 1.5, 0,   1 },
    { "Falena di Cenere",   "floater", "chase",  "none",   0.7, 1.3, 0.7, 0,   1 },
};

/* Nemici "a distanza": move kite/orbit, fire spread/ring/single. */
static const EnemyExample ENEMY_EXAMPLES_RANGED[] = {
    { "Sentinella Corazzata","armored", "kite",  "spread", 1.8, 0.6, 1.3, 0.8, 3 },
    { "Medusa Fluttuante",   "floater", "orbit", "ring",   0.9, 0.9, 1.1, 0.5, 6 },
    { "Arciera di Vetro",    "spiky",   "kite",  "single", 0.7, 1.1, 0.7, 1.5, 1 },
    { "Corona Vagante",      "floater", "orbit", "spread", 1.1, 0.8, 0.9, 0.9, 4 },
    { "Guardiano a Distanza","armored", "kite",  "single", 2.0, 0.5, 1.5, 0.7, 1 },
    { "Sciame Pungente",     "blob",    "orbit", "ring",   0.8, 1.0, 0.8, 1.0, 5 },
    { "Cecchino di Sale",    "spiky",   "kite",  "spread", 0.9, 0.9, 0.9, 1.1, 3 },
    { "Vortice di Piume",    "floater", "orbit", "ring",   1.0, 1.2, 1.0, 0.6, 8 },
};

static const ShotExample SHOT_EXAMPLES[] = {
    { "Chiodi Arrugginiti", "spike", 1.4, 0.7, 0.6, 1.0, 1, 0, 1 },
    { "Raggio Lunare",      "beam",  1.9, 0.6, 0.5, 1.6, 2, 0, 1 },
    { "Scarica Salina",     "arc",   0.8, 0.9, 1.2, 0.9, 0, 2, 1 },
    { "Sfera di Miele",     "orb",   0.7, 1.3, 1.4, 1.1, 0, 0, 1 },
    { "Lama Vagante",       "blade", 1.0, 1.1, 0.9, 1.3, 1, 0, 1 },
    { "Dardo di Grandine",  "spike", 1.6, 0.6, 0.5, 0.8, 0, 0, 3 },
    { "Frusta di Brace",    "arc",   1.1, 0.8, 0.8, 1.0, 0, 3, 1 },
    { "Sciame di Perle",    "orb",   0.9, 0.6, 0.7, 1.0, 0, 0, 3 },
    { "Trapano di Vetro",   "spike", 0.6, 1.8, 0.6, 0.7, 2, 0, 1 },
    { "Falce di Nebbia",    "blade", 1.3, 0.7, 1.1, 0.9, 1, 0, 1 },
    { "Colonna di Vapore",  "beam",  0.5, 1.2, 1.5, 1.4, 1, 0, 1 },
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
             "Ispirazioni per QUESTA run: materia prima, non obblighi. Combinale,\n"
             "trasformale, accorda la grammatica; non copiarle tali e quali e non\n"
             "sentirti in dovere di usarle tutte. Tutto il resto inventalo da zero.\n"
             "- luoghi: %s\n"
             "- qualita' dei luoghi: %s\n"
             "- materia dei colpi: %s\n"
             "- spunti per il bestiario: %s\n"
             "- vincolo creativo della run: %s.",
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
