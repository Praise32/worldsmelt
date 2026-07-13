/* Unico file di melting-sprites che include stable-diffusion.h: tutto il resto
   del tool (a partire da melting_sprites.h) resta indipendente dai tipi di
   sd.cpp. Non e' solo igiene del codice: llama.cpp e stable-diffusion.cpp
   vendorizzano due ggml incompatibili (sd.cpp usa il fork leejet/ggml), quindi
   melting-sprites e melting-gen DEVONO restare eseguibili separati (vedi
   Makefile, SPRITES_LIBS vs GEN_LIBS). Questo file e' la prova che nessun
   simbolo di sd.cpp trapela fuori dal modulo.

   Parametri (modello, LoRA, 512x512, 8 passi, LCM, cfg 1.5, vae_conv_direct,
   flash-attn OFF) misurati nello spike, vedi docs/SPRITES-SPIKE.md: non
   ridiscussi qui. */
#include "melting_sprites.h"

#include "stable-diffusion.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct SpriteSdCtx {
    sd_ctx_t *ctx;
    sd_lora_t lora;
    int hasLora;
    int steps;
    float cfg;
    double vramReportedMB;   /* da "total params memory size (VRAM ...)" nel log di sd.cpp, 0 se non visto */
};

static void SdLogCb(enum sd_log_level_t level, const char *text, void *data)
{
    SpriteSdCtx *self = data;
    if (self && self->vramReportedMB <= 0.0)
    {
        /* sd.cpp (stable-diffusion.cpp:1566 a questo tag) logga una riga
           "total params memory size = ...MB (VRAM 2035.00MB, RAM 0.00MB): ...":
           e' l'unica fonte diretta di VRAM che l'API espone, la catturiamo qui
           invece di ririchiederla al chiamante. */
        const char *v = strstr(text, "(VRAM ");
        if (v) sscanf(v, "(VRAM %lfMB", &self->vramReportedMB);
    }
    if (level < SD_LOG_INFO) return;   /* i log DEBUG di sd.cpp sono troppo rumorosi per il file */
    const char *tag = level == SD_LOG_ERROR ? "errore" : (level == SD_LOG_WARN ? "avviso" : "info");
    /* log_printf (src/core/util.cpp di sd.cpp) garantisce che text finisca
       gia' con '\n': tagliamo li' per non raddoppiarlo nel nostro log. */
    SpritesLogLine("sd.cpp[%s]: %.*s", tag, (int)strcspn(text, "\n"), text);
}

SpriteSdCtx *SpriteSdLoad(const SpriteSdConfig *cfg, double *loadSecs)
{
    *loadSecs = 0;
    if (!SpritesFileExists(cfg->modelPath))
    {
        SpritesLogLine("sd: modello assente: %s", cfg->modelPath);
        return NULL;
    }

    SpriteSdCtx *self = calloc(1, sizeof(SpriteSdCtx));
    if (!self) return NULL;
    self->steps = cfg->steps > 0 ? cfg->steps : 8;
    self->cfg = cfg->cfg;

    sd_set_log_callback(SdLogCb, self);   /* self come user-data: SdLogCb ci scrive vramReportedMB */

    sd_ctx_params_t p;
    sd_ctx_params_init(&p);   /* obbligatorio: zero-init da solo sceglierebbe f32 e uno scheduler sbagliato */
    p.model_path = cfg->modelPath;
    p.taesd_path = (cfg->taesdPath && cfg->taesdPath[0]) ? cfg->taesdPath : NULL;
    p.vae_conv_direct = true;        /* spike: grosso risparmio VRAM/tempo su Vulkan AMD */
    p.flash_attn = false;            /* upstream: non supportata sulla maggior parte dei setup Vulkan, rallenta */
    p.diffusion_flash_attn = false;  /* idem, per il ramo diffusione */

    SpritesProgressWrite(cfg->outDir, "carico-modello", 0, "carico il modello Stable Diffusion (Vulkan)");
    double t0 = SpritesNowSeconds();
    self->ctx = new_sd_ctx(&p);
    *loadSecs = SpritesNowSeconds() - t0;
    if (!self->ctx)
    {
        SpritesLogLine("sd: new_sd_ctx fallita (modello=%s)", cfg->modelPath);
        free(self);
        return NULL;
    }
    if (!sd_ctx_supports_image_generation(self->ctx))
    {
        SpritesLogLine("sd: il contesto caricato non supporta la generazione di immagini (modello=%s)", cfg->modelPath);
        free_sd_ctx(self->ctx);
        free(self);
        return NULL;
    }
    SpritesProgressWrite(cfg->outDir, "carico-modello", 15, "modello caricato");

    if (cfg->loraPath && cfg->loraPath[0])
    {
        if (SpritesFileExists(cfg->loraPath))
        {
            self->lora.is_high_noise = false;
            self->lora.multiplier = 1.0f;
            /* Il puntatore resta valido per tutta la vita di self: cfg->loraPath
               e' o il default statico o un argv del chiamante, entrambi vivi
               per l'intero processo (vedi main.c). */
            self->lora.path = cfg->loraPath;
            self->hasLora = 1;
        }
        else
        {
            SpritesLogLine("sd: LoRA assente: %s (continuo senza; a 8 passi/cfg 1.5 senza LCM la qualita' ne risente)",
                           cfg->loraPath);
        }
    }

    return self;
}

double SpriteSdVramMB(const SpriteSdCtx *ctx)
{
    return ctx ? ctx->vramReportedMB : 0.0;
}

void SpriteSdFree(SpriteSdCtx *ctx)
{
    if (!ctx) return;
    if (ctx->ctx) free_sd_ctx(ctx->ctx);
    free(ctx);
}

int SpriteSdGenerate(SpriteSdCtx *ctx, const char *prompt, const char *negPrompt,
                     unsigned int seed, unsigned char *outRgb512, double *genSecs)
{
    *genSecs = 0;
    if (!ctx || !ctx->ctx || !prompt) return -1;

    sd_img_gen_params_t p;
    sd_img_gen_params_init(&p);   /* obbligatorio: inizializza anche sample_params, cache, hires ecc. */
    p.prompt = prompt;
    p.negative_prompt = negPrompt ? negPrompt : "";
    p.width = SPRITE_SRC;
    p.height = SPRITE_SRC;
    p.seed = (int64_t)seed;
    p.batch_count = 1;
    p.sample_params.sample_method = LCM_SAMPLE_METHOD;
    p.sample_params.scheduler = LCM_SCHEDULER;      /* stesso abbinamento di sd_get_default_scheduler() per LCM */
    p.sample_params.sample_steps = ctx->steps;
    p.sample_params.guidance.txt_cfg = ctx->cfg;
    if (ctx->hasLora)
    {
        p.loras = &ctx->lora;
        p.lora_count = 1;
    }

    sd_image_t *images = NULL;
    int numImages = 0;
    double t0 = SpritesNowSeconds();
    bool ok = generate_image(ctx->ctx, &p, &images, &numImages);
    *genSecs = SpritesNowSeconds() - t0;

    if (!ok || numImages < 1 || !images || !images[0].data)
    {
        SpritesLogLine("sd: generate_image fallita (seed=%u)", seed);
        if (images) free_sd_images(images, numImages);
        return -1;
    }

    sd_image_t img = images[0];
    if (img.width != SPRITE_SRC || img.height != SPRITE_SRC || img.channel != 3)
    {
        SpritesLogLine("sd: immagine inattesa %ux%u canali=%u (atteso %dx%d RGB)",
                       img.width, img.height, img.channel, SPRITE_SRC, SPRITE_SRC);
        free_sd_images(images, numImages);
        return -1;
    }
    memcpy(outRgb512, img.data, (size_t)SPRITE_SRC*SPRITE_SRC*3);
    free_sd_images(images, numImages);
    return 0;
}
