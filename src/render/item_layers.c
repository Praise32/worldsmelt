#include "render/item_layers.h"

#include "core/game_math.h"

#include "raylib.h"

#include <math.h>

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

void DrawItemLayer(PlayerAnchors anchors, ItemLayer layer)
{
    /* Interruttore sullo slot: stessa forma bespoke che DrawEquipment aveva
       prima di questo refactor, solo parametrizzata su anchors/stackIndex
       invece che su p->pos e su contatori locali al ciclo. Quando arriveranno
       gli sprite generati, e' QUESTA l'unica funzione da cambiare (vedi il
       commento su ItemLayer in item_layers.h). */
    switch (layer.slot)
    {
        case SLOT_HAT:
        {
            float y = anchors.hat.y - (float)layer.stackIndex*8.0f;
            DrawRectangleRounded((Rectangle){ anchors.hat.x - 14, y, 28, 8 }, 0.3f, 5, layer.color);
            DrawRectangleRounded((Rectangle){ anchors.hat.x - 9, y - 9, 18, 12 }, 0.35f, 5, layer.color);
            break;
        }
        case SLOT_EYES:
        {
            /* Occhiali sopra occhiali (Isaac docet): ogni paio in piu' si
               impila leggermente piu' in alto, come i cappelli ma con un
               passo piu' piccolo (sono piu' vicini alla testa). */
            float y = anchors.eyes.y - (float)layer.stackIndex*5.0f;
            DrawCircleV((Vector2){ anchors.eyes.x - 6, y }, 4.5f, layer.color);
            DrawCircleV((Vector2){ anchors.eyes.x + 6, y }, 4.5f, layer.color);
            DrawLineEx((Vector2){ anchors.eyes.x - 2, y }, (Vector2){ anchors.eyes.x + 2, y }, 2.0f, layer.color);
            break;
        }
        case SLOT_HAND:
        {
            /* Alterna lato sinistro/destro; ogni coppia successiva (stackIndex
               2 e 3, 4 e 5...) si allunga un po' cosi' le armi/oggetti non si
               sovrappongono esattamente l'uno sull'altro. */
            float side = (layer.stackIndex%2 == 0) ? 1.0f : -1.0f;
            float reach = 19.0f + (float)(layer.stackIndex/2)*6.0f;
            DrawLineEx((Vector2){ anchors.hand.x + side*13.0f, anchors.hand.y },
                       (Vector2){ anchors.hand.x + side*(13.0f + reach), anchors.hand.y - 12.0f }, 5.0f, layer.color);
            break;
        }
        case SLOT_BACK:
        {
            /* Mantelli sovrapposti: ciascuno in piu' e' leggermente piu'
               largo e piu' trasparente di quello sotto, cosi' si legge come
               "strati" invece che coprire per intero quello precedente. */
            int alpha = GameMathClampInt(140 - layer.stackIndex*35, 40, 140);
            float spread = 22.0f + (float)layer.stackIndex*3.0f;
            DrawTriangle((Vector2){ anchors.backTip.x, anchors.backTip.y },
                         (Vector2){ anchors.backTip.x - spread, anchors.backHem.y },
                         (Vector2){ anchors.backTip.x + spread, anchors.backHem.y },
                         GameColorWithAlpha(layer.color, (unsigned char)alpha));
            break;
        }
        case SLOT_BODY:
        {
            /* Cerchi concentrici via via piu' piccoli: il primo oggetto
               resta il piu' grande e visibile, i successivi si vedono come
               un accenno di profondita' invece di sparire dietro al primo. */
            float r = GameMathClampFloat(6.0f - (float)layer.stackIndex*1.2f, 2.0f, 6.0f);
            DrawCircleV(anchors.body, r, layer.color);
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
            DrawCircleV(p, 5, layer.color);
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
