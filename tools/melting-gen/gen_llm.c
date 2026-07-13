#include "melting_gen.h"

#include "llama.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* n_ctx/n_batch della sessione condivisa (fase 3a-L3): fissi per l'intero
   processo, devono coprire la chiamata piu' grande fra le due che la
   sessione serve. JSON: prompt di poche centinaia di token + nPredict fino a
   2048 (args.nPredict di default). Lua: prompt piu' grande (cheat-sheet +
   few-shot, vedi tools/melting-gen/prompts/lua_system.txt) ma nPredict molto
   piu' corto (GEN_LUA_N_PREDICT in gen_lua.c). 4096 tiene comodamente
   entrambe con margine.

   n_batch = n_ctx (non piu' 2048, fase 3): GenLlmComplete sottomette l'intero
   prompt in un colpo solo con llama_batch_get_one (vedi sotto), e
   llama_decode ha un'asserzione interna "n_tokens_all <= cparams.n_batch"
   che NON e' la stessa cosa del controllo "prompt+nPredict <= n_ctx" gia'
   presente in GenLlmComplete (quello logga un errore e ritorna -1, questo
   fa un ggml_abort() e uccide l'intero processo). Scoperto in fase 3
   (docs/superpowers/sdd/phase3-items-report.md): il cheat-sheet Lua e' cresciuto
   (tavolozza di archetipi per gli oggetti attivi + il prompt dedicato agli
   oggetti stat-up) fino a superare 2048 token pur restando ben sotto n_ctx,
   e la sessione condivisa e' andata in crash A META' RUN (dopo il JSON,
   durante il primo item Lua), perdendo l'intera generazione. n_batch = n_ctx
   rende il controllo "prompt+nPredict <= n_ctx" gia' esistente l'UNICO
   vincolo binding: qualunque prompt che lo supera degrada gia' con un
   log+fallback (mai un crash), qualunque prompt che lo rispetta non puo'
   piu' far scattare l'asserzione di llama_decode. Il costo in VRAM e'
   trascurabile: il buffer di calcolo riservato da llama.cpp e' dimensionato
   su n_ubatch (default 512, non toccato qui), non su n_batch. */
#define GEN_LLM_SESSION_N_CTX   4096
#define GEN_LLM_SESSION_N_BATCH GEN_LLM_SESSION_N_CTX

struct GenLlmSession {
    struct llama_model *model;
    struct llama_context *ctx;
    const struct llama_vocab *vocab;
};

static bool LoadProgressCb(float progress, void *user)
{
    const char *outDir = user;
    int pct = (int)(progress*60.0f);   /* caricamento modello = 0..60% della barra */
    GenProgressWrite(outDir, "carico-modello", pct, "carico il modello (Vulkan)");
    return true;   /* false interromperebbe il caricamento */
}

char *GenLlmBuildJsonPrompt(const char *promptsDir, unsigned int seed)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/system.txt", promptsDir);
    char *sys = GenReadFile(path);
    snprintf(path, sizeof(path), "%s/user.txt", promptsDir);
    char *user = GenReadFile(path);
    if (!sys || !user) { free(sys); free(user); return NULL; }

    char seedText[32];
    snprintf(seedText, sizeof(seedText), "%u", seed);
    char *userFinal = GenReplaceAll(user, "{SEED}", seedText);
    free(user);
    if (!userFinal) { free(sys); return NULL; }

    char *prompt = GenChatMlWrap(sys, userFinal);
    free(sys);
    free(userFinal);
    return prompt;
}

GenLlmSession *GenLlmSessionOpen(const char *modelPath, int nGpuLayers, const char *outDir)
{
    GenLlmSession *sess = calloc(1, sizeof *sess);
    if (!sess) return NULL;

    llama_backend_init();

    double t0 = GenNowSeconds();
    struct llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = nGpuLayers;
    mparams.progress_callback = LoadProgressCb;
    mparams.progress_callback_user_data = (void *)outDir;
    sess->model = llama_model_load_from_file(modelPath, mparams);
    if (!sess->model)
    {
        GenLogLine("llm: caricamento fallito: %s (ngl=%d)", modelPath, nGpuLayers);
        llama_backend_free();
        free(sess);
        return NULL;
    }
    sess->vocab = llama_model_get_vocab(sess->model);

    struct llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = GEN_LLM_SESSION_N_CTX;
    cparams.n_batch = GEN_LLM_SESSION_N_BATCH;
    /* Deviazione dal testo del brief originale ("non impostare affatto
       flash-attn"): a b9979 il default di llama_context_default_params() e'
       AUTO, non "spento". Su RDNA1 la flash-attention Vulkan collassa le
       prestazioni (misurato nel primo giro di test-llm): la si disabilita
       esplicitamente, come gia' faceva la versione precedente di questo
       file per il solo percorso JSON. */
    cparams.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_DISABLED;
    sess->ctx = llama_init_from_model(sess->model, cparams);
    if (!sess->ctx)
    {
        GenLogLine("llm: creazione contesto fallita (n_ctx=%d)", GEN_LLM_SESSION_N_CTX);
        llama_model_free(sess->model);
        llama_backend_free();
        free(sess);
        return NULL;
    }
    GenLogLine("llm: modello caricato in %.1fs (%s, ngl=%d)", GenNowSeconds() - t0, modelPath, nGpuLayers);
    return sess;
}

void GenLlmSessionClose(GenLlmSession *sess)
{
    if (!sess) return;
    if (sess->ctx) llama_free(sess->ctx);
    if (sess->model) llama_model_free(sess->model);
    llama_backend_free();
    free(sess);
}

int GenLlmComplete(GenLlmSession *sess, const char *prompt, const char *grammarText,
                    int nPredict, float temp, unsigned int seed,
                    const char *outDir, const char *progressPhase, int progressBase, int progressSpan,
                    char *out, size_t outCap, int *tokensOut)
{
    out[0] = '\0';
    if (tokensOut) *tokensOut = 0;
    if (!sess || !sess->ctx || !prompt) return -1;

    /* Ogni chiamata e' una conversazione a se': azzera la cache KV (quindi
       anche la posizione di sequenza, che riparte da 0) cosi' QUESTO prompt
       non si trova mai concatenato a quanto generato da una chiamata
       precedente sulla stessa sessione condivisa (vedi il commento in
       melting_gen.h su GenLlmSession). */
    llama_memory_clear(llama_get_memory(sess->ctx), true);

    int n_prompt = -llama_tokenize(sess->vocab, prompt, (int32_t)strlen(prompt), NULL, 0, true, true);
    if (n_prompt <= 0) return -1;
    if ((uint32_t)n_prompt + (uint32_t)nPredict > llama_n_ctx(sess->ctx))
    {
        GenLogLine("llm: prompt+nPredict (%d+%d) supera n_ctx della sessione (%u)",
                   n_prompt, nPredict, llama_n_ctx(sess->ctx));
        return -1;
    }
    llama_token *tokens = malloc(sizeof(llama_token)*(size_t)n_prompt);
    if (!tokens) return -1;
    if (llama_tokenize(sess->vocab, prompt, (int32_t)strlen(prompt), tokens, n_prompt, true, true) < 0)
    {
        free(tokens);
        return -1;
    }

    struct llama_sampler *smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    if (grammarText)
    {
        struct llama_sampler *gsmpl = llama_sampler_init_grammar(sess->vocab, grammarText, "root");
        if (!gsmpl)
        {
            GenLogLine("llm: grammatica GBNF non parsabile");
            llama_sampler_free(smpl);
            free(tokens);
            return -1;
        }
        llama_sampler_chain_add(smpl, gsmpl);   /* la grammatica PRIMA del selettore */
    }
    /* Penalita' sulle ripetizioni: senza, il modello riusa gli stessi nomi/
       pattern fra piani o fra oggetti diversi. Quando c'e' una grammatica
       puo' solo spostare le probabilita' fra scelte gia' legali; nel
       percorso Lua (senza grammatica) e' l'unica difesa contro le ripetizioni
       oltre alla temperatura. */
    llama_sampler_chain_add(smpl, llama_sampler_init_penalties(
        GEN_PENALTY_LAST_N, GEN_PENALTY_REPEAT, 0.0f, 0.0f));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(temp));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(seed));

    if (outDir && progressPhase) GenProgressWrite(outDir, progressPhase, progressBase, progressPhase);
    int rc = 0;
    size_t used = 0;
    int generated = 0;
    struct llama_batch batch = llama_batch_get_one(tokens, n_prompt);
    llama_token newToken = 0;
    while (generated < nPredict)
    {
        if (llama_decode(sess->ctx, batch) != 0)
        {
            GenLogLine("llm: llama_decode fallita al token %d", generated);
            rc = -1;
            break;
        }
        newToken = llama_sampler_sample(smpl, sess->ctx, -1);
        if (llama_vocab_is_eog(sess->vocab, newToken)) break;
        char piece[128];
        int n = llama_token_to_piece(sess->vocab, newToken, piece, sizeof(piece), 0, true);
        if (n < 0 || used + (size_t)n + 1 >= outCap)
        {
            rc = -1;
            break;
        }
        memcpy(out + used, piece, (size_t)n);
        used += (size_t)n;
        out[used] = '\0';
        generated++;
        if (outDir && progressPhase && generated%16 == 0)
        {
            int pct = progressBase + (int)((double)progressSpan*generated/nPredict);
            char msg[112];
            snprintf(msg, sizeof(msg), "%s (%d token)", progressPhase, generated);
            GenProgressWrite(outDir, progressPhase, pct > progressBase + progressSpan ? progressBase + progressSpan : pct, msg);
        }
        batch = llama_batch_get_one(&newToken, 1);
    }
    if (tokensOut) *tokensOut = generated;

    llama_sampler_free(smpl);
    free(tokens);
    return rc;
}
