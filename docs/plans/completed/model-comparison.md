---
id: plans-model-comparison
title: Piano — suite di comparazione dei modelli candidati (testo, immagini, audio) — COMPLETATO 2026-07-24
domain: plans
status: implemented
authority: supporting
owner: ai-production
summary: >-
  Obiettivo, metodo e candidati della suite che confronta modelli alternativi ai
  default di produzione nei tre domini (testo GGUF, immagine SD, audio Stable Audio
  Open Small) su throughput/qualità/artefatti, con soglie di accettabilità, struttura
  degli output e istruzioni per rieseguirla. Chiuso con giudizio finale il 24/07/2026
  (gen-2 immagini): nessun modello moderno batte SD1.5 su 6 GB di VRAM.
last_reviewed: 2026-07-27
last_verified_commit: d30890b
topics: [modelli, benchmark, gguf, llama.cpp, stable-diffusion, stable-audio, comparazione, vram]
related: [eng-benchmarks, aiprod-audio-generation-pipeline, aiprod-regole-agenti-ml]
supersedes: []
source_files: [scripts/model-comparison.sh, scripts/model_comparison_report.py, scripts/download-comparison-models.sh, scripts/image-comparison.sh, scripts/audio-benchmark.sh, scripts/audio_benchmark.py, tools/melting-gen/main.c, tools/melting-sprites/main.c]
---

# Piano — suite di comparazione dei modelli candidati (testo, immagini, audio)

> **Piano completato (DEC-157), spostato da `plans/active/` a `plans/completed/` nello
> stesso commit del giudizio.** Giudizio finale il **24/07/2026** (sezione "Estensione
> gen-2" più sotto): testo → Gemma-3-4B-IT Q4 promosso a default (DEC-140); immagini →
> nessun modello moderno (SD3.5/SDXL/Flux) batte SD1.5 su 6 GB di VRAM, il default resta
> SD1.5 (confermato poi da DEC-148); audio → Stable Audio Open Small sbloccato ed eseguito
> (vedi "Aggiornamento 23/07 sera"). Il documento resta la registrazione tecnica completa
> della suite: nessuna riscrittura, solo lo spostamento e questa nota di chiusura.

## Obiettivo

Estensione della missione (decisione dell'utente, sessione 22/07/2026, messaggio di
coordinamento in corso d'opera): la suite copre **ogni dominio di generazione AI del
progetto**, non solo il testo. Per ciascuno, la domanda è la stessa:

1. un modello **più piccolo/veloce/diverso** del default di produzione regge le stesse
   soglie di qualità sull'hardware di riferimento (RX 5600 XT 6 GB, Vulkan)?
2. c'è un segnale MISURATO (non solo impressione a occhio) che giustifichi un cambio?

**Testo**: il 7B Coder Q4_K_M (`qwen2.5-coder-7b-instruct-q4_k_m.gguf`) è il modello di
produzione di `tools/melting-gen` (default in `main.c`, misurato in
`docs/engineering/benchmarks.md`).
**Immagini**: il checkpoint pixel-art `Public-Prompts-Pixel-Model.ckpt` è la baseline di
`tools/melting-sprites` (nessuna LoRA di stile ancora — modello provvisorio, vedi memoria
"Modello immagini provvisorio" — questo è il confronto BASELINE, non un giudizio finale).
**Audio**: DEC-109/DEC-113 (22/07/2026) hanno appena ADOTTATO Stable Audio Open Small come
via primaria (fallback rFXGen → curato); questo piano ne è il **primo prototipo di
benchmark**, nessun runtime audio esiste ancora nel repo.

Questo piano **non cambia** alcun default di produzione da solo: registra la misura. Un
cambio di modello richiede una decisione a parte (`docs/design/governance/decision-log.md`),
non implicita in un report tecnico. Regole ML vincolanti per questo lavoro:
`docs/ai-production/regole-agenti-ml.md` (niente training, niente pubblicazione di pesi,
licenze verificate alla revisione corrente, non solo per sentito dire).

## Struttura degli output (unificata sui tre domini)

```text
logs/model-comparison/<timestamp>/
├── report.md / report.csv        # testo (scripts/model-comparison.sh)
├── <modello-testo>/...           # manifest/corpus/log per modello di testo
├── images/
│   └── <modello-immagine>/       # atlas PNG + index.txt + bench.log per modello
└── audio/
    └── stable-audio-open-small/  # .wav + index.json (SOLO se sbloccato, vedi sotto)
```

Ogni run della suite (testo o immagini) produce una cartella `<timestamp>` NUOVA sotto
`logs/model-comparison/`; l'audio (quando sarà eseguibile) scrive nella stessa cartella
`<timestamp>` se lanciato a ridosso, altrimenti nella sua. `logs/` non è versionato.

## Regola ferrea di sequenza

**Mai due processi GPU insieme** (6 GB VRAM di riferimento): testo (`melting-gen`) e
immagini (`melting-sprites`) non girano mai in parallelo, nello stesso ordine di
`scripts/benchmark.sh` e del vincolo runtime in `src/app/app.c`. Ordine di questa suite:
**testo → immagini → audio**. L'audio è CPU-only per costruzione (niente ROCm per torch
sulla RX 5600 XT) e può girare in parallelo a testo/immagini senza toccare la VRAM — ma
non va lanciato mentre si misura un tempo "pulito" (tok/s o img/s a regime), perché la CPU
è condivisa e un carico concorrente sposta quei numeri.

## Metodo — testo

Harness a due fasi, mai eseguite dentro `make test` (troppo lunghe):

1. **Download + controllo di caricamento** (`scripts/download-comparison-models.sh`,
   poi la fase 1 di `scripts/model-comparison.sh`): ogni modello viene aperto con
   `bin/melting-gen --bench --model <file>` PRIMA di qualunque run vera. Fallisce
   in modo pulito e resta nel report come "non carica" (architettura non
   supportata dal tag llama.cpp pinnato, `deps/llama.cpp` tag `b9979` — vedi
   `scripts/setup-deps.sh` — o VRAM insufficiente a `n_ctx=8192` fisso,
   `tools/melting-gen/melting_gen.h:GEN_LLM_SESSION_N_CTX`): NON è un errore
   dell'harness, è un esito legittimo da riportare.
2. **Generazioni vere** (`scripts/model-comparison.sh`, fase 2): per ogni modello
   che carica, 3 run complete (`bin/melting-gen --model <file> --seed <seed>`) sugli
   STESSI 3 seed di `scripts/gen-metrics.sh` (base 4242, passo 101: 4242, 4343,
   4444) — comparabilità diretta con le misure di varietà già esistenti sul 7B.
   Ogni run scrive `manifest-<seed>.json` + `corpus-<seed>.jsonl` nello stesso
   schema che `scripts/gen_metrics.py` già sa leggere;
   `scripts/model_comparison_report.py` lo importa come modulo invece di
   duplicarne la logica.

I benchmark GPU girano **sempre in sequenza**, mai due modelli in VRAM insieme
(stesso vincolo di `scripts/benchmark.sh` e di `src/app/app.c` per
testo+sprite).

### Metriche misurate (per modello)

- **tok/s**: dal `--bench` (prompt fisso, 128 token, seed fisso — misura la
  macchina/il modello, non il contenuto).
- **tempo totale per run completa**: tempo di parete di `bin/melting-gen`
  (JSON + 15 script Lua), media sulle run riuscite.
- **validità JSON al primo colpo**: percentuale di run in cui il JSON è
  parsabile al PRIMO tentativo (`kind="manifest"` nel corpus, `attempt=1`,
  `ok=true`); nessun ripiego procedurale nel campione è un requisito separato
  (vedi soglie sotto).
- **validità Lua**: riusa `gen_metrics.lua_stats` — primo colpo / dopo retry /
  nessun comportamento dichiarato / ripiegato sulla mini-VM, sui 15 oggetti
  per run.
- **varietà**: riusa `gen_metrics.category_values` — jaccard medio fra le 3 run
  dello STESSO modello su temi/colpi/nemici/boss/stanze/oggetti (0 = nessuna
  sovrapposizione, 1 = run fotocopia).
- **aderenza al tema**: euristica di sovrapposizione lessicale (non semantica —
  nessun giudice LLM in questo harness) fra le parole-contenuto del tema di un
  piano e i nomi generati in quel piano.
- **inglese corretto**: guardia automatica (stesso pattern di
  `scripts/test-llm.sh`, parole-funzione italiane a confine di parola) +
  campione testuale per il giudizio a occhio, che resta necessario (la lingua
  non si riduce a un numero, vedi `scripts/gen_metrics.py`).
- **VRAM**: annotato il peso del file (proxy grezzo, non il consumo Vulkan reale
  — quello richiederebbe strumentazione del processo, fuori scope); un
  fallimento di caricamento è già di per sé la misura che conta ("non sta nei
  6 GB con n_ctx 8192").

### Soglie di accettabilità

- Lua valido (primo colpo + dopo retry) **>= 70%**;
- JSON valido al primo tentativo su **tutte** le run campionate (zero ripieghi
  procedurali nel campione);
- **zero** run con piani fotocopia (< 5 temi distinti su 5 piani, stessa
  guardia anti-fotocopia di `scripts/test-llm.sh`).

Un modello che supera tutte e tre le soglie è "accettabile". Il giudizio
automatico in coda al report (`scripts/model_comparison_report.py`) sceglie fra
gli accettabili il più piccolo per dimensione file, e calcola un punteggio
composito (Lua primo colpo 35%, JSON primo colpo 25%, varietà 25%, aderenza al
tema 15% — pesi documentati nel codice, non un numero magico nascosto) per
"migliore complessivo" e "migliore rapporto qualità/dimensione".

## Candidati — testo

Lista curata (sessione 22/07/2026) + verifica URL/licenza fatta a mano prima del
download (`curl -sIL`, HTTP 200 richiesto):

| Modello | Repo GGUF usato | Quant | Note |
|---|---|---|---|
| Qwen2.5-Coder-3B-Instruct | `Qwen/Qwen2.5-Coder-3B-Instruct-GGUF` | Q4_K_M, Q8_0 | licenza `qwen-research` (non commerciale) — diversa dal 7B/1.5B Coder (Apache 2.0), valutazione tecnica soltanto |
| Qwen3-4B-Instruct-2507 | `bartowski/Qwen_Qwen3-4B-Instruct-2507-GGUF` | Q4_K_M | repo ufficiale `Qwen/Qwen3-4B-Instruct-2507-GGUF` indicato nel task NON esiste (HTTP 401 sull'intero repo, non solo sul file) — quant bartowski, stessa licenza Apache 2.0 del modello base |
| Phi-4-mini-instruct | `bartowski/microsoft_Phi-4-mini-instruct-GGUF` | Q4_K_M | repo bartowski indicato nel task ("Phi-4-mini-instruct-GGUF") ha nome diverso da quello vero (bartowski prefissa sempre con l'org del modello base) — licenza MIT |
| Gemma-3-4b-it | `ggml-org/gemma-3-4b-it-GGUF` | Q4_K_M | repo ufficiale del team llama.cpp/Google — licenza Gemma (permissiva, restrizioni d'uso proprie, non Apache/MIT) |
| Qwen2.5-Coder-7B-Instruct | `Qwen/Qwen2.5-Coder-7B-Instruct-GGUF` | Q5_K_M, Q6_K | stesso repo del Q4_K_M di produzione, Apache 2.0 |
| Qwen2.5-Coder-1.5B-Instruct | `Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF` | Q8_0 | stesso repo del Q4_K_M già in `models/`, Apache 2.0 |

Tutti e sei i repo/URL sono stati verificati (`curl -sIL`, HTTP 200) e gli
hash sha256 presi dall'API HuggingFace (`?blobs=true`) prima del download —
vedi `scripts/download-comparison-models.sh` per URL/sha256/note di licenza
complete. Nessun candidato è stato scartato per assenza di GGUF compatibile.

Supporto architetture nel tag llama.cpp pinnato (`b9979`, `deps/llama.cpp`):
verificato PRIMA del download che `qwen3`/`gemma3`/`phi3` esistano in
`src/llama-arch.cpp` (il rischio indicato dal task — "i modelli non-Coder/nuovi
potrebbero non essere supportati" — non si è materializzato per nessuno dei
quattro candidati nuovi).

## Metodo — immagini

`scripts/image-comparison.sh` generalizza **scripts/sprite-baseline.sh a più modelli**,
tenendo lo script originale invariato (resta il metro di paragone "Esperimento 0" a UN
modello per le LoRA future — non toccarlo). Stesso meccanismo per ogni candidato: le 15
coppie tema|stile CONGELATE di `docs/ai-production/dataset/baseline-prompts.txt` × i seed
fissi di `sprite-baseline.sh` (5, 17), un mini-manifest `current_run.txt` per coppia/seed,
`bin/melting-sprites` a generare l'atlas. Prima di ogni candidato: controllo di
caricamento + "s/immagine a regime" via `--bench` (scarta il warmup Vulkan, stesso
meccanismo di `RunBench` in `tools/melting-sprites/main.c`).

### Metriche misurate (per modello immagine)

- **s/immagine a regime**: dal `--bench` (seconda immagine, la prima paga il warmup).
- **VRAM**: peso del file checkpoint (stesso proxy grezzo del testo).
- **% celle scartate dal quality gate**: da `logs/melting-sprites.log`, riga
  `run: ... celle=N scartate=M ...` (`tools/melting-sprites/main.c:528`), per ogni atlas.
- **dimensione file**, **LoRA usata o no** (LCM-LoRA SD1.5 di default dove compatibile,
  disattivata esplicitamente sui checkpoint non-SD1.5).
- Nessuna metrica di "qualità estetica" automatica: gli atlas PNG sono l'artefatto, il
  giudizio resta umano (vedi cartella `images/<modello>/` nell'output).

## Candidati — immagini

| Modello | Repo | Formato | Note |
|---|---|---|---|
| Public-Prompts-Pixel-Model (baseline attuale) | `PublicPrompts/All-In-One-Pixel-Model` | .ckpt | già in `models/`, CreativeML OpenRAIL-M — il metro di paragone |
| SD1.5 vanilla | `stable-diffusion-v1-5/stable-diffusion-v1-5` | .safetensors | il repo storico `runwayml/stable-diffusion-v1-5` è stato ritirato da HuggingFace — questo è il mirror ufficiale di continuazione, non gated; CreativeML OpenRAIL-M |
| SD_PixelArt_SpriteSheet_Generator (secondo fine-tune pixel-art) | `Onodofthenorth/SD_PixelArt_SpriteSheet_Generator` | .ckpt | già citato come alternativa in `scripts/download-models.sh`/`models/README.md` ("leggermente peggiore in resa"); Apache 2.0, non gated |
| SD3.5-Medium GGUF | `city96/stable-diffusion-3.5-medium-gguf` (diffusion model) | GGUF | **build lo supporta** (SD3 + caricamento GGUF presenti in `deps/stable-diffusion.cpp` tag `master-775-b5d8120`, verificato in `src/model_loader.cpp`/`gguf_io.cpp`); diffusion model Q4_K_M non gated (1.79 GB) e feasibile senza T5-XXL (~3.6 GB totali con CLIP-L+CLIP-G+VAE, sotto i 6 GB) — MA il repo base `stabilityai/stable-diffusion-3.5-medium` (serve per CLIP-G) è **gated**, e il mirror ungated di CLIP-G non è stato trovato in tempi ragionevoli: **non eseguito**, annotato qui invece di uno pseudo-tentativo incompleto |

Tre modelli immagine caricano/sono eseguibili con l'harness (baseline, SD1.5 vanilla,
secondo fine-tune pixel-art); SD3.5-Medium resta un candidato **documentato ma non
eseguito** per il motivo sopra — sblocco: ottenere CLIP-G di SD3.5 da un mirror non gated
legittimo, o accettare la licenza Stability sul repo base con un token utente.

## Metodo — audio

**Nessun runtime audio esiste ancora nel repo**: questo è il PRIMO prototipo
dell'integrazione DEC-109/DEC-113, isolato in un venv Python (`~/venvs/stable-audio`,
CPU-only — niente ROCm per torch sulla RX 5600 XT), MAI linkato a un binario C
(AGENTS.md: il gioco non linka mai un runtime Python). `scripts/audio-benchmark.sh`
controlla le precondizioni e chiama `scripts/audio_benchmark.py` (8 prompt fissi coerenti
col gioco — 5 SFX: fusione completata, danno subito, stanza completata, oggetto raccolto,
boss nuova fase; 3 musica: piano fantasy-dark, Piano 0/crogiolo, ambiente caverna — × 2
seed fissi), misurando s/clip e RAM di picco, salvando i `.wav` per la revisione umana.

### Stato al 23/07/2026: BLOCCATO su due fronti indipendenti

1. **Gate HuggingFace**: `stabilityai/stable-audio-open-small` è gated (`"gated": "auto"`,
   verificato con `curl` → HTTP 401 senza token). Sblocco: (a) accettare la licenza su
   <https://huggingface.co/stabilityai/stable-audio-open-small> con un account HuggingFace
   (Stability AI Community License — DEC-113 l'ha già accettata **a livello di progetto**,
   ma l'accettazione va comunque fatta dall'account che scarica); (b) creare un token in
   <https://huggingface.co/settings/tokens>; (c) `export HF_TOKEN=hf_...` prima di lanciare
   `scripts/audio-benchmark.sh`.
2. **Incompatibilità di dipendenze**: `pip install stable-audio-tools` (0.0.19, l'unica
   versione con wheel su PyPI) hard-pina una quindicina di pacchetti a versioni precise
   precedenti a Python 3.14 (`pandas==2.0.2`, `pytorch_lightning==2.1.0`,
   `sentencepiece==0.1.99`, `wandb==0.15.4`, `torchmetrics==0.11.4`, `encodec==0.1.1`, tra
   gli altri) — nessuna con wheel precompilata per 3.14 (unico Python disponibile su questa
   macchina). La build da sorgente di `pandas==2.0.2` fallisce già al primo pacchetto
   (`ModuleNotFoundError: pkg_resources`, il setuptools moderno usato dall'ambiente di
   build isolato di pip non lo include più di default). `torch` (CPU) invece installa
   pulito su 3.14 (verificato: `pip install torch --index-url
   https://download.pytorch.org/whl/cpu` va a buon fine). Sblocco pulito: un interprete
   Python 3.10–3.12 (via pyenv o conda) per questo venv — non presente su questa macchina;
   non installato da questo lavoro (cambio d'ambiente più grande di quanto un benchmark
   meriti senza autorizzazione esplicita, `docs/ai-production/regole-agenti-ml.md`).

Lo script resta pronto: risolti entrambi i blocchi, `scripts/audio-benchmark.sh` funziona
senza altre modifiche.

## Come rieseguire

```bash
# 1. scarica i candidati testo+immagine (una tantum, riprendibile con Ctrl+C):
scripts/download-comparison-models.sh              # tutto (~35.2 GB, testo+immagini)
scripts/download-comparison-models.sh --skip-extra-quants  # salta solo le quant extra di testo (~18.1 GB)

# 2. build (se serve):
make gen sprites

# 3. suite di TESTO completa (tutti i .gguf in models/, 3 run/modello):
make model-comparison
# equivalente a:
bash scripts/model-comparison.sh

# solo alcuni modelli, o con parametri diversi:
RUNS=5 SEED_BASE=1000 bash scripts/model-comparison.sh \
  models/qwen2.5-coder-3b-instruct-q4_k_m.gguf models/gemma-3-4b-it-q4_k_m.gguf

# 4. suite IMMAGINI (SOLO dopo che la suite testo ha finito -- mai due GPU insieme):
bash scripts/image-comparison.sh \
  pixel-baseline:models/Public-Prompts-Pixel-Model.ckpt \
  sd15-vanilla:models/sd15-vanilla-pruned-emaonly.safetensors \
  pixelart-alt:models/pixelart-spritesheet-generator-v1.ckpt

# 5. benchmark AUDIO (CPU, puo' girare in parallelo a 3/4 se non si misura un
#    tempo "pulito" in quel momento -- BLOCCATO al 23/07/2026, vedi sopra):
export HF_TOKEN=hf_...   # dopo aver accettato la licenza sul modello
bash scripts/audio-benchmark.sh
```

Output testo: `logs/model-comparison/<timestamp>/report.md` (tabella comparativa +
giudizio automatico + dettaglio per modello) e `report.csv` (stesse colonne, valori
grezzi, per riordinare in un foglio di calcolo). Output immagini:
`logs/model-comparison/<timestamp>/images/<modello>/` (atlas PNG + `index.txt` +
`bench.log`, nessun report.md automatico — il giudizio sulla resa visiva resta umano).
Output audio (quando sbloccato): `logs/model-comparison/<timestamp>/audio/
stable-audio-open-small/` (`.wav` + `index.json`). `logs/` non è versionato: ogni run
della suite produce una cartella nuova, quelle vecchie restano per confronto storico
finché non vengono ripulite a mano.

## Rischi e limiti noti

### Testo

- L'euristica di "aderenza al tema" è lessicale (sovrapposizione di parole),
  non semantica: un nome può aderire al tema senza condividerne una parola.
  Il campione testuale nel report resta necessario per i casi dubbi.
- Il peso del file `.gguf` è un proxy per il consumo VRAM, non la misura
  Vulkan reale (che dipende anche da `n_ctx`/batch/KV cache): un fallimento di
  caricamento resta la misura di VRAM più affidabile che l'harness produce.
- Qwen2.5-Coder-3B-Instruct ha licenza non commerciale (`qwen-research`): utile
  per QUESTA valutazione, non spedibile nel gioco senza una decisione di
  licenza dedicata se mai scelto come modello di produzione.
- Nessun giudice LLM: la "qualità" della prosa/coerenza narrativa oltre le
  metriche automatiche resta un giudizio umano sul campione nel report.

### Immagini

- Nessuna metrica automatica di qualità estetica: gli atlas sono l'artefatto, il
  giudizio è umano. La % di celle scartate dal quality gate è un proxy di
  "plausibilità strutturale" (bordi, occhi, patch), non di stile o coerenza col tema.
  Nessuna LoRA di stile è stata addestrata o applicata oltre alla LCM-LoRA di velocità
  (già di default sulla pipeline): questo è un confronto di BASELINE, il modello
  immagini resta provvisorio (memoria "Modello immagini provvisorio", 18/07/2026).
- SD3.5-Medium GGUF è supportato dal build ma NON eseguito: manca un CLIP-G non gated
  facilmente reperibile (il repo base `stabilityai/stable-diffusion-3.5-medium` è
  gated). Non è stato forzato un aggiramento del gate (le licenze si accettano, non si
  aggirano — `docs/ai-production/regole-agenti-ml.md`).

### Audio

- BLOCCATO su due fronti indipendenti (gate HuggingFace + incompatibilità Python 3.14
  delle dipendenze esatte di `stable-audio-tools`): vedi "Metodo — audio" sopra per il
  dettaglio e i passi di sblocco. Nessun tentativo di aggirare il gate con un mirror non
  ufficiale (violerebbe l'accettazione di licenza che DEC-113 presuppone) né di forzare
  un downgrade di Python o dipendenze incompatibili senza autorizzazione esplicita.
- Anche una volta sbloccato, questo resta un **prototipo di misura**, non l'integrazione
  nel gioco: nessun binario C linka mai un runtime Python (AGENTS.md); l'eventuale
  pipeline di produzione (`docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md`) è un
  lavoro separato, con la sua stessa scala di implementazione.


## Limiti noti dello scoring (verifica opus 23/07)

- Il punteggio composito somma le parti disponibili **senza rinormalizzare**: con meno di
  2 run valide il termine varietà (25%) cade in silenzio e il punteggio non è più
  confrontabile sulla scala 0-100. Non si è materializzato (11/11 modelli con 3 run
  pulite); serve una guardia se si riusa la suite su modelli instabili.
- La guardia anti-fotocopia confronta i nomi normalizzati esatti: varianti con suffisso
  («Volcano of Glowing Mold - Rotting/Boiling/…») la eludono — limite reale
  dell'euristica lessicale, da tenere a mente leggendo la colonna Fotocopie.


## Aggiornamento 23/07 sera: AUDIO SBLOCCATO ed eseguito

I due blocchi documentati sono stati superati: il gate HF è stato aperto dal token
dell'utente (licenza accettata su entrambe le varianti) e il problema dei pin Python è
stato aggirato con un venv CPython 3.12 creato da `uv` e la libreria ufficiale
`stable-audio-3` installata da git con torch CPU pinnato (`--no-deps`; ricetta completa in
`scripts/audio-benchmark.sh`). Prima esecuzione reale: SFX 4s in ~7.7s/clip, musica 20s in
~13.6s/clip (0.68x realtime) su CPU, 5 GB RAM di picco — risultati e 20 clip in
`docs/ai-production/experiments/audio-benchmark-2026-07-23.md` e
`logs/model-comparison/audio-20260723-172702/`.


## Estensione gen-2 (23-24/07): modelli immagine "moderni" su sd.cpp raw

Estensione della sezione immagini (finora solo SD1.5) ai modelli che il build pinnato di
`deps/stable-diffusion.cpp` (`master-775-b5d8120`, binari `sd-cli`/`sd-server`) supporta
oltre SD1.x: **SD3.5 Medium**, **SDXL**, **Flux.1-schnell** — verificato leggendo
`README.md`/`docs/*.md` del checkout locale (SD3/SD3.5, FLUX.1-dev/schnell, SDXL/SDXL-Turbo,
GGUF anche per i text encoder — confermato da `docs/wan.md`). Obiettivo: capire quale
candidato "moderno" sia il più piccolo/veloce/allenabile-con-LoRA per una futura campagna.

**Metodo — "raw pipeline"**: generato con `sd-cli` direttamente, non `bin/melting-sprites`
(cablato su SD1.5): niente quality-gate del gioco, niente LoRA/prompt del progetto. Prompt
"pixelsprite, `<tema>`, `<stile>`" sulle 6 coppie rappresentative della baseline (righe 01,
02, 08, 09, 12, 15 di `docs/ai-production/dataset/baseline-prompts.txt`) × seed 5, 17.
Artefatti in `logs/model-comparison/images-gen2-20260723-202847/<config>/`, report
completo in `logs/model-comparison/images-gen2-20260723-202847/risultati-gen2.md`.

**Nota sull'andamento della sessione**: la prima metà ha avuto la GPU condivisa con un
verifier (`bin/melting-gen`) quasi continuamente — la matrice SD3.5 è stata interrotta a
2/12 immagini per questo (dati "sporchi", numero di riferimento preso da un probe isolato).
Dopo la liberazione della GPU (confermata dal coordinatore) la sessione è ripartita e ha
completato le matrici Flux.1-schnell (piena) e SDXL (768 piena, 1024 con 4 varianti
accelerate a campione ridotto) con GPU dedicata.

### Cosa è stato scaricato

- **SD3.5 Medium**: diffusion GGUF Q4_K_M (`city96/stable-diffusion-3.5-medium-gguf`,
  1.79 GB) + `clip_l`/`clip_g` (`Comfy-Org/stable-diffusion-3.5-fp8`) + T5-XXL GGUF Q4_K_M
  (`city96/t5-v1_1-xxl-encoder-gguf`, 2.90 GB, **condiviso con Flux**) + VAE. Il repo
  diffusion-only di city96 non include i tensori VAE (errore esplicito verificato per
  tentativo); il repo ufficiale con VAE è gated (form Stability, dati anagrafici non
  completato — un agente non inventa dati anagrafici per conto dell'utente). Usato invece
  `Shio-Koube/SD-3.5-vae`, **SHA256 identico byte per byte** all'ufficiale (verificato via
  API HF autenticata, che espone gli hash dei repo gated senza doverli scaricare).
- **Flux.1-schnell**: diffusion GGUF Q2_K (4.11 GB) e Q3_K (5.31 GB) da
  `leejet/FLUX.1-schnell-gguf` (non gated) + `clip_l` (`comfyanonymous/flux_text_encoders`)
  + VAE (`Comfy-Org/Lumina_Image_2.0_Repackaged`, dimensione byte-identica all'ufficiale
  gated `black-forest-labs/FLUX.1-schnell`).
- **SDXL**: checkpoint base (`stabilityai/stable-diffusion-xl-base-1.0`, 6.94 GB) + VAE
  fp16-fix (`madebyollin/sdxl-vae-fp16-fix`, necessario per instabilità numerica del VAE
  embedded) + TAESD-XL (`madebyollin/taesdxl`) + LCM-LoRA-XL
  (`latent-consistency/lcm-lora-sdxl`) + SDXL-Lightning 4-step LoRA
  (`ByteDance/SDXL-Lightning`) + pixel-art-xl LoRA di stile (`nerijs/pixel-art-xl`).
- **SDXL-Turbo/SD-Turbo**: volutamente **non scaricati** (esclusione richiesta) — la
  verifica upstream ha trovato che la loro licenza attuale è la Stability AI Community
  License (commerciale sotto-soglia, come SD3.5), NON la vecchia licenza non-commerciale
  assunta come motivo dell'esclusione: correzione registrata in `docs/ai-production/licenze.md`.
- Circa 25 GB totali (budget 30-40 GB), sotto `models/gen2-comparison/` (non versionato).

### SD3.5 Medium Q4_K_M

- **1024×1024: NON REGGE** — crash dopo il 3° step (`vk::DeviceLostError`/"context is
  lost", reset driver RADV per saturazione VRAM). mmdit compute 1878 MB + params 2166 MB
  già oltre margine prima del decode VAE.
- **768×768: FUNZIONA** — probe isolato pulito (GPU/CPU libere): **89.23s/immagine**
  (~15.5s encoding CPU T5-XXL + ~65s sampling, 3.25s/step a regime + ~4.15s decode).
- Matrice reale interrotta a **2/12** (209.14s, 885.41s — contesa GPU con verifier prima
  della liberazione). Non ripresa dopo la liberazione: priorità data a Flux/SDXL su
  richiesta esplicita del coordinatore.

### Flux.1-schnell — matrice completa (GPU dedicata)

| Config | Esito | VRAM (params+compute) |
|---|---|---:|
| Q2_K, 1024×1024 | **CRASH** (device-lost) | 3920+3933 = 7853 MB |
| **Q2_K, 768×768** | **OK — 12/12** | 3920+973 = 4893 MB |
| Q3_K, 768×768 | **CRASH** (device-lost) | 5067+973 = 6040 MB, appena sopra 6 GB |

Q2_K@768 è il punto di equilibrio: Q3_K sfora il budget di soli ~40 MB. **Tempi
(matrice 6×2=12 immagini)**: warmup 65.34s, **regime 62.94s/immagine** (11 immagini,
deviazione ±0.7s). Il decode VAE (15.11s su ~63s totali, 24%) è il singolo passo più
lento — il VAE 16 canali di Flux ha un compute buffer sproporzionato (5652 MB anche a
768×768). 12/12 riuscite, zero fallimenti.

### SDXL base 1.0 — due risoluzioni + 4 accelerazioni

**768×768** (non serve `--vae-tiling`): matrice completa 12/12, warmup 75.40s, **regime
74.28s/immagine** (deviazione ±0.3s).

**1024×1024 — `--vae-tiling` è la scoperta chiave di questa sessione** (non nel task
originale, trovata nell'help della CLI): senza tiling, OOM pulito al decode VAE
(`failed to allocate Vulkan0 buffer of size 8541306888`, 8.54 GB — il sampling UNet
completa senza problemi, il collo di bottiglia è SOLO il decode a piena risoluzione). Con
`--vae-tiling`, il compute VAE crolla da ~8.5 GB a 416 MB.

**Nota su un throttling variabile osservato**: la velocità di sampling a 1024×1024 ha
oscillato fino a 6× fra misure identiche in momenti diversi della sessione — **3.78-3.83
s/step** nei probe isolati dopo una pausa, **~23.6s/step** stabile nelle matrici lanciate a
raffica senza pause. Causa non isolata con certezza (GPU non risultava power-capped né in
P-state basso quando verificato), ma il pattern (lento sotto uso back-to-back, veloce dopo
qualche minuto di pausa) è coerente con throttling termico/sustained-load non riflesso nei
contatori letti. Riportato onestamente entrambi i regimi.

| Config @1024×1024 | Step | Immagini | Tempo/immagine | Regime |
|---|---:|---:|---:|---|
| base + `--vae-tiling` | 25 | 1/12 | 607.98s | rallentato |
| + LCM-LoRA + `--vae-tiling` | 8 | 2/12 | 274.59s, 276.10s | rallentato |
| + Lightning 4-step + `--vae-tiling` | 4 | 2/12 | 119.03s, 130.36s | rallentato |
| + pixel-art-xl+LCM-LoRA + `--vae-tiling` | 8 | 2/12 | 211.97s, 209.74s | rallentato |
| + TAESD-XL (no tiling necessario) | 25 | 1 probe | **101.08s** | **pulito** |

Tutte le config 1024 interrotte deliberatamente dopo una lettura di regime stabile
(2+ misure coerenti), per coprire tutte le configurazioni richieste nel tempo disponibile.

**TAESD-XL è l'accelerazione più efficace osservata**: decode VAE in 1.78s (contro 10-15s
del VAE pieno anche tiled) E non richiede `--vae-tiling` (compute buffer 2784 MB, sotto il
limite che fa fallire il VAE pieno non-tiled). Vantaggio strutturale valido indipendentemente
dal regime di sampling osservato.

### Giudizio finale

**Velocità — nessuno dei tre "moderni" batte SD1.5** (baseline 5.63s/immagine @512, 8 step
LCM): Flux Q2_K@768 62.94s (~11×), SDXL@768 74.28s (~13×), SD3.5@768 89.23s (~16×), SDXL+
TAESD-XL@1024 101.08s regime pulito (~18×). SD1.5 è un UNet piccolo (860M) con un solo
CLIP leggero; i tre moderni sono DiT/MMDiT più pesanti o hanno encoder di testo enormi
(T5-XXL) o VAE dal compute buffer sproporzionato — costi che SD1.5 non paga.

**Il più piccolo/veloce accettabile: Flux.1-schnell Q2_K @768×768.** Il più veloce dei tre
moderni, il file più piccolo su disco, **e la licenza più pulita** (Apache 2.0 puro,
nessuna soglia di ricavi, nessuna Attachment A) — tre criteri allineati sullo stesso
candidato.

**Chi batte SD1.5 per lo use-case sprite: nessuno, su velocità.** Se la priorità resta il
vincolo di 1-2 minuti per l'intera run del gioco, SD1.5 resta l'unica scelta praticabile
oggi. I moderni diventano interessanti solo per un caso d'uso diverso (generazione
offline/batch di una libreria curata, non on-demand a inizio run) — decisione di design,
non di questo report tecnico.

**Leve di accelerazione**: SDXL ha le più mature (`--vae-tiling` obbligatorio a 1024,
TAESD-XL la scoperta più utile, LCM-LoRA/Lightning riducono gli step ma non il costo
per-step osservato); Flux non ha leve testate oltre alla natura già-distillata di schnell
(il VAE resta il collo di bottiglia, 24% del tempo); SD3.5 non ha acceleratori compatibili
nella lista candidati di questa sessione.

**Allenabilità LoRA**: tutti e tre pronti con `kohya-ss/sd-scripts` (rami principali, non
sperimentali) — `sdxl_train_network.py` (il più maturo), `sd3_train_network.py`,
`flux_train_network.py`. Nessun dato sui requisiti VRAM di training raccolto (fuori scope:
solo inferencing misurato).

**Raccomandazione**: **Flux.1-schnell Q2_K** per licenza pulita e velocità relativa
migliore, con la riserva che resta ~11× più lento di SD1.5. **SDXL con TAESD-XL** secondo
candidato per la ricchezza dell'ecosistema (LoRA di stile pubbliche, inclusa
`pixel-art-xl` verificata in questa sessione), più lento e con licenza leggermente meno
pulita (openrail++ con Attachment A vs Apache 2.0 puro).

### Rischi e limiti noti (gen-2)

- Throttling variabile (§ sopra) rende i tempi delle config 1024 meno affidabili come
  singolo numero: riportati entrambi i regimi osservati, non solo il migliore.
- Contesa GPU nella prima metà sessione ha reso "sporchi" i tempi della matrice SD3.5
  oltre le prime due immagini; il numero pulito di riferimento è il probe isolato.
- Diverse config SDXL@1024 e la matrice SD3.5 sono state interrotte prima dei 12/12
  (2 o 1 immagini) per coprire tutte le configurazioni richieste nel tempo disponibile:
  letture di regime comunque coerenti fra loro (deviazione <1% dove misurate 2+ volte).
- Il giudizio di qualità visiva resta umano: gli artefatti PNG in
  `logs/model-comparison/images-gen2-20260723-202847/` sono l'unico output di questo
  confronto raw-pipeline, nessuna metrica automatica (niente quality-gate del gioco).
- Il VAE di SD3.5 e Flux usati in questa valutazione non vengono dal repo ufficiale gated
  ma da mirror non gated verificati byte-identici (SHA256 per SD3.5, dimensione per Flux):
  provenienza documentata in `docs/ai-production/licenze.md`, non un aggiramento del gate
  (Apache 2.0 per Flux non richiede autorizzazione aggiuntiva; Stability Community License
  per SD3.5 permette esplicitamente l'uso di ricerca/valutazione a chiunque, indipendente
  dal click-through del modulo, che è un meccanismo di raccolta contatti di Stability, non
  una condizione legale aggiuntiva nel testo della licenza).
- Flux Q3_K e SDXL-Turbo/SD-Turbo restano candidati documentati ma non pienamente eseguiti
  (il primo fallisce comunque per VRAM; i secondi esclusi su richiesta).
