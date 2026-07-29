---
id: gd-system-bosses
title: Bosses
domain: design
status: approved
authority: canonical
owner: design
summary: "Boss come culmine del piano; sconfiggere il boss del piano 5 chiude la run con vittoria (DEC-006, aggiornata da DEC-031). La prosecuzione in piani extra non è implementata ora: resta un'idea futura (DEC-018). Escalation di fasi per piano (DEC-028, DEC-106): piani 1 e 2 fase singola, dal piano 3 due fasi, piano 5 il più complesso. Bande di potenza e pesi di rarità del pool boss restano default draft (DEC-019). Tutti e 5 i boss della run sono generati dal tema, validati nelle bande boss: nessun boss fisso del gioco (DEC-054). La stanza boss è sempre foglia del grafo di adiacenza del piano, mai un passaggio obbligato per un'altra stanza (DEC-182, fonte unica in rooms-and-floor-generation.md)."
last_reviewed: 2026-07-30
last_verified_commit: 0ec60d0
topics: [boss, vittoria, escalation, fasi, DEC-054, bande di potenza, DEC-182, boss-isolato]
related: []
supersedes: []
source_files: []
---

# Bosses

## Intento per il giocatore

Il boss verifica abilità e decisioni sviluppate nel piano, senza dipendere da una singola build obbligatoria, e segna il ritmo dell'obiettivo di vittoria della run.

## Condizioni di ingresso

- Il giocatore raggiunge la stanza boss dopo aver completato le condizioni previste dal piano (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)).
- La stanza boss è sempre una **foglia** del grafo di adiacenza delle stanze del piano — una sola porta, mai un nodo di passaggio obbligato per raggiungere un'altra stanza (DEC-182). Regola e verifica di dettaglio: fonte unica [rooms-and-floor-generation.md](./rooms-and-floor-generation.md#porte-e-connettività-del-piano-dec-181-dec-182), non ripetuta qui.
- Il boss del piano 5 è l'ultimo boss della run: la sua sconfitta chiude la run con vittoria (DEC-006, aggiornata da DEC-031).

## Input/azioni

Il giocatore usa la build corrente, movimento libero e sparo nelle quattro direzioni per superare fasi e pattern del boss (vedi [combat-and-projectiles.md](./combat-and-projectiles.md)).

## Struttura minima

- identità e tema;
- arena;
- fasi;
- pattern principali;
- transizioni leggibili;
- contromisure;
- ricompensa;
- condizioni anti-stallo;
- fallback.

## Escalation dei boss per piano (DEC-028, DEC-106)

Come gli altri assi dell'escalation del tema (DEC-024, vedi
[Difficulty and Progression](../07-difficulty-and-progression.md) per il principio
generale), anche i boss crescono in complessità piano dopo piano:

- i boss dei **piani 1 e 2** hanno una **fase singola**, leggibile fin dal primo incontro
  (DEC-106: rampa dolce, il salto di complessità coincide con la metà della run);
- dal **piano 3** i boss hanno **due fasi**, con un cambio di comportamento leggibile alla
  transizione;
- il boss del **piano 5** è il **più complesso** della run standard.

Questa progressione resta comunque soggetta al [Limite](#limite) sotto e al budget di
leggibilità di [Combat and Projectiles](./combat-and-projectiles.md): più fasi e più
regole non significano mai perdere la leggibilità delle transizioni.

## Tutti i boss sono generati dal tema (DEC-054)

I **5 boss della run** (uno per piano) sono **tutti generati dal tema** scelto nel Piano 0,
e validati dentro le bande di potenza boss (vedi "Bande di potenza boss" più sotto) come
qualunque altro contenuto generato. **Non esiste un boss fisso del gioco**: nessun boss è
un'entità curata sempre uguale tra le run, a differenza, per esempio, della rosa di
personaggi base (DEC-030), che resta curata per design.

Questo è coerente con l'escalation di fasi per piano (DEC-028, sopra) e con il principio
generale di generazione entro bande di garanzia (vedi
[AI Content Generation Model](../06-ai-content-generation-model.md)): i boss cambiano
identità e tema run dopo run, ma la loro complessità strutturale (fasi, transizioni) segue
sempre la stessa progressione per piano.

Il **fallback curato** dei boss (vedi sotto) resta valido come rete di sicurezza quando la
generazione non supera la validazione: non contraddice questa regola, perché è un contenuto
usato solo in assenza di un boss generato valido, non un boss fisso presentato come identità
ricorrente del gioco.

## Risultato (DEC-006, aggiornato da DEC-031)

- **Boss dei piani 1-4:** la sconfitta apre l'uscita verso il piano successivo.
- **Boss del piano 5:** la sconfitta chiude la run con **vittoria**, valida per classifiche. La run finisce qui: non è prevista una prosecuzione in piani extra in questa fase del gioco (l'idea è parcheggiata tra le idee future, DEC-018, DEC-031).
- **Morte, in qualunque piano:** salute a zero significa run persa (permadeath). L'ordine di consumo tra salute temporanea e salute base è definito in [health-and-resources.md](./health-and-resources.md); questo documento non lo ripete. Alla sconfitta i punti sblocco maturati restano in misura ridotta rispetto alla vittoria, e il catalogo si aggiorna comunque con le creazioni incontrate (DEC-041; dettaglio in [results-and-leaderboards.md](../ui/results-and-leaderboards.md), rimando, non riformulato qui).

## Feedback

- transizioni di fase leggibili e distinte;
- segnale chiaro alla sconfitta del boss del piano 5: la run si chiude con vittoria, senza alcuna scelta di prosecuzione (DEC-031).

## Interazioni

- con il tema di run, che degenera progressivamente fino al boss del piano 5;
- con la build del giocatore, senza dipendenza da una combinazione obbligatoria;
- con la ricompensa (vedi [rewards-and-economy.md](./rewards-and-economy.md)).

## Regole per contenuti generati

Un boss può essere composto da moduli, ma deve apparire come un'entità coerente, non come una lista casuale di attacchi. Dichiara un'origine (curato | composto | variato | nuovo) e supera la validazione prevista.

- **Bande di potenza boss (DEC-019):** oggi i boss generati sono scalati entro una banda di potenza **[1.4–3.2]**. **Stato: draft, default proposto dall'implementazione attuale, da validare col playtest.**
- **Pesi di rarità del pool ricompense boss (DEC-019):** `{comune: 0, non-comune: 0, rara: 70, leggendaria: 30}`. **Stato: draft, default proposto.** La tassonomia generale di oggetti e rarità è definita in [items-pools-and-rarity.md](./items-pools-and-rarity.md); qui i pesi sono riportati solo nell'ottica della ricompensa boss, senza ridefinire la tassonomia.

## Limite

Il boss finale non deve introdurre contemporaneamente troppe regole mai viste nella run.

## Casi limite

- Il boss generato per un piano eccede la banda di potenza [1.4–3.2]: va respinto o riscalato in validazione, come qualunque boss generato.
- Il giocatore abbandona la run prima di affrontare il boss del piano 5: nessuna vittoria registrata.
- La generazione non produce un boss valido per un piano: si usa il fallback curato (vedi sotto), che resta un'eccezione di sicurezza, non un boss fisso ricorrente della run.

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni boss ha un pool curato sufficiente a completare la run senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non definisce il budget di leggibilità del telegraph (vedi [combat-and-projectiles.md](./combat-and-projectiles.md)).
- Non definisce l'ordine di consumo salute (vedi [health-and-resources.md](./health-and-resources.md)).
- Non definisce la regola di fallback dei contenuti generati (vedi [generated-content-validation.md](./generated-content-validation.md)).
- Non ridefinisce la tassonomia oggetti e rarità (vedi [items-pools-and-rarity.md](./items-pools-and-rarity.md)).

## Domande aperte residue

- Valore finale delle bande di potenza boss e dei pesi di rarità dopo playtest (DEC-019).
- Numero esatto di fasi del boss del piano 5, oltre alla qualifica di "il più complesso"
  (DEC-028).

## Scenari

### Scenario 1 — Vittoria al piano 5

Given un giocatore che raggiunge il boss del piano 5 con salute residua
When sconfigge il boss
Then la run si chiude con vittoria, valida per classifiche, e finisce lì: nessuna scelta di prosecuzione in piani extra viene presentata (DEC-031)

### Scenario 2 — Sconfitta prima del piano 5

Given un giocatore che non ha ancora sconfitto il boss del piano 5
When la sua salute base scende a zero in un piano qualunque
Then la run termina in permadeath, con i punti sblocco maturati mantenuti in misura ridotta rispetto alla vittoria (DEC-041)

### Scenario 3 — Boss generato fuori banda

Given un boss generato per un piano con potenza calcolata sopra 3.2 (banda draft DEC-019)
When la validazione lo verifica
Then il boss è respinto o riscalato entro la banda prima di poter apparire nella run

### Scenario 4 — Ricompensa boss con pesi di rarità draft

Given la sconfitta di un boss
When il gioco genera la ricompensa dal pool pesato {comune: 0, non-comune: 0, rara: 70, leggendaria: 30}
Then l'oggetto assegnato è sempre di rarità rara o leggendaria, coerente con il default attuale (draft, da validare col playtest)

### Scenario 5 — Boss del piano 1 a fase singola

Given il giocatore raggiunge il boss del piano 1
When lo affronta
Then il boss ha una sola fase, leggibile fin dal primo incontro, senza cambi di comportamento a metà scontro

### Scenario 6 — Boss dal piano 3 con due fasi

Given il giocatore raggiunge il boss di un piano dal 3 in su
When la salute del boss scende sotto la soglia di transizione
Then il boss cambia fase con un comportamento nuovo, leggibile come cambio di fase (DEC-028)

### Scenario 7 — Tutti i boss sono generati dal tema

Given una run con un tema scelto nel Piano 0
When il giocatore affronta i boss dei 5 piani
Then ciascuno dei 5 boss è generato dal tema di quella run e validato entro le bande di potenza boss, senza che nessuno di essi sia un'entità fissa e identica tra run diverse (DEC-054)

### Scenario 8 — Boss del piano 2 a fase singola

Given il giocatore raggiunge il boss del piano 2
When lo affronta
Then il boss ha una sola fase, leggibile fin dal primo incontro, senza cambi di comportamento a metà scontro, e la prima transizione a due fasi resta riservata al piano 3 (DEC-106)
