#ifndef MELTING_RUN_UI_THEME_H
#define MELTING_RUN_UI_THEME_H

#include "core/game_types.h"

/* I TOKEN dell'interfaccia (WP-UI-0, rifacimento UI del 02/08).
 *
 * PERCHE' UN MODULO A PARTE. Fino a qui ogni schermata sceglieva i propri
 * colori sul posto: il renderer contiene decine di letterali RGBA (il
 * "(14,16,22,240)" del fondo pannello, il "(205,210,220)" del testo di una
 * riga, gli accenti generati per la run usati anche come colore di CORNICE).
 * Il risultato erano tre tavolozze convissute nello stesso frame -- neon
 * rosa/ciano di provenienza procedurale, grigi bluastri di raygui, e la
 * palette Fucina delle immagini -- e nessun punto in cui cambiarne una.
 * Qui c'e' quel punto: i colori dell'INTERFACCIA vengono TUTTI da
 * assets/art-src/palette/worldsmelt-fucina.gpl (DEC-173), mai dal tema
 * generato della run (che resta il colore del MONDO: pavimenti, nemici,
 * colpi).
 *
 * DEC-205 (niente outline, niente cornici): un pannello non e' una cornice
 * colorata da 1 px come lo erano gli overlay fino a WP-UI-0. E' una massa
 * tonale -- riempimento piatto piu' un bevel di due valori (luce in alto e a
 * sinistra, ombra in basso e a destra) -- cioe' la stessa regola con cui
 * l'arte del gioco definisce i bordi per contrasto di valore invece che con
 * una linea. Un solo componente (UiPanel) perche' nessuna schermata possa
 * reinventarne una variante.
 *
 * DUE TAGLIE, NON UNA SCALA CONTINUA. Il font e' bitmap da 5 px
 * (assets/art/ui/font-5px): un moltiplicatore frazionario ne distruggerebbe i
 * tratti da un pixel, e sul canvas 640x360 (DEC-200) la vecchia mappatura
 * "dimensione in px -> scala" collassava comunque tutto su un unico
 * moltiplicatore. Restano quindi due sole taglie dichiarate: TAGLIA_2 per
 * cio' che il giocatore legge a colpo d'occhio (titoli di schermata, voci di
 * menu, numeri delle risorse, riga PIANO), TAGLIA_1 per il secondario
 * (didascalie, note, versione). La correzione arriva dal proprietario sulla
 * v1 del reskin: "scritte tagliate e tutto troppo piccolo". */

/* --- Colori (Fucina, DEC-173) --------------------------------------------
 * 'extern const' e non delle macro: sono Color veri, si passano a raylib
 * senza cast e restano UNA sola copia in memoria per l'intero binario. */
extern const Color UI_GROUND;      /* slag-nero: fondo assoluto, letterbox, ombre del bevel */
extern const Color UI_PANEL;       /* slag-scuro: riempimento di ogni pannello */
extern const Color UI_BEVEL_LUCE;  /* slag-caldo: lato illuminato del bevel, riga a fuoco */
extern const Color UI_TITOLO;      /* oro-fuso: titoli e testo della voce a fuoco */
extern const Color UI_TESTO;       /* bianco-caldo: testo normale */
extern const Color UI_SECONDARIO;  /* cenere-chiara: didascalie e note */
extern const Color UI_MUTO;        /* cenere-scura: cio' che c'e' ma non chiede attenzione */
extern const Color UI_HINT;        /* cenere: etichette dati e suggerimenti -- leggibile su UI_PANEL
                                      (4.4:1), e' il tono dei mock approvati; UI_MUTO resta per cio'
                                      che deve quasi sparire (versione, placeholder) */
extern const Color UI_FOCUS;       /* fiamma: la barra del fuoco, e nient'altro */
extern const Color UI_GLINT;       /* bagliore: punte di luce, evidenziazioni brevi */
extern const Color UI_DIVISORE;    /* bronzo-scuro: filetti fra le sezioni */

/* --- Testo ---------------------------------------------------------------
 * 'taglia' e' 1 o 2 (vedi sopra); un valore fuori range viene riportato
 * dentro invece di disegnare nulla, cosi' un errore di chiamata degrada in
 * una taglia sbagliata e non in testo invisibile. Senza il pacchetto
 * artistico si ricade sul font vettoriale di raylib con un corpo
 * equivalente, esattamente come faceva il renderer prima di WP-UI-0: una
 * schermata deve restare leggibile anche in un checkout senza assets/art/. */
#define UI_TAGLIA_1 1
#define UI_TAGLIA_2 2

void UiTextAt(const char *text, int x, int y, int taglia, Color tint);
int UiTextWidth(const char *text, int taglia);
int UiTextHeight(int taglia);

/* --- Pannello ------------------------------------------------------------
 * Riempimento UI_PANEL, bevel a due valori: riga alta e colonna sinistra in
 * UI_BEVEL_LUCE (la luce viene da sopra-sinistra, come per gli sprite), riga
 * bassa e colonna destra in UI_GROUND. MAI una cornice colorata da 1 px
 * (DEC-205): e' esattamente cio' che questo componente sostituisce. */
void UiPanel(Rectangle box);

/* Filetto orizzontale di separazione (UI_DIVISORE), alto 1 px del canvas:
 * l'unico segno "di linea" ammesso, e solo fra sezioni di uno stesso
 * pannello. */
void UiDivider(int x, int y, int w);

/* --- Riga di menu --------------------------------------------------------
 * La voce a fuoco porta TRE segnali insieme (DEC-058, mai il solo colore):
 * fascia di fondo UI_BEVEL_LUCE su tutta la riga, barra verticale UI_FOCUS
 * larga 4 px a sinistra, testo UI_TITOLO piu' una freccia '<' a destra. Le
 * voci non a fuoco sono solo testo UI_TESTO, senza riquadro: la lista deve
 * leggersi come una lista, non come una fila di bottoni. */
void UiMenuRow(Rectangle row, const char *label, bool focused);

/* Colore di rarita' rimappato sulla palette Fucina -- lo chiama
 * src/render/rarity_style.c, unica fonte di verita' della rarita' a schermo,
 * cosi' la tavolozza resta comunque dichiarata in un posto solo. */
Color UiRarityTint(Rarity rarity);

#endif
