---
id: aiprod-distribuzione
title: Distribuzione
domain: ai-production
status: draft
authority: supporting
owner: ai-production
summary: >-
  Perimetro del tema distribuzione (DEC-158): piattaforme Linux+Windows per la release 1
  (DEC-193), AI disclosure su pagina negozio e crediti in gioco (DEC-194), modelli via
  download al primo avvio (DEC-195), requisiti della pagina negozio incluso l'hardware
  minimo in numeri (DEC-142). Import di contenuti utente rimandato a dopo la release.
last_reviewed: 2026-07-31
last_verified_commit: 4d7a410
topics: [distribuzione, piattaforme, pagina-negozio, ai-disclosure, licenze, steam, DEC-193, DEC-194, DEC-195]
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
su Linux. **Decisione (DEC-193, 2026-07-31):** le piattaforme della **prima release** sono
**Linux e Windows**. Il supporto **Steam Deck** riceve una verifica dedicata in seguito
(compatibilità Proton, controlli, UI a schermo piccolo), ma **non è un requisito della
release 1**. macOS non è nominato, non incluso nella prima release.

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

**Meccanismo di consegna dei modelli — decisione (DEC-195, 2026-07-31):** i modelli
arrivano via **download dal gioco al primo avvio**, per chi sceglie l'esperienza completa
(DEC-070/DEC-086). Chi sceglie "solo curato" non scarica nulla. Il gioco dichiara fonte e
verifica dei pesi scaricati; i pesi non sono mai ridistribuiti insieme al gioco
(DEC-113/DEC-140 invariate). Il dettaglio tecnico (URL di distribuzione, verifica di
integrità/hash, gestione di reti lente o interrotte) resta lavoro di implementazione
futuro, non fissato da questa decisione.

Il formato esatto della pagina e quali altri punti oltre a quelli sopra sono obbligatori
restano domande aperte.

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

**Decisione (DEC-194, 2026-07-31):** la disclosure vive in **due luoghi** — la **pagina
negozio** (dichiarazione pre-acquisto) e i **crediti in gioco**, con una **voce dedicata nel
`MainMenu`** che riporta il dettaglio dei modelli usati, le licenze (Stability Community
License, Gemma Terms of Use) e la provenienza CC0 dei dataset. Il **contenuto minimo esatto**
(quali modelli nominare per nome, come formattare le licenze) e l'eventuale differenziazione
fra modalità solo-curato e modalità completa restano dettagli operativi di questo dominio,
non fissati numericamente dalla decisione.

## Cross-reference

- [`licenze.md`](licenze.md) — fonte canonica delle licenze di modelli e codice; questo
  documento non le riformula, le cita.
- [`docs/engineering/multiplayer-steam.md`](../engineering/multiplayer-steam.md) — nota
  tecnica proposta su Steamworks; rimanda qui per il tema distribuzione e non lo
  ridecide.
- DEC-142 — policy di formulazione numerica dei requisiti hardware minimi.
- DEC-113, DEC-140, DEC-148 — licenze e provenienza da cui dipende l'AI disclosure.

## Domande aperte

Tre delle quattro domande di questo dominio sono state chiuse il 2026-07-31 in
[`docs/design/governance/open-questions.md`](../design/governance/open-questions.md#distribuzione):

- ~~piattaforme di destinazione della prima release~~ (open question 14, chiusa da DEC-193);
- ~~formato e collocazione della AI disclosure~~ (open question 15, chiusa da DEC-194); i
  requisiti esatti restanti della pagina negozio restano da scrivere;
- ~~meccanismo di consegna dei modelli all'utente~~ (open question 16, chiusa da DEC-195);
- modding/import di contenuti utente — modelli, LoRA, prompt pack, rig, AudioSpec, skin
  UI (open question 17): **rimandato esplicitamente a dopo la release**, quando la pipeline
  generativa definitiva esisterà; resta aperta, non decisa.

Questo documento non decide da solo: registra le decisioni prese altrove (decision-log) e
i dettagli operativi ancora da scrivere in questo dominio.
