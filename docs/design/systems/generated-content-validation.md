---
id: gd-system-content-validation
title: Generated Content Validation
domain: design
status: approved
authority: canonical
owner: design
summary: "Fonte unica: modello di generazione, sei stati di validazione, regola di fallback e tabella del pool curato minimo per categoria (DEC-087, con garanzia di almeno un oggetto per rarità DEC-144) per ogni contenuto generato dall'IA nella KB. Gli stessi sei stati regolano anche la riconvalida del catalogo a ogni aggiornamento del gioco (DEC-069, dettaglio in systems/save-and-meta-progression.md) e l'ingresso nel Catalogo dei contenuti fallback-usati con origine curato (DEC-103, dettaglio in systems/save-and-meta-progression.md)."
last_reviewed: 2026-07-28
last_verified_commit: cfeaa47
topics: [validazione, fallback, sei stati, DEC-020, DEC-087, pool curato minimo, DEC-144, DEC-146, DEC-162]
related: []
supersedes: []
source_files: []
---

# Generated Content Validation

> Aggiunta del 22/07 (DEC-135): fra le categorie di validazione esiste la **validazione
> strutturale dei layout di stanza**: croce centrale libera (già garantita dal motore),
> raggiungibilità di porte e ricompense, spazio minimo di manovra. Un layout non valido
> segue la normale catena di fallback.

Questo documento è la fonte UNICA della regola di fallback e degli stati di validazione del
contenuto generato per l'intera knowledge base. Gli altri documenti di sistema vi rimandano
con una riga, senza riformulare: questo testo deve reggersi da solo.

## Intento per il giocatore

Il giocatore non deve mai vedere un contenuto generato rotto, incoerente o incompleto, e non
deve mai restare bloccato in attesa che una generazione finisca.

## Condizioni di ingresso

Questa regola si applica a ogni contenuto che nasce da generazione IA nel corso di una run:
tipi di colpo, nemici, layout di stanze (invenzione parametrica dentro bande di garanzia),
comportamenti di oggetto (scritti e validati in sandbox), personaggio alternativo di run,
temi proposti nel Piano 0, ostacoli generati a tema (DEC-043), offerta e prezzo dello
scambio ad alto rischio (DEC-044). Vale dal Piano 0 fino al boss del piano 5, dove la
vittoria chiude la run (DEC-006, DEC-031).

## Il modello di generazione (DEC-020)

Il modello di generazione reale è canone concettuale per tutta la KB. L'IA non "sceglie da un
menu" chiuso di opzioni pre-scritte: **inventa** contenuti parametrici nuovi dentro bande di
garanzia dichiarate (tipi di colpo, nemici, layout di stanze) e **scrive** comportamenti
eseguibili per gli oggetti, validati in sandbox prima di raggiungere il giocatore. Un fallback
curato è sempre disponibile per ogni categoria generabile: il gioco non mostra mai un
contenuto rotto e non si blocca mai in attesa di una generazione. Questa è la regola generale
che ogni altro documento della KB richiama quando parla di generazione o di fallback.

## Input/azioni (eventi di sistema)

Non ci sono azioni dirette del giocatore qui: gli "input" sono eventi di sistema.

- richiesta di generazione emessa dal sistema di run per una categoria di contenuto;
- generazione completata in tempo e con successo;
- generazione che fallisce con un errore;
- generazione che impiega più del tempo massimo consentito (timeout);
- contenuto generato che non supera un controllo, in una qualunque fase.

## Stati di validazione del contenuto generato

Sei stati, in italiano, uguali per ogni categoria di contenuto generato:

1. **proposto** — il modello ha generato una bozza di contenuto (parametri, testo o
   comportamento) ma non è stata ancora verificata in alcun modo.
2. **strutturalmente-valido** — il contenuto proposto ha superato i controlli di forma: schema
   completo, valori entro le bande di garanzia dichiarate, risorse referenziate esistenti,
   nessuna dipendenza circolare proibita. Non garantisce ancora che funzioni in gioco.
3. **simulato** — il contenuto strutturalmente valido è stato eseguito in un ambiente di prova
   separato dalla run del giocatore, per verificare che produca condizioni di completamento
   raggiungibili, non generi loop o blocchi, e rispetti i budget di potenza, pericolo e
   leggibilità.
4. **approvato-per-run** — il contenuto ha superato la simulazione ed è stato ammesso nel pool
   disponibile per la run corrente; può comparire al giocatore.
5. **respinto** — il contenuto ha fallito un controllo in una qualunque fase (proposto,
   strutturalmente-valido o simulato) e viene scartato; non diventa mai visibile al giocatore
   nella modalità standard.
6. **fallback-usato** — al posto di un contenuto generato mancante, in errore, troppo lento o
   respinto, il gioco ha impiegato un contenuto curato equivalente dal pool di riserva; il
   giocatore riceve un'esperienza completa e coerente.

### Transizioni tipiche

- Percorso normale: proposto → strutturalmente-valido → simulato → approvato-per-run.
- Scarto in fase strutturale: proposto → respinto → fallback-usato.
- Scarto in fase di simulazione: strutturalmente-valido → respinto → fallback-usato.
- Scarto dopo simulazione (es. budget superato in un controllo finale): simulato → respinto →
  fallback-usato.
- Timeout: una generazione che supera il tempo massimo consentito passa direttamente a
  fallback-usato, anche se non ha completato tutte le fasi di controllo (vedi Casi limite).

Se testi precedenti della KB usano termini informali come "validato" o "fortemente validato",
vanno intesi come sostituiti da questi sei stati ufficiali.

### Riconvalida a cambio versione del catalogo (DEC-069)

Gli stessi sei stati sopra definiti si applicano anche alla **riconvalida** del catalogo
persistente a ogni aggiornamento del gioco: un contenuto già approvato-per-run in una run
passata viene ripassato attraverso questo stesso processo. Chi supera di nuovo la
riconvalida resta come prima; chi non la supera passa a **respinto** e diventa una
**Reliquia** nel Catalogo — consultabile ma non più giocabile né sbloccabile nei pool. Fonte
unica del concetto di Reliquie e del suo effetto sul Catalogo:
[Save and Meta Progression](save-and-meta-progression.md) (rimando, non riformulato qui).

### I contenuti fallback-usati nel Catalogo (DEC-103)

Un contenuto incontrato in stato fallback-usato durante una run entra comunque nel Catalogo
persistente, marcato con la sua origine `curato` (tassonomia già esistente in
[`../content/content-taxonomy.md`](../content/content-taxonomy.md)), accanto alle origini
composto/variato/nuovo. Fonte unica della regola del Catalogo:
[Save and Meta Progression](save-and-meta-progression.md) (rimando, non riformulato qui).

## Controlli minimi

- schema completo;
- valori entro le bande di garanzia dichiarate;
- risorse esistenti;
- nessuna dipendenza circolare proibita;
- leggibilità visiva;
- budget di potenza o pericolo;
- compatibilità con il piano;
- condizioni di completamento raggiungibili;
- descrizione coerente con l'effetto;
- originalità.

Il controllo di **leggibilità visiva** usa come proxy primario la percentuale massima di
schermo coperta dai segnali attivi (proiettili, telegraph, particellari, indicatori),
definita come fonte unica in
[Combat and Projectiles — Budget di leggibilità](combat-and-projectiles.md#budget-di-leggibilità)
(DEC-146, soglia esatta ancora da playtest; rimando, non riformulato qui). Restano ammessi
controlli complementari: i cinque elementi non negoziabili elencati in quel documento
(posizione del personaggio, proiettili nemici, hitbox percepita, direzione dell'attacco,
causa del danno) si verificano sempre, indipendentemente dal valore del proxy percentuale.

Il controllo di **budget di potenza** riceve una regola aggiuntiva quando il contenuto
validato è il risultato di una sinergia implicita o di una fusione esplicita: quel
risultato ha un budget di potenza dedicato, più alto del budget di un singolo oggetto
sorgente (DEC-162, dettaglio in [Synergies](synergies.md) e [item-fusion.md](item-fusion.md));
il valore esatto di questo budget dedicato resta `draft`, da playtest.

### Stato di implementazione — cosa gira oggi in `tools/melting-gen` (2026-07-27)

Dei controlli minimi sopra, la validazione dei contenuti generati ne applica oggi tre in
`gen_validate.c`, tutti con la **regola di fallback** di questo documento (contenuto
respinto → equivalente curato/procedurale, run che prosegue), mai una correzione sul posto:

| Controllo | Cosa verifica oggi | Se fallisce |
|---|---|---|
| Leggibilità visiva (DEC-146) | proxy di copertura schermo del tipo di colpo dichiarato, formula e soglia in [Combat and Projectiles](combat-and-projectiles.md#budget-di-leggibilità) (entrambe `draft`) | l'intero tipo di colpo ricade su quello procedurale del piano |
| Budget di potenza — risultato (DEC-162) | lo stesso proxy applicato al **risultato** della sinergia che il tipo di colpo dichiara da sé (`chain`/`pierce`), perimetro e motivazione in [Synergies](synergies.md#budget-di-potenza-del-risultato) | come sopra |
| Descrizione coerente con l'effetto | un oggetto `attivo` deve dichiarare cariche **o** cooldown ([Active Items](active-items.md)) | ripiego sul cooldown di riserva del motore, mai un attivo usabile a ogni frame |

Il pool curato minimo (sotto) e la sua garanzia di copertura per rarità (DEC-144) non sono
un controllo di validazione ma una garanzia di **composizione**: il generatore la applica
prima, precalcolando le rarità dell'intera run, così nessun contenuto respinto può farla
saltare. Gli altri controlli minimi (dipendenze circolari, compatibilità col piano,
condizioni di completamento, originalità) restano `simulato` o coperti altrove.

## Risultato

Il contenuto raggiunge lo stato approvato-per-run ed entra nel pool disponibile per la run,
oppure viene scartato (respinto) e sostituito da un equivalente curato (fallback-usato). In
entrambi i casi la run prosegue senza interruzioni visibili al giocatore.

## Feedback

Quando scatta un fallback, il giocatore percepisce solo continuità: il gioco resta giocabile,
la categoria di contenuto interessata (colpo, nemico, stanza, oggetto, personaggio, tema) è
comunque presente, senza discontinuità evidenti. Non viene mai mostrato un messaggio di errore
tecnico né un contenuto a metà. I dettagli concreti di presentazione (schermate, indicatori,
testi) restano fuori scope in questo documento e vivono nei documenti di `ui/`.

## Interazioni

- `systems/floor-zero.md`: la versione statica curata del Piano 0 è un caso concreto di
  fallback-usato.
- `systems/items-pools-and-rarity.md`: i pool curati fungono da riserva per gli oggetti; fonte
  unica dei pesi di rarità usati anche per distribuire il minimo curato di 20 oggetti (DEC-087).
- `systems/characters.md`: fonte unica della rosa base (DEC-080), che in modalità solo-curato è
  l'intero pool personaggi del minimo curato (DEC-087).
- `systems/rooms-and-floor-generation.md`: fonte unica della variabilità procedurale dei layout
  di stanza (DEC-009), per cui il minimo curato non si applica.
- `systems/run-manifest-and-reproducibility.md`: registra quali fallback sono avvenuti nella
  run.
- `../content/content-taxonomy.md`: l'origine "curato" identifica i contenuti di riserva usati
  nei fallback.
- `save-and-meta-progression.md`: fonte unica delle Reliquie, il risultato per il Catalogo di
  un contenuto respinto in riconvalida a cambio versione (DEC-069), e dell'ingresso nel
  Catalogo dei contenuti fallback-usati con origine curato (DEC-103).

## Regole per contenuti generati

- Ogni categoria generabile deve avere bande di garanzia dichiarate (potenza, pericolo,
  leggibilità) entro cui il contenuto parametrico deve restare (DEC-020).
- Ogni comportamento scritto (oggetti) deve essere validato in sandbox prima di diventare
  approvato-per-run.
- Ogni categoria generabile possiede un pool curato sufficiente a completare l'intera run senza
  generazione nuova.
- Un contenuto respinto non deve mai diventare visibile al giocatore nella modalità standard.

## Casi limite

- Una generazione che eccede il tempo massimo consentito viene trattata come fallback-usato
  anche senza passare esplicitamente per lo stato respinto.
- Se la generazione fallisce ripetutamente per un'intera categoria in una run, il pool curato
  deve coprire l'intera categoria per l'intera run, non solo un singolo slot.
- Un contenuto che supera i controlli strutturali ma fallisce in simulazione dopo un tempo
  lungo deve comunque risolversi entro il tempo massimo consentito, altrimenti scatta il
  fallback per timeout.
- La correzione di fortuna (vedi `../governance/glossary.md`, non riformulare qui) può
  intervenire sulle estrazioni del pool, ma non sostituisce questa regola di fallback.

## Fallback

Ogni categoria generabile possiede un pool curato sufficiente a completare la run senza
generazione nuova (DEC-020). Il fallback è invisibile al giocatore per design: non è uno stato
di errore da recuperare in interfaccia, è una regola di continuità che garantisce che mai un
contenuto rotto e mai un blocco della partita raggiungano il giocatore.

Il contenuto curato di fallback è lo **stato base del gioco** (DEC-153): è precaricato e
disponibile per costruzione, non «diventa disponibile» e non ha un proprio stato di attesa o
di caricamento. Non esiste quindi un caso in cui il gioco resti fermo perché *nemmeno il
fallback è pronto*: quando un contenuto generato viene respinto, il sostituto curato c'è già.
La resa in interfaccia di questa garanzia è in [Generation Status](../ui/generation-status.md)
e, per il Piano 0, in [Floor Zero](floor-zero.md).

**Stato di implementazione (W5b, 2026-07-28):** il formato del pool curato (un file per
categoria, `assets/curated-content/{items,enemies,bosses}.txt`), il loader
(`src/content/curated_catalog.h`/`.c`) e il layer di indirezione immagini
(`content-id -> image-id -> file`, DEC-175(b) esteso da W5b, `src/content/
curated_image_map.h`) esistono lato motore; `RunContentLoad` (`src/content/run_content.c`)
li usa come stato base sopra il fallback puramente procedurale, con precedenza
`curated-content -> fallback deterministico -> generazione reale`, mantenendo l'invariante
"un solo tipo di colpo per piano, mai su uno stat-up" anche quando l'overlay curato
riassegna la categoria di un oggetto. Dettaglio dell'implementazione (compresa la
validazione del floor DEC-144 al caricamento, il perché `bossItem` resta sempre
procedurale, e la marcatura delle immagini curate come "usate") in
[Items, Pools and Rarity](items-pools-and-rarity.md#stato-di-implementazione-del-pool-curato-w5b-dec-153-2026-07-28).
**Stato (2026-07-28):** il contenuto curato vero e proprio ora esiste in
`assets/curated-content/` (25 oggetti, 14 nemici, 5 boss, prodotti da run vere di
`bin/melting-gen` e rivisti dal content designer — dettaglio, semi e voci scartate in
[Items, Pools and Rarity](items-pools-and-rarity.md#stato-di-implementazione-del-pool-curato-w5b-dec-153-2026-07-28)),
quindi il motore lo usa come stato base sopra il fallback procedurale (DEC-153), non più
solo quest'ultimo.

## Pool curato minimo (DEC-087)

Questo documento è la fonte UNICA della tabella del pool curato minimo per categoria: per ogni
categoria di contenuto che il gioco propone al giocatore esiste un minimo curato che, da solo
(nessuna generazione IA disponibile), garantisce una run completa — Piano 0 più i 5 piani — e
la scelta di fallback in ogni punto in cui il giocatore sceglie.

I valori sono default proposti stile DEC-019, da confermare col playtest (vedi
`../governance/open-questions.md`, punto 10):

| Categoria | Minimo curato | Note |
|---|---|---|
| Temi/World | 3 | Coincidono con le carte fallback del Piano 0 (DEC-076, dettaglio in `floor-zero.md`). |
| Boss | 5 | Uno per piano, legati alle bande di difficoltà dei 5 piani. |
| Nemici comuni | 12 | Distribuiti sulle bande di difficoltà dei 5 piani. |
| Oggetti | 20 | Distribuiti sulle 4 rarità con i pesi DEC-019 (dettaglio in `items-pools-and-rarity.md`); il pool minimo garantisce almeno un oggetto per rarità, con l'eccedenza sottratta alle rarità più comuni e il totale di 20 invariato (DEC-144, esempio numerico derivato in `items-pools-and-rarity.md`). |
| Tipi di colpo | 6 | Includono i colpi standard della rosa base. |
| Personaggi | 3 (la rosa base) | La rosa base è l'intero pool personaggi in modalità solo-curato (DEC-080, dettaglio in `characters.md`); il personaggio generato per run non esiste in questa modalità. |
| Layout di stanza | — | Generati proceduralmente senza IA (DEC-009, dettaglio in `rooms-and-floor-generation.md`): nessun minimo curato richiesto. |

Questa tabella è il caso quantificato della regola generale già enunciata sopra ("ogni
categoria generabile possiede un pool curato sufficiente a completare l'intera run senza
generazione nuova"): qui si dichiara quanto basta, categoria per categoria, perché quella
regola sia verificabile.

## Non-obiettivi

- Questo documento non definisce l'interfaccia con cui un fallback viene comunicato (vedi
  documenti in `ui/`).
- Non definisce i valori numerici delle bande di garanzia per categoria (vedi i documenti di
  sistema specifici, es. `combat-and-projectiles.md`, `items-pools-and-rarity.md`; quei valori
  restano stato draft, DEC-019).
- Non copre il bilanciamento fine della qualità generata, solo le garanzie minime di
  continuità.

## Domande aperte residue

- Quanto tempo massimo è accettabile prima che scatti un fallback per timeout (valore non
  ancora definito).
- ~~Se e come comunicare i fallback al giocatore~~: risolto da DEC-132 — una riga
  discreta e aggregata in `RunResults`, nel registro ironico del crogiolo (DEC-105);
  durante la run il fallback resta invisibile.

## Telemetria di design

Registrare motivo del rifiuto, fallback usato e impatto sulla run, senza esporre dati tecnici
nell'interfaccia normale.

## Scenari

**Scenario: percorso normale fino all'approvazione**
- Given un tipo di colpo proposto dall'IA dentro le bande di garanzia dei colpi,
- When il contenuto supera i controlli strutturali e la simulazione in sandbox,
- Then il contenuto passa allo stato approvato-per-run ed entra nel pool disponibile per la
  run corrente.

**Scenario: scarto per controllo strutturale fallito**
- Given un nemico proposto dall'IA con un valore fuori dalle bande di garanzia dichiarate,
- When il controllo strutturale fallisce,
- Then il contenuto passa allo stato respinto e non viene mai mostrato al giocatore nella
  modalità standard.

**Scenario: fallback-usato nel Piano 0**
- Given il gioco deve essere sempre avviabile e il Piano 0 deve fare da sala d'attesa
  giocabile finché non esistono asset dedicati generati (vedi `floor-zero.md`),
- When il sistema non dispone ancora di contenuti generati approvati per quello spazio,
- Then il gioco utilizza la versione statica curata del Piano 0, un caso concreto dello stato
  fallback-usato, e il giocatore non nota mai un blocco o un contenuto rotto.

**Scenario: timeout durante la generazione del piano successivo**
- Given il giocatore sta ancora esplorando un piano mentre il piano successivo viene generato,
- When la generazione del piano successivo supera il tempo massimo consentito o fallisce la
  simulazione,
- Then il sistema sostituisce il contenuto con un layout curato equivalente (fallback-usato) e
  la run prosegue senza che il giocatore venga mai bloccato.

**Scenario: riconvalida del catalogo a un aggiornamento del gioco**
- Given un aggiornamento del gioco avvia la riconvalida del catalogo persistente,
- When un contenuto già approvato-per-run non supera più i controlli minimi con le regole
  della nuova versione,
- Then il contenuto passa allo stato respinto e diventa una Reliquia nel Catalogo del
  giocatore (DEC-069, fonte unica del dettaglio in
  [Save and Meta Progression](save-and-meta-progression.md)).

**Scenario: contenuto fallback-usato registrato nel Catalogo (DEC-103)**
- Given un tipo di colpo passa allo stato fallback-usato durante una run perché la
  generazione IA corrispondente ha fallito la simulazione,
- When la run si conclude e il Catalogo persistente viene aggiornato,
- Then il tipo di colpo incontrato compare nel Catalogo marcato con origine curato, accanto ai
  contenuti di origine composto/variato/nuovo (fonte unica del Catalogo in
  [Save and Meta Progression](save-and-meta-progression.md)).

**Scenario: run completa senza alcuna generazione disponibile (DEC-087)**
- Given nessuna generazione IA è disponibile per nessuna categoria (temi, boss, nemici comuni,
  oggetti, tipi di colpo, personaggi),
- When il giocatore avvia e gioca una run dal Piano 0 fino al boss del piano 5,
- Then il pool curato minimo per categoria copre da solo ogni contenuto necessario e ogni
  scelta del giocatore trova sempre un'opzione di fallback, senza che la run risulti
  incompleta, ripetuta in modo degenere o bloccata.
