# Project instructions for coding agents

## Canonical game-design source

La fonte canonica del comportamento del gioco è `docs/game-design/`.

Prima di modificare comportamento visibile al giocatore:

1. Leggi `docs/game-design/INDEX.md`.
2. Apri i documenti relativi alla funzionalità.
3. Controlla `docs/game-design/governance/open-questions.md`.
4. Distingui regole `approved`, proposte `draft` e idee `experimental`.
5. Non inventare regole mancanti in silenzio.

## In caso di ambiguità

- Conserva il comportamento canonico già documentato.
- Segnala l'assunzione nel piano di implementazione.
- Aggiungi la questione a `open-questions.md` se influisce sul design.
- Non modificare un documento approvato soltanto per adattarlo al codice esistente.

## Separazione dei documenti

- `docs/game-design/`: esperienza, regole, flussi, contenuti e comportamento.
- `docs/technical/`: architettura, tecnologie e contratti tecnici.
- `docs/plans/`: piani temporanei di implementazione.

## Aggiornamento obbligatorio

Qualsiasi cambiamento al comportamento del gioco deve aggiornare nello stesso lavoro il documento di design pertinente e almeno uno scenario verificabile.

## Generazione IA

I contenuti generati non sono automaticamente validi. Devono rispettare i contratti di design, i limiti di difficoltà, le tassonomie, i pool, le rarità, i vincoli di leggibilità e le regole di fallback.

## Originalità

Non copiare contenuti identificabili da giochi esistenti. Produrre nomi, estetica, regole specifiche e combinazioni originali.
