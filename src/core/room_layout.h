#ifndef MELTING_RUN_ROOM_LAYOUT_H
#define MELTING_RUN_ROOM_LAYOUT_H

#include <stdbool.h>

/* Layout delle stanze (fase 3c; la spec dedicata non fu mai scritta, il
 * design canonico e' docs/design/systems/rooms-and-floor-generation.md).
 *
 * STESSO PRINCIPIO di tutto il resto (tipi di colpo, nemici): il motore NON ha un
 * catalogo di stanze. Espone un vocabolario parametrico -- una FORMA di layout
 * (come sono disposti gli ostacoli) e una densita' -- e il MODELLO inventa
 * l'architettura di ogni piano nel JSON. La stanza vuota di sempre resta lo
 * zero-default (ROOM_LAYOUT_OPEN = 0).
 *
 * Fino a qui ogni stanza era un rettangolo vuoto. Ora una stanza di combattimento
 * puo' avere OSTACOLI solidi: coperture dietro cui ripararsi, strozzature, colonne.
 * Cambiano il gioco (posizione, linea di tiro) senza cambiare la struttura del
 * piano (la mappa, le porte, i tipi di stanza restano quelli).
 *
 * Come shot_type.h/enemy_type.h, questo header NON include raylib ne' game_types.h:
 * lo includono sia il gioco sia melting-gen (una sola definizione). Gli OSTACOLI
 * concreti sono rettangoli con coordinate in pixel: qui si usano float nudi, il
 * gioco li converte in Rectangle di raylib dove serve.
 *
 * LA GARANZIA (perche' si puo' lasciare a un 7B l'invenzione delle stanze):
 * RoomLayoutBuild non produce MAI una stanza ingiocabile. Tiene sempre libera una
 * CROCE centrale (la fascia verticale e quella orizzontale che passano per il
 * centro) piu' un cerchio al centro: cosi' il giocatore non nasce mai dentro un
 * muro, e ogni porta (in mezzo a ciascuna parete) e' sempre raggiungibile dal
 * centro lungo la croce. Gli ostacoli vivono solo nei quattro quadranti. La
 * densita' e' clampata: una stanza non puo' riempirsi di muri. */

typedef enum RoomForm {
    ROOM_LAYOUT_OPEN = 0,   /* nessun ostacolo: la stanza vuota di sempre */
    ROOM_LAYOUT_PILLARS,    /* colonne simmetriche, una per quadrante: coperture sparse */
    ROOM_LAYOUT_CORRIDOR,   /* due blocchi sopra e sotto: un corridoio orizzontale al centro */
    ROOM_LAYOUT_ARENA,      /* blocchi negli angoli: un'arena aperta al centro, riparata ai bordi */
    ROOM_LAYOUT_SCATTER,    /* tanti blocchetti sparsi nei quadranti */
    ROOM_LAYOUT_COUNT
} RoomForm;

typedef struct RoomLayoutDef {
    bool active;
    char name[32];     /* inventato dal modello, mostrato nel pannello */
    RoomForm form;
    float density;     /* 0..1: scala numero e dimensione degli ostacoli */
} RoomLayoutDef;

/* WP3 (docs/design/systems/secrets-and-obstacles.md, "Ostacoli generati a
   tema"): la FAMIGLIA di un ostacolo, indipendente dalla sua FORMA (RoomForm
   sopra decide come sono disposti i blocchi nella stanza; ObstacleFamily
   decide come si comporta CIASCUN blocco).
     - OBSTACLE_SOLID (zero-default): il blocco di sempre, blocca movimento,
       linea di tiro, nemici. Comportamento identico a prima di WP3.
     - OBSTACLE_DESTRUCTIBLE: si comporta come SOLID finche' lo strumento di
       breccia (la bomba, CombatExplodeAt in src/gameplay/combat.c) non lo
       rimuove nel raggio dell'esplosione.
     - OBSTACLE_HAZARD ("pericolo passivo"): NON blocca -- il giocatore ci
       cammina sopra/attraverso -- ma danneggia al contatto, sempre
       telegrafato visivamente PRIMA di poter colpire (DEC-058: mai
       un'informazione affidata al solo colore).
   Questo modulo resta un modulo PURO di geometria (RoomLayoutBuild produce
   sempre e solo OBSTACLE_SOLID: assegnare le altre famiglie e' compito del
   chiamante, che sa in quale PIANO e CELLA sta costruendo -- vedi
   WorldBuildObstacles in src/world/world.c, l'unico punto che decide le
   proporzioni fra famiglie secondo la degenerazione del tema, DEC-024). */
typedef enum ObstacleFamily {
    OBSTACLE_SOLID = 0,
    OBSTACLE_DESTRUCTIBLE,
    OBSTACLE_HAZARD,
    OBSTACLE_FAMILY_COUNT
} ObstacleFamily;

/* Un ostacolo: rettangolo solido allineato agli assi, in pixel, con una
   famiglia (vedi sopra). */
typedef struct Obstacle {
    float x, y, w, h;
    ObstacleFamily family;   /* zero-default = OBSTACLE_SOLID, comportamento di sempre */
} Obstacle;

/* DEC-170: una stanza puo' occupare fino a 4 celle, e il layout si espande UNA
   VOLTA PER CELLA (scelta di implementazione ammessa dalla decisione, vedi
   rooms-and-floor-generation.md): il tetto e' quindi 4 volte quello storico di
   una cella (10 blocchi, il massimo che RoomLayoutBuild produce con SCATTER a
   densita' 1.0), piu' UNO per l'eventuale cella-buco di una forma a L, che il
   gioco tratta come un ostacolo solido. Il tetto PER CELLA resta 10: una
   stanza 1x1 ha esattamente gli stessi ostacoli di prima di DEC-170. */
#define MAX_OBSTACLES 41
#define ROOM_LAYOUT_MAX_PER_CELL 10

/* Banda della densita': MODIFICA QUI per ribilanciare quanto una stanza puo'
   riempirsi. Il minimo non e' 0 (un layout ATTIVO ma a densita' zero non sarebbe
   un layout): sotto ROOM_LAYOUT_DENSITY_MIN e' come OPEN. */
#define ROOM_LAYOUT_DENSITY_MIN 0.15f
#define ROOM_LAYOUT_DENSITY_MAX 1.0f

/* Testo canonico <-> enum (gli stessi testi che il modello scrive nel JSON, che
   gen_manifest.c scrive nel manifest e che run_content.c rilegge). Testo
   sconosciuto -> ROOM_LAYOUT_OPEN, cioe' la stanza vuota (mai una forma esotica per
   un dato corrotto). */
RoomForm RoomFormFromText(const char *text);
const char *RoomFormName(RoomForm form);

void RoomLayoutClamp(RoomLayoutDef *def);

/* Espande un layout in ostacoli CONCRETI dentro il rettangolo di gioco (x, y, w, h
   in pixel), scrivendo fino a 'maxOut' ostacoli in 'out' e ritornando quanti ne ha
   scritti. GARANTITO giocabile (vedi il commento in cima): la croce centrale e il
   centro restano sempre liberi, la densita' e' clampata. 'seed' varia solo i
   dettagli (posizioni fini) a parita' di forma, cosi' due stanze con la stessa
   forma non sono identiche ma restano entrambe valide. Un layout non attivo o
   ROOM_LAYOUT_OPEN scrive zero ostacoli. */
int RoomLayoutBuild(const RoomLayoutDef *def, unsigned int seed,
                    float x, float y, float w, float h,
                    Obstacle *out, int maxOut);

/* Layout di ESEMPIO (ripiego procedurale + esempi nel prompt), gia' clampati.
   0..ROOM_LAYOUT_EXAMPLE_COUNT-1. Non sono "i layout del gioco": sono contenuto di
   riserva, come per i tipi di colpo e i nemici. */
#define ROOM_LAYOUT_EXAMPLE_COUNT 4
void RoomLayoutExample(RoomLayoutDef *out, int index);

#endif
