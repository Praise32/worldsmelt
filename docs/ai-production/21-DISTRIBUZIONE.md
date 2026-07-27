---
id: aiprod-distribuzione
title: Distribuzione
domain: ai-production
status: draft
authority: supporting
owner: ai-production
summary: >-
  Perimetro del tema distribuzione (DEC-158): piattaforme di destinazione, requisiti della
  pagina negozio incluso l'hardware minimo in numeri (DEC-142), AI disclosure per i
  contenuti generati a runtime. Le scelte concrete restano domande aperte.
last_reviewed: 2026-07-27
topics: [distribuzione, piattaforme, pagina-negozio, ai-disclosure, licenze, steam]
related: []
supersedes: []
source_files: []
---
# Distribuzione

> **Proprietà del tema assegnata da DEC-158 (2026-07-27):** il tema **distribuzione**
> appartiene al dominio `ai-production`, che già possiede licenze e provenienza dei
> contenuti — le due materie da cui la AI disclosure dipende. `docs/engineering/
> multiplayer-steam.md` rimanda a questo documento senza duplicarne il contenuto. Questo
> documento fissa **lo scope** del tema, non le scelte concrete: piattaforme, formato di
> disclosure e requisiti esatti di pagina restano domande aperte (vedi in fondo).

## Scope

Tre materie, tutte ancora prive di una decisione concreta:

1. **Piattaforme di destinazione** — su quali sistemi operativi/negozi il gioco viene
   pubblicato.
2. **Requisiti della pagina negozio** — cosa la pagina deve dichiarare al giocatore prima
   dell'acquisto, inclusi i **requisiti hardware minimi in numeri**.
3. **AI disclosure** — come e dove il gioco dichiara che i contenuti (sprite, testi,
   audio) sono generati, in parte, a runtime da modelli locali.

## Piattaforme

Il repository conserva sia il ramo Windows sia quello Linux, ma lo sviluppo corrente è
su Linux. Nessuna decisione fissa ancora il set di piattaforme della prima release
(Linux, Windows, Steam Deck, macOS): vedi la domanda aperta collegata sotto.

`docs/engineering/multiplayer-steam.md` valuta Steamworks come possibile via di
distribuzione (leaderboard, achievement, Workshop, cloud save) per il multiplayer
asincrono già approvato (DEC-016, DEC-021), ma **non è una decisione di distribuzione**:
resta una nota tecnica proposta, non ancora scelta.

## Requisiti della pagina negozio

La pagina negozio deve poter dichiarare, quando esisterà:

- **requisiti hardware minimi in numeri** — VRAM, RAM, sistema operativo, misurati sulla
  macchina di riferimento, non nomi di modello (DEC-142: un nome di modello cambia a ogni
  comparison, un numero misurato no; le misure concrete restano un'attività successiva,
  non ancora eseguita);
- eventuale distinzione fra requisito della **modalità solo-curato** e requisito del
  **gioco completo** (invariante di dominio: modalità solo-curato sempre dignitosa, vedi
  `README.md`);
- che i **pesi dei modelli non sono distribuiti col gioco** (li scarica l'utente al primo
  avvio o successivamente — invariante di dominio, ribadita da `licenze.md` e da
  DEC-113/DEC-140/DEC-148 per i modelli oggi in stack);
- l'eventuale menzione di generazione IA come parte dell'esperienza (si veda AI
  disclosure sotto).

Il formato esatto della pagina e quali di questi punti sono obbligatori restano domande
aperte.

## AI disclosure

Il gioco genera a runtime contenuti percepiti dal giocatore (sprite, testi, in prospettiva
audio) usando modelli locali scaricati dall'utente. Questo rende necessaria una qualche
forma di **dichiarazione d'uso dell'IA generativa**, la cui necessità nasce direttamente
dalle licenze e dalla provenienza già registrate in questo dominio:

- `docs/ai-production/licenze.md` — licenze di modelli e codice, incluse le clausole di
  ridistribuzione (mai i pesi) e le soglie di ricavi (Stability Community License,
  DEC-113; Gemma Terms of Use per il modello di testo attivo, DEC-140, e per il
  componente T5Gemma di Stable Audio);
- DEC-148 — provenienza e destino della pipeline immagini (SD1.5, Style LoRA, dataset
  definitivi del proprietario).

Non sono ancora decisi: il **formato** della disclosure (pagina negozio, schermata al
primo avvio, crediti in gioco, una combinazione), il suo **contenuto minimo** (quali
modelli nominare, se nominare le licenze), e se serva differenziarla fra modalità
solo-curato (nessuna generazione) e modalità completa.

## Cross-reference

- [`licenze.md`](licenze.md) — fonte canonica delle licenze di modelli e codice; questo
  documento non le riformula, le cita.
- [`docs/engineering/multiplayer-steam.md`](../engineering/multiplayer-steam.md) — nota
  tecnica proposta su Steamworks; rimanda qui per il tema distribuzione e non lo
  ridecide.
- DEC-142 — policy di formulazione numerica dei requisiti hardware minimi.
- DEC-113, DEC-140, DEC-148 — licenze e provenienza da cui dipende l'AI disclosure.

## Domande aperte

Le scelte concrete di questo dominio sono domande aperte in
[`docs/design/governance/open-questions.md`](../design/governance/open-questions.md#distribuzione):

- piattaforme di destinazione della prima release (open question 14);
- formato e collocazione della AI disclosure, requisiti esatti della pagina negozio (open
  question 15);
- meccanismo di consegna dei modelli all'utente (open question 16);
- modding/import di contenuti utente — modelli, LoRA, prompt pack, rig, AudioSpec, skin
  UI (open question 17).

Questo documento non decide nessuna di queste domande: fissa solo a chi appartiene il
tema e da cosa dipende.
