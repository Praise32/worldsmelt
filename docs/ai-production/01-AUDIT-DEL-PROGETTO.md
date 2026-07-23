---
id: aiprod-audit-del-progetto
title: Audit del progetto Worldsmelt
domain: ai-production
status: superseded
authority: historical
owner: ai-production
summary: >-
  Fotografia dello stato reale della pipeline AI (melting-gen/melting-sprites), limiti correnti (una sola LoRA, atlas fisso, player non generato) e gap principali.
last_reviewed: 2026-07-22
topics: [audit, melting-sprites, melting-gen, atlas, gap-analysis, benchmark]
related: []
supersedes: []
source_files: [tools/melting-gen, tools/melting-sprites]
---
# Audit del progetto Worldsmelt

## Stato rilevato

Il repository non è a zero sul lato IA. Esistono già:

- `tools/melting-gen`: Qwen2.5-Coder tramite llama.cpp/Vulkan, GBNF, validazione e fallback;
- `tools/melting-sprites`: Stable Diffusion tramite stable-diffusion.cpp;
- post-processing con downscale modale, flood fill dello sfondo e quantizzazione;
- atlas PNG caricato dal gioco;
- fallback geometrici per celle assenti o rifiutate;
- benchmark sulla RX 5600 XT;
- preset di generazione 256/512;
- comando `--bench`;
- knowledge base con modalità solo-curato, RunBundle, validazione e Piano 0;
- ledger dataset e training runbook iniziale.

## Prestazioni già misurate

Macchina di riferimento:

- Ryzen 5 3600;
- RX 5600 XT 6 GB;
- Vulkan;
- Qwen 7B Q4_K_M e Stable Diffusion caricati in sequenza.

Misure interne presenti nei documenti del progetto:

- Qwen 7B: circa 50 secondi per il contenuto della run;
- Stable Diffusion: circa 5,3 secondi per immagine nella configurazione dello spike;
- circa 2 GB VRAM per la pipeline immagini;
- caricamento modello SD circa 10 secondi;
- 12 sprite circa 75–85 secondi.

Queste sono misure della macchina e del commit specifico documentato, non promesse per
ogni computer.

## Vincoli correnti del generatore sprite

### Una sola LoRA

`SpriteSdCtx` contiene attualmente:

```c
sd_lora_t lora;
int hasLora;
```

La chiamata di inferenza passa un solo elemento. Questo impedisce di combinare in modo
pulito:

- LCM-LoRA;
- Style LoRA;
- LoRA di ruolo.

Prima modifica ML consigliata: supporto a più LoRA.

### Atlas fisso

La pipeline genera un atlas fisso:

- celle 128×128;
- 8 colonne;
- 13 celle note;
- indici dipendenti dall'enum `AtlasSprite`.

È adatto allo spike e ai fallback globali, ma non a:

- molti nemici per run;
- animazioni;
- body plan modulari;
- più direzioni;
- varianti per piano.

L'atlas fisso deve restare come fallback/compatibilità, mentre i nemici generati passano a
bundle per archetipo.

### Player non usa lo sprite generato

Il renderer disegna il giocatore come stickman fisso per mantenere socket affidabili degli
oggetti. La cella `SPR_PLAYER` viene generata ma non usata.

Questa scelta è coerente con la pipeline modulare. Va resa esplicita:

- vertical slice: stickman/rig modulare;
- fase successiva: skin pixel-art applicata al rig;
- spritesheet completamente raster soltanto quando socket e coerenza sono risolti.

### Sprite nemici statici

Ogni nemico usa una cella statica scelta dalla forma. Non esiste ancora un animator per
clip, frame, eventi e direzione.

## Decisioni della knowledge base da preservare

- Piano 0 come luogo della generazione.
- Primo piano giocabile prima di pubblicare il bundle.
- Generazione non bloccante quando possibile.
- Modalità solo-curato permanente.
- Fallback per ogni contenuto.
- Contenuti comportamentali validati in sandbox.
- RunBundle riproducibile.
- Il giocatore non vede prompt o errori tecnici.
- Il Piano 1 rappresenta alla lettera il tema scelto.
- Nemici leggibili e roster limitato per run.
- Componenti modulari, rig e socket invece di asset completi illimitati.
- Nessuna inferenza nel combattimento.

## Gap principali

1. dataset commerciale curato;
2. notebook Kaggle riproducibile;
3. supporto multi-LoRA;
4. formato SpriteBundle;
5. body plan e rig runtime;
6. animator raylib;
7. validazione automatica degli asset per ruolo;
8. orchestrazione agenti/Kaggle;
9. manifest degli esperimenti;
10. licenze e NOTICE dei pesi/LoRA/dataset.
