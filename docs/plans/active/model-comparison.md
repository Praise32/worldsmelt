---
id: plans-model-comparison
title: Piano — suite di comparazione dei modelli candidati
domain: plans
status: approved
authority: supporting
owner: ai-production
summary: >-
  Obiettivo, metodo e candidati della suite che confronta modelli GGUF alternativi
  al 7B Coder di produzione su throughput, validità JSON/Lua, varietà e aderenza
  al tema, con soglie di accettabilità e istruzioni per rieseguirla.
last_reviewed: 2026-07-23
last_verified_commit: 9b515a6
topics: [modelli, benchmark, gguf, llama.cpp, comparazione, vram]
related: [eng-benchmarks]
supersedes: []
source_files: [scripts/model-comparison.sh, scripts/model_comparison_report.py, scripts/download-comparison-models.sh, tools/melting-gen/main.c]
---

# Piano — suite di comparazione dei modelli candidati

## Obiettivo

Il 7B Coder Q4_K_M (`qwen2.5-coder-7b-instruct-q4_k_m.gguf`) è il modello di produzione
di `tools/melting-gen` (default in `main.c`, misurato in `docs/engineering/benchmarks.md`).
Questo piano copre una sessione di **valutazione tecnica** (decisione dell'utente,
22/07/2026: lista curata + quantizzazioni) per rispondere a due domande separate:

1. un modello **più piccolo/veloce** del 7B regge le stesse soglie di qualità
   (validità JSON/Lua, varietà, aderenza al tema) sulla RX 5600 XT 6 GB?
2. una **quantizzazione diversa** dello stesso 7B (Q5_K_M/Q6_K invece di Q4_K_M) o
   un quant più alto dei modelli piccoli (Q8_0 su 1.5B/3B) cambia qualcosa di
   misurabile, non solo impressione a occhio?

Questo piano **non cambia** il modello di produzione da solo: registra la misura.
Un cambio di default in `tools/melting-gen/main.c` richiede una decisione a parte
(vedi `docs/design/governance/decision-log.md`), non implicita in un report tecnico.

## Metodo

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

## Candidati

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

## Come rieseguire

```bash
# 1. scarica i candidati (una tantum, riprendibile con Ctrl+C):
scripts/download-comparison-models.sh              # tutto (~26.7 GB)
scripts/download-comparison-models.sh --skip-extra-quants  # solo i 4 nuovi Q4_K_M (~9.6 GB)

# 2. build (se serve):
make gen

# 3. suite completa (tutti i .gguf in models/, 3 run/modello):
make model-comparison
# equivalente a:
bash scripts/model-comparison.sh

# solo alcuni modelli, o con parametri diversi:
RUNS=5 SEED_BASE=1000 bash scripts/model-comparison.sh \
  models/qwen2.5-coder-3b-instruct-q4_k_m.gguf models/gemma-3-4b-it-q4_k_m.gguf
```

Output: `logs/model-comparison/<timestamp>/report.md` (tabella comparativa +
giudizio automatico + dettaglio per modello) e `report.csv` (stesse colonne,
valori grezzi, per riordinare in un foglio di calcolo). `logs/` non è
versionato: ogni run della suite produce una cartella nuova, quelle vecchie
restano per confronto storico finché non vengono ripulite a mano.

## Rischi e limiti noti

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
