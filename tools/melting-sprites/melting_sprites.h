#ifndef MELTING_SPRITES_H
#define MELTING_SPRITES_H

#include <stddef.h>
#include <stdio.h>

/* Dimensioni fisse della pipeline, misurate nello spike (docs/ai-production/experiments/sprites-spike.md):
   Stable Diffusion genera di default a 512x512, il gioco usa celle da
   128x128 in un atlas 1024x1024 a 8 colonne (stesso layout di AtlasSprite in
   src/core/game_types.h: cella i alla colonna i%8, riga i/8).

   Dalla fase --gen-size (preset --low-spec del gioco, roadmap 16/07/2026) la
   dimensione di generazione e' un parametro a riga di comando: 512 (default,
   scheda di riferimento 5600 XT) o 256 (hardware sotto la scheda di
   riferimento). SPRITE_SRC resta la dimensione MASSIMA/di default, usata per
   dimensionare i buffer sorgente allocati una sola volta: la dimensione
   EFFETTIVA di ogni run gira come parametro "genSize" nelle funzioni di
   sprite_post.c/sprite_sd.c/main.c. La cella finale nell'atlas resta SEMPRE
   128x128 qualunque sia genSize: cambia solo il fattore del downscale modale
   (genSize/SPRITE_CELL, oggi 4 o 2), il gioco non vede alcuna differenza. */
#define SPRITE_SRC 512
#define SPRITE_CELL 128
/* Il piu' grande fattore di downscale modale possibile (genSize=512 ->
   512/128=4): serve solo a dimensionare l'array di appoggio "key" in
   SpritesModalDownscale, che a runtime ne usa solo f*f con f=genSize/128. */
#define SPRITE_DOWNSCALE_F_MAX (SPRITE_SRC / SPRITE_CELL)
#define SPRITE_ATLAS_COLS 8
#define SPRITE_ATLAS_W (SPRITE_ATLAS_COLS * SPRITE_CELL)
/* Le celle note dell'atlas (vedi AtlasSprite in src/core/game_types.h):
   player, 3 nemici, boss, oggetto, cuore, moneta, bomba, chiave, portale, colpo. */
#define SPRITE_CELLS 13   /* fase 3b: +enemy_floater, aggiunta IN CODA (vedi AtlasSprite in src/core/game_types.h) */

/* Colori nella palette finale di ogni sprite (riduzione con exoquant). */
#define SPRITE_PALETTE_COLORS 16

/* Il gioco fa chroma-key sul quasi-nero (src/assets/game_assets.c): nessun
   pixel opaco dello sprite puo' avere max(r,g,b) sotto questa soglia, o il
   caricamento lo tratterebbe come sfondo trasparente. Vedi "Le due cose
   imparate" in docs/ai-production/experiments/sprites-spike.md. */
#define SPRITE_KEY_FLOOR 16

/* Tolleranze del ritaglio a flood fill: la prima per il flood fill vero e
   proprio dai bordi, la seconda (piu' larga) per i due passaggi di "halo"
   che ripuliscono l'anti-aliasing fra sprite e sfondo. Valori dello spike. */
#define SPRITE_CUT_TOL 40
#define SPRITE_CUT_TOL_HALO 80

typedef struct SpritePostStats {
    int cutPixels;      /* pixel di sfondo ritagliati dal flood fill */
    int opaquePixels;   /* pixel opachi rimasti nello sprite finale */
    int keyRiskPixels;  /* pixel opachi con max(r,g,b) < SPRITE_KEY_FLOOR (deve essere 0) */
} SpritePostStats;

/* sprite_post.c: downscale modale, ritaglio dello sfondo, riduzione palette.
   Porting diretto della pipeline provata su hardware reale (vedi
   docs/ai-production/experiments/sprites-spike.md). Operano su un buffer di cella SPRITE_CELL x
   SPRITE_CELL, RGBA a meno che sia specificato altrimenti. */

/* src: genSize*genSize*3 (RGB), genSize deve essere 256 o 512 (vedi
   --gen-size in main.c). dst: SPRITE_CELL*SPRITE_CELL*3 (RGB). */
void SpritesModalDownscale(const unsigned char *src, unsigned char *dst, int genSize);

/* cellRgba: SPRITE_CELL*SPRITE_CELL*4, alpha gia' impostato a 255 in ingresso.
   Ritaglia lo sfondo (colore preso dal bordo dell'immagine) con un flood
   fill dai bordi: un pixel scuro dentro lo sprite non e' raggiungibile senza
   attraversare lo sprite, quindi sopravvive. Ritorna il numero di pixel
   trasformati in trasparenti. */
int SpritesCutBackground(unsigned char *cellRgba, int tol, int tolHalo);

/* Riduce i pixel opachi di cellRgba a ncolors colori (exoquant) e applica
   SPRITE_KEY_FLOOR ai colori troppo scuri della palette risultante. */
void SpritesQuantize(unsigned char *cellRgba, int ncolors);

/* Orchestrazione dei tre passi sopra: da sorgente genSize*genSize RGB a
   cella 128x128 RGBA pronta per l'atlas (genSize 256 o 512, vedi --gen-size
   in main.c). outCellRgba deve avere spazio per SPRITE_CELL*SPRITE_CELL*4
   byte. statsOut puo' essere NULL. */
void SpritesPostProcessCell(const unsigned char *src512Rgb, unsigned char *outCellRgba,
                             int ncolors, SpritePostStats *statsOut, int genSize);

/* sprite_atlas.c: composizione dell'atlas 1024x1024 RGBA e scrittura PNG. */

/* Alloca e azzera (trasparente) un atlas SPRITE_ATLAS_W x SPRITE_ATLAS_W
   RGBA. Il chiamante libera con free(). NULL su memoria esaurita. */
unsigned char *SpritesAtlasNew(void);

/* Ricentra lo sprite dentro la sua cella usando il riquadro dei pixel
   opachi: dopo il ritaglio lo sprite potrebbe non essere al centro esatto
   della cella. Cella vuota (nessun pixel opaco): non fa nulla. */
void SpritesCenterInCell(unsigned char *cellRgba);

/* Ricentra (vedi sopra) e copia cellRgba (SPRITE_CELL*SPRITE_CELL*4) nella
   cella cellIndex dell'atlas (colonna cellIndex%SPRITE_ATLAS_COLS, riga
   cellIndex/SPRITE_ATLAS_COLS). atlas deve avere spazio per
   SPRITE_ATLAS_W*SPRITE_ATLAS_W*4 byte. */
void SpritesComposeAtlas(unsigned char *atlas, int cellIndex, unsigned char *cellRgba);

/* Scrive atlas come "<outDir>/current_atlas.png" (file temporaneo + rename
   atomico, come i file scritti da melting-gen). 0 se ok, -1 su errore. */
int SpritesWriteAtlasPng(const unsigned char *atlas, const char *outDir);

/* Riquadro dei pixel opachi che tocca il bordo della cella: probabile ritaglio
   fallito (lo sprite non aveva margine di sfondo su almeno un lato, il flood
   fill non ha potuto tagliare nulla su quel lato). Usato dal gate di qualita'
   in main.c. */
int SpritesOpaqueTouchesBorder(const unsigned char *cellRgba);

/* ---------------------------------------------------------------------
 * sprite_util.c: utilita' condivise (file, progresso, log, VRAM). Stesso
 * pattern di gen_util.c in tools/melting-gen, MA duplicato e non condiviso:
 * i due tool vendorizzano due ggml incompatibili e devono restare eseguibili
 * separati (vedi Makefile e il commento in cima a sprite_sd.c). */

int SpritesEnsureDir(const char *path);
char *SpritesReadFile(const char *path);   /* buffer malloc, terminato da zero; NULL su errore */
int SpritesFileExists(const char *path);
double SpritesNowSeconds(void);            /* orologio monotono, per le misure di tempo nei log */

/* Scrive "<outDir>/gen_progress.txt" nello stesso formato "fase|percentuale|messaggio"
   di GenProgressWrite in tools/melting-gen/gen_util.c: il gioco legge questo file
   a ogni frame durante la generazione (un task successivo collega i due passi). */
void SpritesProgressWrite(const char *outDir, const char *phase, int percent, const char *message);

void SpritesLogLine(const char *fmt, ...);

/* Pubblica atomicamente un file "definitivo" (manifest, atlas): f e' gia' stato
   aperto su tmpPath e scritto dal chiamante. Controlla gli errori di scrittura,
   chiude il file, e solo se tutto e' andato bene fa rename() su finalPath. */
int SpritesPublishFile(FILE *f, const char *tmpPath, const char *finalPath);

/* Byte di VRAM occupati adesso, letti da
   /sys/class/drm/card*\/device/mem_info_vram_used (driver amdgpu). -1 se
   nessuna card espone quel file (altra GPU, altro driver, altro OS): e' un
   dato "a miglior sforzo" per il log, mai un requisito per funzionare. */
long long SpritesReadVramUsedBytes(void);

/* ---------------------------------------------------------------------
 * sprite_manifest.c: lettura di generated/current_run.txt (tema/stile del
 * piano 1) e aggiornamento del campo atlas.path dopo aver scritto il PNG. */

typedef struct SpriteManifest {
    char theme[96];
    char style[64];
    int loaded;   /* 0 se il manifest non c'e' o non contiene floor1.theme */
} SpriteManifest;

/* Legge "<outDir>/current_run.txt". out->loaded resta 0 (con theme/style
   vuoti) se il file manca: il chiamante deve ripiegare su un tema generico,
   mai fallire. */
void SpritesLoadManifest(const char *outDir, SpriteManifest *out);

/* Aggiorna (o aggiunge) la riga "atlas.path=" di "<outDir>/current_run.txt" al
   valore fisso "generated/current_atlas.png" (stesso schema del percorso
   fisso non derivato da outDir usato da melting-gen, vedi gen_manifest.c:
   il gioco legge sempre "generated/current_run.txt" e basta). Non fa nulla
   (e non fallisce) se il manifest non esiste ancora. */
void SpritesUpdateManifestAtlasPath(const char *outDir);

/* ---------------------------------------------------------------------
 * sprite_prompt.c: costruzione dei prompt dai template su disco, sotto
 * tools/melting-sprites/prompts/ (un file .txt per cella), stesso schema di
 * tools/melting-gen/prompts/ (testo con segnaposto, editabile senza
 * ricompilare). */

/* Nomi delle celle, nello stesso ordine di AtlasSprite in src/core/game_types.h:
   usati sia per il nome del file di prompt (<prompts>/<nome>.txt) sia nei log. */
extern const char *SPRITE_CELL_NAMES[SPRITE_CELLS];

/* Sostituisce ogni occorrenza di {THEME} e {STYLE} in templateText. Ritorna
   una stringa malloc'ata (il chiamante fa free()), o NULL su memoria esaurita.
   theme/style NULL sono trattati come stringa vuota. */
char *SpritesExpandTemplate(const char *templateText, const char *theme, const char *style);

/* Legge "<promptsDir>/<SPRITE_CELL_NAMES[cellIndex]>.txt" ed espande
   {THEME}/{STYLE}. NULL se il file manca, e' vuoto, o cellIndex e' fuori
   intervallo. */
char *SpritesLoadCellPrompt(const char *promptsDir, int cellIndex, const char *theme, const char *style);

/* Legge "<promptsDir>/negative.txt" (nessuna espansione: e' fisso per tutte
   le celle). NULL se il file manca. */
char *SpritesLoadNegativePrompt(const char *promptsDir);

/* ---------------------------------------------------------------------
 * sprite_sd.c: unico file che include stable-diffusion.h. Tutti gli altri
 * file di melting-sprites (incluso questo header) restano indipendenti dai
 * tipi di sd.cpp, cosi' l'isolamento fra i due ggml incompatibili (vedi
 * Makefile) e' anche una proprieta' del codice sorgente, non solo del link. */

typedef struct SpriteSdConfig {
    const char *modelPath;
    const char *loraPath;    /* NULL o "" = nessuna LoRA */
    const char *taesdPath;   /* NULL = usa la VAE reale (default, piu' nitida, vedi spike) */
    int steps;
    float cfg;
    const char *outDir;      /* per il file di progresso durante il caricamento */
} SpriteSdConfig;

typedef struct SpriteSdCtx SpriteSdCtx;   /* opaco */

/* Carica il modello (una volta sola, riusato per tutte le celle). NULL su
   errore (modello mancante, file non valido, VRAM insufficiente): il
   chiamante deve ripiegare su celle sintetiche, mai andare in crash. */
SpriteSdCtx *SpriteSdLoad(const SpriteSdConfig *cfg, double *loadSecs);
void SpriteSdFree(SpriteSdCtx *ctx);

/* MB di VRAM riportati dal log di sd.cpp durante il caricamento ("total
   params memory size ... VRAM ...MB"), 0 se quella riga non e' arrivata (ctx
   NULL, o una versione futura di sd.cpp che logga diversamente). */
double SpriteSdVramMB(const SpriteSdCtx *ctx);

/* Genera un'immagine genSize x genSize RGB (genSize 256 o 512, vedi
   --gen-size in main.c; outRgb512 ha gia' SPRITE_SRC*SPRITE_SRC*3 byte
   allocati dal chiamante, cioe' la dimensione MASSIMA: con genSize=256 solo
   la porzione iniziale del buffer viene scritta/letta). 0 se ok, -1 su
   errore (il chiamante puo' ritentare con un altro seed o rinunciare alla
   cella). */
int SpriteSdGenerate(SpriteSdCtx *ctx, const char *prompt, const char *negPrompt,
                     unsigned int seed, unsigned char *outRgb512, double *genSecs, int genSize);

#endif
