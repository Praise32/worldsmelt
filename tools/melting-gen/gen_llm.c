#include "melting_gen.h"

#include "llama.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double NowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

static bool LoadProgressCb(float progress, void *user)
{
    const char *outDir = user;
    int pct = (int)(progress*60.0f);   /* caricamento modello = 0..60% della barra */
    GenProgressWrite(outDir, "carico-modello", pct, "carico il modello (Vulkan)");
    return true;   /* false interromperebbe il caricamento */
}

/* Prompt ChatML di Qwen2.5: <|im_end|> subito dopo il contenuto, poi newline. */
static char *BuildPrompt(const GenLlmConfig *cfg)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/system.txt", cfg->promptsDir);
    char *sys = GenReadFile(path);
    snprintf(path, sizeof(path), "%s/user.txt", cfg->promptsDir);
    char *user = GenReadFile(path);
    if (!sys || !user)
    {
        free(sys);
        free(user);
        return NULL;
    }

    char seedText[32];
    snprintf(seedText, sizeof(seedText), "%u", cfg->seed);
    size_t userCap = strlen(user) + sizeof(seedText) + 1;
    char *userFinal = malloc(userCap);
    if (!userFinal)
    {
        free(sys);
        free(user);
        return NULL;
    }
    const char *mark = strstr(user, "{SEED}");
    if (mark)
    {
        size_t head = (size_t)(mark - user);
        memcpy(userFinal, user, head);
        userFinal[head] = '\0';
        strncat(userFinal, seedText, userCap - strlen(userFinal) - 1);
        strncat(userFinal, mark + 6, userCap - strlen(userFinal) - 1);
    }
    else snprintf(userFinal, userCap, "%s", user);

    size_t total = strlen(sys) + strlen(userFinal) + 128;
    char *prompt = malloc(total);
    if (prompt)
    {
        snprintf(prompt, total,
                 "<|im_start|>system\n%s<|im_end|>\n<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n",
                 sys, userFinal);
    }
    free(sys);
    free(user);
    free(userFinal);
    return prompt;
}

int GenLlmGenerate(const GenLlmConfig *cfg, char *out, size_t outCap,
                   double *loadSecs, double *genSecs, int *tokensOut)
{
    out[0] = '\0';
    *loadSecs = 0;
    *genSecs = 0;
    *tokensOut = 0;

    char *grammar = GenReadFile(cfg->grammarPath);
    char *prompt = BuildPrompt(cfg);
    if (!grammar || !prompt)
    {
        GenLogLine("llm: grammatica o prompt mancanti (%s, %s)", cfg->grammarPath, cfg->promptsDir);
        free(grammar);
        free(prompt);
        return -1;
    }

    llama_backend_init();
    int rc = -1;
    struct llama_model *model = NULL;
    struct llama_context *ctx = NULL;
    struct llama_sampler *smpl = NULL;
    llama_token *tokens = NULL;

    double t0 = NowSeconds();
    struct llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = cfg->nGpuLayers;
    mparams.progress_callback = LoadProgressCb;
    mparams.progress_callback_user_data = (void *)cfg->outDir;
    model = llama_model_load_from_file(cfg->modelPath, mparams);
    if (!model)
    {
        GenLogLine("llm: caricamento fallito: %s (ngl=%d)", cfg->modelPath, cfg->nGpuLayers);
        goto cleanup;
    }
    *loadSecs = NowSeconds() - t0;

    {
        const struct llama_vocab *vocab = llama_model_get_vocab(model);

        int n_prompt = -llama_tokenize(vocab, prompt, (int32_t)strlen(prompt), NULL, 0, true, true);
        if (n_prompt <= 0) goto cleanup;
        tokens = malloc(sizeof(llama_token)*(size_t)n_prompt);
        if (!tokens) goto cleanup;
        if (llama_tokenize(vocab, prompt, (int32_t)strlen(prompt), tokens, n_prompt, true, true) < 0) goto cleanup;

        struct llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx = (uint32_t)(n_prompt + cfg->nPredict);
        cparams.n_batch = (uint32_t)n_prompt;
        /* Deviazione dal testo del brief ("non impostare affatto flash-attn"):
           a b9979 il default di llama_context_default_params() e' AUTO, non
           "spento". AUTO si risolve in ENABLED ogni volta che l'op fusa puo'
           girare su un solo device (qui sempre vero: tutto su Vulkan0), come
           verificato nel primo run di test-llm ("Flash Attention enabled" nel
           log). Su RDNA1 la flash-attention Vulkan collassa le prestazioni:
           la regola non negoziabile vince sulla lettera del brief, quindi qui
           la disabilitiamo esplicitamente invece di lasciare il default. */
        cparams.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_DISABLED;
        ctx = llama_init_from_model(model, cparams);
        if (!ctx)
        {
            GenLogLine("llm: creazione contesto fallita (n_ctx=%d)", n_prompt + cfg->nPredict);
            goto cleanup;
        }

        smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
        struct llama_sampler *gsmpl = llama_sampler_init_grammar(vocab, grammar, "root");
        if (!gsmpl)
        {
            GenLogLine("llm: grammatica GBNF non parsabile: %s", cfg->grammarPath);
            goto cleanup;
        }
        llama_sampler_chain_add(smpl, gsmpl);   /* la grammatica PRIMA del selettore */
        /* Penalita' sulle ripetizioni: senza, il modello riusa gli stessi nomi tra
           i piani ("Ignea Ignea", due boss identici). La grammatica continua a
           mascherare i token non validi, quindi la penalita' puo' solo spostare le
           probabilita' fra scelte gia' legali: il JSON resta valido per costruzione. */
        llama_sampler_chain_add(smpl, llama_sampler_init_penalties(
            GEN_PENALTY_LAST_N, GEN_PENALTY_REPEAT, 0.0f, 0.0f));
        llama_sampler_chain_add(smpl, llama_sampler_init_temp(cfg->temp));
        llama_sampler_chain_add(smpl, llama_sampler_init_dist(cfg->seed));

        GenProgressWrite(cfg->outDir, "genero", 62, "genero i contenuti della run");
        t0 = NowSeconds();
        size_t used = 0;
        int generated = 0;
        struct llama_batch batch = llama_batch_get_one(tokens, n_prompt);
        llama_token newToken = 0;
        rc = 0;
        while (generated < cfg->nPredict)
        {
            if (llama_decode(ctx, batch) != 0)
            {
                GenLogLine("llm: llama_decode fallita al token %d", generated);
                rc = -1;
                break;
            }
            newToken = llama_sampler_sample(smpl, ctx, -1);
            if (llama_vocab_is_eog(vocab, newToken)) break;
            char piece[128];
            int n = llama_token_to_piece(vocab, newToken, piece, sizeof(piece), 0, true);
            if (n < 0 || used + (size_t)n + 1 >= outCap)
            {
                rc = -1;
                break;
            }
            memcpy(out + used, piece, (size_t)n);
            used += (size_t)n;
            out[used] = '\0';
            generated++;
            if (generated%32 == 0)
            {
                int pct = 62 + (int)(30.0*generated/cfg->nPredict);
                char msg[96];
                snprintf(msg, sizeof(msg), "genero i contenuti (%d token)", generated);
                GenProgressWrite(cfg->outDir, "genero", pct > 92 ? 92 : pct, msg);
            }
            batch = llama_batch_get_one(&newToken, 1);
        }
        *genSecs = NowSeconds() - t0;
        *tokensOut = generated;
    }

cleanup:
    if (smpl) llama_sampler_free(smpl);   /* libera l'intera catena, grammatica inclusa */
    if (ctx) llama_free(ctx);
    if (model) llama_model_free(model);
    llama_backend_free();
    free(tokens);
    free(grammar);
    free(prompt);
    return rc;
}
