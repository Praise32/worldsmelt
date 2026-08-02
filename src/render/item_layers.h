#ifndef MELTING_RUN_ITEM_LAYERS_H
#define MELTING_RUN_ITEM_LAYERS_H

#include "core/game_types.h"

/* Il personaggio a strati (vision doc, docs/engineering/specs/2026-07-13-
   items-synergy-vision.md, sezione 3; APPUNTI.md sezioni 4 e 6): la base e'
   uno stickman minimale e FISSO (vedi DrawPlayer in game_renderer.c), e ogni
   oggetto equipaggiato aggiunge un layer sopra di essa, ancorato a uno slot
   fisso (testa, occhi, mano, schiena, corpo, aura). Questo file e' il
   modello pluggable del layer: PURO (BuildItemLayers non tocca lo schermo,
   non alloca, non dipende da Game), cosi' e' testabile da solo e -- da
   WP-ASSET-1 -- disegnato con un overlay sprite quando l'asset e' presente,
   con la forma geometrica di sempre come ripiego (vedi il commento su
   ItemLayer sotto). */

/* Quanti oggetti nello STESSO slot ottengono un layer disegnato per davvero.
   Oltre questo tetto l'oggetto resta pienamente funzionante (i suoi trait e
   le sue callback continuano a valere, vedi src/script/script_items.c: qui
   si tocca SOLO il disegno) ma non aggiunge un'altra forma sul personaggio,
   altrimenti una dozzina di oggetti nello stesso slot diventerebbe poltiglia
   illeggibile invece di leggersi come una build. Al posto della forma in
   piu', l'ultimo layer visibile dello slot mostra un piccolo "+N" (vedi
   DrawItemLayer in item_layers.c). 6 e' abbastanza per leggere chiaramente
   uno stack (il cappello piu' alto arriva a malapena sopra l'HUD) restando
   sotto MAX_ITEMS (18, game_types.h) anche se un giocatore concentrasse
   tutto in un solo slot. */
#define ITEM_LAYER_MAX_PER_SLOT 6

/* Punti di aggancio del personaggio BASE (lo stickman minimale, vedi
   DrawPlayer), in coordinate schermo, calcolati da posizione e raggio del
   giocatore -- non costanti fisse -- cosi' restano corretti anche se in
   futuro Player.radius cambiasse (oggi e' sempre 14.0f, vedi game.c). Ogni
   slot ha qui il suo punto per lo stackIndex 0 (il PRIMO oggetto raccolto in
   quello slot); i layer successivi nello stesso slot si dispongono attorno
   a questo punto (vedi DrawItemLayer): l'aggancio "di base" di uno slot non
   si sposta mai in funzione di COSA e' equipaggiato, solo in funzione DI DOVE
   e' il giocatore. E' la garanzia di affidabilita' richiesta dalla vision
   doc ("se il personaggio base cambia ad ogni run, non so piu' dove
   attaccare il cappello"). */
typedef struct PlayerAnchors {
    Vector2 hat;         /* cappello piu' basso; gli altri si impilano sopra */
    Vector2 eyes;
    Vector2 hand;
    Vector2 backTip;     /* punta superiore del mantello, verso il collo */
    Vector2 backHem;     /* orlo inferiore del mantello */
    Vector2 body;
    Vector2 aura;         /* centro dell'orbita (= posizione del giocatore) */
    float auraRadius;
} PlayerAnchors;

PlayerAnchors PlayerComputeAnchors(Vector2 pos, float radius);

/* Cosa disegna un oggetto equipaggiato al suo slot. WP-ASSET-1: DrawItemLayer
   (item_layers.c) prova prima un overlay sprite generico in assets/art/equip/
   (2-3 varianti per slot, MAI legate a un oggetto specifico -- vedi
   scripts/gen_equip_overlays.py) e ricade sulla forma geometrica di sempre
   solo quando l'asset manca (checkout parziale, degrado standard). Il
   cambio e' rimasto confinato a UNA funzione (DrawItemLayer), come promesso
   qui sotto: BuildItemLayers, l'ordine degli slot e gli agganci non sanno da
   dove viene un layer. */
typedef struct ItemLayer {
    ItemSlot slot;
    Color color;
    int stackIndex;   /* posizione 0-based fra i layer con lo stesso slot, nell'ordine di raccolta */
    int stackTotal;    /* quanti oggetti occupano DAVVERO questo slot (puo' superare ITEM_LAYER_MAX_PER_SLOT) */
    /* Hash FNV-1a del nome dell'oggetto (BuildItemLayers, item_layers.c):
       sceglie DETERMINISTICAMENTE quale delle 2-3 varianti sprite dello slot
       disegnare (variantSeed % numero varianti dello slot). Per NOME, non
       per istanza: due copie dello stesso oggetto nello stesso slot mostrano
       sempre la stessa variante, e la stessa run rigenerata con lo stesso
       seed produce lo stesso personaggio a schermo -- nessun dado ad ogni
       frame. Non e' un colore ne' un ID di inventario: sceglie solo la
       "faccia" grafica fra le varianti generiche dello slot. */
    unsigned int variantSeed;
} ItemLayer;

/* Costruisce, in ORDINE DI DISEGNO fisso (corpo, mantello, mano, occhi,
   cappello, aura -- dal piu' dietro al piu' davanti, vedi il commento
   sull'ordine in DrawPlayer/game_renderer.c), la lista dei layer visibili
   per gli oggetti equipaggiati in items[0..itemCount). Pura: non tocca lo
   schermo, non alloca (out e' un buffer del chiamante, mai piu' grande di
   itemCount elementi davvero scritti), non dipende da Game ne' da Player.
   Un oggetto NON attivo (item->active false) non produce mai un layer. Oltre
   ITEM_LAYER_MAX_PER_SLOT oggetti nello stesso slot, i successivi restano
   contati in stackTotal ma non aggiungono un altro elemento a out (vedi il
   commento sulla costante sopra). Ritorna quanti elementi sono stati
   scritti in out (mai piu' di outCapacity). */
int BuildItemLayers(const Item *items, int itemCount, ItemLayer *out, int outCapacity);

/* L'UNICA funzione che disegna un layer sullo schermo. Interruttore sullo
   slot: prova un overlay sprite neutro (assets/art/equip/, variante scelta
   da layer.variantSeed) con un piccolo accento di layer.color sovrapposto
   (MAI un tint dell'intero sprite, DEC-199), posizionato rispetto ad anchors
   e layer.stackIndex; se l'asset manca ricade sulla forma geometrica bespoke
   tinta di layer.color che il gioco aveva prima di WP-ASSET-1. Se
   layer.stackTotal supera ITEM_LAYER_MAX_PER_SLOT e questo e' l'ultimo layer
   visibile dello slot, disegna anche un piccolo "+N" (vedi il commento sulla
   costante sopra), sempre allo stesso modo indipendentemente dal ramo. */
void DrawItemLayer(PlayerAnchors anchors, ItemLayer layer);

/* Vero se lo slot va disegnato PRIMA della base (dietro di essa): corpo e
   mantello. Falso per tutti gli altri (mano, occhi, cappello, aura), che
   vanno disegnati DOPO (davanti). BuildItemLayers scrive gia' out[] in
   quest'ordine (vedi kSlotDrawOrder in item_layers.c), quindi DrawPlayer
   (game_renderer.c) puo' disegnare i layer in sequenza e infilare il
   disegno della base esattamente dove questa funzione smette di essere
   vera per la prima volta. */
bool ItemLayerIsBehindBase(ItemSlot slot);

#endif
