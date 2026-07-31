---
id: gd-difficulty-progression
title: Difficulty and Progression
domain: design
status: draft
authority: canonical
owner: design
summary: "Crescita della sfida dentro e tra le run; escalation leggibile del tema su quattro assi piano dopo piano (DEC-024), incluso il budget di stanza condiviso tra ostacoli e nemici (DEC-043, dettaglio in systems/rooms-and-floor-generation.md). Nessun livello di difficoltà selezionabile: la curva dei 5 piani è unica e uguale per tutti (DEC-038)."
last_reviewed: 2026-07-18
topics: [difficoltà, progressione, escalation, bande-di-potenza, curva-unica]
related: []
supersedes: []
source_files: []
---

# Difficulty and Progression

## Durata obiettivo (DEC-017, approved)

Una run completa vinta (Piano 0 + cinque piani, fino al boss del piano 5) ha una durata
obiettivo di **30–45 minuti**. La vittoria al boss del piano 5 chiude la run (DEC-006,
DEC-031, vedi [Run Structure](04-run-structure.md)): non esiste una prosecuzione attiva oltre
il piano 5 da coprire con questo obiettivo di durata; resta solo un'idea futura non
implementata (DEC-018).

## Evoluzione/degenerazione del tema per piano (DEC-005)

Il tema scelto nel Piano 0 evolve o degenera piano dopo piano fino al boss del piano 5, dove
la run si chiude con la vittoria (DEC-006, DEC-031). La difficoltà cresce insieme a questa evoluzione:
piani più avanzati non sono solo più difficili in astratto, ma esprimono una forma più
intensa o corrotta dello stesso tema, leggibile dal giocatore tramite segnali visivi e di
comportamento coerenti (vedi [Content Taxonomy](content/content-taxonomy.md) per i tag di
elemento/forma usati per esprimere questa evoluzione).

## Escalation leggibile del tema per piano (DEC-024)

Piano dopo piano il tema scelto si intensifica su **quattro assi**: aspetto, nemici, regole
di stanza, audio. Questa intensificazione deve però restare sempre un'**escalation
leggibile**, non una distorsione arbitraria:

- l'aspetto e l'audio devono sempre rispettare uno schema visivo comprensibile e un audio
  ascoltabile: il budget di leggibilità (fonte unica:
  [Combat and Projectiles](systems/combat-and-projectiles.md)) vale su **tutti** i piani,
  anche i più avanzati;
- il senso è **progressione dentro il tema**, non trasformazione in qualcosa di
  irriconoscibile. Esempio canonico del proprietario: con un tema "fantasy medievale", il
  piano 1 presenta cavalieri di grado infimo, il piano 5 cavalieri esperti, e il gameplay
  cambia di conseguenza — con eventuali elementi di degrado ambientale coerenti con lo
  stesso tema;
- nei piani avanzati sono ammessi **modificatori di stanza generati**, sempre dentro le
  garanzie di giocabilità definite in
  [Generated Content Validation](systems/generated-content-validation.md) (vedi
  [Rooms and Floor Generation](systems/rooms-and-floor-generation.md) per il dettaglio);
- i **Veterani** (vedi [Enemies](systems/enemies.md)) compaiono con frequenza crescente nei
  piani alti, come parte dell'asse "nemici".

I quattro assi sono descritti in dettaglio nei rispettivi documenti di sistema: aspetto in
[Visual Language](content/visual-language.md), nemici in [Enemies](systems/enemies.md),
regole di stanza in [Rooms and Floor Generation](systems/rooms-and-floor-generation.md),
audio in [Audio and Feedback](content/audio-and-feedback.md). Questo documento non
ridefinisce i dettagli di ciascun asse, solo il principio comune di escalation leggibile
che li lega.

**Soglia confermata (DEC-191, 31/07):** i tre assi aspetto (variante degradata del tileset),
audio (seconda traccia di gameplay) e nemici/boss (boss a due fasi) scattano tutti dallo
stesso confine, il **piano 3** — coincidenza deliberata per la massima leggibilità. Dettaglio
tecnico in [Rooms and Floor Generation](systems/rooms-and-floor-generation.md), sezione
"Stato di implementazione: la stanza vestita dal tileset".

## Difficoltà unica (DEC-038)

Non esiste un livello di difficoltà selezionabile dal giocatore. La curva di difficoltà è
quella dei 5 piani descritta sopra (evoluzione/degenerazione del tema, DEC-005, DEC-024):
è **unica e identica per tutte le run e per tutti i giocatori**, non regolabile in `RunSetup`
né altrove (vedi [Run Setup](ui/run-setup.md), che non espone un selettore di difficoltà).
Questo rende le classifiche **immediatamente confrontabili** tra run diverse, senza bisogno
di normalizzare i risultati per un livello di difficoltà scelto.

## Progressione nella run

La potenza del giocatore cresce tramite oggetti, risorse, sinergie, fusioni esplicite e
decisioni. La difficoltà cresce tramite composizione delle stanze, nuovi pattern, densità
controllata e richieste decisionali.

## Principio di equità

La difficoltà può sorprendere, ma non deve dipendere da informazioni impossibili da leggere o da combinazioni prive di contromisure.

## Progressione del giocatore

Il giocatore migliora in tre modi:

- abilità meccanica;
- conoscenza della grammatica di tag e pattern;
- capacità di costruire e valutare sinergie.

## Meta-progressione proposta

Preferire sblocco di varietà, opzioni e rischio rispetto a incrementi permanenti obbligatori di statistiche.

## Curve da definire

- durata media di un piano (entro l'obiettivo di 30–45 minuti per la run completa);
- numero medio di oggetti per piano;
- crescita dei budget nemici;
- recupero salute e risorse;
- frequenza di contenuti completamente nuovi (origine `nuovo`, vedi
  [AI Content Generation Model](06-ai-content-generation-model.md)).

## Valori numerici di default — stato draft (DEC-019)

I seguenti valori sono già presenti nell'implementazione attuale. Sono **default proposti
dall'implementazione**, non decisioni di design prese: restano `draft` e vanno validati con
il playtest prima di poter essere considerati canone.

- Pesi di rarità: `{55, 30, 12, 3}` per le quattro rarità comune / non-comune / rara /
  leggendaria.
- Pesi di rarità boss: `{0, 0, 70, 30}`.
- Bande di potenza dei colpi generati: `[0.75–1.25]`.
- Bande di potenza dei nemici generati: `[0.7–1.35]`.
- Bande di potenza dei boss generati: `[1.4–3.2]`.

Questi valori sono le "bande di garanzia" richiamate in
[AI Content Generation Model](06-ai-content-generation-model.md) per la generazione
parametrica: quel documento ne descrive il ruolo concettuale, questo ne registra i valori
attuali come proposta da validare.

## Scenari

**Scenario: progressione dentro il tema, non distorsione arbitraria**
- Given una run con tema "fantasy medievale" scelto nel Piano 0
- When il giocatore passa dal piano 1 al piano 5
- Then i nemici passano da cavalieri di grado infimo a cavalieri esperti, il gameplay
  cambia di conseguenza, e il tema resta riconoscibile come lo stesso "fantasy medievale"
  in ogni piano

**Scenario: budget di leggibilità rispettato anche nei piani avanzati**
- Given un piano avanzato (es. piano 5) con l'aspetto e l'audio del tema intensificati al
  massimo previsto
- When il gioco compone la scena con tutti gli elementi generati per quel piano
- Then lo schema visivo resta comprensibile e l'audio resta ascoltabile, perché il budget
  di leggibilità di [Combat and Projectiles](systems/combat-and-projectiles.md) si applica
  a quel piano come a tutti gli altri

**Scenario: Veterani più frequenti nei piani alti**
- Given due piani della stessa run, uno basso (es. piano 1) e uno alto (es. piano 5)
- When il gioco genera i nemici di ciascun piano
- Then la frequenza di Veterani nel piano alto è maggiore di quella nel piano basso,
  coerente con l'asse "nemici" dell'escalation del tema

**Scenario: modificatore di stanza generato in un piano avanzato**
- Given un piano avanzato che genera un modificatore di stanza legato al tema
- When il modificatore viene validato
- Then rispetta le garanzie di giocabilità definite in
  [Generated Content Validation](systems/generated-content-validation.md) prima di poter
  apparire nella run

**Scenario: nessun selettore di difficoltà**
- Given un giocatore che avvia una nuova run da `RunSetup`
- When cerca un'opzione per scegliere il livello di difficoltà
- Then non la trova: la curva di difficoltà dei 5 piani è unica e uguale per tutte le run
  (DEC-038), e le classifiche restano confrontabili senza bisogno di normalizzazione
