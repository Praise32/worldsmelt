#ifndef MELTING_RUN_ART_ATLAS_H
#define MELTING_RUN_ART_ATLAS_H

#include "core/game_types.h"

/* Pacchetto artistico ORIGINALE di Worldsmelt (W8): gli spritesheet disegnati a
 * mano in assets/art/ e i loro manifest.
 *
 * PERCHE' STA IN src/assets E NON IN src/render (confine, AGENTS.md):
 * qui si CARICA e si POSSIEDE (Texture2D + geometria dei fotogrammi), esattamente
 * la responsabilita' dichiarata per src/assets ("caricamento e rilascio delle
 * risorse Raylib"); chi DISEGNA sta in src/render (render/art_draw.h), e riceve
 * da qui solo dati. E' lo stesso taglio gia' in atto fra content/curated_images.c
 * (legge il manifest, mai un'immagine) e assets/game_assets.c
 * (AssetsCuratedTexture, l'unico che tocca la GPU): un modulo che sa DOV'E'
 * l'arte, un modulo che sa COME si mette a schermo.
 *
 * PERCHE' UN PARSER DEDICATO E NON UNA LIBRERIA: il binario del gioco non linka
 * mai cJSON (AGENTS.md). I manifest degli spritesheet sono JSON, ma scritti da
 * uno script della pipeline (scripts/, docs/ai-production/08-PIPELINE-SPRITE-
 * ANIMAZIONI.md), non da un modello: ASCII, profondita' fissa di due livelli,
 * nessun escape, nessuna virgoletta dentro i valori. Basta quindi uno scanner
 * sequenziale minimale (ArtAtlasParseManifest qui sotto), nella stessa
 * tradizione di ReadJsonString in content/curated_images.c -- e a differenza di
 * quello, sequenziale invece che a strstr, perche' qui le chiavi si ripetono
 * annidate ("row"/"w" compaiono una volta per animazione/glifo) e una ricerca
 * per sottostringa leggerebbe la chiave della voce sbagliata.
 * La forma testuale NON si pre-genera in build: aggiungerebbe un passo alla
 * catena assets->gioco (e un secondo formato da tenere sincronizzato) per
 * risparmiare ~150 righe di scanner, e romperebbe il ciclo di lavoro
 * dell'artista (salva da Aseprite, riavvia il gioco, vede).
 *
 * TRE SAPORI DI MANIFEST, un solo ArtSheet (i tre insiemi di chiavi sono
 * disgiunti, e tenerli in una struct sola evita tre registri e tre cache):
 *   1. SPRITESHEET   frame_w, frame_h, anchor[2], anims{nome:{row,frames,fps,loop}}
 *                    (+ slice[4] opzionale: e' un 9-patch, vedi ArtSheet.slice*)
 *   2. TILESET       tile_w, tile_h, grid[2], tiles{ruolo:[col,row]}
 *   3. FONT BITMAP   glyph_h, baseline_y, space_w, letter_spacing,
 *                    glyphs{carattere:{x,w}}
 *
 * FALLBACK: ogni funzione qui torna NULL/false quando l'asset manca, il
 * manifest e' illeggibile o la texture non si carica. Non e' un errore: chi
 * disegna ricade sul percorso precedente (immagine curata, cella d'atlas,
 * primitiva geometrica) come ha sempre fatto. Un checkout senza assets/art/ e
 * un PNG troncato devono dare lo stesso gioco di prima di W8, mai un crash. */

#define ART_ATLAS_DIR "assets/art/"
#define ART_KEY_LEN 64            /* "enemies/goblin-di-slag", senza estensione */
#define ART_ANIM_NAME_LEN 20
#define ART_ANIM_MAX 10           /* il massimo consegnato e' 7 (character/fonditrice) */
#define ART_ROLE_NAME_LEN 20
#define ART_ROLE_MAX 48           /* il tileset consegnato ne dichiara 38 */
#define ART_GLYPH_MAX 80          /* font-5px ne dichiara 51 */
/* Il registro deve contenere DUE cose, non una: gli sheet veri (73 coppie
   consegnate) E le voci NEGATIVE che la scansione a priorita' lascia dietro di
   se'. ArtAtlasFindByImageId prova "<categoria>/<id>" su sei categorie, e le
   cinque che non contengono quell'id diventano altrettante voci negative --
   ricordarle e' il punto (senza, si riproverebbero due file per categoria a ogni
   frame di disegno), ma vanno CONTATE nel limite: coi 25 oggetti, 14 nemici e 5
   boss del catalogo curato costa 71 voci di registro, non 44 (misurato da
   --art-atlas-test, che stampa il conteggio e fallisce se supera l'85%).
   160 lascia margine per il pacchetto artistico completo; superarlo non e' un
   crash (ArtAtlasGet torna NULL e chi disegna ricade sul percorso precedente) ma
   e' un degrado silenzioso, quindi ArtAtlasGet lo segnala su stderr UNA volta. */
#define ART_SHEET_MAX 160

/* Una animazione: una RIGA dello spritesheet letta a 'fps' fotogrammi al
   secondo. 'loop' falso = si ferma sull'ultimo fotogramma (colpo/morte). */
typedef struct ArtAnim {
    char name[ART_ANIM_NAME_LEN];
    int row;
    int frames;
    int fps;
    bool loop;
} ArtAnim;

/* Un RUOLO del tileset: la cella (colonna, riga) della griglia che disegna
   quel pezzo di stanza ("floor", "wall_n", "door_e_chiusa", "obst_pillar"...). */
typedef struct ArtTileRole {
    char name[ART_ROLE_NAME_LEN];
    int col;
    int row;
} ArtTileRole;

/* Un glifo del font bitmap: 'x' e' l'ascissa nella riga unica del PNG, 'w' la
   larghezza in pixel (proporzionale: 'I' e 'M' non occupano lo stesso spazio). */
typedef struct ArtGlyph {
    char ch;
    short x;
    short w;
} ArtGlyph;

typedef struct ArtSheet {
    char key[ART_KEY_LEN];
    /* I due esiti sono SEPARATI di proposito: un manifest valido con la
       texture mancante (PNG cancellato a mano, disco pieno) deve restare un
       "asset assente" per chi disegna, non una texture id 0 disegnata come
       rettangolo bianco. Chi chiama guarda solo ArtAtlasGet != NULL, che
       richiede entrambi. */
    bool manifestOk;
    bool textureOk;

    /* Sapore 1: spritesheet. */
    int frameW, frameH;
    int anchorX, anchorY;       /* punto dello sprite che va appoggiato sulla posizione richiesta */
    int sliceL, sliceT, sliceR, sliceB;   /* 9-patch; tutti 0 = non e' un 9-patch */
    /* Vero se il CENTRO del 9-patch e' un colore unico (misurato sull'immagine
       al caricamento, prima di caricarla in GPU). Serve a chi disegna: un
       centro uniforme si riempie con UN rettangolo invece di ripetere il tile,
       e la differenza non e' cosmetica -- ripetere un centro di 12x12 px su un
       pannello grande come mezzo schermo 4K sono ~15.000 quad per pannello e
       per frame, un costo che PEGGIORA con la risoluzione (il posto peggiore
       dove nasconderlo). Con un centro non uniforme si ripete come sempre:
       stirare un motivo pixel art lo renderebbe blocchi, ripeterlo lo
       affianca, ed e' la ripetizione la resa giusta.
       'sliceCenterColor' vale solo quando questo flag e' vero. */
    bool sliceCenterUniform;
    Color sliceCenterColor;
    ArtAnim anims[ART_ANIM_MAX];
    int animCount;

    /* Sapore 2: tileset. */
    int tileW, tileH;
    int gridCols, gridRows;
    ArtTileRole roles[ART_ROLE_MAX];
    int roleCount;

    /* Sapore 3: font bitmap. */
    int glyphH, baselineY, spaceW, letterSpacing;
    ArtGlyph glyphs[ART_GLYPH_MAX];
    int glyphCount;

    Texture2D texture;
} ArtSheet;

/* Legge 'text' (il contenuto di un manifest) in '*out', che viene azzerato per
 * primo -- sempre, anche quando il testo e' spazzatura: mai una struct a meta'.
 * Ritorna true se ne ha ricavato qualcosa di USABILE: uno spritesheet con
 * frame_w/frame_h > 0 e almeno un'animazione, un tileset con tile_w/tile_h > 0 e
 * almeno un ruolo, o un font con glyph_h > 0 e almeno un glifo. Tutto il resto
 * (chiave sconosciuta, valore mancante, graffa non chiusa) viene ignorato in
 * silenzio: un manifest esteso domani da una nuova chiave deve continuare a
 * caricarsi oggi, non far sparire lo sprite.
 * Funzione PURA (nessun file, nessuna texture, nessuna finestra aperta): e'
 * quella che i test esercitano su fixture di testo. */
bool ArtAtlasParseManifest(const char *text, ArtSheet *out);

/* Lo sheet di 'key' (percorso relativo a ART_ATLAS_DIR senza estensione, es.
 * "enemies/goblin-di-slag"), caricato PIGRAMENTE al primo uso e tenuto in cache
 * per tutto il processo: gli asset di assets/art/ sono STATICI (non contenuto di
 * run), quindi non seguono il ciclo di vita di Game -- un GameResetRun non deve
 * ricaricare 73 PNG. Rilascio: ArtAtlasShutdown.
 * NULL quando: 'key' e' vuota/troppo lunga, il .json o il .png mancano, il
 * manifest e' illeggibile, la texture non si carica, o la cache e' piena. Un
 * fallimento si RICORDA (voce negativa in cache): un file rotto viene tentato
 * una volta sola, non a ogni frame -- stessa disciplina di
 * AssetsCuratedTexture/atlasCellPresent. */
const ArtSheet *ArtAtlasGet(const char *key);

/* Risoluzione a PRIORITA' di un image-id (DEC-175(b): il contenuto referenzia
 * un image-id, mai un file). Cerca "<categoria>/<imageId>" fra le categorie di
 * assets/art/ in ordine FISSO -- items, enemies, bosses, props, shots,
 * character -- e ritorna il primo sheet che c'e'. NULL = nessun originale per
 * quell'id, e chi chiama scende al gradino successivo della priorita' (il
 * ponte CC0 di assets/curated/, DEC-171) e poi alla resa geometrica.
 * L'ordine e' fisso e non "prima la categoria che ti aspetti" perche' un
 * image-id e' unico nel pacchetto: la scansione serve solo a non dover
 * portare la categoria fin qui da ogni chiamante. */
const ArtSheet *ArtAtlasFindByImageId(const char *imageId);

/* L'animazione di nome 'name', o NULL. Il confronto e' esatto: i nomi canonici
 * per famiglia li fissa docs/ai-production/08-PIPELINE-SPRITE-ANIMAZIONI.md
 * (walk_down/walk_up/walk_left/walk_right/idle/hit/death per il personaggio,
 * walk/hit/death per i nemici, idle/attack/hit/death per i boss, fly/impact per
 * i colpi, aperta/chiusa/bloccata per le porte). */
const ArtAnim *ArtSheetAnim(const ArtSheet *sheet, const char *name);

/* La prima animazione presente fra i nomi passati (elenco chiuso da NULL): il
 * modo canonico di dire "walk se c'e', altrimenti idle, altrimenti la prima".
 * Mai NULL se lo sheet ha almeno un'animazione -- ripiega su anims[0]. */
const ArtAnim *ArtSheetAnimAny(const ArtSheet *sheet, const char *const *names);

/* Il rettangolo sorgente del fotogramma 'frame' della riga 'row'. Riga/colonna
 * fuori dalla texture vengono clampate: un manifest che dichiara piu'
 * fotogrammi di quanti il PNG contenga disegna l'ultimo valido, mai pixel
 * casuali fuori dalla texture. */
Rectangle ArtSheetFrameRect(const ArtSheet *sheet, int row, int frame);

/* Il rettangolo sorgente del RUOLO 'role' di un tileset. false (e '*outSrc'
 * intatto) se lo sheet non e' un tileset o non dichiara quel ruolo: chi chiama
 * ricade sul colore piatto di sempre per QUEL pezzo, non per l'intera stanza. */
bool ArtSheetTileRect(const ArtSheet *sheet, const char *role, Rectangle *outSrc);

/* Il glifo di 'ch', o NULL. Il font consegnato ha solo maiuscole: chi misura o
 * disegna converte a maiuscolo PRIMA (ArtDrawText lo fa), qui il confronto e'
 * esatto. */
const ArtGlyph *ArtSheetGlyph(const ArtSheet *sheet, char ch);

/* Il fotogramma da mostrare dopo 'elapsed' secondi dall'inizio
 * dell'animazione. Funzioni PURE e DETERMINISTICHE (nessun GetTime dentro):
 * stesso 'elapsed' -> stesso fotogramma, sempre, ed e' quello che il test
 * dell'animatore verifica senza finestra aperta.
 * Difese: frames <= 0 o fps <= 0 -> fotogramma 0 (uno sprite fermo, mai una
 * divisione per zero); 'elapsed' negativo -> fotogramma 0; un'animazione che
 * NON cicla si ferma sull'ultimo fotogramma e ci resta. */
int ArtAnimFrameAt(const ArtAnim *anim, float elapsed);

/* Vero quando un'animazione che non cicla ha finito di scorrere (l'ultimo
 * fotogramma e' stato mostrato per intero). Sempre falso per un'animazione che
 * cicla: non finisce mai, per definizione. Pura come sopra. */
bool ArtAnimDone(const ArtAnim *anim, float elapsed);

/* Rilascia le texture caricate e svuota la cache. Chiamata accanto ad
 * AudioShutdown nei percorsi di uscita del gioco vero (src/app/app.c): come
 * per il modulo audio, i percorsi di test escono subito dopo e lasciano la
 * pulizia al teardown del contesto OpenGL di CloseWindow. Idempotente. */
void ArtAtlasShutdown(void);

/* Cartella radice degli asset per i TEST (fixture): se settata (non-NULL) si
 * usa al posto di ART_ATLAS_DIR. Stesso schema di CuratedCatalogSetTestDir/
 * CuratedImagesSetTestManifestPath, e serve alla stessa cosa -- un test non
 * deve mai dipendere dal pacchetto di produzione (che cambia a ogni sessione
 * artistica) ne' scriverci. Svuota la cache: le voci gia' caricate vengono
 * dalla cartella precedente. */
void ArtAtlasSetTestDir(const char *dir);

/* Quanti sheet sono in cache (voci negative comprese): diagnostica per i
 * test, non usato dal gioco. */
int ArtAtlasCachedCount(void);

#endif
