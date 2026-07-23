---
id: gd-project-brief
title: Project Brief
domain: design
status: draft
authority: canonical
owner: design
summary: "Sintesi iniziale del gioco descritta dal creatore."
last_reviewed: 2026-07-18
topics: [pitch, sintesi, storico, worldsmelt]
related: []
supersedes: []
source_files: []
---

# Project Brief

## Elevator pitch

**Worldsmelt** (titolo definitivo, `DEC-071`) è un action roguelite a stanze con run uniche, nel quale un'IA locale genera durante il gioco nuovi oggetti, nemici, boss, combinazioni visive e sinergie. La casualità è controllata da pool, rarità, progressione e regole di qualità, così ogni run è sorprendente ma non priva di struttura.

## Concetti già dichiarati

Aggiornato il 2026-07-17 in base alle decisioni approvate nel registro delle decisioni
(`DEC-001`…`DEC-020`). Il dettaglio operativo di ciascun punto vive nel documento specifico
linkato; questa sezione è solo una sintesi orientativa.

- Una run è composta da **Piano 0** più cinque piani (`DEC-001`). Il Piano 0 è un hub sempre
  giocabile — rifugio, museo delle creazioni migliori, scelta di tema e personaggio — che fa
  da spazio di attesa mentre l'IA genera il primo piano vero e proprio (`DEC-002`, `DEC-004`;
  vedi [Run Structure](04-run-structure.md)).
- Il giocatore sceglie un tema tra 2–3 proposti dall'IA nel Piano 0; il tema evolve o
  degenera piano dopo piano fino al boss del piano 5 (`DEC-005`).
- Oggetti, nemici, boss, sprite, layout di stanza e sinergie possono essere generati o
  composti dall'IA locale entro bande di garanzia, con validazione in sandbox e fallback
  curato sempre presente (`DEC-020`; vedi [AI Content Model](06-ai-content-generation-model.md)).
- Le sinergie hanno doppio binario: implicite/automatiche tra oggetti compatibili, e
  **fusione esplicita** — la meccanica-firma del progetto — nella quale il giocatore consuma
  due oggetti nella stanza di fusione e ottiene un oggetto nuovo generato dall'IA che eredita
  comportamento e presentazione da entrambi (`DEC-012`; vedi [Synergies](systems/synergies.md)
  e [Item Fusion](systems/item-fusion.md)).
- Gli oggetti appartengono a pool e possiedono rarità o pesi di estrazione; la tassonomia
  completa è attivi, passivi, stat-up, Innesti (`DEC-011`; vedi
  [Items, Pools and Rarity](systems/items-pools-and-rarity.md) e [Innesti](systems/grafts.md)).
- Esiste un personaggio base sempre disponibile; ogni run l'IA genera un personaggio
  alternativo con un tratto unico e statistiche casuali entro bande, selezionabile nel
  Piano 0 (`DEC-014`; vedi [Characters](systems/characters.md)).
- Le risorse sono ri-tematizzate e definite per funzione, non per nome preso da altri
  giochi: salute stratificata, valuta principale, strumento di breccia, strumento di
  apertura, catalizzatore di fusione (`DEC-013`; vedi
  [Health and Resources](systems/health-and-resources.md)).
- Il gioco deve permettere apprendimento e miglioramento del giocatore, evitando il caos
  totale come unica esperienza.
- Sconfiggere il boss del piano 5 chiude la run ufficiale (valida per classifiche) e la run
  finisce lì (`DEC-006`, aggiornata da `DEC-031`); una prosecuzione in piani extra resta solo
  un'idea futura non implementata (`DEC-018`).
- Durata obiettivo di una run completa vinta (Piano 0 + 5 piani): 30–45 minuti (`DEC-017`).
- Meta-progressione: persistono il catalogo di tutti i contenuti generati, il museo del
  Piano 0 e punti guadagnati in singleplayer per sbloccare contenuti nei pool delle run
  future; nessun potenziamento permanente del personaggio; sblocchi disattivati nelle
  modalità competitive (`DEC-015`).
- È desiderato un multiplayer competitivo: la visione fissata è quella di gare asincrone
  sulla stessa run/seed con classifiche a tempo/punteggio (`DEC-016`; vedi
  [Multiplayer and Competition](08-multiplayer-and-competition.md)).
- Il nome definitivo del gioco è Worldsmelt, scelto dal proprietario e verificato libero da
  collisioni con giochi esistenti (`DEC-071`); risolve la domanda aperta sul nome citata in
  `DEC-003`. La nomenclatura inglese in-game dei termini di lavoro (Piano 0, Innesto,
  Veterano, valuta, ecc.) è fissata da `DEC-072` (vedi
  [Glossary](governance/glossary.md)).

## Principale rischio di design

La generazione infinita può produrre contenuti incoerenti, illeggibili, sbilanciati o non memorizzabili. Il progetto deve quindi trattare l'IA come generatore vincolato, non come arbitro assoluto.

## Stato

Questo brief registra l'intenzione iniziale e resta `draft` come **registro storico**: non è
la fonte canonica del comportamento del gioco. Il canone vive nei documenti specifici
(`00`–`09` e `systems/`) e nel [registro delle decisioni](governance/decision-log.md); in
caso di divergenza tra questo brief e un documento specifico, prevale il documento
specifico.
