#ifndef MELTING_RUN_GAME_ASSETS_H
#define MELTING_RUN_GAME_ASSETS_H

#include "core/game_types.h"

void GameUnloadAssets(Game *game);

/* DEC-171 (ponte provvisorio della demo): la texture di un'immagine CURATA,
 * dato il percorso relativo che l'oggetto porta con se' (Item.imagePath, es.
 * "items/potion-red.png" -> assets/curated/items/potion-red.png). Caricata
 * pigramente al primo disegno e tenuta in cache dentro Game
 * (Game.curatedTextures), liberata da GameUnloadAssets insieme all'atlas:
 * un solo ciclo di vita per tutto cio' che Game possiede.
 *
 * Ritorna NULL -- e chi disegna ricade sulla forma geometrica di sempre --
 * quando: il percorso e' vuoto (ogni oggetto che non nasce da una fusione),
 * il file non esiste (checkout senza il pacchetto curato), la texture non si
 * carica, o la cache e' piena (MAX_CURATED_TEXTURES). Nessuno di questi casi
 * e' un errore: il pacchetto curato e' un ponte, non un requisito.
 *
 * Sta qui e non in src/content perche' e' l'unico punto che tocca la GPU:
 * src/content/curated_images.h legge SOLO il manifest di testo, cosi' il
 * motore di fusione (e i test senza finestra) non aprono mai un'immagine. */
const Texture2D *AssetsCuratedTexture(Game *game, const char *relativePath);

#endif
