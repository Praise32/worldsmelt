---
id: aiprod-asset-curation-and-floor-zero
title: Curation degli asset e Piano 0
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Pipeline di curation (candidate->curated->fallback) con stati, directory art/, manifest e proposta di Piano 0 ibrido curato+generato.
last_reviewed: 2026-07-22
topics: [curation, floor-zero, manifest, licenze, asset-review]
related: []
supersedes: []
source_files: []
---
# Curation degli asset e Piano 0

## Obiettivo

Ogni esperimento utile deve produrre valore permanente. Le migliori generazioni non restano
in cartelle casuali: diventano una libreria curata e versionata.

```text
candidate
→ validazione automatica
→ review umana
→ accepted
→ approved-curated
→ catalogo/fallback/Piano 0
```

## Stati

- `candidate`: appena prodotto;
- `auto-rejected`: non supera controlli tecnici;
- `needs-review`: tecnicamente valido;
- `accepted`: utile per test;
- `approved-curated`: può apparire nel gioco venduto;
- `fallback`: approvato come sostituto;
- `quarantine-license`: provenienza o licenza incompleta;
- `deprecated`: non usare;
- `rejected`: conservare solo metadati essenziali.

## Directory

```text
art/
├── candidates/
├── reviewed/
├── curated/
│   ├── floor-zero/
│   ├── enemies/
│   ├── characters/
│   ├── items/
│   ├── environments/
│   ├── ui/
│   ├── vfx/
│   └── audio/
├── fallback/
└── quarantine/
```

Gli artifact grandi possono vivere fuori Git; nel repository restano manifest, miniature
e checksum secondo la policy scelta.

## Manifest

```json
{
  "id": "ui_panel_bone_04",
  "type": "ui_nineslice",
  "status": "approved-curated",
  "source_branch": "commercial-clean",
  "generation": {
    "model": "sd15",
    "model_sha256": "",
    "loras": [],
    "seed": 184923,
    "prompt": "",
    "pipeline_version": ""
  },
  "review": {
    "reviewer": "",
    "date": "",
    "scores": {},
    "notes": ""
  },
  "approved_for": ["floor-zero", "fallback", "theme-generation"]
}
```

## Piano 0 ibrido

Proposta:

- struttura, navigazione e leggibilità completamente curate;
- asset iconici selezionati dalle migliori generazioni;
- contenuti della run generati/cache/fallback;
- nessun aspetto essenziale del Piano 0 dipende dal successo di un modello;
- il Piano 0 migliora nel tempo con il catalogo curato.

Il Piano 0 non deve sembrare un menu tecnico né una demo della generazione.

## Browser di curation

Tool interno proposto:

```text
bin/melting-curator
```

Funzioni:

- filtri per tipo, esperimento e licenza;
- anteprima in scala reale;
- sfondo chiaro/scuro/in-game;
- animazione;
- audio;
- confronto A/B;
- approve/reject;
- tagging;
- promozione;
- duplicati;
- esportazione manifest;
- motivazione della decisione.

La prima versione può essere una tool UI raygui.

## Review

Immagini:

- leggibilità;
- stile;
- originalità;
- body plan;
- animabilità;
- alpha;
- palette;
- ruolo;
- uso in-engine.

Audio:

- mix;
- categoria;
- impatto;
- affaticamento;
- loop;
- somiglianza;
- priorità;
- accessibilità.

UI:

- 9-slice;
- focus;
- stati;
- testo;
- controller;
- scala;
- contrasto;
- coerenza.

## Regola di non-contaminazione

Un asset proveniente da esperimenti con dataset incerto non viene promosso nel ramo
commerciale soltanto perché è bello. Deve essere rigenerato o ricostruito con una pipeline
commercial-clean.

## Risultato atteso

Alla pre-alpha:

- il gioco funziona con curati e fallback;
- la generazione produce candidate;
- la review alimenta il catalogo;
- il Piano 0 usa il meglio del catalogo;
- la qualità aumenta senza rendere fragile il runtime.
