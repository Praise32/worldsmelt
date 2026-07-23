---
id: gd-system-secrets-obstacles
title: Secrets and Obstacles
domain: design
status: approved
authority: canonical
owner: design
summary: "Ostacoli distruttibili/attraversabili e uso dello strumento di breccia (DEC-013, funzione che sostituisce le bombe). Ostacoli generati a tema, con croce centrale libera e telegraph leggibili, a budget di difficoltà condiviso con i nemici (DEC-043). Stanze segrete a due livelli — normali e super-segrete (DEC-025)."
last_reviewed: 2026-07-18
last_verified_commit: 0ec60d0
topics: [ostacoli, segreti, strumento-di-breccia, telegraph, budget-difficoltà]
related: []
supersedes: []
source_files: []
---

# Secrets and Obstacles

## Intento per il giocatore

Gli ostacoli devono creare decisioni tattiche immediate (aprire un varco o no, quando e con cosa); i segreti devono premiare l'osservazione (stanze segrete "normali") o la preparazione/intuizione (stanze "super-segrete"), senza dipendere solo da tentativi casuali (DEC-025).

## Condizioni di ingresso

Ostacoli e passaggi segreti compaiono all'interno di qualunque stanza generata o curata del piano (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)), inclusa la stanza segreta come archetipo speciale (vedi [special-rooms.md](./special-rooms.md)).

## Ostacoli

Gli ostacoli possono bloccare movimento, linea di tiro, ricompense o percorsi. Devono avere regole coerenti di distruzione o attraversamento.

## Ostacoli generati a tema (DEC-043)

Gli ostacoli ambientali sono un tipo di contenuto **generato dal tema** della run: forma e
comportamento semplice, sempre dentro le garanzie di giocabilità:

- **croce centrale libera**: la stanza mantiene sempre un corridoio centrale a croce libero
  da ostacoli, così il movimento e la linea di tiro principali restano garantiti anche nella
  stanza più densa di ostacoli;
- **telegraph leggibili**: ogni ostacolo che rappresenta un pericolo (non solo un blocco
  passivo) segnala il proprio comportamento in anticipo, secondo il budget di leggibilità di
  [Combat and Projectiles](./combat-and-projectiles.md), non riformulato qui.

Gli ostacoli generati a tema comprendono due famiglie:

- **blocchi**: ostacoli passivi che limitano movimento, linea di tiro o accesso, senza
  infliggere danno diretto;
- **pericoli passivi telegrafati**: ostacoli che infliggono danno o effetti se il giocatore
  li tocca o li attraversa senza attenzione, sempre segnalati in anticipo (telegraph).

La **degenerazione del tema** (DEC-024) rende gli ostacoli più insidiosi nei piani alti:
più densi, più aggressivi nei pericoli passivi, ma sempre dentro la croce centrale libera e
i telegraph leggibili. Il **budget di difficoltà della stanza** copre insieme ostacoli e
nemici: spendere budget in ostacoli riduce quanto resta disponibile per i nemici della
stessa stanza, e viceversa (fonte del principio di budget condiviso:
[Rooms and Floor Generation](./rooms-and-floor-generation.md), non riformulato qui).

## Input/azioni

Il giocatore usa lo **strumento di breccia** (DEC-013; in-game: **Blast Charges**, DEC-072) per distruggere ostacoli, muri deboli o aprire varchi. Le regole della risorsa stessa (come si ottiene, cap massimo, ordine di consumo, rarità e fonti) sono definite in [health-and-resources.md](./health-and-resources.md) come fonte unica; questo documento descrive solo il suo **uso** nel contesto di ostacoli e segreti, senza ripetere quelle regole.

## Risultato

- Uso riuscito dello strumento di breccia su un ostacolo compatibile: l'ostacolo viene rimosso, aprendo un percorso, una scorciatoia o una ricompensa.
- Uso su un bersaglio incompatibile o senza scorte disponibili: nessun effetto; dove possibile, va segnalato chiaramente prima della conferma per evitare di sprecare la risorsa.

## Feedback

- indicazione visiva che un ostacolo è distruttibile/attraversabile con lo strumento di breccia;
- conferma visiva della distruzione e di cosa si apre;
- per i segreti: indizio ambientale deducibile (grammatica), non puramente casuale.

## Segreti (DEC-025, due livelli)

Le stanze segrete si dividono in due livelli, entrambi deducibili tramite una grammatica
diversa, mai da tentativi puramente casuali:

- **Stanze segrete "normali"**: hanno indizi visivi leggibili (crepe, anomalie del tema)
  che il giocatore può osservare e interpretare; si aprono con lo **strumento di breccia**
  (vedi sotto), come qualunque altro ostacolo distruttibile compatibile.
- **Stanze "super-segrete"**: non hanno alcun indizio visivo leggibile. Si trovano solo
  con oggetti o Innesti rivelatori dedicati, oppure per intuizione estrema del giocatore
  (esplorazione sistematica senza alcun aiuto di sistema).

Questo risolve la domanda di design precedentemente aperta sul metodo di scoperta delle
stanze segrete.

## Interazioni

- con lo strumento di breccia e il suo costo (vedi [health-and-resources.md](./health-and-resources.md));
- con la stanza segreta come archetipo speciale (vedi [special-rooms.md](./special-rooms.md));
- con la generazione del piano, che colloca ostacoli e segreti nella griglia e condivide il budget di difficoltà tra ostacoli e nemici (DEC-043, vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md));
- con l'escalation leggibile del tema, che rende gli ostacoli più insidiosi nei piani alti (DEC-024, vedi [Difficulty and Progression](../07-difficulty-and-progression.md));
- con il budget di leggibilità dei telegraph (vedi [combat-and-projectiles.md](./combat-and-projectiles.md)).

## Regole per contenuti generati

L'IA può variare forma e presentazione di ostacoli e indizi, ma non deve nascondere completamente gli indizi previsti dal sistema per le stanze segrete "normali" (DEC-025): l'indizio (crepa, anomalia del tema) deve restare deducibile. Per le stanze "super-segrete" l'IA può variare liberamente la forma dell'oggetto/Innesto rivelatore, ma non deve introdurre indizi ambientali impliciti che le rendano scopribili senza quell'oggetto o senza intuizione estrema. Ogni ostacolo/segreto generato dichiara un'origine (curato | composto | variato | nuovo) e rispetta la grammatica di deducibilità richiesta per il proprio livello.

Per gli ostacoli generati a tema (DEC-043), l'IA inventa forma e comportamento semplice
dentro le bande di garanzia dichiarate, ma non può mai generare un ostacolo che occupi la
croce centrale libera della stanza né un pericolo passivo privo di telegraph leggibile: sono
garanzie di giocabilità non negoziabili, verificate secondo
[Generated Content Validation](./generated-content-validation.md).

## Casi limite

- Il giocatore esaurisce lo strumento di breccia davanti a un ostacolo necessario per il progresso principale (non un segreto opzionale): l'ostacolo non deve bloccare l'unico percorso critico della stanza.
- Un indizio generato risulta illeggibile in validazione: il contenuto va respinto o corretto prima di apparire nella run.
- Una stanza segreta generata resta irraggiungibile per un difetto di layout: va trattata come caso limite di generazione, non come segreto "voluto" (va respinta in validazione).
- Una stanza "super-segreta" generata in un piano dove nessun oggetto/Innesto rivelatore è ancora apparso nella run: la stanza resta comunque scopribile per intuizione estrema, e non deve essere l'unico modo di completare un requisito obbligatorio della run.
- Un ostacolo generato che finirebbe per occupare la croce centrale libera della stanza: va respinto o riposizionato prima di comparire nella run (DEC-043).
- Una stanza con budget di difficoltà quasi interamente speso in ostacoli: il budget residuo per i nemici va comunque rispettato, anche se minimo (DEC-043); non deve azzerarsi la presenza di nemici quando la tassonomia della stanza la richiede.

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni categoria di ostacolo/indizio ha un contenuto curato sufficiente a completare la run senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non definisce le regole generali dello strumento di breccia come risorsa (come si ottiene, cap, ecc.): vedi [health-and-resources.md](./health-and-resources.md).
- Non definisce il tasso esatto di comparsa degli oggetti/Innesti rivelatori delle stanze super-segrete (vedi Domande aperte residue).
- Non dettaglia l'archetipo di stanza segreta come tipo di stanza (vedi [special-rooms.md](./special-rooms.md)).

## Domande aperte residue

- Quali oggetti/Innesti rivelatori esistono per le stanze "super-segrete" (DEC-025) e con
  quale tasso compaiono nei pool.
- Se lo strumento di breccia ha un solo tipo di bersaglio o più categorie di ostacolo con
  costi diversi.
- Proporzione esatta tra blocchi e pericoli passivi telegrafati nella generazione a tema, e
  valori numerici del budget di difficoltà condiviso tra ostacoli e nemici (DEC-043 fissa il
  principio, non i numeri).

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

### Scenario 4 — Stanza segreta "normale" con indizio leggibile

Given un giocatore che esplora un piano e nota una crepa o un'anomalia del tema in una parete
When usa lo strumento di breccia sull'indizio
Then la stanza segreta "normale" si apre, coerente con la grammatica deducibile di DEC-025

### Scenario 5 — Stanza "super-segreta" senza indizi

Given un giocatore in possesso di un oggetto o Innesto rivelatore
When esplora un'area priva di qualunque indizio visivo
Then può individuare una stanza "super-segreta" solo grazie all'oggetto/Innesto rivelatore o a un'intuizione estrema, senza alcun aiuto ambientale

### Scenario 6 — Ostacolo generato a tema rispetta la croce centrale libera

Given una stanza generata con ostacoli a tema per un piano avanzato (DEC-043)
When la validazione controlla il layout della stanza
Then il corridoio centrale a croce resta libero da ostacoli, indipendentemente da quanto il tema si sia intensificato

### Scenario 7 — Pericolo passivo telegrafato nei piani alti

Given una stanza di un piano alto con un pericolo passivo generato a tema, reso più insidioso dalla degenerazione (DEC-024)
When il giocatore vi si avvicina
Then il pericolo mostra sempre un telegraph leggibile prima di poter infliggere danno, coerente col budget di leggibilità di [Combat and Projectiles](./combat-and-projectiles.md)
