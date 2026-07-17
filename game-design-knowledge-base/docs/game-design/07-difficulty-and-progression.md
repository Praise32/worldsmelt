---
id: gd-difficulty-progression
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Crescita della sfida dentro e tra le run."
---

# Difficulty and Progression

## Durata obiettivo (DEC-017, approved)

Una run completa vinta (Piano 0 + cinque piani, fino al boss del piano 5) ha una durata
obiettivo di **30–45 minuti**. I piani extra dopo il boss del piano 5 (vedi
[Run Structure](04-run-structure.md)) non sono coperti da questo obiettivo di durata: sono
per definizione una prosecuzione oltre l'arco principale.

## Evoluzione/degenerazione del tema per piano (DEC-005)

Il tema scelto nel Piano 0 evolve o degenera piano dopo piano fino al boss del piano 5, e
continua a degenerare nei piani extra. La difficoltà cresce insieme a questa evoluzione:
piani più avanzati non sono solo più difficili in astratto, ma esprimono una forma più
intensa o corrotta dello stesso tema, leggibile dal giocatore tramite segnali visivi e di
comportamento coerenti (vedi [Content Taxonomy](content/content-taxonomy.md) per i tag di
elemento/forma usati per esprimere questa evoluzione).

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
