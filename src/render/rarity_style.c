#include "render/rarity_style.h"

/* Tavolozza "classica" del design doc (sezione 1), indicizzata come Rarity
   (core/game_types.h: RARITY_COMMON=0 .. RARITY_LEGENDARY=3). MODIFICA QUI
   per ribilanciare i colori: nessun altro punto del renderer definisce un
   colore di rarita' per conto suo (vedi il commento nell'header). */
static const Color RARITY_COLOR_TABLE[4] = {
    { 205, 208, 214, 255 },   /* RARITY_COMMON -- bianco/grigio */
    { 84, 210, 112, 255 },    /* RARITY_UNCOMMON -- verde */
    { 74, 150, 235, 255 },    /* RARITY_RARE -- blu */
    { 235, 158, 40, 255 },    /* RARITY_LEGENDARY -- arancione/oro */
};

/* Stessa indicizzazione della tavola sopra. */
static const char *RARITY_NAME_TABLE[4] = {
    "Comune", "Non-comune", "Raro", "Leggendario"
};

Color RarityColor(Rarity rarity)
{
    if (rarity < RARITY_COMMON || rarity > RARITY_LEGENDARY) return RARITY_COLOR_TABLE[RARITY_COMMON];
    return RARITY_COLOR_TABLE[rarity];
}

const char *RarityName(Rarity rarity)
{
    if (rarity < RARITY_COMMON || rarity > RARITY_LEGENDARY) return RARITY_NAME_TABLE[RARITY_COMMON];
    return RARITY_NAME_TABLE[rarity];
}
