---
id: aiprod-asset-curation-and-floor-zero
title: Curation degli asset e Piano 0
domain: ai-production
status: approved
authority: supporting
owner: ai-production
summary: >-
  Pipeline di curation (candidate->curated->fallback) con stati, directory art/, manifest e proposta di Piano 0 ibrido curato+generato. La demo delle meccaniche usa il dataset Kenney CC0 come fonte curata provvisoria, in attesa della Style LoRA (DEC-171). Il lavoro immagini della demo è ora la produzione pixel-art con Aseprite, fuori dallo scope testi/parametri del catalogo curato (DEC-175); il campo `id` del manifest è l'image-id del layer di indirezione contenuto→immagine.
last_reviewed: 2026-07-28
last_verified_commit: 892911a
topics: [curation, floor-zero, manifest, licenze, asset-review, DEC-171, demo, kenney, dataset-cc0, DEC-175, aseprite, indirezione-image-id]
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

Il campo `id` del manifest è l'**image-id** del layer di indirezione contenuto→immagine
descritto sotto (DEC-175): il contenuto (oggetto, nemico, boss, ecc.) non referenzia mai
`body.png` o un percorso file, ma la categoria/famiglia che questo manifest risolve in un
file concreto.

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

## Demo delle meccaniche: dataset Kenney CC0 come fonte curata provvisoria (DEC-171)

L'obiettivo della demo è **coprire tutti i sistemi documentati** (fusioni, sinergie,
correzione di fortuna, economia, ecc.), con priorità sulla copertura del design rispetto a
una run end-to-end rifinita. Il contenuto curato della demo usa le **immagini del dataset di
training già registrato**, in `dataset-raw/` (pacchetti **Kenney** più
`superpowers-asset-packs`, licenza **CC0**, **~2.567 PNG**, verificato nel ledger CC0 di
[Dataset e licenze](04-DATASET-LICENZE.md)): non un dataset separato creato apposta per la
demo.

L'immagine di un oggetto **nato da fusione** (che non ha un'immagine curata propria) si
**pesca dal dataset fra le immagini non ancora usate nella run corrente**, con scelta
**deterministica dal seed di run** (dettaglio della meccanica in
[Item Fusion](../design/systems/item-fusion.md)). **Nessun modello immagine gira a runtime
nella demo**: coerente con la regola di `AGENTS.md` sull'indipendenza del motore dai modelli
AI (il runtime legge solo file locali già validati).

Questa soluzione è **esplicitamente provvisoria**: serve solo al playtest delle meccaniche
finché SD non è stato addestrato con la Style LoRA (pipeline definitiva DEC-148, che resta
invariata). Non cambia la regola di non-contaminazione qui sopra: le immagini Kenney CC0
restano una fonte già verificata, non un asset da esperimenti con dataset incerto.

## Il lavoro immagini della demo passa alla produzione Aseprite (DEC-175)

Il **catalogo curato** di cui parlano `generated-content-validation.md` e `item-fusion.md`
si limita a **testi e parametri** (nome, comportamento, statistiche, bande di garanzia) con
un **auto-mapping provvisorio alle immagini CC0 per categoria/tag** — il meccanismo di
pesca descritto sopra e in [Item Fusion](../design/systems/item-fusion.md), DEC-171. Il
**lavoro immagini** vero e proprio — disegnare gli sprite del gioco — **esce da questo
scope** e passa alla **produzione pixel-art con Aseprite** (toolchain locale + server MCP,
vedi `08-PIPELINE-SPRITE-ANIMAZIONI.md`): non è più (solo) un problema di mappatura verso
un corpus preesistente, è produzione originale diretta.

Le due cose convivono senza contraddirsi: finché gli sprite Aseprite non coprono una
categoria, il ponte provvisorio DEC-171 (auto-mapping CC0 per tag) resta il fallback
attivo della demo. Quando uno sprite originale esiste, sostituisce il placeholder tramite
il layer di indirezione **contenuto → image-id** (il campo `id` del manifest sopra): il
contenuto non cambia, cambia solo quale immagine risolve il suo `id`.

Gli stessi sprite Aseprite sono anche il **dataset definitivo delle LoRA** (DEC-148/168),
organizzato per famiglia in `dataset/worldsmelt-style/` (dettaglio in
`03-PIANO-LORA.md`). I **189 sprite curati CC0** già rimappati alla palette ufficiale
«Fucina di Worldsmelt» (DEC-173) possono integrare l'aggregato `_general/` di quel
dataset, con provenienza distinta (`cc0`, non `own`) nel ledger.

## Risultato atteso

Alla pre-alpha:

- il gioco funziona con curati e fallback;
- la generazione produce candidate;
- la review alimenta il catalogo;
- il Piano 0 usa il meglio del catalogo;
- la qualità aumenta senza rendere fragile il runtime.
