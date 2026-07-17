---
id: gd-project-brief
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Sintesi iniziale del gioco descritta dal creatore."
---

# Project Brief

## Elevator pitch

Action roguelite a stanze con run uniche, nel quale un'IA locale genera durante il gioco nuovi oggetti, nemici, boss, combinazioni visive e sinergie. La casualità è controllata da pool, rarità, progressione e regole di qualità, così ogni run è sorprendente ma non priva di struttura.

## Concetti già dichiarati

- Una run standard è composta inizialmente da cinque piani.
- Il primo piano è sempre giocabile e usa contenuti preesistenti, verificati e appartenenti a pool curati.
- Durante il primo piano vengono generati i piani successivi.
- Se le risorse e il tempo lo consentono, il gioco può preparare contenuti per una run futura.
- Oggetti, nemici, boss, sprite e sinergie possono essere generati o composti dall'IA locale.
- Le sinergie devono modificare sia il comportamento sia la presentazione visiva.
- Gli oggetti appartengono a pool e possiedono rarità o pesi di estrazione.
- Il gioco deve permettere apprendimento e miglioramento del giocatore, evitando il caos totale come unica esperienza.
- Una modalità separata può massimizzare la casualità.
- Sono previste risorse e stanze speciali: salute, chiavi, bombe, tesori, segreti, ostacoli e ricompense speciali.
- Sono previste categorie quali oggetti passivi, oggetti attivi e trinket.
- È desiderato un multiplayer competitivo basato su corse e classifiche.

## Principale rischio di design

La generazione infinita può produrre contenuti incoerenti, illeggibili, sbilanciati o non memorizzabili. Il progetto deve quindi trattare l'IA come generatore vincolato, non come arbitro assoluto.

## Stato

Questo brief registra l'intenzione iniziale. I dettagli mancanti devono essere risolti nei documenti specifici e nel registro delle decisioni.
