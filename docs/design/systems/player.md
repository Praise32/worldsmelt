---
id: gd-system-player
title: Player
domain: design
status: approved
authority: canonical
owner: design
summary: "Capacità e responsabilità del personaggio giocato: movimento, mira, gestione di risorse e oggetti. Controlli decisi da DEC-007. Per identità e statistiche dei personaggi vedi characters.md."
last_reviewed: 2026-07-18
last_verified_commit: 0ec60d0
topics: [player, controlli, movimento, mira, statistiche]
related: []
supersedes: []
source_files: []
---

# Player

Questo documento è la fonte per le **capacità e responsabilità generiche** del personaggio controllato dal giocatore (movimento, mira, gestione di risorse e oggetti). Non è la fonte per *quali* personaggi esistono: per la piccola rosa di personaggi base (DEC-030) e per il personaggio generato per run vedi [characters.md](characters.md) — quel documento definisce identità, trait e statistiche specifiche; questo documento resta la fonte per le capacità che ogni personaggio, di rosa base o generato, condivide.

## Intento per il giocatore

Controllo diretto e prevedibile: il giocatore deve poter leggere in ogni istante dove si trova, dove sta mirando e quali risorse/oggetti ha a disposizione, senza ambiguità tra intenzione e risultato.

## Controlli (DEC-007 — approved)

- **Movimento:** libero in tutte le direzioni, non vincolato a una griglia o a passi discreti. Sostituisce la precedente voce "da definire" sullo schema di movimento.
- **Mira e sparo:** nelle 4 direzioni cardinali (alto, basso, sinistra, destra); non è prevista mira libera a 360°. Sostituisce la precedente voce "da definire" sulle direzioni di mira.
- Movimento e mira sono **indipendenti**: il giocatore può muoversi in una direzione mentre spara in un'altra.
- I vincoli che questo schema impone alla leggibilità dei proiettili (giocatore e nemici) sono trattati in [combat-and-projectiles.md](combat-and-projectiles.md) (fonte unica sul budget di leggibilità).

## Statistiche di base, per funzione

Le statistiche non hanno valori numerici definiti qui: restano categorie concettuali, in attesa di bilanciamento (draft). Ogni personaggio (di rosa base, DEC-030, o generato, DEC-014) applica le proprie variazioni a queste categorie — vedi [characters.md](characters.md).

- **Salute:** capacità di assorbire danno prima della sconfitta; struttura stratificata definita in [health-and-resources.md](health-and-resources.md) (DEC-008). La salute base ha un tetto proprio di ciascun personaggio, parte delle sue statistiche (DEC-033): personaggi diversi possono avere tetti diversi per design; i contenitori di salute crescono con stat-up e oggetti fino al tetto di quel personaggio, non oltre. Le bande min/max dei tetti sono un default da playtest; fonte unica per il dettaglio in [health-and-resources.md](health-and-resources.md) (rimando, non riformulato qui).
- **Velocità di movimento:** rapidità di spostamento nello spazio libero definito dal controllo sopra.
- **Cadenza di sparo:** frequenza con cui il personaggio può emettere attacchi nella direzione di mira scelta.
- **Danno di sparo:** danno per colpo; si combina con le proprietà dell'attacco definite in [combat-and-projectiles.md](combat-and-projectiles.md).
- **Fortuna:** influenza probabilità legate a pool, rarità e correzione di fortuna — vedi [items-pools-and-rarity.md](items-pools-and-rarity.md) e [../governance/glossary.md](../governance/glossary.md).

## Condizioni di ingresso

Il controllo del personaggio è attivo in ogni stato di tipo `Gameplay` (vedi [../05-game-states-and-flow.md](../05-game-states-and-flow.md)), incluse le arene di sfida opzionali del Piano 0 (DEC-004). Fuori da `Gameplay` (menu, pausa) il personaggio non riceve input di movimento o mira.

## Input/azioni

- movimento libero;
- mira/sparo in una delle 4 direzioni cardinali;
- uso di oggetti attivi (vedi [active-items.md](active-items.md));
- interazione con oggetti, porte e prese ambientali;
- pausa (ferma la simulazione in singleplayer; vedi [../05-game-states-and-flow.md](../05-game-states-and-flow.md)).

## Risultato

Il movimento e lo sparo del giocatore producono spostamento, danno, raccolta di risorse e interazioni con stanze e nemici, secondo le regole di [combat-and-projectiles.md](combat-and-projectiles.md), [health-and-resources.md](health-and-resources.md) e [rooms-and-floor-generation.md](rooms-and-floor-generation.md).

## Feedback

Il personaggio comunica sempre la propria posizione e la direzione di mira attiva in modo distinguibile dal resto della scena; ogni oggetto o sinergia che altera l'aspetto del personaggio deve rispettare la regola visiva sotto e il budget di leggibilità di [combat-and-projectiles.md](combat-and-projectiles.md).

## Interazioni

- con nemici e proiettili: [combat-and-projectiles.md](combat-and-projectiles.md), [enemies.md](enemies.md);
- con risorse: [health-and-resources.md](health-and-resources.md);
- con oggetti: [active-items.md](active-items.md), [passive-items.md](passive-items.md), [grafts.md](grafts.md);
- con stanze e ostacoli: [rooms-and-floor-generation.md](rooms-and-floor-generation.md), [secrets-and-obstacles.md](secrets-and-obstacles.md).

## Regole per contenuti generati

Le capacità di base del personaggio (schema di movimento e mira) non sono contenuto generato. Le statistiche del personaggio generato per run sono variate dall'IA entro bande — origine: `variato` — regola e dettagli in [characters.md](characters.md) (rimando, non riformulare qui). Oggetti e sinergie generati che alterano movimento o mira devono restare entro il budget di leggibilità di [combat-and-projectiles.md](combat-and-projectiles.md).

## Casi limite

- movimento contro ostacoli o muri;
- sparo durante un cambio rapido di direzione di movimento;
- cambio rapido di direzione di mira tra i 4 assi cardinali;
- oggetti o sinergie generati che modificano lo schema di movimento/mira: devono restare leggibili e coerenti con i controlli approvati sopra.

## Fallback

Se un contenuto generato altera le capacità del personaggio in modo non valido, si applica la regola generale di fallback — vedi [generated-content-validation.md](generated-content-validation.md) (rimando, non riformulare).

## Non-obiettivi

- Non definisce valori numerici finali di bilanciamento delle statistiche.
- Non progetta l'interfaccia (HUD, indicatori di mira) — vedi `ui/`, fuori scope.
- Non definisce mapping tecnici di input (tasti, dispositivi, schemi di controller).

## Domande aperte residue

Proprietà non coperte da nessuna DEC — restano aperte, non vanno considerate decise:

- invulnerabilità dopo il danno (durata, feedback, se esiste);
- interazione dettagliata con ostacoli (knockback, blocco totale, scivolamento);
- dettagli di interazione tra il personaggio e gli slot di equipaggiamento oltre al numero iniziale già fissato da DEC-011 (1 attivo + 1 Innesto, ampliabili);
- comportamento del personaggio in multiplayer (`experimental`; la visione di gara asincrona è in DEC-016, vedi [../08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md), ma non è definito se ogni partecipante ha le stesse regole di controllo o se esistono differenze).

## Regola visiva

Le trasformazioni della build devono rimanere riconoscibili senza rendere il personaggio illeggibile o coprire segnali importanti (vedi budget di leggibilità in [combat-and-projectiles.md](combat-and-projectiles.md)).

## Scenari verificabili

### Scenario 1 — movimento libero

Given il giocatore è in `Gameplay` con un piano attivo,  
When muove l'input direzionale in una direzione qualsiasi,  
Then il personaggio si sposta liberamente in quella direzione, senza vincoli a una griglia discreta.

### Scenario 2 — sparo cardinale indipendente dal movimento

Given il giocatore si sta muovendo liberamente in una direzione,  
When preme l'input di mira verso una delle 4 direzioni cardinali,  
Then il personaggio spara in quella direzione cardinale indipendentemente dalla direzione di movimento in corso.

### Scenario 3 — uso di un oggetto attivo

Given il giocatore possiede un oggetto attivo con cariche disponibili,  
When attiva l'oggetto,  
Then l'effetto si applica secondo il contratto di [active-items.md](active-items.md) e il feedback comunica uso e stato residuo, rispettando il budget di leggibilità di [combat-and-projectiles.md](combat-and-projectiles.md).

### Scenario 4 — sparo senza bersaglio nella direzione di mira

Given nessun nemico è presente nella direzione cardinale scelta,  
When il giocatore spara,  
Then il proiettile viene comunque emesso in quella direzione e segue le regole di collisione con muri e ostacoli descritte in [combat-and-projectiles.md](combat-and-projectiles.md).
