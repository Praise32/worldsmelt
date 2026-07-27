---
id: aiprod-stack-modelli
title: Stack dei modelli
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Motiva la scelta di SD1.5 come base immagini, il ruolo di LCM-LoRA e TAESD, i compiti/limiti del modello di testo attivo e i criteri per valutare modelli alternativi.
last_reviewed: 2026-07-27
topics: [stable-diffusion, lcm-lora, taesd, qwen, model-selection]
related: []
supersedes: []
source_files: []
---
# Stack dei modelli

## Scelta principale: Stable Diffusion 1.5

Motivi:

- ecosistema LoRA maturo;
- risoluzione nativa 512×512 coerente con la pipeline esistente;
- peso e memoria inferiori ai modelli moderni più grandi;
- supporto già funzionante in stable-diffusion.cpp;
- backend CPU, CUDA, Vulkan, Metal e altri;
- supporto LoRA, LCM/LCM-LoRA, TAESD e ControlNet SD1.5;
- benchmark già raccolti sul PC di sviluppo.

SD1.5 non è scelto perché produce la qualità massima assoluta, ma perché offre il miglior
rapporto fra:

- dimensioni;
- compatibilità;
- addestrabilità su Kaggle;
- inferenza locale;
- ecosistema;
- costo d'integrazione.

La comparison di seconda generazione del 23/07 (SD3.5 Medium, SDXL base 1.0 e
Flux.1-schnell confrontati con SD1.5 sul vincolo dei 6 GB) ha confermato la scelta:
nessun modello moderno ha superato SD1.5 nel rapporto qualità/hardware del progetto —
DEC-148 registra la conferma.

## Base consigliata

Per il ramo canonico:

```text
stable-diffusion-v1-5/stable-diffusion-v1-5
```

Conservare:

- repository esatto;
- revisione/commit;
- hash dei file;
- licenza;
- data di download.

## Checkpoint pixel-art di terze parti

Utilizzarli come benchmark, non come fondamento automatico del ramo commerciale.

Problemi possibili:

- dataset non documentato;
- derivazione non chiara;
- stili e personaggi memorizzati;
- trigger proprietari;
- compatibilità non garantita;
- licenza della pagina diversa dai diritti reali sui dati.

Confronto corretto:

```text
A: SD1.5 vanilla + Worldsmelt Style LoRA
B: checkpoint pixel di terze parti + Worldsmelt Style LoRA
```

Vince il sistema che ottiene migliori asset in-engine, non l'immagine più appariscente in
isolamento.

**Base di training della Style LoRA (DEC-148):** la Style LoRA si addestra su **SD1.5
vanilla**, non sul checkpoint `pixel-baseline` di terze parti — provenienza pulita e
risultato portabile. Solo a **validazione avvenuta** la LoRA può essere fusa nella base
per produrre il checkpoint proprietario del progetto, candidato a sostituire
`pixel-baseline` nel runtime previa asset review; fino ad allora il runtime resta su
`pixel-baseline`.

## LCM-LoRA

Uso:

- accelerare l'inferenza a pochi step;
- permettere una modalità rapida;
- ridurre attesa nel Piano 0.

Non deve sostituire la Style LoRA.

Stack previsto:

```text
SD1.5
+ worldsmelt-style
+ worldsmelt-enemies/items/...
+ lcm-lora-sdv1-5
```

Serve benchmark A/B perché la somma degli adattatori può cambiare:

- palette;
- silhouette;
- adesione al prompt;
- pulizia dello sfondo.

## TAESD

TAESD può ridurre memoria e latenza del decoding, ma la pipeline corrente ha osservato una
resa più nitida con la VAE reale. Il preset low-spec è stato rimosso (DEC-110): TAESD resta
solo un'opzione tecnica interna per ridurre memoria, non un livello offerto al giocatore né
un default automatico.

## Modello di testo

Ruolo (il modello di testo attivo, oggi Gemma-3-4B-IT Q4 — DEC-140; fallback su errore di
caricamento Qwen2.5-Coder 1.5B, 7B selezionabile con `--model`):

- generazione di RunManifest;
- specifiche di nemici;
- comportamento Lua limitato;
- temi, nomi e descrizioni;
- selezione di body plan e rig;
- richiesta degli asset necessari.

Non deve:

- decidere collisioni arbitrarie;
- inventare API;
- modificare regole fondamentali;
- generare codice senza schema e validazione;
- produrre una descrizione grafica non traducibile in un rig supportato.

## Modelli più piccoli

Dopo la Style LoRA funzionante si può valutare un modello SD compresso, ma solo con test
separati. Modifiche architetturali dell'UNet possono rendere incompatibili LoRA addestrate
sulla base SD1.5 standard.

Criterio di adozione:

```text
qualità in-engine >= soglia
tempo <= target
VRAM <= tier
LoRA compatibili o riaddestrabili
licenza/provenienza documentate
```

## Modelli moderni più grandi

FLUX, SDXL e modelli analoghi possono essere utili come strumenti di concept art o
benchmark esterni. Non sono la baseline runtime per Worldsmelt finché:

- aumentano il requisito hardware;
- non migliorano abbastanza il tasso di asset validi;
- complicano training e distribuzione;
- non rispettano il target local-first.
