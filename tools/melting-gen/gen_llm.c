#include "melting_gen.h"

#include "gen_inspire.h"
#include "gen_novelty.h"

#include "llama.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GEN_LLM_SESSION_N_CTX/N_BATCH: vedi melting_gen.h (spostate li' in fase 3b
   review perche' anche gen_lua.h, che non include llama.h, ha bisogno di
   GEN_LLM_SESSION_N_CTX per il ceiling byte del prompt Lua,
   GEN_LUA_PROMPT_BYTE_CEILING). */

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

/* M5 (DEC-005): il testo di sostituzione per {CHOSEN_THEME} in prompts/user.txt
 * riga 7 (vedi logs/m5-content-notes.md, sezione "Meccanismo di degradazione").
 * SEMPRE non-vuoto: il placeholder non deve mai restare nel prompt. 'buf' e'
 * del chiamante (dimensionato per il caso peggiore: raw + la frase fissa di
 * rinforzo). */
static void BuildChosenThemeText(const GenChosenTheme *chosen, char *buf, size_t bufSize)
{
    if (chosen && chosen->raw[0])
    {
        /* Il blurb della proposta (raw = "<name> -- <blurb>") a volte finisce
           gia' con un segno di fine frase (l'esempio di propose_system.txt
           ce l'ha; il prompt non lo vieta ne' lo impone) e a volte no: senza
           questo controllo un blurb gia' terminato produrrebbe "...singing..
           Stay inside" (doppio punto). Un solo punto garantito, sempre. */
        size_t len = strlen(chosen->raw);
        bool endsWithPunct = len > 0 &&
            (chosen->raw[len - 1] == '.' || chosen->raw[len - 1] == '!' || chosen->raw[len - 1] == '?');
        snprintf(buf, bufSize,
                 "%s%s Stay inside this world: do not invent a different one, only escalate it floor after floor.",
                 chosen->raw, endsWithPunct ? "" : ".");
    }
    else
    {
        snprintf(buf, bufSize,
                 "not chosen this time -- invent one yourself, in the same two-word place-plus-quality "
                 "shape used for every floor's theme below, and then treat your own invention exactly "
                 "like a chosen world for the rest of this section.");
    }
}

char *GenLlmBuildJsonPrompt(const char *promptsDir, unsigned int seed, const GenChosenTheme *chosen)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/system.txt", promptsDir);
    char *sys = GenReadFile(path);
    snprintf(path, sizeof(path), "%s/user.txt", promptsDir);
    char *user = GenReadFile(path);
    if (!sys || !user) { free(sys); free(user); return NULL; }

    char seedText[32];
    snprintf(seedText, sizeof(seedText), "%u", seed);
    char *userSeeded = GenReplaceAll(user, "{SEED}", seedText);
    free(user);
    if (!userSeeded) { free(sys); return NULL; }

    /* Semi d'ispirazione (roadmap 16/07/2026): 'seed' qui e' l'attemptSeed di
       chi chiama (seed + attempt*7919, vedi main.c), non il seed della run --
       quindi un retry riceve ispirazioni DIVERSE dal primo tentativo. E'
       voluto: se il primo giro produce un piano-fotocopia, il retry deve
       poter ripartire da un ancoraggio creativo diverso, non dallo stesso. */
    char inspire[1024];
    GenInspireBuild(seed, inspire, sizeof(inspire));
    char *userInspired = GenReplaceAll(userSeeded, "{ISPIRAZIONI}", inspire);
    free(userSeeded);
    if (!userInspired) { free(sys); return NULL; }

    /* Ledger di novita' fra RUN (gen_novelty.c, piano strategico "check contro
       le ultime ~20 run"): a differenza delle ispirazioni sopra, deterministiche
       sul seed, questo blocco dipende dalla STORIA su disco -- stesso elenco per
       ogni attemptSeed della stessa run (i due tentativi leggono lo stesso
       ledger, ed e' l'effetto giusto: il vocabolario da evitare non cambia a
       meta' run). Nessun blocco monco: elenco vuoto (ledger assente o nessuna
       parola ripetuta in almeno 2 delle ultime 20 run) = placeholder sostituito
       con la stringa vuota, mai con la frase introduttiva senza seguito. */
    char avoidWords[GEN_NOVELTY_AVOID_BUF_SIZE];
    GenNoveltyAvoidList(avoidWords, sizeof(avoidWords));
    char evita[GEN_NOVELTY_AVOID_BUF_SIZE + 128];
    if (avoidWords[0])
        snprintf(evita, sizeof(evita),
                 "Words already seen in your recent runs, do NOT use them (nor obvious derivatives): %s",
                 avoidWords);
    else evita[0] = '\0';
    char *userFinal0 = GenReplaceAll(userInspired, "{EVITA}", evita);
    free(userInspired);
    if (!userFinal0) { free(sys); return NULL; }

    /* M5 (DEC-005): {CHOSEN_THEME} SEMPRE sostituito (mai lasciato nel
       prompt, vedi BuildChosenThemeText sopra) -- il tema scelto dal
       giocatore (--theme-file) o il ramo di degrado "invent one yourself". */
    /* sizeof(...->raw): raw e' gia' il testo intero (name+" -- "+blurb), +128
       per la frase fissa di rinforzo (92 char) o il ramo di degrado piu'
       lungo (~230 char) -- margine ampio, verificato dal compilatore
       (-Wformat-truncation, niente troncamenti silenziosi qui). */
    char themeText[sizeof(((GenChosenTheme *)0)->raw) + 128];
    BuildChosenThemeText(chosen, themeText, sizeof(themeText));
    char *userFinal = GenReplaceAll(userFinal0, "{CHOSEN_THEME}", themeText);
    free(userFinal0);
    if (!userFinal) { free(sys); return NULL; }

    /* Esempi rotanti nel SYSTEM prompt (stesso seed delle ispirazioni sopra,
       stream RNG diverso -- vedi gen_inspire.c): l'esempio letterale di
       "room" ("Colonnato Sacro") era l'unico ancoraggio residuo dopo le
       ispirazioni, misurato duplicato x2 dalla A/B. NB: questi placeholder
       stanno nel SYSTEM prompt ('sys'), non nello USER ('userFinal'). */
    char esRoom[256], esNemici[512], esColpi[768];
    GenInspireExamples(seed, "room", esRoom, sizeof(esRoom));
    GenInspireExamples(seed, "enemies", esNemici, sizeof(esNemici));
    GenInspireExamples(seed, "shots", esColpi, sizeof(esColpi));
    char *sys1 = GenReplaceAll(sys, "{ESEMPIO_ROOM}", esRoom);
    free(sys);
    if (!sys1) { free(userFinal); return NULL; }
    char *sys2 = GenReplaceAll(sys1, "{ESEMPIO_NEMICI}", esNemici);
    free(sys1);
    if (!sys2) { free(userFinal); return NULL; }
    char *sysFinal = GenReplaceAll(sys2, "{ESEMPIO_COLPI}", esColpi);
    free(sys2);
    if (!sysFinal) { free(userFinal); return NULL; }

    char *prompt = GenChatMlWrap(sysFinal, userFinal);
    free(sysFinal);
    free(userFinal);
    return prompt;
}

/* M5: gemello di GenLlmBuildJsonPrompt per --propose-themes -- stesso schema
 * di placeholder (SEED/ISPIRAZIONI/EVITA) ma su propose_system.txt/
 * propose_user.txt, e SENZA {ESEMPIO_*}: quel prompt e' minuscolo (nPredict
 * ~320, 3 coppie nome+blurb), un solo esempio marcato "do NOT copy it" nel
 * system prompt gia' basta (vedi logs/m5-content-notes.md, sezione (a)). */
char *GenLlmBuildProposePrompt(const char *promptsDir, unsigned int seed)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/propose_system.txt", promptsDir);
    char *sys = GenReadFile(path);
    snprintf(path, sizeof(path), "%s/propose_user.txt", promptsDir);
    char *user = GenReadFile(path);
    if (!sys || !user) { free(sys); free(user); return NULL; }

    char seedText[32];
    snprintf(seedText, sizeof(seedText), "%u", seed);
    char *userSeeded = GenReplaceAll(user, "{SEED}", seedText);
    free(user);
    if (!userSeeded) { free(sys); return NULL; }

    char inspire[1024];
    GenInspireBuild(seed, inspire, sizeof(inspire));
    char *userInspired = GenReplaceAll(userSeeded, "{ISPIRAZIONI}", inspire);
    free(userSeeded);
    if (!userInspired) { free(sys); return NULL; }

    char avoidWords[GEN_NOVELTY_AVOID_BUF_SIZE];
    GenNoveltyAvoidList(avoidWords, sizeof(avoidWords));
    char evita[GEN_NOVELTY_AVOID_BUF_SIZE + 128];
    if (avoidWords[0])
        snprintf(evita, sizeof(evita),
                 "Words already seen in your recent runs, do NOT use them (nor obvious derivatives): %s",
                 avoidWords);
    else evita[0] = '\0';
    char *userFinal = GenReplaceAll(userInspired, "{EVITA}", evita);
    free(userInspired);
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

/* Corpo del ciclo campiona-un-token condiviso da GenLlmComplete e
   GenLlmCompleteFromPrefix (fase 3b step B1): l'unica differenza fra le due
   chiamate e' COSA c'e' gia' nella KV cache e quale batch va decodificato
   PER PRIMO (l'intero prompt in un caso, solo il suffisso nell'altro) -- da
   quel primo llama_decode() in poi il ciclo campiona-token-per-token e' lo
   stesso, quindi vive qui una volta sola invece che duplicato. 'batch' e'
   gia' pronto per il PRIMO llama_decode() (chi chiama l'ha costruito con
   llama_batch_get_one sui token del prompt/suffisso); 'nDecodedFirst' e' solo
   per il log (quanti token nuovi quel primo giro ha davvero decodificato:
   n_prompt intero per GenLlmComplete, solo n_suffix per
   GenLlmCompleteFromPrefix, visto che il prefisso in quel caso e' gia' in
   cache da prima e non viene ridecodificato). */
static int GenLlmSampleLoop(GenLlmSession *sess, struct llama_sampler *smpl, struct llama_batch batch,
                             int nDecodedFirst, int nPredict,
                             const char *outDir, const char *progressPhase, int progressBase, int progressSpan,
                             char *out, size_t outCap, int *tokensOut)
{
    int rc = 0;
    size_t used = 0;
    int generated = 0;
    llama_token newToken = 0;
    /* Strumentazione (step B1, diagnosi tempi di caricamento): il PRIMO giro
       del ciclo sotto e' quello che decodifica il prompt/suffisso passato in
       'batch' (il "riprocessamento del prefisso condiviso" indagato dal task
       brief, quando e' GenLlmComplete a chiamare con l'intero prompt); i
       giri successivi decodificano un token alla volta (la generazione vera
       e propria). tPromptDone segna la fine di quel primo giro, cosi' il log
       qui sotto separa i due costi invece di dare un unico tempo totale che
       li confonde. */
    double tCallStart = GenNowSeconds();
    double tPromptDone = -1.0;
    while (generated < nPredict)
    {
        if (llama_decode(sess->ctx, batch) != 0)
        {
            GenLogLine("llm: llama_decode fallita al token %d", generated);
            rc = -1;
            break;
        }
        if (tPromptDone < 0.0) tPromptDone = GenNowSeconds();
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
    double tEnd = GenNowSeconds();
    double promptSecs = (tPromptDone >= 0.0) ? tPromptDone - tCallStart : tEnd - tCallStart;
    double genSecs = (tPromptDone >= 0.0) ? tEnd - tPromptDone : 0.0;
    GenLogLine("llm: completamento fase=%s prompt=%d token generati=%d token -- prompt %.2fs, generazione %.2fs (totale %.2fs)",
               progressPhase ? progressPhase : "?", nDecodedFirst, generated, promptSecs, genSecs, tEnd - tCallStart);
    return rc;
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
    struct llama_batch batch = llama_batch_get_one(tokens, n_prompt);
    int rc = GenLlmSampleLoop(sess, smpl, batch, n_prompt, nPredict,
                               outDir, progressPhase, progressBase, progressSpan,
                               out, outCap, tokensOut);

    llama_sampler_free(smpl);
    free(tokens);
    return rc;
}

int GenLlmPrefixPrime(GenLlmSession *sess, const char *prefixPrompt, int *nPrefixOut)
{
    if (nPrefixOut) *nPrefixOut = 0;
    if (!sess || !sess->ctx || !prefixPrompt) return -1;

    /* Stesso azzeramento di GenLlmComplete: e' comunque l'inizio di una
       conversazione nuova (la prima delle 20 di questa fase Lua). Da qui in
       poi pero' NESSUNA chiamata successiva (GenLlmCompleteFromPrefix)
       azzera piu' la cache: e' proprio il punto, vedi il commento sopra
       GenLlmCompleteFromPrefix piu' sotto. */
    llama_memory_clear(llama_get_memory(sess->ctx), true);

    int n_prefix = -llama_tokenize(sess->vocab, prefixPrompt, (int32_t)strlen(prefixPrompt), NULL, 0, true, true);
    if (n_prefix <= 0) return -1;
    if ((uint32_t)n_prefix >= llama_n_ctx(sess->ctx))
    {
        GenLogLine("llm: prefisso Lua condiviso (%d token) supera da solo n_ctx della sessione (%u)",
                   n_prefix, llama_n_ctx(sess->ctx));
        return -1;
    }
    llama_token *tokens = malloc(sizeof(llama_token)*(size_t)n_prefix);
    if (!tokens) return -1;
    if (llama_tokenize(sess->vocab, prefixPrompt, (int32_t)strlen(prefixPrompt), tokens, n_prefix, true, true) < 0)
    {
        free(tokens);
        return -1;
    }

    double t0 = GenNowSeconds();
    struct llama_batch batch = llama_batch_get_one(tokens, n_prefix);
    int rc = llama_decode(sess->ctx, batch);
    free(tokens);
    if (rc != 0)
    {
        GenLogLine("llm: decodifica del prefisso Lua condiviso fallita (%d token)", n_prefix);
        return -1;
    }
    GenLogLine("llm: prefisso Lua condiviso decodificato una volta sola: %d token in %.2fs (riusato per ogni oggetto, vedi GenLlmCompleteFromPrefix)",
               n_prefix, GenNowSeconds() - t0);
    if (nPrefixOut) *nPrefixOut = n_prefix;
    return 0;
}

int GenLlmCompleteFromPrefix(GenLlmSession *sess, int nPrefix, const char *suffix,
                              int nPredict, float temp, unsigned int seed,
                              const char *outDir, const char *progressPhase, int progressBase, int progressSpan,
                              char *out, size_t outCap, int *tokensOut)
{
    out[0] = '\0';
    if (tokensOut) *tokensOut = 0;
    if (!sess || !sess->ctx || !suffix || nPrefix <= 0) return -1;

    /* NIENTE llama_memory_clear qui (a differenza di GenLlmComplete): la KV
       cache contiene gia' 'nPrefix' token dal GenLlmPrefixPrime iniziale (o
       dal GenLlmRewindToPrefix dell'oggetto precedente, che riporta la
       sessione esattamente allo stesso stato). add_special=false perche'
       'suffix' e' la CONTINUAZIONE di quella sequenza, non l'inizio: un
       secondo BOS qui produrrebbe una tokenizzazione diversa da quella che
       llama_tokenize(prefisso+suffix, add_special=true) avrebbe dato tutta
       insieme (verificato con test-tokenizer-0 sul vocabolario di
       Qwen2.5-Coder: tokenize(prefisso, add_special=true) ++
       tokenize(suffix, add_special=false) == tokenize(prefisso+suffix,
       add_special=true), token per token, sul prompt Lua vero). */
    int n_suffix = -llama_tokenize(sess->vocab, suffix, (int32_t)strlen(suffix), NULL, 0, false, true);
    if (n_suffix <= 0) return -1;
    if ((uint32_t)nPrefix + (uint32_t)n_suffix + (uint32_t)nPredict > llama_n_ctx(sess->ctx))
    {
        GenLogLine("llm: prefisso+suffix+nPredict (%d+%d+%d) supera n_ctx della sessione (%u)",
                   nPrefix, n_suffix, nPredict, llama_n_ctx(sess->ctx));
        return -1;
    }
    llama_token *tokens = malloc(sizeof(llama_token)*(size_t)n_suffix);
    if (!tokens) return -1;
    if (llama_tokenize(sess->vocab, suffix, (int32_t)strlen(suffix), tokens, n_suffix, false, true) < 0)
    {
        free(tokens);
        return -1;
    }

    /* Stesso sampler chain di GenLlmComplete sul percorso Lua: MAI una
       grammatica (spec sezione 6, script Lua troppo vari per un GBNF
       utile), penalita'+temp+dist. Nuovo ad ogni chiamata, come prima: la
       penalita' sulle ripetizioni deve vedere solo i token di QUESTO
       tentativo, mai quelli di un oggetto precedente. */
    struct llama_sampler *smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl, llama_sampler_init_penalties(
        GEN_PENALTY_LAST_N, GEN_PENALTY_REPEAT, 0.0f, 0.0f));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(temp));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(seed));

    if (outDir && progressPhase) GenProgressWrite(outDir, progressPhase, progressBase, progressPhase);
    struct llama_batch batch = llama_batch_get_one(tokens, n_suffix);
    int rc = GenLlmSampleLoop(sess, smpl, batch, n_suffix, nPredict,
                               outDir, progressPhase, progressBase, progressSpan,
                               out, outCap, tokensOut);

    llama_sampler_free(smpl);
    free(tokens);
    return rc;
}

void GenLlmRewindToPrefix(GenLlmSession *sess, int nPrefix)
{
    if (!sess || !sess->ctx || nPrefix <= 0) return;
    /* Rimuove dalla sequenza 0 tutto cio' che sta dalla posizione nPrefix in
       poi (p1=-1 = "fino a infinito", vedi llama.h): riporta la KV cache
       esattamente allo stato appena dopo GenLlmPrefixPrime, pronta per il
       prossimo oggetto. */
    llama_memory_seq_rm(llama_get_memory(sess->ctx), 0, nPrefix, -1);
}
