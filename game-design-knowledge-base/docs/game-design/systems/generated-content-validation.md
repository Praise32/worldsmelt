---
id: gd-system-content-validation
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Fonte unica: modello di generazione, sei stati di validazione e regola di fallback per ogni contenuto generato dall'IA nella KB. Gli stessi sei stati regolano anche la riconvalida del catalogo a ogni aggiornamento del gioco (DEC-069, dettaglio in systems/save-and-meta-progression.md)."
---

# Generated Content Validation

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
- `systems/items-pools-and-rarity.md`: i pool curati fungono da riserva per gli oggetti.
- `systems/run-manifest-and-reproducibility.md`: registra quali fallback sono avvenuti nella
  run.
- `../content/content-taxonomy.md`: l'origine "curato" identifica i contenuti di riserva usati
  nei fallback.
- `save-and-meta-progression.md`: fonte unica delle Reliquie, il risultato per il Catalogo di
  un contenuto respinto in riconvalida a cambio versione (DEC-069).

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
- Se e come comunicare al giocatore, in forma aggregata e non tecnica, quante volte un
  fallback è scattato in una run (telemetria vs interfaccia).
- Se i contenuti fallback-usati entrano comunque nel catalogo dei contenuti generati (vedi
  `save-and-meta-progression.md`) o restano esclusi in quanto non "generati" in quella run.

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
