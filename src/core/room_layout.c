#include "core/room_layout.h"

#include <stdio.h>
#include <string.h>

/* Vedi room_layout.h per il principio (le stanze le inventa il modello, il C
   garantisce che siano giocabili) e per la GARANZIA (croce centrale + centro sempre
   liberi, densita' clampata). */

static const char *ROOM_FORM_TEXT[ROOM_LAYOUT_COUNT] = {
    "open", "pillars", "corridor", "arena", "scatter"
};

RoomForm RoomFormFromText(const char *text)
{
    if (!text) return ROOM_LAYOUT_OPEN;
    for (int i = 0; i < (int)ROOM_LAYOUT_COUNT; i++)
    {
        if (strcmp(ROOM_FORM_TEXT[i], text) == 0) return (RoomForm)i;
    }
    return ROOM_LAYOUT_OPEN;
}

const char *RoomFormName(RoomForm form)
{
    if (form < 0 || form >= ROOM_LAYOUT_COUNT) return ROOM_FORM_TEXT[0];
    return ROOM_FORM_TEXT[form];
}

static float ClampF(float v, float min, float max)
{
    if (!(v > min)) return min;   /* NaN-safe */
    if (v > max) return max;
    return v;
}

void RoomLayoutClamp(RoomLayoutDef *def)
{
    if (!def) return;
    def->density = ClampF(def->density, ROOM_LAYOUT_DENSITY_MIN, ROOM_LAYOUT_DENSITY_MAX);
    if (def->form < 0 || def->form >= ROOM_LAYOUT_COUNT) def->form = ROOM_LAYOUT_OPEN;
}

/* Semiampiezza della CROCE centrale sempre libera: la fascia verticale e quella
   orizzontale che passano per il centro restano vuote, cosi' ogni porta (in mezzo a
   ciascuna parete) e' raggiungibile dal centro. 90 px coprono la mezza-porta
   (DOOR_HALF=50 nel gioco) con margine per il giocatore (raggio 14). Costante di
   questo modulo, non del gioco: e' cio' che rende un layout giocabile PER
   COSTRUZIONE, quindi vive qui. */
#define ROOM_CROSS_HALF 90.0f
/* Raggio del cerchio libero al centro: il giocatore ci nasce, non deve mai stare
   dentro un muro. */
#define ROOM_CENTER_CLEAR 74.0f

/* Un blocco proposto e' ammesso solo se sta INTERAMENTE in un quadrante, cioe' non
   invade la croce centrale ne' il cerchio centrale. E' la rete di sicurezza: gli
   archetipi sotto sono gia' disegnati per rispettarla, ma un blocco che la violasse
   (per un cambio futuro, o un arrotondamento) viene semplicemente SCARTATO invece
   di rompere la giocabilita'. Meglio una stanza con un ostacolo in meno che una con
   una porta murata. */
static bool BlockInQuadrant(float bx, float by, float bw, float bh, float cx, float cy)
{
    /* Interseca la fascia verticale della croce? */
    if (bx < cx + ROOM_CROSS_HALF && bx + bw > cx - ROOM_CROSS_HALF) return false;
    /* La fascia orizzontale? */
    if (by < cy + ROOM_CROSS_HALF && by + bh > cy - ROOM_CROSS_HALF) return false;
    return true;
}

static int AddBlock(Obstacle *out, int count, int maxOut, float cx, float cy,
                    float bx, float by, float bw, float bh)
{
    if (count >= maxOut) return count;
    if (bw < 8.0f || bh < 8.0f) return count;
    if (!BlockInQuadrant(bx, by, bw, bh, cx, cy)) return count;   /* rete di sicurezza: mai sulla croce/centro */
    out[count].x = bx; out[count].y = by; out[count].w = bw; out[count].h = bh;
    return count + 1;
}

/* RNG minuscola locale (LCG), per non dipendere da game_math.c: 'seed' varia solo i
   dettagli fini a parita' di forma. */
static float NextF(unsigned int *s, float lo, float hi)
{
    *s = *s*1664525u + 1013904223u;
    float t = (float)((*s >> 8) & 0xFFFFFF)/(float)0x1000000;   /* 0..1 */
    return lo + t*(hi - lo);
}

int RoomLayoutBuild(const RoomLayoutDef *def, unsigned int seed,
                    float x, float y, float w, float h,
                    Obstacle *out, int maxOut)
{
    if (!def || !out || maxOut <= 0 || !def->active || def->form == ROOM_LAYOUT_OPEN) return 0;

    RoomLayoutDef d = *def;
    RoomLayoutClamp(&d);

    float cx = x + w*0.5f;
    float cy = y + h*0.5f;
    unsigned int rng = seed ? seed : 1u;
    int count = 0;

    /* La "cella" di un quadrante: lo spazio fra il bordo della stanza e la croce
       centrale. Gli ostacoli si dimensionano come frazione di questa cella, scalata
       dalla densita'. */
    float qw = (cx - ROOM_CROSS_HALF) - (x + 24.0f);   /* larghezza utile di un quadrante */
    float qh = (cy - ROOM_CROSS_HALF) - (y + 24.0f);
    if (qw < 20.0f || qh < 20.0f) return 0;

    /* I centri dei quattro quadranti (dove appoggiare un blocco simmetrico). */
    float qx[2] = { x + 24.0f + qw*0.5f, x + w - 24.0f - qw*0.5f };
    float qy[2] = { y + 24.0f + qh*0.5f, y + h - 24.0f - qh*0.5f };

    switch (d.form)
    {
        case ROOM_LAYOUT_PILLARS:
        {
            /* Una colonna al centro di ciascun quadrante, quadrata, dimensionata
               dalla densita'. */
            float side = ClampF(qw*(0.32f + 0.30f*d.density), 26.0f, qw*0.7f);
            side = ClampF(side, 26.0f, qh*0.7f);
            for (int i = 0; i < 2; i++)
                for (int j = 0; j < 2; j++)
                {
                    float jitter = NextF(&rng, -12.0f, 12.0f);
                    count = AddBlock(out, count, maxOut, cx, cy,
                                     qx[i] - side*0.5f + jitter, qy[j] - side*0.5f, side, side);
                }
            break;
        }
        case ROOM_LAYOUT_CORRIDOR:
        {
            /* Due blocchi larghi, uno in alto e uno in basso, che lasciano un
               corridoio orizzontale al centro. Occupano quasi tutta la larghezza dei
               quadranti superiori/inferiori. */
            float bw = w*0.5f - ROOM_CROSS_HALF - 40.0f;
            float bh = ClampF(qh*(0.45f + 0.4f*d.density), 30.0f, qh*0.85f);
            for (int i = 0; i < 2; i++)
                for (int j = 0; j < 2; j++)
                {
                    float bx = (i == 0) ? (x + 30.0f) : (cx + ROOM_CROSS_HALF + 10.0f);
                    float by = (j == 0) ? (cy - ROOM_CROSS_HALF - bh) : (cy + ROOM_CROSS_HALF);
                    count = AddBlock(out, count, maxOut, cx, cy, bx, by, bw, bh);
                }
            break;
        }
        case ROOM_LAYOUT_ARENA:
        {
            /* Un blocco in ciascun ANGOLO della stanza (nei quadranti, contro il
               bordo esterno): il centro resta un'arena aperta. */
            float bw = ClampF(qw*(0.40f + 0.35f*d.density), 30.0f, qw*0.85f);
            float bh = ClampF(qh*(0.40f + 0.35f*d.density), 30.0f, qh*0.85f);
            float corners[4][2] = {
                { x + 26.0f,            y + 26.0f },
                { x + w - 26.0f - bw,   y + 26.0f },
                { x + 26.0f,            y + h - 26.0f - bh },
                { x + w - 26.0f - bw,   y + h - 26.0f - bh },
            };
            for (int k = 0; k < 4; k++)
                count = AddBlock(out, count, maxOut, cx, cy, corners[k][0], corners[k][1], bw, bh);
            break;
        }
        case ROOM_LAYOUT_SCATTER:
        {
            /* Blocchetti piccoli sparsi, uno per quadrante a giro. Il numero scala
               con la densita'. Per ogni blocco si sceglie un quadrante (i,j) e si
               piazza il blocco DENTRO i limiti di quel quadrante -- niente
               specchiamenti a mano: i limiti del quadrante li conosciamo gia'.
               BlockInQuadrant scarta comunque cio' che sfiorasse la croce. */
            int n = 4 + (int)(d.density*6.0f);
            if (n > maxOut) n = maxOut;
            for (int k = 0; k < n; k++)
            {
                int i = (k & 1);           /* quadrante sinistro (0) o destro (1) */
                int j = (k >> 1) & 1;      /* alto (0) o basso (1) */
                float bs = NextF(&rng, 22.0f, 22.0f + 26.0f*d.density);
                /* Limiti del quadrante scelto (fra il bordo esterno e la croce). */
                float loX = (i == 0) ? (x + 26.0f) : (cx + ROOM_CROSS_HALF + 8.0f);
                float hiX = (i == 0) ? (cx - ROOM_CROSS_HALF - 8.0f - bs) : (x + w - 26.0f - bs);
                float loY = (j == 0) ? (y + 26.0f) : (cy + ROOM_CROSS_HALF + 8.0f);
                float hiY = (j == 0) ? (cy - ROOM_CROSS_HALF - 8.0f - bs) : (y + h - 26.0f - bs);
                if (hiX <= loX || hiY <= loY) continue;   /* quadrante troppo stretto per questo blocco */
                float bx = NextF(&rng, loX, hiX);
                float by = NextF(&rng, loY, hiY);
                count = AddBlock(out, count, maxOut, cx, cy, bx, by, bs, bs);
            }
            break;
        }
        default:
            break;
    }

    return count;
}

void RoomLayoutExample(RoomLayoutDef *out, int index)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->active = true;

    /* Quattro esempi che coprono le quattro forme non vuote. Nomi generici: nel
       gioco il modello li rimpiazza con nomi a tema. */
    switch (((index % ROOM_LAYOUT_EXAMPLE_COUNT) + ROOM_LAYOUT_EXAMPLE_COUNT) % ROOM_LAYOUT_EXAMPLE_COUNT)
    {
        case 1:
            snprintf(out->name, sizeof(out->name), "Corridoio");
            out->form = ROOM_LAYOUT_CORRIDOR; out->density = 0.6f;
            break;
        case 2:
            snprintf(out->name, sizeof(out->name), "Arena");
            out->form = ROOM_LAYOUT_ARENA; out->density = 0.55f;
            break;
        case 3:
            snprintf(out->name, sizeof(out->name), "Detriti");
            out->form = ROOM_LAYOUT_SCATTER; out->density = 0.5f;
            break;
        default:
            snprintf(out->name, sizeof(out->name), "Colonne");
            out->form = ROOM_LAYOUT_PILLARS; out->density = 0.5f;
            break;
    }
    RoomLayoutClamp(out);
}
