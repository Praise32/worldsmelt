---
id: aiprod-architettura-runtime
title: Architettura runtime della generazione
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Base tecnica per scheduling del modello di testo attivo e di SD, chiave di cache, pubblicazione atomica e failure policy; la scala a 4 tier descritta e' superata da DEC-110/111 (scelta binaria completo/solo-curato) e resta come materiale da riscrivere.
last_reviewed: 2026-07-27
topics: [tier, scheduling, cache, pubblicazione-atomica, failure-policy, vram]
related: []
supersedes: []
source_files: []
---
# Architettura runtime della generazione

> **Nota (2026-07-27, DEC-110/DEC-111):** i tier intermedi (Tier 1-2) e il flusso «il
> benchmark propone un tier» descritti sotto sono superati: al giocatore è offerta solo la
> scelta binaria completo / solo-curato, senza tier di qualità né auto-run del benchmark al
> primo avvio. Il documento resta `proposed` come base tecnica (scheduling, cache, failure
> policy) in attesa di riscrittura; il drift è tracciato in `_meta/DOC-CODE-DRIFT.md`.

## Obiettivo

Permettere a più computer possibili di giocare, mantenendo la generazione locale come
funzione scalabile e non come requisito assoluto.

## Tier

### Tier 0 — Solo curato

- nessun modello;
- asset inclusi;
- RunManifest curato/procedurale;
- partenza immediata;
- esperienza completa e supportata.

### Tier 1 — Testo generativo, grafica curata

- il modello di testo attivo (oggi Gemma-3-4B-IT Q4, DEC-140) genera contenuto e comportamento;
- sprite curati o composti;
- adatto a GPU con poca memoria o CPU.

### Tier 2 — Grafica leggera

- il modello di testo attivo crea le specifiche;
- SD genera una reference o pochi componenti per archetipo;
- rig raylib anima;
- LCM e 256 px opzionali;
- cache aggressiva.

### Tier 3 — Grafica completa

- più candidati;
- 512 px;
- più LoRA;
- frame chiave controllati;
- ControlNet quando necessario;
- variazioni per piano.

## Scheduling

```text
1. il modello di testo carica
2. genera e valida RunManifest/EnemySpec
3. il modello di testo si scarica
4. SD carica
5. genera asset minimi
6. SD si scarica
7. motore valida e pubblica SpriteBundle
8. gioco entra nel piano
```

Il modello di testo e SD non devono coesistere in VRAM sulla macchina da 6 GB.

## Priorità di generazione

1. anteprime delle carte tema;
2. asset indispensabili al Piano 1;
3. boss Piano 1;
4. oggetti/proiettili indispensabili;
5. decorazioni;
6. piani successivi;
7. varianti cosmetiche.

## Cache

Chiave:

```text
pipeline_version
model_sha256
lora_sha256[]
prompt_hash
negative_prompt_hash
seed
generation_size
steps
cfg
postprocess_version
asset_role
rig_version
```

Un asset valido già in cache non viene rigenerato.

## Pubblicazione atomica

Ogni bundle viene scritto in una cartella temporanea:

```text
generated/tmp/<bundle-id>/
```

Dopo validazione completa:

```text
rename -> generated/bundles/<bundle-id>/
```

Il manifest della run viene aggiornato soltanto dopo la pubblicazione.

## Failure policy

- modello assente: Tier inferiore;
- caricamento fallito: fallback curato;
- cella vuota: fallback geometrico;
- asset tocca bordo: rigenerazione o fallback;
- alpha errato: postprocess;
- silhouette non valida: altro seed;
- timeout: interrompere, non bloccare;
- run incompleta: usare bundle precompilato.

## Nessuna inferenza in combattimento

Durante il combattimento sono ammessi:

- caricamento texture;
- animazione;
- compositing;
- palette swap;
- particelle;
- shader;
- rig procedurali;
- lettura cache.

Non sono ammessi:

- Stable Diffusion;
- il modello di testo;
- download;
- compilazione di nuovi script;
- validazione pesante.

## Benchmark al primo avvio

Misurare separatamente:

- token/s del modello di testo;
- SD secondi per immagine;
- VRAM;
- tempo load/unload;
- memoria sistema;
- spazio disco.

Il benchmark propone un tier, ma il giocatore può cambiarlo.
