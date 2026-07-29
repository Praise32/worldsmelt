#ifndef MELTING_RUN_ART_DRAW_H
#define MELTING_RUN_ART_DRAW_H

#include "assets/art_atlas.h"

/* Come si METTE A SCHERMO il pacchetto artistico di assets/art/ (W8).
 *
 * CONFINE (AGENTS.md): src/assets possiede e carica (art_atlas.h), src/render
 * disegna -- questo file. La divisione non e' formale: art_atlas.c non chiama
 * una sola funzione di disegno di raylib, quindi il suo parser e il suo
 * animatore restano esercitabili dai test senza finestra aperta, mentre tutto
 * cio' che tocca la pipeline grafica sta qui, dove c'e' gia' il resto del
 * rendering.
 *
 * TUTTE le funzioni qui sono NO-OP SICURI quando l'asset manca (sheet NULL,
 * ruolo/animazione/glifo assenti): non disegnano nulla e -- dove chi chiama ha
 * bisogno di saperlo per ripiegare sul percorso precedente -- ritornano false.
 * Nessuna e' mai un crash: un checkout senza assets/art/ deve dare lo stesso
 * gioco di prima di W8.
 *
 * SISTEMA DI COMPONENTI (docs/ai-production/15-UI-DESIGN-PIPELINE.md,
 * "componenti minimi"): i quattro asset di assets/art/ui -- font 5px, atlas
 * icone, cornice a pannello e cornice a slot -- sono l'UNICA veste di tutta
 * l'interfaccia, HUD e schermate. Si raggiungono da qui con
 * ArtUiFont/ArtUiIcons/ArtUiPanel/ArtUiSlot invece di ripetere la loro chiave
 * in ogni punto di disegno. */

/* I quattro componenti di sistema, caricati pigramente al primo uso e tenuti
 * in cache da art_atlas. NULL = quel componente non c'e' (checkout parziale),
 * e chi disegna ricade sulle primitive di sempre. */
const ArtSheet *ArtUiFont(void);
const ArtSheet *ArtUiIcons(void);
const ArtSheet *ArtUiPanel(void);
const ArtSheet *ArtUiSlot(void);

/* Vero quando font e cornici sono disponibili, cioe' quando l'interfaccia puo'
 * essere disegnata nella veste pixel art. Falso = si resta interamente sul
 * percorso a primitive+DrawText di prima di W8, senza mescolare i due stili
 * (mezza schermata in pixel art e mezza a primitive sarebbe peggio di
 * entrambe). */
bool ArtUiReady(void);

/* --- Testo bitmap (font-5px) ---------------------------------------------
 * 'scale' e' un moltiplicatore INTERO (>=1): un font pixel art scalato di
 * 1.5 perde le proporzioni dei tratti da 1 pixel, ed e' esattamente cio' che
 * i mock del layout V3 evitano usando solo scale 1/2/3/5.
 * Il font consegnato ha SOLO maiuscole e un set chiuso di segni (A-Z 0-9 e
 * : / - . [ ] > + ? ! , ' % =): queste funzioni convertono a maiuscolo da
 * sole, e un carattere fuori dal set avanza come uno spazio senza disegnare
 * nulla -- una lettera accentata lascia un buco, mai un glifo sbagliato.
 * GAP DICHIARATO: le accentate italiane non esistono nel font (CP4). */
int ArtTextWidth(const ArtSheet *font, const char *text, int scale);
int ArtTextHeight(const ArtSheet *font, int scale);
void ArtDrawText(const ArtSheet *font, const char *text, int x, int y, int scale, Color tint);
/* Stessa cosa con un contorno nero di 1 pixel scalato: e' la cifra del layout
 * V3 ("niente pannelli: elementi flottanti con contorno"), e serve alla
 * leggibilita' del testo appoggiato direttamente sulla scena. */
void ArtDrawTextOutlined(const ArtSheet *font, const char *text, int x, int y, int scale, Color tint);

/* --- Icone (icons.png, 16x16, una riga per icona) ------------------------
 * 'name' e' il nome dell'animazione nel manifest: ingot, charge, key, flux,
 * heart, heart_temp, heart_empty, heart_half, graft, active. */
bool ArtDrawIcon(const char *name, float x, float y, float scale, Color tint);

/* --- Cornici 9-patch -----------------------------------------------------
 * Gli angoli restano a scala 1:1 (mai stirati: uno spigolo pixel art stirato
 * si vede subito), i bordi si RIPETONO lungo il lato e il centro riempie.
 * 'dst' piu' piccolo della somma dei bordi viene disegnato come cornice sola,
 * senza centro. */
void ArtDraw9Patch(const ArtSheet *sheet, Rectangle dst, Color tint);
/* I due usi canonici: un PANNELLO (cornice con rivetti, per le aree grandi) e
 * uno SLOT (cornice sottile, per le caselle di oggetto/attivo/innesto).
 * Ritornano false se il componente manca: chi chiama disegna il rettangolo di
 * sempre. */
bool ArtDrawPanel(Rectangle dst, Color tint);
bool ArtDrawSlot(Rectangle dst, Color tint);

/* --- Sprite -------------------------------------------------------------- */
/* Disegna il fotogramma 'frame' della riga 'row' appoggiando l'ANCORA dello
 * sprite (manifest, "anchor") sul punto 'anchorPos' -- per i personaggi e i
 * nemici l'ancora e' ai piedi, quindi 'anchorPos' e' il punto in cui l'entita'
 * poggia sul pavimento, lo stesso a cui e' agganciata la sua ombra. 'scale' e'
 * il numero di pixel di destinazione per pixel di sorgente.
 * 'flipX' specchia orizzontalmente (i nemici hanno UNA sola camminata, la si
 * specchia per la direzione opposta: e' quanto prescrive il contratto della
 * pipeline, che pre-specchia solo il personaggio). */
void ArtDrawFrame(const ArtSheet *sheet, int row, int frame, Vector2 anchorPos,
                  float scale, bool flipX, Color tint);
/* Come sopra ma scegliendo il fotogramma dall'animazione: 'elapsed' secondi
 * dall'inizio. false se l'animazione non esiste (chi chiama ripiega). */
bool ArtDrawAnim(const ArtSheet *sheet, const char *animName, float elapsed,
                 Vector2 anchorPos, float scale, bool flipX, Color tint);

/* La scala con cui disegnare uno sprite di larghezza 'frameW' perche' occupi
 * 'wanted' pixel: agganciata a mezzi passi con un minimo di 1, per non
 * ammorbidire la griglia dei pixel (stessa disciplina della scala del canvas
 * in UiComputeLayoutFor, che si aggancia a 1/8). */
float ArtScaleForWidth(int frameW, float wanted);

/* --- Tile --------------------------------------------------------------- */
/* Disegna il ruolo 'role' del tileset dentro 'dst'. Se 'dst' e' piu' piccolo
 * del tile (l'ultima colonna/riga di una stanza, che non e' multipla di 32) il
 * RETTANGOLO SORGENTE viene ritagliato in proporzione invece di comprimere il
 * tile: comprimere avrebbe deformato i pixel proprio sul bordo, dove l'occhio
 * confronta il tile col suo vicino intero.
 * false se il ruolo manca: chi chiama riempie col colore piatto di sempre. */
bool ArtDrawTile(const ArtSheet *sheet, const char *role, Rectangle dst, Color tint);

#endif
