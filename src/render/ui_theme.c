#include "render/ui_theme.h"

#include "render/art_draw.h"

/* Vedi ui_theme.h per il principio (una sola tavolozza, pannelli tonali, due
   taglie di testo). Qui c'e' solo l'attuazione. */

const Color UI_GROUND     = { 20, 16, 14, 255 };    /* slag-nero */
const Color UI_PANEL      = { 36, 26, 22, 255 };    /* slag-scuro */
const Color UI_BEVEL_LUCE = { 58, 38, 32, 255 };    /* slag-caldo */
const Color UI_TITOLO     = { 232, 183, 74, 255 };  /* oro-fuso */
const Color UI_TESTO      = { 244, 242, 236, 255 }; /* bianco-caldo */
const Color UI_SECONDARIO = { 167, 167, 181, 255 }; /* cenere-chiara */
const Color UI_MUTO       = { 74, 74, 85, 255 };    /* cenere-scura */
const Color UI_FOCUS      = { 224, 91, 35, 255 };   /* fiamma */
const Color UI_GLINT      = { 255, 196, 107, 255 }; /* bagliore */
const Color UI_DIVISORE   = { 122, 74, 43, 255 };   /* bronzo-scuro */

static int UiTagliaClamp(int taglia)
{
    if (taglia < UI_TAGLIA_1) return UI_TAGLIA_1;
    if (taglia > UI_TAGLIA_2) return UI_TAGLIA_2;
    return taglia;
}

/* Corpo del ripiego vettoriale: 5 px di glifo per taglia, arrotondati verso
   l'alto perche' DrawText a corpo 5 e' illeggibile. Non deve combaciare al
   pixel col font bitmap -- e' il percorso "assets/art/ mancante", dove conta
   solo che la schermata resti leggibile. */
static int UiFallbackSize(int taglia)
{
    return taglia*8;
}

void UiTextAt(const char *text, int x, int y, int taglia, Color tint)
{
    if (!text) return;
    taglia = UiTagliaClamp(taglia);
    const ArtSheet *font = ArtUiFont();
    if (font) ArtDrawText(font, text, x, y, taglia, tint);
    else DrawText(text, x, y, UiFallbackSize(taglia), tint);
}

int UiTextWidth(const char *text, int taglia)
{
    if (!text) return 0;
    taglia = UiTagliaClamp(taglia);
    const ArtSheet *font = ArtUiFont();
    if (font) return ArtTextWidth(font, text, taglia);
    return MeasureText(text, UiFallbackSize(taglia));
}

int UiTextHeight(int taglia)
{
    taglia = UiTagliaClamp(taglia);
    const ArtSheet *font = ArtUiFont();
    if (font) return ArtTextHeight(font, taglia);
    return UiFallbackSize(taglia);
}

void UiPanel(Rectangle box)
{
    if (box.width < 2.0f || box.height < 2.0f) return;
    int x = (int)box.x, y = (int)box.y;
    int w = (int)box.width, h = (int)box.height;
    DrawRectangle(x, y, w, h, UI_PANEL);
    /* Il bevel e' spesso UN pixel di canvas, non uno "scalato": a 640x360 il
       blit finale lo moltiplica gia' per la scala intera della finestra, e un
       bevel di 2 px qui diventerebbe una cornice spessa a 1080p. */
    DrawRectangle(x, y, w, 1, UI_BEVEL_LUCE);              /* luce: riga alta */
    DrawRectangle(x, y, 1, h, UI_BEVEL_LUCE);              /* luce: colonna sinistra */
    DrawRectangle(x, y + h - 1, w, 1, UI_GROUND);          /* ombra: riga bassa */
    DrawRectangle(x + w - 1, y, 1, h, UI_GROUND);          /* ombra: colonna destra */
}

void UiDivider(int x, int y, int w)
{
    if (w <= 0) return;
    DrawRectangle(x, y, w, 1, UI_DIVISORE);
}

void UiMenuRow(Rectangle row, const char *label, bool focused)
{
    if (!label) return;
    int textH = UiTextHeight(UI_TAGLIA_2);
    /* Il testo si centra verticalmente nella riga: la fascia del fuoco e' piu'
       alta dei glifi apposta (una riga stretta come il testo sembrerebbe una
       barra di evidenziazione, non una voce selezionata). */
    int textY = (int)(row.y + (row.height - (float)textH)*0.5f);
    int textX = (int)row.x + 10;
    if (focused)
    {
        DrawRectangleRec(row, UI_BEVEL_LUCE);
        DrawRectangle((int)row.x, (int)row.y, 4, (int)row.height, UI_FOCUS);
        UiTextAt(label, textX, textY, UI_TAGLIA_2, UI_TITOLO);
        /* La freccia sta DENTRO la riga, appoggiata al margine destro: e' il
           terzo segnale del fuoco, quello che sopravvive anche a chi non
           distingue oro da bianco (DEC-058). Disegnata a colonne di pixel e
           non col font: il set di glifi consegnato (assets/art/ui/font-5px)
           contiene '>' ma NON '<', e un carattere assente avanzerebbe come
           uno spazio -- cioe' nessuna freccia, in silenzio. Quattro colonne
           alte 1, 3, 5, 7: un triangolo pixel-art che punta alla voce. */
        int tipX = (int)(row.x + row.width) - 10 - 4;
        int midY = textY + UiTextHeight(UI_TAGLIA_2)/2;
        for (int i = 0; i < 4; i++) DrawRectangle(tipX + i, midY - i, 1, 1 + 2*i, UI_FOCUS);
        return;
    }
    UiTextAt(label, textX, textY, UI_TAGLIA_2, UI_TESTO);
}

Color UiRarityTint(Rarity rarity)
{
    switch (rarity)
    {
        case RARITY_UNCOMMON: return (Color){ 58, 125, 99, 255 };    /* verderame */
        case RARITY_RARE: return (Color){ 106, 134, 160, 255 };      /* ardesia-chiara */
        case RARITY_LEGENDARY: return UI_TITOLO;                     /* oro-fuso */
        case RARITY_COMMON:
        default: return (Color){ 115, 115, 130, 255 };               /* cenere */
    }
}
