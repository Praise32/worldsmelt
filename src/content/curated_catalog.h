#ifndef MELTING_RUN_CURATED_CATALOG_H
#define MELTING_RUN_CURATED_CATALOG_H

#include "core/game_types.h"

/* Formato e caricamento del pool CURATO di contenuto (W5b, DEC-153): oggetti
 * nelle 4 categorie (ItemKind, src/content/run_content.c), nemici e boss,
 * scritti a mano nello STESSO stile riga-chiave=valore che run_content.c gia'
 * legge dal manifest di una run generata -- si RIUSANO qui le stesse funzioni
 * di traduzione testo->enum (ItemKindFromText/RarityFromText, run_content.h;
 * EnemyFormFromText/EnemyMoveFromText/EnemyFireFromText/EnemyTypeBalance,
 * core/enemy_type.h), non se ne inventa un secondo vocabolario.
 *
 * Un FILE PER CATEGORIA (items.txt/enemies.txt/bosses.txt dentro la cartella
 * passata a CuratedCatalogLoad, di norma CURATED_CATALOG_DIR): rispecchia la
 * stessa spartizione per categoria di assets/curated/{items,enemies,bosses,
 * props} (vedi il README li'), invece di un solo file con tre sezioni o un
 * file per singola voce -- piu' facile da curare a mano categoria per
 * categoria. Ogni voce e' un record numerato ("item1.*", "item2.*", ...),
 * stesso sentinella "chiave .name= assente = fine pool" gia' in uso per i
 * record dei piani (ReadEnemyType/ReadRoomLayout in run_content.c): regge
 * senza modifiche per un elenco di lunghezza variabile.
 *
 * Ogni voce ha un campo "id" (content-id) SEPARATO dal nome mostrato in
 * gioco: e' la chiave che il layer di indirezione (curated_image_map.h) usa
 * per risolvere l'immagine, mai il nome (che puo' cambiare o ripetersi, e
 * un content-id resta stabile anche se il nome viene rigenerato).
 *
 * "Budget di potenza" (items-pools-and-rarity.md) non e' un campo a parte
 * qui: come per ogni altro oggetto del motore, si esprime attraverso
 * rarita' + traits + script (esattamente come items generati da melting-gen o
 * dal ripiego procedurale, run_content.c/gen_fallback.c) -- niente doppio
 * schema. */

#define CURATED_CATALOG_DIR "assets/curated-content"
#define CURATED_CATALOG_ITEM_MAX 64
#define CURATED_CATALOG_ENEMY_MAX 32
#define CURATED_CATALOG_BOSS_MAX 16
#define CURATED_CATALOG_ID_LEN 40

typedef struct CuratedCatalogItem {
    char id[CURATED_CATALOG_ID_LEN];   /* content-id, chiave di curated_image_map.h; puo' restare vuoto (nessuna indirezione immagine per questa voce) */
    Item item;                         /* stesso Item del motore (core/game_types.h): kind/rarity/slot/traits/color/script/luaSource/charges/cooldown gia' risolti dal testo */
} CuratedCatalogItem;

typedef struct CuratedCatalogEnemy {
    char id[CURATED_CATALOG_ID_LEN];
    EnemyTypeDef def;   /* gia' passato per EnemyTypeBalance al caricamento */
} CuratedCatalogEnemy;

typedef struct CuratedCatalogPool {
    CuratedCatalogItem items[CURATED_CATALOG_ITEM_MAX];
    int itemCount;
    CuratedCatalogEnemy enemies[CURATED_CATALOG_ENEMY_MAX];
    int enemyCount;
    CuratedCatalogEnemy bosses[CURATED_CATALOG_BOSS_MAX];   /* stesso formato dei nemici normali, def.boss sempre true */
    int bossCount;
} CuratedCatalogPool;

/* Carica '<dirPath>/items.txt' (+ 'enemies.txt'/'bosses.txt', opzionali) in
 * 'out' (azzerato per primo, sempre -- mai a meta' anche in caso di
 * fallimento). Ritorna false se 'dirPath' e' NULL/vuoto, o items.txt manca o
 * non contiene nemmeno una voce valida: e' il caso NORMALE di un checkout
 * senza contenuto curato ancora scritto (o di una fixture di test assente),
 * e chi chiama deve limitarsi a restare sul contenuto di ripiego (DEC-153),
 * mai fallire. enemies.txt/bosses.txt mancanti non fanno fallire il
 * caricamento degli oggetti: 'enemyCount'/'bossCount' restano 0 e chi chiama
 * ricade sui tipi di nemico procedurali per quel piano, stesso schema
 * "per-key fallback" di run_content.c. */
bool CuratedCatalogLoad(const char *dirPath, CuratedCatalogPool *out);

/* DEC-144: il pool curato minimo garantisce almeno un oggetto per rarita'.
 * Non e' un vincolo che questo loader possa FAR rispettare (il pool e'
 * scritto a mano o generato altrove) -- lo si puo' solo VERIFICARE dopo il
 * caricamento e segnalarlo chiaramente (fprintf su stderr, una riga per
 * ogni rarita' scoperta), mai bloccare il caricamento: un pool che viola il
 * floor resta comunque migliore di nessun pool, e chi ha scritto il
 * contenuto ha un log chiaro da correggere. Ritorna true se il floor e'
 * rispettato (nessuna rarita' a peso positivo in ItemPoolWeightsStandard,
 * gameplay/item_pool.h, e' scoperta), false altrimenti. */
bool CuratedCatalogValidateFloor(const CuratedCatalogPool *pool);

/* Pesca UNA voce di 'pool' con rarita' ESATTAMENTE 'rarity' (mai un'altra:
 * la corrispondenza per rarita' e' l'unica garanzia richiesta da chi chiama,
 * vedi RunContentLoad in run_content.c) usando 'roll' (gia' derivato dal
 * seed di run da chi chiama -- questo modulo non possiede alcun RNG, stesso
 * contratto di CuratedImagesPickUnused, content/curated_images.h). Ritorna
 * NULL se il pool non ha nessuna voce di quella rarita': chi chiama ricade
 * sul contenuto gia' presente (procedurale), mai un crash o un indice
 * inventato. */
const CuratedCatalogItem *CuratedCatalogPickItem(const CuratedCatalogPool *pool, Rarity rarity, unsigned int roll);

/* Percorso del catalogo curato per i TEST: se settato (non-NULL),
 * RunContentLoad (src/content/run_content.c) lo usa al posto di
 * CURATED_CATALOG_DIR. Stesso identico schema di RunCatalogSetTestPath/
 * RunCatalogGetTestPath (content/run_catalog.h): isola la fixture del test
 * dalla cartella di produzione, che un'altra sessione sul working tree
 * condiviso potrebbe star scrivendo proprio adesso -- mai il motore di test
 * tocca CURATED_CATALOG_DIR reale. */
void CuratedCatalogSetTestDir(const char *dir);
const char *CuratedCatalogGetTestDir(void);

#endif
