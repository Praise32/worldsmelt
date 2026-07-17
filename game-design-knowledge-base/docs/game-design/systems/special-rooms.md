---
id: gd-system-special-rooms
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Dettaglio dei quattro archetipi speciali (DEC-010): fusione, segreta, arena di sfida, scambio ad alto rischio — sottoinsieme dichiarato della tassonomia di rooms-and-floor-generation.md."
---

# Special Rooms

## Intento per il giocatore

Le stanze speciali offrono decisioni fuori dal combattimento standard: rischio, scambio, scoperta o sfida opzionale, con un segnale chiaro che le distingue dalle stanze standard (dove la scoperta non è parte del design).

## Condizioni di ingresso

La tassonomia completa dei tipi di stanza (standard + speciali) è definita in [rooms-and-floor-generation.md](./rooms-and-floor-generation.md); questo documento non la ridefinisce. Qui si dettagliano solo i quattro archetipi speciali dichiarati da DEC-010: stanza di fusione, stanza segreta, arena di sfida, scambio ad alto rischio.

**Nota sul negozio:** il negozio è un tipo di stanza **standard** (definito in [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)), non uno dei quattro archetipi speciali qui descritti. Lo scambio ad alto rischio è un archetipo diverso dal negozio: offre scambi rischiosi o non convenzionali, con presentazione e regole originali — mai nominato o presentato con riferimenti a giochi esistenti.

## I quattro archetipi

### Stanza di fusione

Ospita la meccanica-firma di fusione esplicita tra due oggetti. Il dettaglio della meccanica (input, catalizzatore di fusione, risultato generato) è descritto in [item-fusion.md](./item-fusion.md); questo documento non lo ripete, colloca solo la stanza nella tassonomia e ne descrive l'accesso.

### Stanza segreta

Stanza non indicata direttamente sulla mappa. La sua scoperta resta una domanda di design aperta: vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md) per lo stato della questione; questo documento non anticipa una risposta.

### Arena di sfida

Stanza opzionale con combattimento più impegnativo in cambio di ricompensa maggiore. È anche accessibile in versione "best-of" dal Piano 0, usando contenuti già validati delle run passate (DEC-004): vedi [floor-zero.md](./floor-zero.md) per il dettaglio di questo accesso alternativo; questo documento descrive solo la versione incontrata durante il piano.

### Scambio ad alto rischio

Stanza che propone uno scambio non convenzionale (es. cedere una risorsa o una parte della build per un guadagno maggiore ma incerto), ri-tematizzata in modo originale. Nome e presentazione precisi restano da definire in fase di contenuto, ma la funzione — rischio dichiarato in cambio di un guadagno superiore alla media — è fissata da DEC-010.

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
- con l'economia per lo scambio ad alto rischio ([rewards-and-economy.md](./rewards-and-economy.md)).

## Regola di originalità

I nomi, la presentazione e la logica precisa dei quattro archetipi devono essere originali. Gli archetipi sono funzioni di design, non contenuti da copiare da giochi esistenti (vedi `09-originality-guardrails.md`).

## Regole per contenuti generati

Ogni istanza di stanza speciale generata dichiara un'origine (curato | composto | variato | nuovo) e rispetta il contratto: accesso, costo, ricompensa, frequenza, segnale visivo, uscita, interazioni con risorse.

## Casi limite

- Una stanza di fusione generata senza almeno due oggetti fondibili posseduti dal giocatore: resta accessibile ma senza azione disponibile finché non sono soddisfatti i requisiti.
- Uno scambio ad alto rischio proposto quando il giocatore non ha nulla di cedibile: la stanza non deve bloccare il progresso, deve offrire un'uscita senza penalità.
- L'arena di sfida "best-of" nel Piano 0 richiede contenuti già validati che potrebbero non esistere ancora nelle prime run: va gestita con il fallback previsto in [floor-zero.md](./floor-zero.md).

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni archetipo speciale ha un contenuto curato sufficiente a comparire senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non ridefinisce la tassonomia completa dei tipi di stanza (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)).
- Non dettaglia la meccanica di fusione (vedi [item-fusion.md](./item-fusion.md)).
- Non risponde alla domanda su come si scoprono le stanze segrete (vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md)).
- Non dettaglia l'accesso "best-of" dal Piano 0 (vedi [floor-zero.md](./floor-zero.md)).

## Domande aperte residue

- Nome e presentazione definitivi dello scambio ad alto rischio.
- Meccanismo di scoperta della stanza segreta (vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md)).
- Frequenza esatta di ciascun archetipo per piano.

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
