---
id: gd-system-active-items
title: Active Items
domain: design
status: approved
authority: canonical
owner: design
summary: "Oggetti attivabili volontariamente dal giocatore, una delle 4 categorie della tassonomia oggetti. Ricarica a doppio canale di base — stanze completate ed energia droppata dai nemici — estensibile da oggetti che aggiungono ulteriori modi di ricarica (DEC-059)."
last_reviewed: 2026-07-27
last_verified_commit: 8210480
topics: [oggetti, attivi, ricarica, slot, DEC-059, budget di potenza]
related: []
supersedes: []
source_files: []
---

# Active Items

Gli attivi sono una delle 4 categorie della tassonomia oggetti (attivo, passivo,
stat-up, Innesto). I campi obbligatori di un oggetto sono definiti in
[Items, Pools and Rarity](items-pools-and-rarity.md): questo documento non li
riformula, aggiunge solo le specificità del sottotipo attivo.

## Intento per il giocatore

Un attivo deve creare una decisione sul momento d'uso ("lo uso ora o lo tengo?"), non
essere un bonus automatico privo di scelta.

## Slot

Si parte con **1 slot attivo**; oggetti o eventi rari durante la run possono aggiungere
slot attivi aggiuntivi (vedi [Items, Pools and Rarity](items-pools-and-rarity.md#slot)).
Con più slot attivi il giocatore sceglie quale attivare in un dato momento, non tutti
insieme automaticamente.

## Condizioni di ingresso

Un attivo entra in gioco quando viene raccolto da un **piedistallo** e assegnato a uno
slot attivo libero; a slot pieni la raccolta è uno **scambio col piedistallo** (DEC-117,
sezione dedicata più sotto).

## Input/azioni

- input di attivazione dedicato (esplicito, non automatico);
- condizioni d'uso (es. richiede bersaglio, richiede stanza non in transizione, ecc.).

## Come si attivano e ricaricano

Ogni attivo dichiara uno tra:

- **cariche**: un numero finito di usi che si ricaricano con una fonte esterna (vedi
  "Ricarica a doppio canale" sotto);
- **cooldown**: un tempo di attesa fisso dopo l'uso, indipendente da azioni successive.

Il metodo di ricarica scelto è parte della fantasia dell'oggetto e va dichiarato
esplicitamente, non lasciato implicito.

### Ricarica a doppio canale (DEC-059)

Gli attivi a **cariche** si ricaricano attraverso **due canali di base**, sempre attivi:

1. **completando stanze**;
2. **raccogliendo energia droppata dai nemici**.

Il dosaggio esatto (quanta carica per stanza, quanta energia serve, quanto ne droppa un
nemico) fa parte del budget di potenza dell'oggetto e resta specifico di ciascun attivo,
non un valore unico globale.

Oltre ai due canali di base, alcuni oggetti possono **aggiungere** modi di ricarica
ulteriori (es. danno inflitto, tempo, un evento specifico): il sistema è estensibile per
design, anche da contenuti generati, sempre dentro la validazione descritta in
[Generated Content Validation](generated-content-validation.md) (rimando, non
riformulato qui).

## Risultato

L'effetto dichiarato si applica al momento dell'attivazione, rispettando il budget di
potenza e le eventuali interazioni con altri attivi.

## Feedback

- stato disponibile/in ricarica sempre visibile;
- conferma immediata dell'attivazione;
- comportamento chiaro e comunicato se l'oggetto non può essere usato in quel momento
  (condizione d'uso non soddisfatta, in ricarica, ecc.).

## Interazioni

- con altri oggetti attivi (se più slot attivi sono posseduti);
- con le sinergie implicite e la fusione esplicita, vedi [Synergies](synergies.md);
- persistenza tra stanze e piani: un attivo raccolto resta nella build per tutta la run,
  salvo effetti espliciti che lo rimuovano.

## Regole per contenuti generati

Un attivo generato deve comunque dichiarare input di attivazione, cariche o cooldown,
condizioni d'uso, effetto, feedback e comportamento di fallback prima di poter essere
`approvato-per-run` (vedi [Generated Content Validation](generated-content-validation.md)).
Il budget di potenza limita l'entità dell'effetto proposto dal modello.

## Casi limite

- Attivazione richiesta mentre l'oggetto è in ricarica: nessun effetto, feedback chiaro.
- Attivazione senza bersaglio o condizione valida: nessun effetto, feedback chiaro,
  nessuna perdita di carica se la condizione d'uso non è mai stata soddisfatta.
- Più slot attivi occupati da oggetti con effetti che si sovrappongono sulla stessa
  proprietà: si applicano le regole di priorità descritte in
  [Synergies](synergies.md#priorità).

## Fallback

Se un attivo generato non supera la validazione in tempo utile, si applica la regola
unica descritta in [Generated Content Validation](generated-content-validation.md): non
riformulata qui.

## Non-obiettivi

- Questo documento non definisce singoli effetti o numeri di bilanciamento specifici.
- Non ridefinisce i campi obbligatori dell'oggetto: vivono in
  [Items, Pools and Rarity](items-pools-and-rarity.md).

## Raccolta e scambio: i piedistalli (DEC-117)

Gli attivi offerti dal gioco (stanza del tesoro, negozio, drop) **fluttuano su un
piedistallo**. Raccogliere un attivo nuovo lo mette nell'inventario — visibile nella
sezione della UI dedicata agli attivi — e, se gli slot sono pieni, **l'attivo che
possedevi finisce sul piedistallo al suo posto** — con più slot pieni, quello
**attualmente selezionato per l'attivazione** (DEC-117): lo scambio è sempre reversibile
finché resti nella stanza, perché il vecchio attivo non sparisce, fluttua lì. Con uno slot
libero la raccolta riempie lo slot senza scambio.

## Stato di implementazione (2026-07-27)

Il motore implementa la categoria e il ciclo d'uso; `tools/melting-gen` ora produce
anche attivi (aggiornamento dello stesso 2026-07-27, dopo la prima stesura di questa
sezione).

**Implementato**

- `ItemKind` ha le 4 categorie del documento fonte; `ITEM_ACTIVE` significa finalmente
  "attivabile" (prima era il nome del passivo) — `src/core/game_types.h`.
- Slot attivo singolo, espandibile, derivato dall'inventario senza indici da mantenere —
  `src/gameplay/item_slots.{h,c}`.
- Cariche **oppure** cooldown, con la regola "le cariche vincono" per un contenuto che
  dichiarasse entrambi; un attivo che non dichiara nulla ricade su un cooldown di riserva
  del motore, mai su "usabile a ogni frame".
- Ricarica a doppio canale (DEC-059): stanza completata (`WorldCheckRoomClear`) ed
  energia droppata dai nemici (`PICKUP_ENERGY`, che cade solo se un attivo a cariche
  posseduto non è pieno).
- Attivazione con effetto: callback Lua `on_use` in sandbox per un oggetto che la
  definisce, altrimenti un ripiego deterministico in C scelto sul trait dell'oggetto —
  la stessa promessa "mai un dud" degli stat-up. Attivazione in ricarica: nessun effetto,
  nessuna carica persa, feedback che dice quanto manca.
- Piedistallo di scambio (DEC-117): il gap dichiarato nella versione precedente di questo
  documento **è chiuso**. Lo scambio è reversibile nella stanza e non ricarica l'oggetto
  scambiato.
- HUD: riga permanente con nome dell'attivo, stato (cariche `n/m` o secondi mancanti) e
  tasto d'uso.

**Non ancora implementato**

- Nessun prompt di `tools/melting-gen` chiede al modello la callback Lua `on_use`
  (l'unica che `src/script/script_items.c` mette in cache per un `ITEM_ACTIVE`): ogni
  attivo generato ricade quindi sempre sul ripiego C generico scelto sul trait
  dell'oggetto (`CombatActiveFallbackEffect`, `src/gameplay/combat.c`) — mai un dud,
  ma anche mai un effetto d'uso scritto dal modello. Continua comunque a eseguire
  `on_fire`/`on_hit`/`on_tick` se lo script li definisce (solo `on_use` manca): un
  attivo generato oggi si comporta come un passivo con in più un effetto generico sul
  tasto E.
- UI di selezione fra più attivi quando gli slot sono più di uno (oggi la selezione è
  l'ordinale `Player.activeSelected`, che nessun input muove ancora).
- Nessuna fonte di slot attivi aggiuntivi esiste in gioco.

### Default proposti dall'implementazione

Stile DEC-019: valori scelti dal codice perché il documento non li fissa, da confermare.

| Scelta | Default adottato |
|---|---|
| Input di attivazione | tasto **E** (l'unico libero insieme a G; sotto la mano che sta già su WASD) |
| Stato iniziale di un attivo appena trovato | **carico** (cariche al massimo, nessuna attesa) |
| Dosaggio dei due canali quando l'oggetto non lo dichiara | **1 carica** per stanza completata, **1 carica** per energia raccolta |
| Probabilità che un nemico ucciso lasci energia | **30%**, e solo se un attivo a cariche non è pieno |
| Cooldown di riserva per un attivo che non dichiara né cariche né cooldown | **12 s** |
| Bande di sicurezza | cariche `[1, 12]`, cooldown `[0.5 s, 90 s]` |

## Domande aperte residue

- Numero massimo di slot attivi ottenibili in una run.
- Dosaggio esatto della ricarica a doppio canale (quanta carica per stanza, quanta
  energia per nemico) — DEC-059 fissa solo i due canali di base, non i numeri.
- Conferma del tasto di attivazione (default proposto: E).

## Scenari verificabili

### Scenario 1 — attivazione riuscita

Given un attivo con cariche disponibili e condizione d'uso soddisfatta,  
When il giocatore preme l'input di attivazione,  
Then l'effetto si applica, una carica viene consumata e il feedback conferma
l'attivazione.

### Scenario 2 — attivazione in ricarica

Given un attivo ancora in cooldown,  
When il giocatore preme l'input di attivazione,  
Then nessun effetto si applica e il feedback comunica lo stato "in ricarica" senza
ambiguità.

### Scenario 3 — secondo slot attivo sbloccato

Given un giocatore che ha ottenuto un evento raro che aggiunge uno slot attivo,  
When raccoglie un secondo oggetto attivo,  
Then entrambi gli attivi restano disponibili in slot separati e il giocatore può
attivarli indipendentemente.

### Scenario 4 — ricarica a doppio canale

Given un attivo a cariche con carica non piena,  
When il giocatore completa una stanza oppure raccoglie energia droppata da un nemico,  
Then la carica dell'attivo aumenta secondo il dosaggio dichiarato dall'oggetto, indipendentemente da quale dei due canali di base ha attivato la ricarica (DEC-059).

### Scenario 5 — scambio sul piedistallo, reversibile (DEC-117)

Given un giocatore con l'unico slot attivo occupato,  
When raccoglie un altro attivo da un piedistallo e poi torna sul piedistallo senza
lasciare la stanza,  
Then il primo scambio ha messo il nuovo attivo nello slot e il vecchio sul piedistallo, e
il secondo li riscambia: l'attivo tornato indietro conserva le cariche che aveva quando è
stato scambiato via, non ne guadagna.

### Scenario 6 — l'attivazione non è mai inerte

Given un attivo generato che non porta alcuno script valido,  
When il giocatore lo attiva con cariche disponibili,  
Then un effetto deterministico si applica comunque (nessuna attivazione a vuoto) e la
carica viene consumata.
