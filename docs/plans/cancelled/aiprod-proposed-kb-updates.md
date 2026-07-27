---
id: plans-aiprod-proposed-kb-updates
title: Aggiornamenti proposti alla knowledge base (ANNULLATO da DEC-147)
domain: plans
status: superseded
authority: historical
owner: ai-production
summary: >-
  ANNULLATO il 2026-07-27 (DEC-147): piano che duplicava la coda di open questions come seconda coda decisionale. Le sue proposte residue sono trasferite in docs/design/governance/open-questions.md; quelle già risolte sono chiuse citando la DEC pertinente. Testo originale conservato sotto.
last_reviewed: 2026-07-27
topics: [kb-updates, audio, floor-zero, run-manifest, visual-language, annullato]
related: []
supersedes: []
source_files: []
---
# Aggiornamenti proposti alla knowledge base (ANNULLATO)

> **Annullato il 2026-07-27 (DEC-147):** questo piano teneva una seconda coda di proposte
> in concorrenza con `docs/design/governance/open-questions.md`, in violazione della
> regola «niente registri paralleli» di `docs/CLAUDE.md`. Stato dei punti sotto:
>
> - **Audio** — `Q-AUD-001` è chiusa da DEC-109 (generazione audio ammessa nel Piano 0,
>   con fallback curato); la sezione DEC-036 di `audio-and-feedback.md` è materia già
>   coperta da quella decisione.
> - **Visual Language** — proposta non trasferita come domanda: resta materiale di
>   riferimento per una futura design review dedicata all'interfaccia (vedi le domande
>   aperte 11-13 su risoluzione logica, scala dei pixel e strumento di design).
> - **Floor Zero** — `Q-F0-001` è chiusa da DEC-004/DEC-063/DEC-085; il residuo sul
>   catalogo dei candidate (`Q-F0-002`) è trasferito in open question 19.
> - **Generated Content Validation** — le proposte (stati candidate/curated/quarantine,
>   validazione UI/audio, promozione umana, non-contaminazione research/commercial) sono
>   già canone in `docs/design/systems/generated-content-validation.md`; nessun trasferimento
>   necessario.
> - **Run Manifest** — i contratti `EnemySpec`/`SpriteBundle`/`UISkinSpec`/`AudioSpec`
>   citati qui sono già canone in `docs/design/systems/run-manifest-and-reproducibility.md`
>   e nei rispettivi template.
> - **Open Questions** — le domande tecniche/produttive elencate qui come "da non
>   duplicare" sono ora tutte trasferite o chiuse: vedi la nota di chiusura in cima a
>   `docs/archive/superseded/19-DECISION-QUESTIONNAIRE.md` per il dettaglio domanda per
>   domanda.
>
> Il testo originale segue per memoria storica; non è più un piano attivo.

# Aggiornamenti proposti alla knowledge base

Questo file non modifica automaticamente il design canonico.

## Audio

Documento:

```text
docs/design/content/audio-and-feedback.md
```

Conflitto:

- stato attuale: audio curato/statico, generazione futura;
- proposta: pipeline generativa opzionale o di produzione.

Azione:

1. risolvere `Q-AUD-001`;
2. aggiungere una decisione;
3. aggiornare summary, sezione DEC-036, non-obiettivi e scenari;
4. mantenere fallback curato.

## Visual Language

Aggiunte possibili:

- UI costruita nello stesso linguaggio pixel-art;
- componenti modulari;
- compatibilità 9-slice;
- token semantici;
- vincoli per asset AI senza testo.

Richiede design review, non è solo implementazione.

## Floor Zero

Aggiunta possibile:

- struttura curata;
- libreria costruita dalle migliori generazioni storiche;
- generazione runtime soltanto per contenuto della run;
- identità del Piano 0 non dipendente dal successo dei modelli.

Risolvere `Q-F0-001`.

## Generated Content Validation

Aggiungere, se approvato:

- stati candidate/curated/quarantine;
- validazione UI/audio;
- promozione umana;
- non-contaminazione fra research e commercial.

## Run Manifest

Aggiungere contratti:

- `EnemySpec`;
- `SpriteBundle`;
- `UISkinSpec`;
- `AudioSpec`;
- hash e versioni;
- fallback.

Soltanto i campi percepiti dal giocatore appartengono alla KB; dettagli di serializzazione
restano tecnici.

## Open Questions

Non duplicare:

- controllo e camera;
- piattaforme;
- pool curato;
- valori da playtest.

Le questioni tecniche/produttive restano nel questionario della blueprint; quelle che
cambiano l'esperienza vanno promosse nella KB.
