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
