---
id: ai-prod-decision-questionnaire
status: active
owner: product
last_reviewed: 2026-07-20
summary: "Coda di domande che l'orchestratore deve porre prima di assumere decisioni mancanti."
---

# Questionario decisionale

## Regole d'uso

- Non porre domande già risolte nella knowledge base.
- Non porre tutte le domande in una volta.
- Selezionare al massimo sette domande rilevanti per il task corrente.
- Una risposta deve essere trasferita nel documento canonico e nel decision log.
- Le raccomandazioni non sono decisioni.
- `BLOCKING` impedisce l'implementazione relativa.
- `SOON` consente un prototipo con default reversibile.
- `LATER` non blocca la vertical slice.

## Audio

### Q-AUD-001 — Generazione audio nella pre-alpha

**Priorità:** BLOCKING  
**Conflitto:** DEC-036 considera audio generativo futuro; la nuova strategia lo include.

Quale regola diventa canonica?

A. Audio completamente curato fino alla release iniziale.  
B. Generazione audio solo come tool di sviluppo/curation, non nel Piano 0.  
C. Generazione audio opzionale nel Piano 0, con fallback curato.  
D. Generazione audio parte centrale obbligatoria del Piano 0.

**Raccomandazione:** C oppure B.  
**Blocca:** AudioSpec runtime, Stable Audio nel Piano 0, modifica DEC-036.

### Q-AUD-002 — Musica o solo SFX

**Priorità:** SOON

Il primo esperimento Stable Audio deve riguardare:

A. soltanto SFX;  
B. soltanto musica;  
C. entrambi con due milestone separate.

**Raccomandazione:** C, iniziando dagli SFX.

### Q-AUD-003 — Distribuzione del modello audio

**Priorità:** SOON

A. mai distribuito, solo produzione;  
B. download opzionale;  
C. incluso nel gioco;  
D. modello selezionato dall'utente.

**Raccomandazione:** A nella pre-alpha, B successivamente.

### Q-AUD-004 — Licenza con soglia di ricavi

**Priorità:** BLOCKING prima della vendita

Accetti una dipendenza con Community License gratuita sotto 1 milione USD di ricavi annui,
con necessità di licenza Enterprise oltre la soglia?

A. sì;  
B. sì solo per produzione, non distribuzione;  
C. no, cercare alternativa permissiva;  
D. rinviare decisione ma non usare output nella release.

## UI

### Q-UI-001 — Penpot come fonte di design

**Priorità:** SOON

A. Penpot canonico;  
B. file Markdown + PNG/SVG senza Penpot;  
C. altro strumento;  
D. Penpot solo per mockup.

**Raccomandazione:** A o D.

### Q-UI-002 — Risoluzione logica

**Priorità:** BLOCKING per implementazione UI

Qual è la risoluzione logica iniziale?

A. 640×360;  
B. 960×540;  
C. 1280×720;  
D. altra.

**Nota:** gli appunti propongono 640×360, ma non è una decisione approved.

### Q-UI-003 — Pixel scale

**Priorità:** BLOCKING per art bible

A. pixel nativi molto grandi;  
B. pixel medi;  
C. dettaglio alto ma pixel snapping;  
D. scala diversa fra mondo e UI.

### Q-UI-004 — Input al lancio

**Priorità:** BLOCKING per focus/navigation

Selezionare:

- mouse;
- tastiera;
- controller;
- touch.

Indicare quali sono obbligatori nella prima release.

### Q-UI-005 — Self-host Penpot

**Priorità:** LATER

A. cloud Penpot;  
B. self-host;  
C. entrambi;  
D. nessuna integrazione MCP iniziale.

## Immagini e training

### Q-IMG-001 — Distribuzione delle LoRA

**Priorità:** BLOCKING prima della distribuzione

A. private, usate solo per creare asset;  
B. download opzionale;  
C. incluse;  
D. modding/import di LoRA utente.

### Q-IMG-002 — Generazione nel prodotto

**Priorità:** BLOCKING

A. soltanto produzione;  
B. opzionale nel Piano 0;  
C. obbligatoria;  
D. tier in base all'hardware con modalità solo-curato.

**Raccomandazione:** D.

### Q-IMG-003 — Dataset originale

**Priorità:** SOON

Quante immagini originali/commissionate puoi realisticamente revisionare?

A. 100–300;  
B. 300–1000;  
C. oltre 1000;  
D. quasi nessuna.

### Q-IMG-004 — Review manuale

**Priorità:** BLOCKING

Ogni asset `approved-curated` richiede approvazione umana?

A. sì sempre;  
B. sì per release, no per test;  
C. solo campionamento;  
D. no.

**Raccomandazione:** B.

### Q-IMG-005 — Target hardware minimo

**Priorità:** BLOCKING per tier

Definire CPU, RAM, GPU/VRAM e sistema operativo minimo per:

- modalità solo-curato;
- Qwen;
- SD;
- audio.

## Animazioni

### Q-ANIM-001 — Body plan iniziali

**Priorità:** SOON

Scegliere i primi quattro:

- blob;
- flying;
- tentacled;
- biped;
- quadruped;
- mounted;
- crawler;
- serpentine;
- turret.

**Raccomandazione:** blob, flying, tentacled, biped.

### Q-ANIM-002 — Player

**Priorità:** SOON

A. rig modulare con skin;  
B. spritesheet completo;  
C. ibrido;  
D. stickman fino a vertical slice.

**Raccomandazione:** C, con D come primo passo.

### Q-ANIM-003 — Direzioni di mira

**Priorità:** BLOCKING

Controllo e direzioni:

- twin-stick/mira libera;
- quattro direzioni;
- otto direzioni;
- altra soluzione.

Questa domanda esiste già negli appunti storici: risolverla nella KB, non duplicarla.

## Piano 0 e curation

### Q-F0-001 — Identità del Piano 0

**Priorità:** BLOCKING per art direction

A. completamente curato;  
B. curato con dettagli generati;  
C. composto dalle migliori generazioni storiche;  
D. cambia ad ogni run.

**Raccomandazione:** B+C: struttura curata, libreria costruita dalle migliori generazioni.

### Q-F0-002 — Catalogo dei candidate

**Priorità:** SOON

Vuoi un tool interno con approve/reject già nella pre-alpha?

A. sì;  
B. dopo il primo training;  
C. solo script/manifest;  
D. usare cartelle manualmente.

**Raccomandazione:** B.

## Agenti

### Q-AG-001 — Autorità dell'orchestrator

**Priorità:** BLOCKING

L'orchestrator può:

A. solo preparare piani;  
B. modificare documentazione ma non codice;  
C. delegare implementazione dopo i gate;  
D. avviare anche training autorizzati.

**Raccomandazione:** C, con D soltanto quando `approved_gpu_run=true`.

### Q-AG-002 — Commit e push

**Priorità:** BLOCKING

Il root `CLAUDE.md` dice commit/push su main dopo verifica, mentre gli agent file dicono che
l'implementer non committa.

Definire:

A. orchestrator committa dopo APPROVA;  
B. utente committa;  
C. PR/branch;  
D. main diretto.

**Raccomandazione:** C per task ML/UI/audio; A per piccoli fix se già usato dal progetto.

### Q-AG-003 — Sessione domande

**Priorità:** SOON

A. una sessione dedicata che risolve tutte le decisioni di una milestone;  
B. domande durante ogni task;  
C. entrambe: design session prima, escalation locale dopo.

**Raccomandazione:** C.

## Distribuzione

### Q-DIST-001 — Piattaforme iniziali

**Priorità:** BLOCKING

- Linux;
- Windows;
- Steam Deck;
- macOS.

Il repository conserva Windows, ma Linux è principale: definire il target di release.

### Q-DIST-002 — Downloader modelli

**Priorità:** SOON

A. installer separato;  
B. primo avvio;  
C. Steam DLC/tool;  
D. nessun modello distribuito.

### Q-DIST-003 — Modding

**Priorità:** LATER

Il giocatore può importare:

- modelli;
- LoRA;
- prompt pack;
- rig;
- AudioSpec;
- UI skin?

## Budget

### Q-BUD-001 — Budget cloud iniziale

**Priorità:** SOON

A. zero;  
B. fino a 50 USD;  
C. fino a 150 USD;  
D. oltre 150 USD.

### Q-BUD-002 — Tempo di review settimanale

**Priorità:** SOON

Quante ore puoi dedicare a:

- rispondere alle domande;
- review di PR;
- review immagini;
- ascolto audio;
- playtest?

L'orchestrator deve dimensionare i batch sul tempo reale disponibile.
