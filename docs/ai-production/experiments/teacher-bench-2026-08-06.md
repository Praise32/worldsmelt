---
id: aiprod-exp-teacher-bench-2026-08
title: "Bake-off teacher/runtime immagini 06/08/2026 — due track (canone 32px, caccia libera 64px)"
domain: ai-production
status: approved
authority: supporting
owner: ai-production
summary: >-
  Comparison a 19 configurazioni su RX 5600 XT: Track P (5 basi a canone, giudizio 32px)
  e Track F (9 config a settings nativi + 4 few-step, giudizio 64px). 292/292 immagini,
  zero fallimenti. La baseline SD1.5 vanilla è battuta da quasi tutto; il verdetto umano
  del proprietario (review.html, 9 criteri) resta il gate di promozione.
last_reviewed: 2026-08-06
last_verified_commit: 3f0f38b
topics: [esperimenti, comparison, bake-off, immagini, teacher, lora, few-step]
related: [aiprod-protocollo-esperimenti]
supersedes: []
source_files:
  - scripts/teacher-bench.sh
  - scripts/teacher_bench_post.py
  - scripts/teacher_bench_review.py
  - docs/ai-production/dataset/teacher-bench-2026-08-prompts.json
  - docs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json
---

# Bake-off teacher/runtime immagini — 06/08/2026

Variante **comparison/bake-off** del protocollo (DEC-164). Origine: le due ricerche del
proprietario del 04/08 (dossier "image model research august 2026" + "retro diffusion
strategy") più il censimento SOTA del 06/08 fatto in sessione (API Civitai/HF, licenze e
hash verificati). Mandato del proprietario: due domande separate, due track.

## Domande

- **Track P** (canone, giudizio a 32px nella scena 640×360): quale base SD1.5 produce il
  miglior teacher per la futura Worldsmelt LoRA sotto il canone attuale (DEC-173/177/205/
  206/207)?
- **Track F** (caccia libera, giudizio a 64px): qual è il massimo di qualità e velocità
  per asset di gioco che questa macchina (RX 5600 XT 6 GB, Vulkan, stable-diffusion.cpp)
  può produrre oggi, con ogni modello ai settings del suo creatore?

## Metodo

8 soggetti canonici × 2 seed congelati (4242, 90210), contratti versionati
(`teacher-bench-2026-08-prompts{,-trackF}.json` — il P presidia DEC-205 nei negative, il
F la rimuove di proposito), harness `scripts/teacher-bench.sh` (sd-cli sequenziale,
resume-safe, manifest per immagine con hash/licenza/latenza), postproc deterministico
(flood-fill a moda di cornice, canvas 32+64 BOX+NEAREST, palette Fucina via
`remap_fucina.py`), `metrics.csv`, `review.html` a 9 criteri.
**Esito tecnico: 292/292 immagini, 0 fallimenti, VRAM di picco ~2,8 GB.**
Nota di misura: l'harness invoca sd-cli per immagine, quindi la latenza include il
caricamento del modello (~5-6 s); il vantaggio few-step reale a modello residente è
maggiore di quello misurato.

## Pulizia disco registrata (autorizzazione esplicita del proprietario, 06/08)

Eliminati ~55 GB di pesi dei benchmark di luglio, su conferma puntuale della lista in
chat (deroga consapevole al divieto n.5 di `regole-agenti-ml.md`, che vincola gli agenti,
non il proprietario): `models/gen2-comparison/` (24 GB, SDXL/Flux/SD3.5), 8 GGUF testo
del confronto pre-DEC-140 (~27 GB), `pixelart-spritesheet-generator-v1.ckpt` (4 GB).
Conservati: gemma (default), coder-7B/1.5B q4 (fallback), Public-Prompts (baseline viva),
sd15-vanilla, lcm-lora, taesd, stable-audio. Report e harness di luglio intatti; tutto
riscaricabile dagli script del repo.

## Candidati e licenze (ledger completo: `artifacts/image-model-research/manifests/model-ledger.json`)

Snapshot licenze in `models/teacher-bench-2026-08/license-snapshots/`. Punti da ricordare:
**PIXHELL vieta i derivati ridistribuiti** (benchmark sì, merge in pipeline MAI);
**Hyper-SD** ha un caveat registrato (la dichiarazione openrail++ è sparita dal README
attuale, resta nello storico); **DreamShaper 8** su HF risulta `license: other` senza
file LICENSE (divergenza dal dossier del proprietario, registrata nel ledger);
Pixel Art Sprite Diffusion è **Apache-2.0** (la più pulita); Pokemon Trainer Sprite
scartato a monte (IP Nintendo, mai scaricato); AnimeKawa scartato (commerciale vietato).

## Risultati automatici (mediana latenza per immagine; giudizio alla scala del track)

| Config | Base + LoRA | Step/CFG | Lat. med. | Silh. conn. | Prima lettura (Fable) |
|---|---|---|---:|---:|---|
| A0 | SD1.5 vanilla (baseline) | 25/7 | 17,8 s | 16/16 | La più debole: sfondi sporchi, item in griglia, armi con personaggi |
| A1 | DreamShaper 8 | 25/7 | 17,2 s | 14/16 | Pulito ma morbido, poco "sprite" |
| A2 | DS8 + basepixel 0.6 | 25/7 | 18,5 s | 16/16 | Belle masse senza outline; un seed su due debole |
| A3 | DS8 + 8bitdiffuser 1.0 | 25/7 | 18,5 s | 16/16 | Il feel più sprite del track P; personaggi fragili, martello→bastone |
| A4 | DreamShaper PixelArt | 25/7 | 20,2 s | 14/16 | **Sorpresa del track P**: stile più coerente e dark-fantasy; armi confuse |
| T0 | TokForge Q4 (velocità) | 6/1.5 | 8,0 s | 16/16 | Baseline velocità, non teacher |
| F1 | AnyLoRA + 8bitdiffuser | 20/7 | 17,1 s | 15/16 | Sprite nitidi e chunky sulla base raccomandata dal creatore |
| F2 | DS8 + 8bitdiffuser | 20/7 | 15,5 s | 14/16 | Controprova base: confronto diretto con F1 |
| F3 | DS PixelArt, trigger pieno | 25/7 | 19,3 s | 13/16 | Variante nativa di A4 |
| F4 | All-in-one `pixelsprite` | 20/10 | 22,3 s | 16/16 | Pulitissimo stile console; bias verso oggetti-macchina e facce |
| F5 | DS8 + M_Pixel v3 | 25/7 | 18,5 s | 13/16 | Il LoRA pixel più scaricato di Civitai |
| F6 | Pixel Art Sprite Diffusion | 25/7 | 24,2 s | 14/16 | **Collasso totale**: ogni soggetto diventa un personaggino umano |
| F7 | DS8 + PIXHELL | 25/7 | 18,5 s | 16/16 | Generalista; licenza no-derivati |
| F8 | DS8 + basepixel 1.0 nativo | 28/7 | 20,3 s | 15/16 | Più ricco e cupo della A2 |
| F9 | DS8 + RPG Icons (armi/item) | 25/7 | 18,5 s | 4/4 | Look icona giusto, aderenza al soggetto debole |
| S1 | DS8 + basepixel + LCM-LoRA | 6/1.5 | 8,4 s | 15/16 | Few-step classico |
| S2 | DS8 + basepixel + Hyper-SD | 8/5 | 9,7 s | 16/16 | Few-step con CFG/negative preservati |
| S3 | AnyLoRA-LCM + 8bitdiffuser | 6/1.5 | 10,2 s | 11/16 | **Notizia dell'asse velocità**: a 6 step quasi al livello dei 25 |
| S4 | DreamShaper 8 LCM | 8/2 | 10,0 s | 14/16 | Il runtime coerente-col-teacher della ricerca |

Colori fuori palette dopo il fixer: **0 su tutte le 584 righe** (il rimappatore Fucina
regge ogni config).

## Conclusioni della comparison (in attesa del verdetto umano)

1. **La baseline è battuta**: A0 (SD1.5 vanilla) è visivamente l'ultima su quasi ogni
   soggetto — la premessa della ricerca del proprietario (partire da una base migliore)
   è confermata dai dati.
2. **Il teacher non è scontato**: la ricerca candidava DreamShaper 8; il bake-off mostra
   che **DreamShaper PixelArt (A4/F3)** e **8bitdiffuser su AnyLoRA (F1)** lo sfidano
   davvero. Nessuna promozione automatica: A4 ha però la provenienza meno documentata
   (audit licenza richiesto dal dossier stesso prima di qualunque promozione).
3. **Armi = dominio debole ovunque** (martello→bastone/cerchio/spada). Né i teacher né
   RPG Icons lo risolvono da soli: servirà il giro silhouette-guided (ControlNet scribble
   già scaricato) e/o dataset dedicato nella Worldsmelt LoRA.
4. **F6 (Sprite Diffusion): REJECTED_QUALITY** nonostante la licenza migliore — genera
   solo persone.
5. **Asse velocità promettente**: S2 (Hyper-SD, CFG preservato) e S3 (AnyLoRA-LCM)
   tengono una qualità sorprendente a 6-8 step; con modello residente il tempo per
   immagine scenderebbe sotto i ~5 s.
6. **64 vs 32**: a 64px sopravvive molto più dettaglio (griglia naturale di SD1.5 a
   512). Se il proprietario lo conferma alla review, va aperta la open question sul
   canone 32px (DEC-177/208) — la decisione non è di questo report.

## Decisioni provvisorie per configurazione

`TRAINING_BASE` candidate (in attesa del verdetto): **A4/DS-PixelArt** (con audit
licenza), **A1/DS8** (fallback documentato), **F1-base AnyLoRA** (per lo stile 8bit).
`RUNTIME_BASE` candidate: **S2**, **S3**, **S4**. `BENCHMARK_ONLY`: F7 (licenza), T0.
`REJECTED_QUALITY`: A0 (come teacher), F6. `NEEDS_MORE_DATA`: F9 (prompt più vincolati),
tutto il fronte silhouette-guided (Stage successivo con le due config promosse).

## Prossimi passi

1. Verdetto del proprietario da `review.html` (9 criteri) e dalla galleria pubblicata.
2. Registrare le decisioni con `worldsmelt-decision-facilitator` (teacher scelto,
   eventuale open question 64px, audit licenza DS-PixelArt se promosso).
3. Solo dopo: Stage B (dataset 200-400 + LoRA rank 16/32 su Kaggle, autorizzazione GPU
   esplicita richiesta) e giro silhouette-guided sulle due config promosse.

Artefatti: `artifacts/image-model-research/` (raw, canvas, preview, metrics, review,
ledger); log run in scratchpad di sessione; galleria pubblicata (link in chat, stessa
URL aggiornabile).

## Addendum — notte 06-07/08: runtime, architettura di prompting, fusioni

Mandato del proprietario: "diversifica i test, più materiale per la mattina". Eseguito
(~1000 immagini totali, zero fallimenti di generazione):

- **Runtime appaiato (batch 1)**: 48 richieste inventate da Gemma (`--visualspecs`,
  spec GBNF + prompt libero sulla STESSA idea) × S1-S4 × 2 architetture = 384 img.
  Silhouette connesse a 64px: S4 92-94%, S1 83/81, S2 79/79, S3 54/62.
  **Spec vs libero: quasi pari sulle metriche automatiche** — il verdetto vero è
  visivo (aderenza al soggetto richiesto) e spetta alla review appaiata.
- **Batch 2 (50 richieste nuove, seed diverso, S2/S3)**: 200 img. S3 risale a 82/72 —
  la debolezza sul batch 1 era in parte varianza da soggetti, non solo dalla config:
  motivo per cui i batch sono due.
- **Curva step (S2/S3 a 4/6/8, 64 img)**: in galleria con tabella dedicata — dove il
  few-step si rompe si vede a occhio.
- **Fusioni (stadio 2, prima prova assoluta)**: 8 coppie × 3 tecniche × S2/S3 = 48 img.
  **Tutte e tre le tecniche funzionano nei 6 GB**, ControlNet scribble compreso
  (~10,4 s contro ~7 s delle altre). Confronto in galleria: spec-fusion / img2img sul
  composto / ControlNet sulla silhouette combinata.
- **Onda combinazioni**: S5 (DS-PixelArt + Hyper-SD) completa; S6/S7/S8 interrotte da
  stop esterni ripetuti dei task GPU (2:10-2:20 di notte) — si completano con
  `scripts/teacher-bench.sh S6 S7 S8` (~8 min). Nessun costo di rigenerazione: tutto
  resume-safe.
- **BOX vs NEAREST**: sezione dedicata in galleria (variante `__nearest` esistente per
  ogni immagine) — chiude a dati la questione aperta dalla ricerca di luglio.

Nota di misura invariata: la latenza per immagine include il caricamento modello
(~5-6 s a invocazione); a modello residente le config few-step scendono sotto i ~5 s.

**Decisioni che il pacchetto mattutino mette sul tavolo** (tutte del proprietario):
1. La S-config runtime da cablare in melting-sprites per sviluppare il gioco
   (sostituibile a contratto invariato quando arriverà la Worldsmelt LoRA).
2. L'architettura di prompting: VisualSpec+template vs prompt libero di Gemma
   (dalla review appaiata; le metriche automatiche non separano abbastanza).
3. Le due basi teacher finaliste per lo smoke di Stage B (candidabili solo le
   non-distillate: DS8, DS-PixelArt, AnyLoRA fp16).
4. La tecnica di fusione per lo stadio 2.
