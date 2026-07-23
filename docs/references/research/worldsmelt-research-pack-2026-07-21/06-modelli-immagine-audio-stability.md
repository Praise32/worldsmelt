---
id: ref-pack-modelli-immagine-audio-stability
title: Modelli Stability AI — immagini e audio
domain: references
status: proposed
authority: supporting
owner: design
summary: >-
  Confronta SD 1.5 pixel+LCM (baseline), SD 3.5 Medium quantizzato (candidato) e SD 3.5 Large Turbo (studio); propone strategia audio con Stable Audio 3 e licenze da tracciare.
last_reviewed: 2026-07-22
topics: [stable-diffusion, stable-audio, pipeline-immagini, licenze, modello-audio]
related: []
supersedes: []
source_files: []
---

# Modelli Stability AI — immagini e audio

## Ruolo nel prodotto

La tesi più forte è la generazione di meccaniche Lua. Immagini e audio locali possono rafforzare la run, ma aumentano:

- dimensione della build;
- memoria;
- tempo di preparazione;
- compatibilità hardware;
- superficie di test.

Devono essere caricati in sequenza rispetto all’LLM.

## Immagini: decisione corrente

### SD 1.5 pixel + LCM

Vantaggi:

- già integrato e misurato;
- circa 2 GB di VRAM nello spike;
- 512×512, 8 step;
- circa 5,3 s per sprite sulla RX 5600 XT;
- ecosistema LoRA/ControlNet maturo;
- compatibilità migliore.

Resta il baseline pratico per la prima build AI completa.

### SD 3.5 Medium quantizzato

È il candidato moderno più realistico:

- molto più piccolo di SD 3.5 Large;
- qualità generale e prompt adherence superiori;
- supporto al fine-tuning;
- plausibile con diffusione su Vulkan e text encoder su CPU;
- richiede nuovo benchmark e nuove LoRA, non è drop-in.

Configurazione da provare:

```text
modello diffusione: Q5, poi Q4
text encoder: CPU
VAE: Vulkan
risoluzione iniziale: 512×512
output finale: 128×128
```

### SD 3.5 Large Turbo

È un modello studio/teacher, non il runtime ideale della RX 5600 XT:

- modello molto grande;
- quattro step non eliminano il costo per step;
- 6 GB non bastano per residenza completa di tutti i componenti;
- streaming/offload rischia di annullare il vantaggio Turbo.

Uso possibile:

- concept;
- dataset revisionato;
- asset premium offline;
- futura GPU/cloud.

### Raccolta AMD Optimized

I pacchetti AMD sono principalmente ONNX destinati a percorsi AMD/Amuse. Non sono sostituzioni dirette dei checkpoint usati da `stable-diffusion.cpp`.

La RX 5600 XT non è il target ideale delle configurazioni recenti SD 3.5 AMD Optimized. Integrare quel percorso richiederebbe un backend separato.

Decisione: continuare con Vulkan/GGUF/safetensors nel runtime esistente; valutare AMD ONNX solo come backend opzionale futuro.

## Audio

### Stable Audio 3 Small SFX

Candidato per:

- creature;
- porte;
- materia organica;
- effetti ambientali;
- boss e trasformazioni.

### Stable Audio 3 Small Music

Candidato per:

- loop;
- droni;
- ambienti;
- musica tematica della run.

### Stable Audio 3 Medium

Più adatto alla produzione master offline su hardware esterno.

### Strategia audio

```text
rFXGen / sintesi procedurale = UI, pickup, impatti semplici
Stable Audio Small SFX      = effetti caratteristici
Stable Audio Small Music    = loop/ambienti
libreria fallback           = avvio garantito
```

## Training

Non addestrare checkpoint quantizzati o pacchetti ONNX di inferenza.

```text
training: checkpoint BF16/FP16 originale
inferenza: GGUF/quantizzazione/ONNX ottimizzato
```

Le LoRA SD 1.5 non sono trasferibili a SD 3.5. Per l’audio, il progetto Stable Audio 3 distingue checkpoint base per l’adattamento e checkpoint post-trained per l’uso.

## Licenza Stability AI

La Community License consente uso commerciale dei Core Models sotto la soglia di ricavi annuali indicata da Stability AI; oltre la soglia serve Enterprise. Poiché il passaggio Enterprise è accettabile per il progetto, la licenza non deve guidare la scelta qualitativa, ma va mantenuto un registro di:

- modello e revision;
- hash;
- licenza;
- base model e LoRA;
- dataset;
- dipendenze terze;
- data di download;
- condizioni di distribuzione dei pesi.
