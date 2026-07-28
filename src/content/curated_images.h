#ifndef MELTING_RUN_CURATED_IMAGES_H
#define MELTING_RUN_CURATED_IMAGES_H

#include "core/game_types.h"

/* Pacchetto immagini CURATE della demo (DEC-171, ponte provvisorio).
 *
 * Finche' la Style LoRA non e' addestrata (DEC-148), lo sprite di un oggetto
 * COMPOSTO da una fusione non lo genera alcun modello a runtime: si pesca fra
 * le immagini CC0 gia' selezionate in assets/curated/ (vedi il README li' e
 * docs/ai-production/17-ASSET-CURATION-AND-FLOOR-ZERO.md), in modo
 * DETERMINISTICO dal seed di run e SOLO fra quelle non ancora usate nella run
 * corrente. Questo modulo e' l'unico punto che sa come e' fatto quel
 * manifest.
 *
 * Sta in src/content (manifest e contenuti della run, AGENTS.md) e non in
 * src/assets di proposito: qui si legge SOLO testo -- id, percorso, categoria
 * -- e non si apre mai un'immagine ne' si crea una texture. Il caricamento
 * della texture vera (e quindi raylib/OpenGL) resta di src/assets
 * (AssetsCuratedTexture), cosi' il motore di fusione, che gira anche nei test
 * senza finestra, non tocca mai la GPU.
 *
 * Il manifest e' JSON, ma il gioco NON linka cJSON (AGENTS.md): si legge con
 * la stessa tecnica gia' usata per generated/theme_proposals.json in
 * src/app/app.c -- schema fisso, chiavi cercate in ordine. Lo scrive
 * scripts/curated-pack.py, non un modello: nessuna virgoletta o backslash
 * dentro i valori. */

#define CURATED_MANIFEST_PATH "assets/curated/manifest.json"
#define CURATED_IMAGE_DIR "assets/curated/"

typedef struct CuratedImage {
    char id[40];
    char file[64];       /* percorso RELATIVO a assets/curated/ (es. "items/potion-red.png") */
    char category[16];   /* "item", "enemie", "bosse", "prop" (i nomi del manifest, non ritoccati) */
} CuratedImage;

/* Quante voci ha il manifest, al massimo CURATED_IMAGE_MAX. 0 se il file
   manca o non contiene una sola voce valida: e' un caso NORMALE (un checkout
   senza il pacchetto curato) e chi chiama deve limitarsi a rinunciare
   all'immagine, mai fallire. */
int CuratedImagesCount(const char *manifestPath);

/* Pesca UNA voce fra quelle non ancora usate: scorre il manifest in ordine,
   scarta gli indici gia' marcati in 'usedMask' e -- se 'category' non e' NULL
   -- quelli di categoria diversa, poi prende la (roll % disponibili)-esima
   rimasta. 'roll' e' un numero qualunque gia' derivato dal seed di run da chi
   chiama: questo modulo non possiede alcun RNG, cosi' resta puro e testabile.
   Se la categoria richiesta non ha piu' nulla di libero, riprova SENZA
   filtro di categoria (meglio un'immagine di un'altra famiglia che nessuna
   immagine); se anche cosi' non resta nulla, ritorna false lasciando '*out'
   intatto -- il chiamante ricade sulla resa geometrica di sempre.
   'outIndex' (se non NULL) riceve l'indice di manifest della voce scelta:
   e' quello che va marcato in 'usedMask' perche' la stessa immagine non
   ricompaia piu' in questa run. */
bool CuratedImagesPickUnused(const char *manifestPath, unsigned int roll, const char *category,
                             const unsigned char *usedMask, int maskBytes,
                             CuratedImage *out, int *outIndex);

/* Cerca la voce con id ESATTAMENTE 'id' (confronto stringa, nessun caso
   particolare): usata dal layer di indirezione (content/curated_image_map.h,
   DEC-175(b)/W5b) per risolvere l'image-id che una voce del pool curato
   referenzia. Ritorna false -- '*out' intatta -- se il manifest manca, 'id'
   e' vuoto/NULL, o non compare in nessuna voce: caso NORMALE (id storpiato,
   o immagine non ancora aggiunta al pacchetto), chi chiama ricade sulla resa
   geometrica di sempre, mai un crash.
   'outIndex' (se non NULL) riceve l'indice di manifest della voce trovata,
   stessa convenzione di CuratedImagesPickUnused sopra: chi risolve
   un'immagine curata FUORI da PickUnused (il pool curato di contenuto,
   content/run_content.c, ApplyCuratedCatalog) lo usa per marcarla subito in
   Game.curatedImageUsed, cosi' FusionPerform non la ripesca (DEC-171: "fra
   le immagini non ancora usate nella run corrente"). -1 se 'out' non e'
   stato trovato. */
bool CuratedImagesFindById(const char *manifestPath, const char *id, CuratedImage *out, int *outIndex);

/* Stessa ricerca, ma sul testo del manifest GIA' in memoria: chi deve
   risolvere molti image-id di fila (i 15 slot di una run nel pool curato,
   ApplyCuratedCatalog in content/run_content.c) apre il file una volta sola
   invece di riaprirlo e riparsarlo -- 189 voci -- a ogni oggetto. Stesso
   principio del doppio passaggio su un solo LoadFileText in PickFrom qui
   sotto. 'manifestText' NULL = nessun pacchetto: false, come un file
   assente. */
bool CuratedImagesFindByIdInText(const char *manifestText, const char *id, CuratedImage *out, int *outIndex);

/* Manifest immagini per i TEST: se settato (non-NULL), il pool curato di
   contenuto (ApplyCuratedCatalog, content/run_content.c) lo usa al posto di
   CURATED_MANIFEST_PATH. Stesso identico schema di CuratedCatalogSetTestDir
   (content/curated_catalog.h), e serve alla stessa cosa: un test di
   integrazione del layer di indirezione deve poter verificare un'immagine
   risolta DAVVERO end-to-end senza dipendere dal contenuto del pacchetto di
   produzione (che cambia a ogni ricurazione).
   Lo onora SOLO ApplyCuratedCatalog: FusionPerform (gameplay/fusion.c) passa
   sempre il pacchetto vero, e i suoi test si limitano a leggerlo. */
void CuratedImagesSetTestManifestPath(const char *path);
const char *CuratedImagesGetTestManifestPath(void);

/* La maschera "gia' usata": un bit per indice di manifest (Game.curatedImageUsed).
   Fuori range = "usata" in lettura e no-op in scrittura, cosi' un indice
   sballato non puo' ne' corrompere memoria ne' far ripescare la stessa
   immagine all'infinito. */
bool CuratedImageMaskGet(const unsigned char *mask, int maskBytes, int index);
void CuratedImageMaskSet(unsigned char *mask, int maskBytes, int index);

#endif
