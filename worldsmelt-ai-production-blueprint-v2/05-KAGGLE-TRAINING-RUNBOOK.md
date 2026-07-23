# Kaggle Training Runbook — SD1.5 LoRA

## Architettura

Il Notebook deve essere sottile. La logica vive nel repository:

```text
ml/
├── train_lora.py
├── validate_dataset.py
├── make_captions.py
├── generate_eval_grid.py
├── compare_checkpoints.py
└── configs/
```

Il kernel Kaggle:

1. installa versioni fissate;
2. recupera codice e dati;
3. valida il dataset;
4. esegue smoke test;
5. esegue training;
6. genera griglie;
7. scrive artifact e report in `/kaggle/working`.

## Struttura proposta

```text
kaggle/worldsmelt-style-lora/
├── run.py
├── kernel-metadata.json
└── README.md
```

## Versioni

Bloccare le versioni in un file, per esempio:

```text
requirements-ml.txt
```

Non aggiornare automaticamente Diffusers/Transformers in ogni run. Un cambio di versione
è una variabile sperimentale.

## Preflight

Il job deve fallire prima di usare la GPU se:

- file senza ledger;
- licenza mancante;
- immagine corrotta;
- dimensione non supportata;
- caption vuota;
- duplicato nello split opposto;
- frame della stessa animazione in split diversi;
- output directory non scrivibile.

## Smoke test

Prima del training completo:

```text
20–100 step
1 prompt di validazione
1 checkpoint
1 immagine
```

Verificare:

- memoria;
- NaN;
- caricamento dataset;
- salvataggio;
- ripresa da checkpoint;
- caricamento della LoRA;
- generazione finale.

## Configurazione v0

```yaml
experiment_id: style-lora-v0
base_model: stable-diffusion-v1-5/stable-diffusion-v1-5
revision: pinned
resolution: 512
rank: 8
alpha: 8
learning_rate: 0.0001
train_text_encoder: false
mixed_precision: fp16
batch_size: 1
gradient_accumulation: 4
max_train_steps: 1500
checkpointing_steps: 250
validation_steps: 250
seed: 20260720
```

## Dataset iniziale

- 150–300 immagini;
- un solo stile;
- prospettiva coerente;
- personaggi, creature, oggetti e pochi ambienti;
- niente JPEG;
- niente sprite sheet interi come singola immagine;
- niente duplicati;
- caption strutturate.

## Prompt di validazione

Congelare almeno:

- 5 nemici;
- 3 oggetti;
- 2 personaggi;
- 2 VFX;
- 2 ambienti;
- 1 caso difficile.

Per ogni prompt usare seed fissi. Generare la stessa matrice per:

- SD1.5 base;
- checkpoint intermedi;
- LoRA finale;
- combinazione con LCM-LoRA.

## Output obbligatori

```text
artifacts/<experiment_id>/
├── config.yaml
├── environment.txt
├── dataset_manifest.jsonl
├── dataset_hash.txt
├── training.log
├── metrics.jsonl
├── checkpoints/
├── final/
│   └── pytorch_lora_weights.safetensors
├── eval/
│   ├── baseline/
│   ├── step-0250/
│   └── ...
└── report.md
```

## Comando di riferimento

Usare lo script ufficiale Diffusers come base, ma incapsularlo in uno script del progetto:

```bash
accelerate launch ml/train_lora.py \
  --config ml/configs/style-lora-v0.yaml
```

## Ripresa

Il run deve supportare:

- checkpoint frequenti;
- resume esplicito;
- nessun overwrite silenzioso;
- ID esperimento immutabile;
- hash della configurazione.

## Kaggle

La disponibilità di GPU e quote varia. Non codificare l'assunzione che una specifica GPU
sia sempre presente.

Il job deve stampare:

```text
GPU
VRAM
driver
torch
cuda
diffusers
transformers
accelerate
commit git
dataset hash
```

## Fine run

Prima della chiusura:

- salvare LoRA;
- generare griglia;
- scrivere report;
- comprimere artifact;
- verificare checksum;
- scaricare gli output tramite MCP o Kaggle CLI.
