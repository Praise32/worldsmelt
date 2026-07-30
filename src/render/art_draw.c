#include "render/art_draw.h"

#include <math.h>
#include <string.h>

/* Le chiavi dei quattro componenti di sistema stanno QUI e in nessun altro
   punto del renderer: se domani un componente cambia nome di file, cambia una
   riga. */
#define ART_UI_FONT_KEY  "ui/font-5px"
#define ART_UI_ICONS_KEY "ui/icons"
#define ART_UI_PANEL_KEY "ui/panel-9patch"
#define ART_UI_SLOT_KEY  "ui/slot-9patch"

const ArtSheet *ArtUiFont(void)  { return ArtAtlasGet(ART_UI_FONT_KEY); }
const ArtSheet *ArtUiIcons(void) { return ArtAtlasGet(ART_UI_ICONS_KEY); }
const ArtSheet *ArtUiPanel(void) { return ArtAtlasGet(ART_UI_PANEL_KEY); }
const ArtSheet *ArtUiSlot(void)  { return ArtAtlasGet(ART_UI_SLOT_KEY); }

bool ArtUiReady(void)
{
    /* Il font e le DUE cornici insieme: sono il minimo per vestire una
       schermata intera. Le icone no -- un'icona mancante lascia un buco in un
       cluster, il font mancante rende illeggibile tutto. */
    return ArtUiFont() != NULL && ArtUiPanel() != NULL && ArtUiSlot() != NULL;
}

/* ============================================================
   Testo bitmap
   ============================================================ */

static char ArtUpper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

/* Decodifica UN codepoint UTF-8 a partire da 'p'. Scrive in '*consumed' quanti
   byte ha letto (1 o 2 -- font-5px copre solo il Latin-1 Supplement,
   U+0080-U+07FF, che basta per l'italiano accentato: nessuna sequenza a 3/4
   byte prevista). Una sequenza troncata o un byte non valido: codepoint = il
   byte cosi' com'e', consumed = 1 -- il cursore avanza SEMPRE (stessa
   disciplina di SkipValue in assets/art_atlas.c), mai un ciclo a vuoto su
   testo corrotto. */
static int ArtUtf8Decode(const char *p, int *consumed)
{
    unsigned char c0 = (unsigned char)p[0];
    if (c0 < 0x80) { *consumed = 1; return c0; }
    if ((c0 & 0xE0) == 0xC0 && p[1] != '\0')
    {
        unsigned char c1 = (unsigned char)p[1];
        if ((c1 & 0xC0) == 0x80) { *consumed = 2; return ((c0 & 0x1F) << 6) | (c1 & 0x3F); }
    }
    *consumed = 1;
    return c0;
}

/* Equivalente di ArtUpper per le sei accentate che font-5px conosce oggi
   (vedi assets/art/ui/font-5px.json, chiave "glyphs_ext"): fold minuscola
   accentata -> maiuscola accentata. Qualunque altro codepoint >=128 passa
   invariato -- resta "fuori dal set esteso", stesso trattamento di una
   lettera ASCII senza glifo (ArtResolveGlyph sotto lo fa avanzare come uno
   spazio). Aggiungere un'accentata nuova domani significa aggiungere UNA riga
   qui e la sua entry in glyphs_ext, non toccare il resto della pipeline. */
static int ArtUpperCodepoint(int cp)
{
    switch (cp)
    {
        case 0xE0: return 0xC0;   /* a grave -> A grave */
        case 0xE8: return 0xC8;   /* e grave -> E grave */
        case 0xE9: return 0xC9;   /* e acuto -> E acuto */
        case 0xEC: return 0xCC;   /* i grave -> I grave */
        case 0xF2: return 0xD2;   /* o grave -> O grave */
        case 0xF9: return 0xD9;   /* u grave -> U grave */
        default:   return cp;
    }
}

/* Risolve UN "carattere logico" (un codepoint ASCII o un'accentata estesa) a
   partire da 'p'. Scrive in '*consumed' quanti byte ha letto (SEMPRE >=1: chi
   chiama avanza il cursore anche quando questa funzione ritorna false). Se il
   carattere si disegna (un glifo -- ASCII o esteso -- e' stato trovato),
   scrive la sua geometria in '*outX'/'*outW' e ritorna true. Altrimenti (uno
   spazio esplicito, o un carattere fuori dal set anche esteso) ritorna false:
   chi chiama avanza di spaceW+letterSpacing SENZA disegnare nulla -- ESATTAMENTE
   il comportamento di sempre per un glifo assente, ora esteso anche ai
   codepoint multi-byte (font-integration-notes, WP-INT). Fattorizzato UNA
   volta sola perche' ArtTextWidth e ArtDrawText devono restare identiche nel
   percorso di risoluzione: prima di questo helper duplicavano lo stesso
   ramo ASCII in due posti, un rischio di divergenza silenziosa. */
static bool ArtResolveGlyph(const ArtSheet *font, const char *p, int *consumed, int *outX, int *outW)
{
    if (*p == ' ') { *consumed = 1; return false; }
    int cp = ArtUtf8Decode(p, consumed);
    if (cp < 128)
    {
        const ArtGlyph *glyph = ArtSheetGlyph(font, ArtUpper((char)cp));
        if (!glyph) return false;
        *outX = glyph->x; *outW = glyph->w;
        return true;
    }
    const ArtGlyphExt *glyph = ArtSheetGlyphExt(font, ArtUpperCodepoint(cp));
    if (!glyph) return false;
    *outX = glyph->x; *outW = glyph->w;
    return true;
}

int ArtTextHeight(const ArtSheet *font, int scale)
{
    if (!font || scale < 1) return 0;
    return font->glyphH*scale;
}

int ArtTextWidth(const ArtSheet *font, const char *text, int scale)
{
    if (!font || !text || scale < 1) return 0;
    int width = 0;
    for (const char *p = text; *p; )
    {
        int consumed, gx, gw;
        /* Glifo assente (o spazio esplicito) = spazio: la parola resta
           spaziata come se il carattere ci fosse, cosi' un'accentata fuori
           dal set non fa collassare il resto della riga su se stesso. */
        if (ArtResolveGlyph(font, p, &consumed, &gx, &gw)) width += (gw + font->letterSpacing)*scale;
        else width += (font->spaceW + font->letterSpacing)*scale;
        p += consumed;
    }
    /* L'ultimo carattere non porta la spaziatura di coda: senza questo, una
       stringa allineata a destra risulterebbe spostata di uno spazio. */
    if (width > 0) width -= font->letterSpacing*scale;
    return width;
}

void ArtDrawText(const ArtSheet *font, const char *text, int x, int y, int scale, Color tint)
{
    if (!font || !text || scale < 1) return;
    float cursor = (float)x;
    for (const char *p = text; *p; )
    {
        int consumed, gx, gw;
        if (ArtResolveGlyph(font, p, &consumed, &gx, &gw))
        {
            /* baseline_y: la striscia del font ha una riga di guardia sopra i
               glifi (il PNG e' alto glyph_h+2), quindi il glifo comincia a
               baseline_y e non a 0 -- stessa geometria per un glifo ASCII o
               esteso, vedi il commento su ArtGlyphExt in assets/art_atlas.h. */
            Rectangle src = { (float)gx, (float)font->baselineY, (float)gw, (float)font->glyphH };
            Rectangle dst = { cursor, (float)y, (float)(gw*scale), (float)(font->glyphH*scale) };
            DrawTexturePro(font->texture, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
            cursor += (float)((gw + font->letterSpacing)*scale);
        }
        else cursor += (float)((font->spaceW + font->letterSpacing)*scale);
        p += consumed;
    }
}

void ArtDrawTextOutlined(const ArtSheet *font, const char *text, int x, int y, int scale, Color tint)
{
    if (!font || !text || scale < 1) return;
    /* Contorno su quattro direzioni e non otto: a scala 1-3 le diagonali
       ispessiscono il glifo fino a chiuderne i contro-forme (l'occhiello della
       'O' a scala 1 sparisce). Nero pieno: il contorno serve al contrasto
       contro la scena, non a colorare. */
    const Color outline = (Color){ 8, 8, 12, 220 };
    ArtDrawText(font, text, x - scale, y, scale, outline);
    ArtDrawText(font, text, x + scale, y, scale, outline);
    ArtDrawText(font, text, x, y - scale, scale, outline);
    ArtDrawText(font, text, x, y + scale, scale, outline);
    ArtDrawText(font, text, x, y, scale, tint);
}

/* ============================================================
   Icone
   ============================================================ */

bool ArtDrawIcon(const char *name, float x, float y, float scale, Color tint)
{
    const ArtSheet *icons = ArtUiIcons();
    if (!icons || !name) return false;
    const ArtAnim *anim = ArtSheetAnim(icons, name);
    if (!anim) return false;
    Rectangle src = ArtSheetFrameRect(icons, anim->row, 0);
    if (src.width <= 0.0f) return false;
    Rectangle dst = { x, y, src.width*scale, src.height*scale };
    DrawTexturePro(icons->texture, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    return true;
}

/* ============================================================
   Cornici 9-patch
   ============================================================ */

/* Un pezzo della cornice, ripetuto (non stirato) per coprire 'dst'. Il
   ritaglio dell'ultima ripetizione avviene sul rettangolo SORGENTE, cosi' i
   pixel restano quadrati anche quando il lato non e' multiplo del pezzo. */
static void DrawRepeated(Texture2D texture, Rectangle src, Rectangle dst, Color tint)
{
    if (src.width <= 0.0f || src.height <= 0.0f || dst.width <= 0.0f || dst.height <= 0.0f) return;
    for (float oy = 0.0f; oy < dst.height; oy += src.height)
    {
        float h = fminf(src.height, dst.height - oy);
        for (float ox = 0.0f; ox < dst.width; ox += src.width)
        {
            float w = fminf(src.width, dst.width - ox);
            Rectangle s = { src.x, src.y, w, h };
            Rectangle d = { dst.x + ox, dst.y + oy, w, h };
            DrawTexturePro(texture, s, d, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
        }
    }
}

void ArtDraw9Patch(const ArtSheet *sheet, Rectangle dst, Color tint)
{
    if (!sheet || sheet->frameW <= 0 || sheet->frameH <= 0) return;
    float l = (float)sheet->sliceL, t = (float)sheet->sliceT;
    float r = (float)sheet->sliceR, b = (float)sheet->sliceB;
    if (l <= 0.0f || t <= 0.0f || r <= 0.0f || b <= 0.0f) return;
    /* Un rettangolo piu' piccolo della somma dei bordi non ha centro ne'
       lati: si stringono i bordi in proporzione, cosi' una casella minuscola
       resta una cornice e non un miscuglio di angoli sovrapposti. */
    if (dst.width < l + r) { float k = dst.width/(l + r); l *= k; r *= k; }
    if (dst.height < t + b) { float k = dst.height/(t + b); t *= k; b *= k; }

    float sw = (float)sheet->frameW, sh = (float)sheet->frameH;
    float midSW = sw - (float)sheet->sliceL - (float)sheet->sliceR;
    float midSH = sh - (float)sheet->sliceT - (float)sheet->sliceB;
    float midDW = dst.width - l - r;
    float midDH = dst.height - t - b;
    Texture2D tex = sheet->texture;

    /* Angoli, mai scalati. */
    DrawTexturePro(tex, (Rectangle){ 0.0f, 0.0f, l, t }, (Rectangle){ dst.x, dst.y, l, t }, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    DrawTexturePro(tex, (Rectangle){ sw - r, 0.0f, r, t }, (Rectangle){ dst.x + dst.width - r, dst.y, r, t }, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    DrawTexturePro(tex, (Rectangle){ 0.0f, sh - b, l, b }, (Rectangle){ dst.x, dst.y + dst.height - b, l, b }, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    DrawTexturePro(tex, (Rectangle){ sw - r, sh - b, r, b }, (Rectangle){ dst.x + dst.width - r, dst.y + dst.height - b, r, b }, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);

    if (midSW > 0.0f && midDW > 0.0f)
    {
        DrawRepeated(tex, (Rectangle){ l, 0.0f, midSW, t }, (Rectangle){ dst.x + l, dst.y, midDW, t }, tint);
        DrawRepeated(tex, (Rectangle){ l, sh - b, midSW, b }, (Rectangle){ dst.x + l, dst.y + dst.height - b, midDW, b }, tint);
    }
    if (midSH > 0.0f && midDH > 0.0f)
    {
        DrawRepeated(tex, (Rectangle){ 0.0f, t, l, midSH }, (Rectangle){ dst.x, dst.y + t, l, midDH }, tint);
        DrawRepeated(tex, (Rectangle){ sw - r, t, r, midSH }, (Rectangle){ dst.x + dst.width - r, dst.y + t, r, midDH }, tint);
    }
    if (midSW > 0.0f && midSH > 0.0f && midDW > 0.0f && midDH > 0.0f)
    {
        Rectangle centerDst = { dst.x + l, dst.y + t, midDW, midDH };
        /* Centro a colore unico (il caso dei due componenti consegnati, misurato
           al caricamento): UN rettangolo invece di migliaia di quad ripetuti --
           vedi il commento su ArtSheet.sliceCenterUniform. Il risultato e'
           identico pixel per pixel, non un'approssimazione. La tinta si
           moltiplica a mano perche' qui non passa da DrawTexturePro. */
        if (sheet->sliceCenterUniform)
        {
            Color c = sheet->sliceCenterColor;
            Color mixed = { (unsigned char)((int)c.r*tint.r/255), (unsigned char)((int)c.g*tint.g/255),
                            (unsigned char)((int)c.b*tint.b/255), (unsigned char)((int)c.a*tint.a/255) };
            DrawRectangleRec(centerDst, mixed);
        }
        else DrawRepeated(tex, (Rectangle){ l, t, midSW, midSH }, centerDst, tint);
    }
}

bool ArtDrawPanel(Rectangle dst, Color tint)
{
    const ArtSheet *panel = ArtUiPanel();
    if (!panel) return false;
    ArtDraw9Patch(panel, dst, tint);
    return true;
}

bool ArtDrawSlot(Rectangle dst, Color tint)
{
    const ArtSheet *slot = ArtUiSlot();
    if (!slot) return false;
    ArtDraw9Patch(slot, dst, tint);
    return true;
}

/* ============================================================
   Sprite
   ============================================================ */

float ArtScaleForWidth(int frameW, float wanted)
{
    if (frameW <= 0 || wanted <= 0.0f) return 1.0f;
    float raw = wanted/(float)frameW;
    float snapped = floorf(raw*2.0f + 0.5f)/2.0f;
    return snapped < 1.0f ? 1.0f : snapped;
}

void ArtDrawFrame(const ArtSheet *sheet, int row, int frame, Vector2 anchorPos,
                  float scale, bool flipX, Color tint)
{
    if (!sheet || scale <= 0.0f) return;
    Rectangle src = ArtSheetFrameRect(sheet, row, frame);
    if (src.width <= 0.0f) return;
    float w = src.width*scale;
    float h = src.height*scale;
    /* L'ancora specchiata si misura dall'ALTRO bordo: senza questo, un nemico
       che gira verso sinistra "salta" orizzontalmente di (frameW - 2*anchorX)
       pixel ad ogni cambio di direzione. */
    float ax = flipX ? (float)(sheet->frameW - sheet->anchorX) : (float)sheet->anchorX;
    Rectangle dst = { anchorPos.x - ax*scale, anchorPos.y - (float)sheet->anchorY*scale, w, h };
    if (flipX) src.width = -src.width;
    DrawTexturePro(sheet->texture, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

bool ArtDrawAnim(const ArtSheet *sheet, const char *animName, float elapsed,
                 Vector2 anchorPos, float scale, bool flipX, Color tint)
{
    if (!sheet) return false;
    const ArtAnim *anim = ArtSheetAnim(sheet, animName);
    if (!anim) return false;
    ArtDrawFrame(sheet, anim->row, ArtAnimFrameAt(anim, elapsed), anchorPos, scale, flipX, tint);
    return true;
}

/* ============================================================
   Tile
   ============================================================ */

bool ArtDrawTile(const ArtSheet *sheet, const char *role, Rectangle dst, Color tint)
{
    Rectangle src;
    if (!ArtSheetTileRect(sheet, role, &src)) return false;
    if (dst.width <= 0.0f || dst.height <= 0.0f) return false;
    /* Ritaglio proporzionale del sorgente per le celle di bordo (vedi il
       commento in art_draw.h): il tile mostra la sua parte iniziale, i pixel
       restano quadrati. */
    if (dst.width < src.width) src.width = dst.width;
    if (dst.height < src.height) src.height = dst.height;
    DrawTexturePro(sheet->texture, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    return true;
}
