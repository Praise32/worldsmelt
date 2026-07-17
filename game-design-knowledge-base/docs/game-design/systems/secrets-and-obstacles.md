---
id: gd-system-secrets-obstacles
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Ostacoli distruttibili/attraversabili e uso dello strumento di breccia (DEC-013, funzione che sostituisce le bombe). La scoperta delle stanze segrete resta esplicitamente domanda aperta."
---

# Secrets and Obstacles

## Intento per il giocatore

Gli ostacoli devono creare decisioni tattiche immediate (aprire un varco o no, quando e con cosa); i segreti devono premiare l'osservazione, senza dipendere solo da tentativi casuali — anche se il meccanismo esatto di scoperta è ancora da definire.

## Condizioni di ingresso

Ostacoli e passaggi segreti compaiono all'interno di qualunque stanza generata o curata del piano (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)), inclusa la stanza segreta come archetipo speciale (vedi [special-rooms.md](./special-rooms.md)).

## Ostacoli

Gli ostacoli possono bloccare movimento, linea di tiro, ricompense o percorsi. Devono avere regole coerenti di distruzione o attraversamento.

## Input/azioni

Il giocatore usa lo **strumento di breccia** (DEC-013, nome placeholder per funzione, sostituisce "bombe") per distruggere ostacoli, muri deboli o aprire varchi. Le regole della risorsa stessa (come si ottiene, cap massimo, ordine di consumo, rarità e fonti) sono definite in [health-and-resources.md](./health-and-resources.md) come fonte unica; questo documento descrive solo il suo **uso** nel contesto di ostacoli e segreti, senza ripetere quelle regole.

## Risultato

- Uso riuscito dello strumento di breccia su un ostacolo compatibile: l'ostacolo viene rimosso, aprendo un percorso, una scorciatoia o una ricompensa.
- Uso su un bersaglio incompatibile o senza scorte disponibili: nessun effetto; dove possibile, va segnalato chiaramente prima della conferma per evitare di sprecare la risorsa.

## Feedback

- indicazione visiva che un ostacolo è distruttibile/attraversabile con lo strumento di breccia;
- conferma visiva della distruzione e di cosa si apre;
- per i segreti: indizio ambientale deducibile (grammatica), non puramente casuale.

## Segreti

Le stanze e i passaggi segreti devono essere deducibili tramite una grammatica, non dipendere soltanto da tentativi casuali. **Il meccanismo esatto con cui il giocatore scopre una stanza segreta resta esplicitamente una domanda di design aperta** (vedi "Domande aperte residue"): questo documento non inventa una risposta definitiva.

## Interazioni

- con lo strumento di breccia e il suo costo (vedi [health-and-resources.md](./health-and-resources.md));
- con la stanza segreta come archetipo speciale (vedi [special-rooms.md](./special-rooms.md));
- con la generazione del piano, che colloca ostacoli e segreti nella griglia (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)).

## Regole per contenuti generati

L'IA può variare forma e presentazione di ostacoli e indizi, ma non deve nascondere completamente gli indizi previsti dal sistema. Ogni ostacolo/segreto generato dichiara un'origine (curato | composto | variato | nuovo) e rispetta la grammatica di deducibilità richiesta.

## Casi limite

- Il giocatore esaurisce lo strumento di breccia davanti a un ostacolo necessario per il progresso principale (non un segreto opzionale): l'ostacolo non deve bloccare l'unico percorso critico della stanza.
- Un indizio generato risulta illeggibile in validazione: il contenuto va respinto o corretto prima di apparire nella run.
- Una stanza segreta generata resta irraggiungibile per un difetto di layout: va trattata come caso limite di generazione, non come segreto "voluto" (va respinta in validazione).

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni categoria di ostacolo/indizio ha un contenuto curato sufficiente a completare la run senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non definisce le regole generali dello strumento di breccia come risorsa (come si ottiene, cap, ecc.): vedi [health-and-resources.md](./health-and-resources.md).
- Non risolve il meccanismo di scoperta delle stanze segrete: resta domanda aperta.
- Non dettaglia l'archetipo di stanza segreta come tipo di stanza (vedi [special-rooms.md](./special-rooms.md)).

## Domande aperte residue

- **Come vengono individuate le stanze segrete** (meccanismo di scoperta): domanda esplicitamente aperta, non risolta da questo aggiornamento.
- Se esistono più livelli di "nascondimento" (indizio debole vs indizio forte).
- Se lo strumento di breccia ha un solo tipo di bersaglio o più categorie di ostacolo con costi diversi.

## Scenari

### Scenario 1 — Uso riuscito dello strumento di breccia

Given un giocatore con almeno una scorta di strumento di breccia
When lo usa su un ostacolo distruttibile compatibile
Then l'ostacolo viene rimosso, il percorso o la ricompensa dietro di esso diventa accessibile, e la scorta si riduce secondo le regole di [health-and-resources.md](./health-and-resources.md)

### Scenario 2 — Scorte esaurite davanti a un ostacolo critico

Given un giocatore senza scorte di strumento di breccia
When incontra un ostacolo che blocca l'unico percorso verso l'uscita della stanza
Then la generazione deve garantire un percorso alternativo, perché l'ostacolo non può essere l'unico blocco critico

### Scenario 3 — Indizio di segreto illeggibile

Given un indizio ambientale generato per una stanza segreta
When la validazione lo controlla per leggibilità
Then un indizio non deducibile viene respinto o corretto prima di poter apparire nella run

### Scenario 4 — Domanda aperta sulla scoperta

Given un giocatore che esplora un piano
When cerca di capire come individuare una stanza segreta
Then la KB non fornisce ancora una regola definitiva: la questione resta registrata come domanda aperta, in attesa di decisione di design
