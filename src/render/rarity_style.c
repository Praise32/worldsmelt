#include "render/rarity_style.h"

#include "render/ui_theme.h"

/* Stessa indicizzazione di Rarity (core/game_types.h: RARITY_COMMON=0 ..
   RARITY_LEGENDARY=3). */
static const char *RARITY_NAME_TABLE[4] = {
    "Comune", "Non-comune", "Raro", "Leggendario"
};

/* WP-UI-0: i quattro colori non sono piu' una tavola locale ma la
   RIMAPPATURA della scala classica sulla palette Fucina (DEC-173): cenere,
   verderame, ardesia-chiara, oro-fuso. Il salto di TONO fra i quattro livelli
   resta quello del design doc (neutro -> verde -> blu -> oro), cambia la
   saturazione: il verde/blu accesi di prima erano gli ultimi due colori
   dell'interfaccia estranei alla palette. La tavola vera vive in
   src/render/ui_theme.c insieme a tutti gli altri token, questa funzione
   resta comunque l'unico punto da cui il renderer legge un colore di rarita'
   (vedi il commento nell'header). */
Color RarityColor(Rarity rarity)
{
    if (rarity < RARITY_COMMON || rarity > RARITY_LEGENDARY) return UiRarityTint(RARITY_COMMON);
    return UiRarityTint(rarity);
}

const char *RarityName(Rarity rarity)
{
    if (rarity < RARITY_COMMON || rarity > RARITY_LEGENDARY) return RARITY_NAME_TABLE[RARITY_COMMON];
    return RARITY_NAME_TABLE[rarity];
}
