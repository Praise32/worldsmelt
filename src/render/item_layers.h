#ifndef MELTING_RUN_ITEM_LAYERS_H
#define MELTING_RUN_ITEM_LAYERS_H

#include "core/game_types.h"

/* Il personaggio a strati (vision doc, docs/superpowers/specs/2026-07-13-
   items-synergy-vision.md, sezione 3; APPUNTI.md sezioni 4 e 6): la base e'
   uno stickman minimale e FISSO (vedi DrawPlayer in game_renderer.c), e ogni
   oggetto equipaggiato aggiunge un layer geometrico sopra di essa, ancorato
   a uno slot fisso (testa, occhi, mano, schiena, corpo, aura). Questo file
   e' il modello pluggable del layer: PURO (BuildItemLayers non tocca lo
   schermo, non alloca, non dipende da Game), cosi' e' testabile da solo e
   pronto ad accogliere sprite generati al posto della forma geometrica senza
   che il resto del motore se ne accorga (vedi il commento su ItemLayer
   sotto). */

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

/* Cosa disegna un oggetto equipaggiato al suo slot. OGGI: una forma
   geometrica tinta del colore dell'oggetto (vedi DrawItemLayer in
   item_layers.c) -- esattamente cio' che il gioco gia' faceva prima di
   questo refactor, solo riorganizzato. DOMANI: quando arriveranno gli
   sprite 128x128 generati per oggetto (vision doc, sezione 3), questo
   struct guadagnera' un handle Texture2D/cella d'atlas e DrawItemLayer
   scegliera' lo sprite quando presente, la forma quando no -- un cambio
   confinato a UNA funzione (DrawItemLayer), mai al resto del motore
   (BuildItemLayers, l'ordine degli slot, gli agganci) che non deve sapere
   da dove viene un layer. */
typedef struct ItemLayer {
    ItemSlot slot;
    Color color;
    int stackIndex;   /* posizione 0-based fra i layer con lo stesso slot, nell'ordine di raccolta */
    int stackTotal;    /* quanti oggetti occupano DAVVERO questo slot (puo' superare ITEM_LAYER_MAX_PER_SLOT) */
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

/* L'UNICA funzione che disegna un layer sullo schermo. Oggi: interruttore
   sullo slot, forma geometrica bespoke (la stessa che il gioco aveva prima
   del refactor), tinta di layer.color, posizionata rispetto ad anchors e
   layer.stackIndex. Se layer.stackTotal supera ITEM_LAYER_MAX_PER_SLOT e
   questo e' l'ultimo layer visibile dello slot, disegna anche un piccolo
   "+N" (vedi il commento sulla costante sopra). */
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
