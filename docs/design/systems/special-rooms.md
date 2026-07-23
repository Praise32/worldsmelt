---
id: gd-system-special-rooms
title: Special Rooms
domain: design
status: approved
authority: canonical
owner: design
summary: "Dettaglio dei cinque archetipi speciali (DEC-010, esteso da DEC-051): fusione, segreta a due livelli (DEC-025), arena di sfida, scambio ad alto rischio — unico luogo per patti a costo salute (DEC-026), con offerta e prezzo generati dentro un budget di equità (DEC-044) — e stanza a tempo nei piani avanzati (DEC-051) — sottoinsieme dichiarato della tassonomia di rooms-and-floor-generation.md."
last_reviewed: 2026-07-22
last_verified_commit: 0ec60d0
topics: [stanze-speciali, fusione, scambio-alto-rischio, arena-di-sfida, stanza-a-tempo]
related: []
supersedes: []
source_files: []
---

# Special Rooms

## Intento per il giocatore

Le stanze speciali offrono decisioni fuori dal combattimento standard: rischio, scambio, scoperta o sfida opzionale, con un segnale chiaro che le distingue dalle stanze standard (dove la scoperta non è parte del design).

## Condizioni di ingresso

La tassonomia completa dei tipi di stanza (standard + speciali) è definita in [rooms-and-floor-generation.md](./rooms-and-floor-generation.md); questo documento non la ridefinisce. Qui si dettagliano i quattro archetipi speciali dichiarati da DEC-010 — stanza di fusione, stanza segreta, arena di sfida, scambio ad alto rischio — più il quinto archetipo aggiunto da DEC-051: la stanza a tempo.

**Nota sul negozio:** il negozio è un tipo di stanza **standard** (definito in [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)), non uno dei quattro archetipi speciali qui descritti. Il negozio ha prezzi base fissi per fascia di rarità più un'offerta speciale generata per negozio (DEC-026): il dettaglio economico vive in [rewards-and-economy.md](./rewards-and-economy.md), non ripetuto qui. Lo scambio ad alto rischio — in-game **Pourhouse**, «Casa della Colata» (DEC-136) — è un archetipo diverso dal negozio: offre scambi rischiosi o non convenzionali, con presentazione e regole originali — mai nominato o presentato con riferimenti a giochi esistenti. **Confine netto (DEC-026):** i "patti" a costo salute (cedere salute in cambio di un guadagno) non esistono nel negozio; restano esclusivi dello scambio ad alto rischio.

## I cinque archetipi

### Stanza di fusione

Ospita la meccanica-firma di fusione esplicita tra due oggetti. Il dettaglio della meccanica (input, catalizzatore di fusione, risultato generato) è descritto in [item-fusion.md](./item-fusion.md); questo documento non lo ripete, colloca solo la stanza nella tassonomia e ne descrive l'accesso.

### Stanza segreta

Stanza non indicata direttamente sulla mappa, a **due livelli** (DEC-025): "normale", con
indizi visivi leggibili (crepe, anomalie del tema) apribile con lo strumento di breccia; e
"super-segreta", senza indizi, trovabile solo con oggetti/Innesti rivelatori o intuizione
estrema. Il dettaglio dei due livelli è descritto in
[secrets-and-obstacles.md](./secrets-and-obstacles.md); questo documento non lo ripete,
colloca solo l'archetipo nella tassonomia.

### Arena di sfida

Stanza opzionale con combattimento più impegnativo in cambio di ricompensa maggiore. È anche accessibile in versione "best-of" dal Piano 0, usando contenuti già validati delle run passate (DEC-004): vedi [floor-zero.md](./floor-zero.md) per il dettaglio di questo accesso alternativo; questo documento descrive solo la versione incontrata durante il piano.

### Scambio ad alto rischio

Stanza che propone uno scambio non convenzionale (es. cedere una risorsa, salute o una parte della build per un guadagno maggiore ma incerto), ri-tematizzata in modo originale. È l'**unico** archetipo dove sono ammessi scambi a costo salute (DEC-026): il negozio non li offre mai. Nome e presentazione precisi restano da definire in fase di contenuto, ma la funzione — rischio dichiarato in cambio di un guadagno superiore alla media — è fissata da DEC-010.

### Stanza a tempo (DEC-051)

Stanza fissa dei **piani avanzati**: se il giocatore la raggiunge entro una soglia di tempo,
ottiene una ricompensa aggiuntiva. Coerente con il timer di run sempre visibile nell'HUD
(DEC-051, vedi [HUD](../ui/hud.md)): il gioco si dichiara esplicitamente una corsa, e questo
archetipo rende quella dichiarazione parte del level design nei piani avanzati. Il dettaglio
della ricompensa vive in [Rewards and Economy](./rewards-and-economy.md) come fonte unica;
questo documento colloca solo l'archetipo nella tassonomia e ne descrive l'accesso.

**Estensione della tassonomia (DEC-051):** questo è un quinto archetipo speciale, aggiunto
da DEC-051 ai quattro originali di DEC-010.

#### Puntata generata dentro un budget di equità (DEC-044)

Nella stanza di scambio l'IA genera **sia l'offerta sia il prezzo** dentro un **budget di
equità**: non sono coppie fisse curate, ma una puntata composta per l'occasione. Il prezzo
può essere:

- salute, immediata o massima (riduzione del tetto, non solo del valore corrente);
- un oggetto o Innesto posseduto dal giocatore;
- valuta principale;
- perfino un catalizzatore di fusione.

Ogni scambio proposto è diverso dagli altri. L'**equità** tra offerta e prezzo è garantita
dal budget di equità, non da una tabella fissa; la validazione della coppia offerta/prezzo
segue le regole generali di
[Generated Content Validation](./generated-content-validation.md), non riformulate qui. Il
negozio resta invariato (DEC-026): questa generazione è esclusiva della stanza di scambio ad
alto rischio.

## Input/azioni

Il giocatore decide se entrare (quando l'ingresso è opzionale o a costo) e compie l'azione specifica dell'archetipo: fondere due oggetti, cercare un varco nascosto, affrontare la sfida, accettare o rifiutare lo scambio.

## Risultato

Ogni stanza speciale produce un esito dichiarato (oggetto fuso, accesso a una ricompensa nascosta, ricompensa da sfida superata, esito dello scambio) proporzionato al rischio o costo pagato (vedi [rewards-and-economy.md](./rewards-and-economy.md) per il principio generale di proporzione rischio/ricompensa).

## Feedback

- segnale visivo che distingue una stanza speciale da una stanza standard prima dell'ingresso, dove la scoperta non è parte del design (fusione, arena, scambio);
- per la stanza segreta, l'assenza di segnale diretto è intenzionale (vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md));
- conferma esplicita prima di un'azione irreversibile (fusione, scambio ad alto rischio).

## Interazioni

- con la tassonomia generale delle stanze ([rooms-and-floor-generation.md](./rooms-and-floor-generation.md));
- con la meccanica di fusione ([item-fusion.md](./item-fusion.md)) e il catalizzatore di fusione ([health-and-resources.md](./health-and-resources.md), [items-pools-and-rarity.md](./items-pools-and-rarity.md));
- con il Piano 0 per l'accesso "best-of" all'arena di sfida ([floor-zero.md](./floor-zero.md));
- con l'economia per lo scambio ad alto rischio ([rewards-and-economy.md](./rewards-and-economy.md));
- con la regola di validazione generale per la coppia offerta/prezzo generata ([generated-content-validation.md](./generated-content-validation.md), DEC-044);
- con la ricompensa della stanza a tempo ([rewards-and-economy.md](./rewards-and-economy.md), DEC-051) e con il timer sempre visibile ([hud.md](../ui/hud.md)).

## Regola di originalità

I nomi, la presentazione e la logica precisa dei quattro archetipi devono essere originali. Gli archetipi sono funzioni di design, non contenuti da copiare da giochi esistenti (vedi `09-originality-guardrails.md`).

## Regole per contenuti generati

Ogni istanza di stanza speciale generata dichiara un'origine (curato | composto | variato | nuovo) e rispetta il contratto: accesso, costo, ricompensa, frequenza, segnale visivo, uscita, interazioni con risorse.

Per lo scambio ad alto rischio, la coppia offerta/prezzo generata (DEC-044) deve restare
dentro il budget di equità dichiarato: un prezzo sproporzionato rispetto all'offerta, o
un'offerta priva di un prezzo coerente, non supera la validazione e non viene mai proposta al
giocatore.

## Casi limite

- Una stanza di fusione generata senza almeno due oggetti fondibili posseduti dal giocatore: resta accessibile ma senza azione disponibile finché non sono soddisfatti i requisiti.
- Uno scambio ad alto rischio proposto quando il giocatore non ha nulla di cedibile: la stanza non deve bloccare il progresso, deve offrire un'uscita senza penalità.
- Una puntata generata (offerta/prezzo, DEC-044) risulta squilibrata rispetto al budget di equità: va respinta o rigenerata in validazione prima di essere proposta al giocatore.
- Il prezzo generato richiederebbe più salute massima di quella posseduta dal giocatore: il prezzo non deve mai superare risorse che il giocatore non ha, la generazione deve restare compatibile con lo stato corrente del giocatore.
- L'arena di sfida "best-of" nel Piano 0 richiede contenuti già validati che potrebbero non esistere ancora nelle prime run: va gestita con il fallback previsto in [floor-zero.md](./floor-zero.md).
- Il giocatore raggiunge una stanza a tempo dopo la scadenza della soglia: non deve mai bloccare il progresso del piano; resta almeno accessibile come stanza ordinaria, anche senza il bonus a tempo (soglia esatta e comportamento di mancato rispetto da definire col playtest, vedi `governance/open-questions.md`).

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni archetipo speciale ha un contenuto curato sufficiente a comparire senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non ridefinisce la tassonomia completa dei tipi di stanza (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)).
- Non dettaglia la meccanica di fusione (vedi [item-fusion.md](./item-fusion.md)).
- Non risponde alla domanda su come si scoprono le stanze segrete (vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md)).
- Non dettaglia l'accesso "best-of" dal Piano 0 (vedi [floor-zero.md](./floor-zero.md)).

## Domande aperte residue

- ~~Nome e presentazione definitivi dello scambio ad alto rischio~~: risolto da DEC-136 — **Pourhouse** («Casa della Colata»), presentazione canonica nel glossario.
- Frequenza esatta di ciascun archetipo per piano.
- Valori numerici esatti del budget di equità della puntata generata (DEC-044 fissa il principio, non i numeri).
- ~~Quali rivelatori esistono per le super-segrete~~: risolto da DEC-127 (Innesti sensore + oggetti rari), vedi `secrets-and-obstacles.md`.

## Scenari

### Scenario 1 — Negozio non è un archetipo speciale

Given un giocatore che entra in un negozio durante un piano
When cerca questa stanza nella tassonomia
Then la trova descritta come tipo standard in [rooms-and-floor-generation.md](./rooms-and-floor-generation.md), non tra i quattro archetipi speciali di questo documento

### Scenario 2 — Accesso "best-of" all'arena di sfida dal Piano 0

Given un giocatore nel Piano 0 con contenuti già validati disponibili dal museo delle creazioni
When sceglie di affrontare un'arena di sfida "best-of"
Then accede a un'arena costruita con contenuti "best-of" di run passate, distinta dall'arena di sfida incontrata durante un piano generato

### Scenario 3 — Scambio ad alto rischio senza nulla da cedere

Given un giocatore senza risorse o oggetti cedibili che entra in una stanza di scambio ad alto rischio
When valuta le opzioni disponibili
Then la stanza offre comunque un'uscita senza penalità, senza bloccare il progresso

### Scenario 4 — Stanza di fusione senza requisiti soddisfatti

Given un giocatore con un solo oggetto fondibile
When entra nella stanza di fusione
Then la stanza è visitabile ma l'azione di fusione resta non disponibile finché non possiede almeno due oggetti fondibili e il catalizzatore richiesto

### Scenario 5 — Negozio senza patti a costo salute

Given un giocatore in un negozio con l'offerta speciale generata per quel negozio
When osserva le opzioni di acquisto disponibili
Then nessuna di esse chiede di cedere salute in cambio di uno sconto o di un oggetto: quel tipo di patto esiste solo nella stanza di scambio ad alto rischio (DEC-026)

### Scenario 6 — Stanza segreta a due livelli

Given un giocatore che esplora un piano con una stanza segreta "normale" e una "super-segreta"
When cerca di individuarle
Then trova la "normale" tramite un indizio visivo leggibile apribile con lo strumento di breccia, mentre individua la "super-segreta" solo con un oggetto/Innesto rivelatore o intuizione estrema (DEC-025)

### Scenario 7 — Puntata generata in una stanza di scambio ad alto rischio

Given un giocatore che entra in una stanza di scambio ad alto rischio
When il gioco genera l'offerta e il prezzo della puntata
Then il prezzo appartiene a una delle categorie ammesse (salute immediata o massima, oggetto o Innesto posseduto, valuta principale, catalizzatore di fusione) ed è proporzionato all'offerta dentro il budget di equità (DEC-044)

### Scenario 8 — Ogni scambio è diverso

Given un giocatore che entra in due stanze di scambio ad alto rischio diverse nella stessa run
When confronta le due puntate proposte
Then offerta e prezzo delle due stanze sono diversi tra loro, perché ogni scambio è generato per l'occasione (DEC-044)

### Scenario 9 — Stanza a tempo raggiunta in tempo

Given un giocatore che raggiunge una stanza a tempo entro la soglia richiesta in un piano avanzato
When entra nella stanza
Then riceve la ricompensa aggiuntiva descritta in [rewards-and-economy.md](./rewards-and-economy.md), coerente col timer di run sempre visibile nell'HUD (DEC-051)
