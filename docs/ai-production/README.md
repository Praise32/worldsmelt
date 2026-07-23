---
id: aiprod-readme
title: AI production — modelli, LoRA, dataset e asset generati
domain: ai-production
status: approved
authority: canonical
owner: ai-production
summary: >-
  Punto d'ingresso del dominio ai-production: come si selezionano, allenano e integrano
  modelli, LoRA, dataset e asset generati. Erede dei contenuti canonici della
  worldsmelt-ai-production-blueprint-v2.
last_reviewed: 2026-07-22
last_verified_commit: fe27f6d
topics: [ai-production, modelli, lora, dataset, training]
related: []
supersedes: []
source_files: [scripts/download-models.sh, scripts/dataset_ledger.py]
---

# AI production

Questo dominio governa la **produzione IA**: scelta dei modelli, training delle LoRA,
dataset e licenze, cura degli asset generati. Il design del gioco sta in `docs/design/`
(che vince in caso di conflitto: gerarchia in `docs/_meta/DOCUMENT-STANDARDS.md`); lo stato
tecnico del motore sta in `docs/engineering/`.

Nota di contesto (memoria di progetto, 18/07): gli sprite attuali sono **provvisori** — la
qualità visiva vera arriverà con le LoRA; non investire ora in rifiniture visive.

## Percorso di lettura

1. [00-DECISIONI-CANONICHE.md](00-DECISIONI-CANONICHE.md) — la baseline tecnica operativa
   (SD1.5, LoRA prima dei checkpoint, dataset research vs commercial-clean, processi in
   sequenza). Per il design vince sempre il decision-log.
2. [02-STACK-MODELLI.md](02-STACK-MODELLI.md), [03-PIANO-LORA.md](03-PIANO-LORA.md),
   [04-DATASET-LICENZE.md](04-DATASET-LICENZE.md) — stack e piano di training.
3. [05-KAGGLE-TRAINING-RUNBOOK.md](05-KAGGLE-TRAINING-RUNBOOK.md) e
   [06-AGENTI-KAGGLE-MCP.md](06-AGENTI-KAGGLE-MCP.md) — esecuzione del training remoto;
   il runbook RunPod/kohya locale è in [dataset/TRAINING-RUNBOOK.md](dataset/TRAINING-RUNBOOK.md).
4. [07-ARCHITETTURA-RUNTIME.md](07-ARCHITETTURA-RUNTIME.md) …
   [11-PROTOCOLLO-ESPERIMENTI.md](11-PROTOCOLLO-ESPERIMENTI.md) — integrazione e metodo.
5. Pipeline approvate il 22/07:
   [15-UI-DESIGN-PIPELINE.md](15-UI-DESIGN-PIPELINE.md),
   [16-AUDIO-GENERATION-PIPELINE.md](16-AUDIO-GENERATION-PIPELINE.md) (adottata da DEC-109,
   licenza DEC-113), [17-ASSET-CURATION-AND-FLOOR-ZERO.md](17-ASSET-CURATION-AND-FLOOR-ZERO.md),
   [18-AGENT-ORCHESTRATION.md](18-AGENT-ORCHESTRATION.md) (la scala di CLAUDE.md prevale
   per i ruoli). Resta proposta viva: [19-DECISION-QUESTIONNAIRE.md](19-DECISION-QUESTIONNAIRE.md)
   (coda di domande verso il decision-log).

## Contenuti operativi

- [licenze.md](licenze.md) — licenze di modelli, codice e asset (canonico).
- [dataset/README.md](dataset/README.md) — regole d'oro del dataset e registro di
  provenienza (`dataset/ledger.jsonl`, gestito da `scripts/dataset_ledger.py`).
- [experiments/sprites-spike.md](experiments/sprites-spike.md) — lo spike che ha fissato la
  pipeline sprite (misure reali, trappole note).
- [regole-agenti-ml.md](regole-agenti-ml.md) — regole vincolanti per gli agenti nei task ML.
- `templates/` — template operativi (asset review, experiment report, spec…).

## Invarianti (non negoziabili senza decisione)

Nessuna inferenza in combattimento; generazione nel Piano 0 o fra piani; Qwen/SD/audio
caricati in sequenza nei 6 GB; cache e pubblicazione atomica; modalità solo-curato sempre
dignitosa; fallback curati o geometrici per ogni asset non valido; dataset `research` mai
mischiato col ramo `commercial-clean`; LoRA prima dei checkpoint completi; SD1.5 baseline
immagini fino a decisione contraria; nessuna distribuzione di pesi col gioco.
