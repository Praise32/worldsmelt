#ifndef MELTING_RUN_CURATED_IMAGE_MAP_H
#define MELTING_RUN_CURATED_IMAGE_MAP_H

#include <stdbool.h>

/* Layer di INDIREZIONE fra il content-id di una voce del pool curato
 * (content/curated_catalog.h) e l'image-id del manifest di assets/curated/
 * (content/curated_images.h) -- domani degli sprite Aseprite di
 * assets/art/. E' l'attuazione di DEC-175(b) ("fra contenuto e immagini
 * esiste un layer di indirezione contenuto->image-id... il contenuto non
 * referenzia mai un file immagine direttamente"), esteso da W5b al pool
 * curato di testi/parametri (il caso che DEC-175(b) descriveva era gia'
 * gli sprite Aseprite; qui la stessa indirezione vale anche verso il ponte
 * provvisorio CC0 di DEC-171). Il motore risolve un'immagine SOLO passando
 * da qui (mai un
 * content-id usato direttamente come image-id): cosi' il pool di contenuto
 * e il pacchetto immagini possono evolvere e ricombinarsi senza toccarsi a
 * vicenda -- una fusione di sprite, un nuovo pacchetto Aseprite, o una
 * ricurazione delle immagini non tocca mai un solo file di contenuto.
 *
 * Formato: una riga per voce, "<content-id> = <image-id>" (spazi opzionali
 * intorno al '='). Righe vuote ignorate; una riga il cui primo carattere e'
 * '#' e' un commento (per chi cura a mano dopo l'auto-mapping). Scritto da
 * scripts/curated-map.py alla prima costruzione (match dei tag del
 * contenuto coi tag del manifest immagini, nessuna curation estetica: solo
 * un punto di partenza riproducibile) ed e' comunque un file di testo
 * modificabile a mano in seguito.
 *
 * La mappa vive DENTRO la cartella del catalogo curato (CURATED_CATALOG_DIR,
 * content/curated_catalog.h): qui si dichiara solo il nome del file, e chi
 * legge compone il percorso a partire dalla stessa cartella da cui ha
 * caricato il pool. Non e' un dettaglio cosmetico (correzione round 1): con
 * un percorso di produzione hardcoded, CuratedCatalogSetTestDir avrebbe
 * spostato il catalogo sulla fixture ma NON la mappa, e un test di
 * integrazione avrebbe letto il file di produzione appena esistito --
 * proprio la cartella condivisa che le fixture non devono mai toccare. */

#define CURATED_IMAGE_MAP_FILE "image-map.txt"

/* Cerca 'contentId' come chiave e copia il suo valore (image-id) in
 * 'outImageId'. Ritorna false -- 'outImageId' sempre azzerata -- se
 * 'mapPath' non esiste, 'contentId' e' vuoto/NULL, o la chiave non compare
 * nel file: caso NORMALE (contenuto senza voce nella mappa, o mappa non
 * ancora costruita), chi chiama ricade sulla resa geometrica di sempre, mai
 * un crash. */
bool CuratedImageMapResolve(const char *mapPath, const char *contentId, char *outImageId, int outSize);

/* Stessa risoluzione, ma sul testo della mappa GIA' in memoria: chi deve
 * risolvere molti content-id di fila (il pool curato dei 15 slot di una run,
 * ApplyCuratedCatalog in content/run_content.c) apre il file una volta sola
 * invece di riaprirlo e riparsarlo a ogni voce. Stessa convenzione del
 * doppio passaggio su un solo LoadFileText gia' usata da PickFrom in
 * content/curated_images.c. 'mapText' NULL = nessuna mappa: false, come un
 * file assente. */
bool CuratedImageMapResolveInText(const char *mapText, const char *contentId, char *outImageId, int outSize);

#endif
