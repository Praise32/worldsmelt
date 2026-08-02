#include "render/item_layers.h"

#include "assets/art_atlas.h"
#include "core/game_math.h"
#include "render/art_draw.h"

#include "raylib.h"

#include <math.h>
#include <stdio.h>

/* Ordine di disegno FISSO degli slot, dal piu' dietro al piu' davanti: il
   corpo (una tinta sul busto) e il mantello stanno dietro alla base (sono
   "sotto" il personaggio, vedi DrawPlayer), mano/occhi/cappello stanno
   davanti (si vedono chiaramente sopra la base), l'aura orbita e va sopra a
   tutto per restare sempre visibile qualunque cosa sia equipaggiata. Questo
   e' anche l'ordine in cui BuildItemLayers scrive out[], quindi il chiamante
   (DrawPlayer) puo' semplicemente disegnare i layer in sequenza e infilare
   il disegno della base a meta', dove SLOT_BACK finisce e SLOT_HAND inizia
   (vedi PlayerLayerIsBehindBase, usata da game_renderer.c). */
static const ItemSlot kSlotDrawOrder[] = {
    SLOT_BODY, SLOT_BACK, SLOT_HAND, SLOT_EYES, SLOT_HAT, SLOT_AURA
};
static const int kSlotDrawOrderCount = (int)(sizeof(kSlotDrawOrder)/sizeof(kSlotDrawOrder[0]));

/* Hash FNV-1a a 32 bit sul nome dell'oggetto: sceglie deterministicamente la
   variante sprite dello slot (ItemLayer.variantSeed, vedi il commento in
   item_layers.h). Stessa tecnica di RoomTileset in game_renderer.c (non
   condivisa da li': sei righe non meritano un'intestazione comune), qui
   sul nome invece che sul tema perche' e' l'unico dato stabile e leggibile
   che un Item porta sempre con se'. */
static unsigned int ItemLayerHashName(const char *name)
{
    unsigned int hash = 2166136261u;
    for (int i = 0; name[i]; i++) { hash ^= (unsigned char)name[i]; hash *= 16777619u; }
    return hash;
}

PlayerAnchors PlayerComputeAnchors(Vector2 pos, float radius)
{
    /* Le distanze sotto sono tarate a occhio per il raggio di riferimento
       del giocatore (14.0f, vedi game.c, Player.radius non cambia mai oggi):
       si scalano proporzionalmente al raggio vero, cosi' l'aggancio resta
       corretto per posizione E raggio (non solo posizione) se in futuro
       qualcosa facesse variare Player.radius. */
    const float refRadius = 14.0f;
    float k = radius/refRadius;

    PlayerAnchors a;
    a.hat = (Vector2){ pos.x, pos.y - 32.0f*k };
    a.eyes = (Vector2){ pos.x, pos.y - 18.0f*k };
    a.hand = (Vector2){ pos.x, pos.y - 2.0f*k };
    a.backTip = (Vector2){ pos.x, pos.y - 5.0f*k };
    a.backHem = (Vector2){ pos.x, pos.y + 31.0f*k };
    a.body = (Vector2){ pos.x, pos.y + 3.0f*k };
    a.aura = pos;
    a.auraRadius = 34.0f*k;
    return a;
}

int BuildItemLayers(const Item *items, int itemCount, ItemLayer *out, int outCapacity)
{
    if (itemCount > MAX_ITEMS) itemCount = MAX_ITEMS;   /* difesa: mai leggere oltre l'array reale del chiamante */
    int written = 0;
    for (int s = 0; s < kSlotDrawOrderCount; s++)
    {
        ItemSlot slot = kSlotDrawOrder[s];
        /* Due passaggi sullo stesso slot: il primo conta quanti oggetti
           attivi ci sono DAVVERO (stackTotal, serve al badge "+N" anche
           quando il tetto viene raggiunto prima di aver visto l'ultimo
           oggetto), il secondo scrive i layer nell'ordine di raccolta fino
           al tetto. itemCount e' al massimo MAX_ITEMS (18): il costo di
           scandirlo due volte per ciascuno dei sei slot resta trascurabile
           e non alloca nulla. */
        int total = 0;
        for (int i = 0; i < itemCount; i++) if (items[i].active && items[i].slot == slot) total++;
        if (total == 0) continue;

        int idx = 0;
        for (int i = 0; i < itemCount && idx < ITEM_LAYER_MAX_PER_SLOT; i++)
        {
            if (!items[i].active || items[i].slot != slot) continue;
            if (written >= outCapacity) return written;
            out[written].slot = slot;
            out[written].color = items[i].color;
            out[written].stackIndex = idx;
            out[written].stackTotal = total;
            out[written].variantSeed = ItemLayerHashName(items[i].name);
            written++;
            idx++;
        }
    }
    return written;
}

/* Posizione approssimativa dell'ultimo layer visibile di uno slot, usata
   SOLO per piazzare il badge "+N" quando lo slot va oltre il tetto: non deve
   essere pixel-perfect (e' un'etichetta di testo, non un altro layer), deve
   solo capitare vicino allo stack cosi' il giocatore capisce a cosa si
   riferisce. */
static Vector2 OverflowBadgePos(PlayerAnchors anchors, ItemSlot slot)
{
    const int lastIdx = ITEM_LAYER_MAX_PER_SLOT - 1;
    switch (slot)
    {
        case SLOT_HAT: return (Vector2){ anchors.hat.x + 20.0f, anchors.hat.y - (float)lastIdx*8.0f };
        case SLOT_EYES: return (Vector2){ anchors.eyes.x + 16.0f, anchors.eyes.y - (float)lastIdx*5.0f };
        case SLOT_HAND: return (Vector2){ anchors.hand.x, anchors.hand.y - 26.0f };
        case SLOT_BACK: return (Vector2){ anchors.backHem.x, anchors.backHem.y + 12.0f };
        case SLOT_BODY: return (Vector2){ anchors.body.x + 14.0f, anchors.body.y };
        default: return (Vector2){ anchors.aura.x, anchors.aura.y - anchors.auraRadius - 14.0f };   /* SLOT_AURA */
    }
}

/* WP-ASSET-1: quante varianti generiche esistono per slot in
   assets/art/equip/ (scripts/gen_equip_overlays.py e' la fonte che le
   genera, questi numeri devono restare sincronizzati con SLOTS li' dentro).
   Giro 2, verdetto D5: le scale erano scese sotto la disciplina di
   ArtScaleForWidth (art_draw.c, "si aggancia a mezzi passi... un minimo di
   1"): 1.8/1.4 non sono mezzi passi E non erano nemmeno quelle usate dal
   floor test del generatore (bocciato per lo stesso motivo -- vedi
   ENGINE_SCALE in gen_equip_overlays.py, che DEVE restare sincronizzato
   VALORE PER VALORE con queste sei righe). Portate a 2.0/1.5: l'ingombro a
   schermo resta vicino a quello della vecchia forma geometrica dello stesso
   slot (misurato a occhio, non da una formula), la griglia dei pixel no. */
#define EQUIP_HAT_VARIANTS  3
#define EQUIP_EYES_VARIANTS 3
#define EQUIP_HAND_VARIANTS 3
#define EQUIP_BACK_VARIANTS 2
#define EQUIP_BODY_VARIANTS 2
#define EQUIP_AURA_VARIANTS 2
#define EQUIP_HAT_SCALE  2.0f
#define EQUIP_EYES_SCALE 2.0f
#define EQUIP_HAND_SCALE 2.0f
#define EQUIP_BACK_SCALE 2.0f
#define EQUIP_BODY_SCALE 1.5f
#define EQUIP_AURA_SCALE 1.5f

/* L'UNICO punto in cui il colore VERO dell'oggetto (layer.color) tocca uno
   sprite d'equip: un piccolo blocco quadrato -- 1 pixel nativo per la
   maggior parte degli slot, 5 per l'aura (vedi il commento su SLOTS['aura']
   in gen_equip_overlays.py) -- disegnato SOPRA lo sprite gia' tracciato. Mai
   un tint sull'intero sprite: ricolorare l'intera base neutra "sporcherebbe"
   il materiale slag/cenere disegnato (stessa cautela di DEC-199 sul
   personaggio generato).
   Giro 2, verdetto D1: 'dxNative'/'dyNative' sono l'angolo ALTO-SINISTRA del
   blocco (non piu' il suo centro) nello STESSO sistema di coordinate che usa
   ArtDrawFrame per posizionare un texel -- 'dst.y = anchorPos.y -
   anchorY*scale', cioe' il pixel nativo 'p' finisce a schermo su
   'anchorPos + (p - anchor)*scale'. Il vecchio '- size*0.5f' centrava il
   blocco su quel punto invece di farlo INIZIARE li', sfalsando l'accento di
   mezzo pixel nativo su entrambi gli assi rispetto al texel che gen_equip_
   overlays.py aveva davvero in mente (misurabile sullo screenshot: l'accento
   sconfinava oltre la sagoma sul lato alto-sinistro). Ora 'dxNative'/
   'dyNative' sono l'offset dall'ancora dello sprite in pixel NATIVI (prima
   della scala) allo STESSO angolo alto-sinistra, e si moltiplicano per
   'scale' qui dentro cosi' il chiamante non deve rifare il conto per ogni
   slot. Le coordinate devono combaciare ESATTAMENTE con 'accent' in
   gen_equip_overlays.py per quello slot, o l'accento cade fuori sagoma. */
static void DrawEquipAccent(Vector2 anchorPos, float scale, float dxNative, float dyNative, float sizeNative, Color color)
{
    float size = sizeNative*scale;
    Rectangle r = { anchorPos.x + dxNative*scale, anchorPos.y + dyNative*scale, size, size };
    DrawRectangleRec(r, color);
}

void DrawItemLayer(PlayerAnchors anchors, ItemLayer layer)
{
    /* Interruttore sullo slot: prova prima l'overlay sprite (chiave
       "equip/<slot>_<variante>", variante scelta da layer.variantSeed), e
       ricade sulla forma bespoke di sempre solo se ArtAtlasGet non trova
       l'asset (checkout senza assets/art/equip/, degrado standard come il
       resto del pacchetto W8). La forma geometrica di ogni case sotto resta
       BIT PER BIT quella di prima di WP-ASSET-1: e' il ripiego, non un
       secondo design da mantenere allineato al primo. */
    char key[24];
    switch (layer.slot)
    {
        case SLOT_HAT:
        {
            /* Giro 2: l'ancora del JSON e' stata spostata da riga11 a riga7
               (scripts/gen_equip_overlays.py, SLOTS['hat']) perche' con
               riga11 l'orlo della falda finiva a filo di 'pos', 8 unita' piu'
               in alto di dove cadeva l'orlo della vecchia forma geometrica
               (DrawRectangleRounded qui sotto, brim da y a y+8, ultimo pixel
               coperto y+7) -- la pila sedeva visibilmente troppo in alto
               sulla testa. Con riga7 e EQUIP_HAT_SCALE=2.0, riga11 (l'orlo)
               comincia a pos.y+(11-7)*2 = pos.y+8 e, essendo alta 2 schermo-
               pixel alla scala 2.0, arriva a pos.y+9 come ultimo pixel
               coperto -- VICINO a dove cadeva l'orlo di prima (pos.y+7), non
               piu' esattamente li' (verdetto R3, giro 3: la frase originale
               "ESATTAMENTE" ignorava che una riga nativa non e' un punto ma
               una banda larga quanto la scala). 2px di scarto restano, contro
               i ~13px di prima: irrilevanti a occhio, non vale un secondo
               giro di ritaratura dell'ancora per inseguirli. */
            float y = anchors.hat.y - (float)layer.stackIndex*8.0f;
            Vector2 pos = { anchors.hat.x, y };
            snprintf(key, sizeof(key), "equip/hat_%u", (layer.variantSeed % EQUIP_HAT_VARIANTS) + 1);
            const ArtSheet *sheet = ArtAtlasGet(key);
            if (sheet)
            {
                ArtDrawFrame(sheet, 0, 0, pos, EQUIP_HAT_SCALE, false, WHITE);
                /* Giro 2, verdetto D3 (regressione): l'accento era in riga2
                   (la cima della cupola). Col passo di impilamento a 8 unita'
                   contro un'altezza sprite di 24 (12 righe*scala2), ogni
                   cappello copre quasi per intero quello sotto TRANNE le sue
                   ultime tre righe (9-11, l'orlo, vedi il commento sopra su
                   'y'): la cima finiva coperta per 5 cappelli su 6, solo
                   quello in cima restava visibile. Riga10 (dentro quella
                   banda sempre scoperta) sopravvive per OGNI cappello della
                   pila -- dimostrato in GameLayerTest (game_tests.c),
                   conteggio pixel dei sei colori sullo screenshot vero. */
                DrawEquipAccent(pos, EQUIP_HAT_SCALE, 0.0f, 3.0f, 1.0f, layer.color);
            }
            else
            {
                DrawRectangleRounded((Rectangle){ anchors.hat.x - 14, y, 28, 8 }, 0.3f, 5, layer.color);
                DrawRectangleRounded((Rectangle){ anchors.hat.x - 9, y - 9, 18, 12 }, 0.35f, 5, layer.color);
            }
            break;
        }
        case SLOT_EYES:
        {
            /* Occhiali sopra occhiali (Isaac docet): ogni paio in piu' si
               impila leggermente piu' in alto, come i cappelli ma con un
               passo piu' piccolo (sono piu' vicini alla testa). */
            float y = anchors.eyes.y - (float)layer.stackIndex*5.0f;
            Vector2 pos = { anchors.eyes.x, y };
            snprintf(key, sizeof(key), "equip/eyes_%u", (layer.variantSeed % EQUIP_EYES_VARIANTS) + 1);
            const ArtSheet *sheet = ArtAtlasGet(key);
            if (sheet)
            {
                /* Ancora JSON a riga2 (non riga3, giro 2): le lenti occupano
                   le righe 1-3, il loro centro verticale vero e' la riga2 --
                   con l'ancora li' lo sprite si impila centrato sul punto
                   dove stavano i vecchi DrawCircleV (centrati su anchors.
                   eyes.y), non 2 unita' piu' in basso. */
                ArtDrawFrame(sheet, 0, 0, pos, EQUIP_EYES_SCALE, false, WHITE);
                DrawEquipAccent(pos, EQUIP_EYES_SCALE, 3.0f, 1.0f, 1.0f, layer.color);
            }
            else
            {
                DrawCircleV((Vector2){ anchors.eyes.x - 6, y }, 4.5f, layer.color);
                DrawCircleV((Vector2){ anchors.eyes.x + 6, y }, 4.5f, layer.color);
                DrawLineEx((Vector2){ anchors.eyes.x - 2, y }, (Vector2){ anchors.eyes.x + 2, y }, 2.0f, layer.color);
            }
            break;
        }
        case SLOT_HAND:
        {
            /* Alterna lato sinistro/destro; ogni coppia successiva (stackIndex
               2 e 3, 4 e 5...) si allunga un po' cosi' le armi/oggetti non si
               sovrappongono esattamente l'uno sull'altro. */
            float side = (layer.stackIndex%2 == 0) ? 1.0f : -1.0f;
            float reach = 19.0f + (float)(layer.stackIndex/2)*6.0f;
            /* Lo sprite non ha un "raggio" variabile come la linea geometrica:
               lo scarto verticale per coppia (stessa cadenza di 'reach' sopra)
               resta l'unico segnale di stack, oltre al lato alternato. */
            Vector2 pos = { anchors.hand.x + side*13.0f, anchors.hand.y - (float)(layer.stackIndex/2)*4.0f };
            snprintf(key, sizeof(key), "equip/hand_%u", (layer.variantSeed % EQUIP_HAND_VARIANTS) + 1);
            const ArtSheet *sheet = ArtAtlasGet(key);
            if (sheet)
            {
                bool flip = side < 0.0f;
                ArtDrawFrame(sheet, 0, 0, pos, EQUIP_HAND_SCALE, flip, WHITE);
                /* Verdetto N1 (giro 3): il vecchio commento qui ("lo specchio
                   non sposta l'accento") era FALSO -- misurato: con flip
                   attivo l'accento cadeva fuori sagoma su ogni stackIndex
                   dispari in hand_2/hand_3. Il frame e' largo 10 nativi
                   (colonne 0..9), l'ancora sta in colonna 5: NON il centro
                   geometrico esatto (un frame largo un numero PARI di
                   colonne non ne ha uno, il centro cade fra colonna 4 e 5).
                   ArtDrawFrame (art_draw.c) specchia il campionamento del
                   TESTO intorno a quel mezzo punto entro lo STESSO
                   rettangolo di destinazione, non intorno alla colonna
                   dell'ancora: la colonna 5 (dove vive l'accento, SLOTS
                   ['hand']['accent'] in gen_equip_overlays.py) finisce
                   specchiata sulla colonna 4, che e' vuota in hand_2/hand_3
                   (dipinta per caso solo in hand_1). La formula generale,
                   per un accento 1x1 in colonna c con ancora in colonna A:
                   dx specchiato = (A-1-c) = -dx_non_specchiato - 1; qui
                   A=c=5, quindi dx specchiato = -1, MAI 0. Prova end-to-end
                   (predice il pixel esatto, non solo "esiste da qualche
                   parte"): GameLayerTest, sonda N1, 3 varianti x 2 flip. */
                DrawEquipAccent(pos, EQUIP_HAND_SCALE, flip ? -1.0f : 0.0f, -11.0f, 1.0f, layer.color);
            }
            else
            {
                DrawLineEx((Vector2){ anchors.hand.x + side*13.0f, anchors.hand.y },
                           (Vector2){ anchors.hand.x + side*(13.0f + reach), anchors.hand.y - 12.0f }, 5.0f, layer.color);
            }
            break;
        }
        case SLOT_BACK:
        {
            /* Mantelli sovrapposti: ciascuno in piu' e' leggermente piu'
               largo e piu' trasparente di quello sotto, cosi' si legge come
               "strati" invece che coprire per intero quello precedente. */
            int alpha = GameMathClampInt(140 - layer.stackIndex*35, 40, 140);
            float spread = 22.0f + (float)layer.stackIndex*3.0f;
            snprintf(key, sizeof(key), "equip/back_%u", (layer.variantSeed % EQUIP_BACK_VARIANTS) + 1);
            const ArtSheet *sheet = ArtAtlasGet(key);
            if (sheet)
            {
                /* Lo sprite ha una sagoma fissa (non si allarga come il
                   triangolo geometrico sotto): lo stack si legge da un
                   piccolo scarto orizzontale -- ogni mantello in piu'
                   "sbircia" di lato -- e dalla stessa dissolvenza in alpha,
                   applicata al BIANCO del tint (mai al colore, DEC-199) e
                   ripetuta sull'accento cosi' anche il fermaglio sfuma. */
                Vector2 pos = { anchors.backTip.x + (float)layer.stackIndex*3.0f, anchors.backTip.y };
                ArtDrawFrame(sheet, 0, 0, pos, EQUIP_BACK_SCALE, false, (Color){ 255, 255, 255, (unsigned char)alpha });
                DrawEquipAccent(pos, EQUIP_BACK_SCALE, 0.0f, 1.0f, 1.0f, GameColorWithAlpha(layer.color, (unsigned char)alpha));
            }
            else
            {
                DrawTriangle((Vector2){ anchors.backTip.x, anchors.backTip.y },
                             (Vector2){ anchors.backTip.x - spread, anchors.backHem.y },
                             (Vector2){ anchors.backTip.x + spread, anchors.backHem.y },
                             GameColorWithAlpha(layer.color, (unsigned char)alpha));
            }
            break;
        }
        case SLOT_BODY:
        {
            /* Cerchi concentrici via via piu' piccoli (ripiego geometrico
               sotto): il primo oggetto resta il piu' grande e visibile, i
               successivi si vedono come un accenno di profondita' invece di
               sparire dietro al primo. */
            float r = GameMathClampFloat(6.0f - (float)layer.stackIndex*1.2f, 2.0f, 6.0f);
            snprintf(key, sizeof(key), "equip/body_%u", (layer.variantSeed % EQUIP_BODY_VARIANTS) + 1);
            const ArtSheet *sheet = ArtAtlasGet(key);
            if (sheet)
            {
                /* Giro 2, verdetto D2: NON si replica piu' il rimpicciolimento
                   della circonferenza geometrica ("shrink" = r/6, fino a
                   0.33) moltiplicandolo nella scala dello sprite --
                   ArtScaleForWidth (art_draw.c) vieta di scendere sotto 1.0
                   proprio perche' un pixel art sottocampionato si rompe, e a
                   scala 0.5 l'accento (1 pixel nativo) smetteva di produrre
                   ANCHE UN SOLO pixel intero a schermo (spariva del tutto per
                   stackIndex 4-5). La profondita' dello stack si legge invece
                   da un piccolo scarto di posizione, come per SLOT_HAND: la
                   scala resta SEMPRE EQUIP_BODY_SCALE, un mezzo passo >= 1. */
                Vector2 pos = { anchors.body.x + (float)layer.stackIndex*2.0f, anchors.body.y - (float)layer.stackIndex*1.5f };
                ArtDrawFrame(sheet, 0, 0, pos, EQUIP_BODY_SCALE, false, WHITE);
                DrawEquipAccent(pos, EQUIP_BODY_SCALE, 0.0f, 0.0f, 1.0f, layer.color);
            }
            else
            {
                DrawCircleV(anchors.body, r, layer.color);
            }
            break;
        }
        default:   /* SLOT_AURA */
        {
            /* L'angolo dipende dallo stackIndex (0..5), non da un indice
               globale sull'inventario: con il tetto a 6 lo stack si
               distribuisce gia' su un giro quasi completo (~1 radiante a
               elemento), senza bisogno di dividere per il totale. */
            float a = (float)GetTime()*2.4f + (float)layer.stackIndex;
            Vector2 p = { anchors.aura.x + cosf(a)*anchors.auraRadius, anchors.aura.y + sinf(a)*anchors.auraRadius };
            snprintf(key, sizeof(key), "equip/aura_%u", (layer.variantSeed % EQUIP_AURA_VARIANTS) + 1);
            const ArtSheet *sheet = ArtAtlasGet(key);
            if (sheet)
            {
                /* Giro 2, verdetto D6: lo sprite non e' piu' 4-8 pixel isolati
                   ma un anello vero (~12.6px nativi di diametro esterno,
                   scripts/gen_equip_overlays.py, build_ring) -- "piu' massa",
                   si legge come un alone/orbita invece di una spolverata di
                   puntini. L'accento resta il piu' grande del set (blocco
                   5x5 nativi, non 1x1): e' l'unico slot il cui scopo
                   primario e' segnalare colore (strato 3, visual-language.md),
                   riempie il cavo al centro dell'anello. */
                ArtDrawFrame(sheet, 0, 0, p, EQUIP_AURA_SCALE, false, WHITE);
                DrawEquipAccent(p, EQUIP_AURA_SCALE, -3.0f, -3.0f, 5.0f, layer.color);
            }
            else
            {
                DrawCircleV(p, 5, layer.color);
            }
            break;
        }
    }

    if (layer.stackIndex == ITEM_LAYER_MAX_PER_SLOT - 1 && layer.stackTotal > ITEM_LAYER_MAX_PER_SLOT)
    {
        Vector2 badge = OverflowBadgePos(anchors, layer.slot);
        DrawText(TextFormat("+%d", layer.stackTotal - ITEM_LAYER_MAX_PER_SLOT), (int)badge.x - 8, (int)badge.y - 6, 12, RAYWHITE);
    }
}

bool ItemLayerIsBehindBase(ItemSlot slot)
{
    return slot == SLOT_BODY || slot == SLOT_BACK;
}
