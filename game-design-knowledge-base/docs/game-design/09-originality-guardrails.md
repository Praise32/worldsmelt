---
id: gd-originality
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Regole per mantenere il progetto distinto dalle sue ispirazioni."
---

# Originality Guardrails

## Principio

Il progetto può adottare strutture di genere comuni, ma deve possedere identità, contenuti e presentazione originali.

## Non copiare

- nomi di personaggi, oggetti, stanze o boss;
- sprite, silhouette iconiche o animazioni riconoscibili;
- descrizioni, battute, suoni o musica;
- layout di interfaccia distintivi;
- combinazioni specifiche di meccaniche presentate nello stesso modo;
- lore, simboli o temi narrativi identificabili.

## Rendere originale

Definire:

- tema narrativo proprio;
- vocabolario visivo proprio;
- struttura delle risorse e dei rischi;
- logica delle sinergie;
- ruolo diegetico dell'IA generativa;
- categorie di stanze e ricompense con nomi originali.

## Controllo per agenti

Quando viene usato un riferimento esterno, tradurlo in un requisito astratto. Esempio: non “crea la stanza X di un altro gioco”, ma “crea una stanza rara che offre potere in cambio di un costo persistente”.

## Termini esterni vietati e sostituzioni canoniche

I termini seguenti sono presi da giochi esistenti e **non devono comparire** in nessun
documento della knowledge base né in nessun contenuto generato o testo rivolto al
giocatore. Ogni documento che li usava li sostituisce con il termine canonico del progetto:

| Termine esterno vietato | Sostituzione canonica | Note |
|---|---|---|
| "trinket" | **Innesto** (in-game: **Graft**, DEC-072) | Vedi [Innesti](systems/grafts.md). |
| "élite" | **Veterano** | Nemico potenziato non-boss. |
| "pity" | **correzione di fortuna** | Garanzia che dopo N estrazioni sfortunate la qualità minima sale. |
| set risorse "cuori / monete / bombe / chiavi" | **risorse per funzione** | Salute (stratificata), valuta principale, strumento di breccia, strumento di apertura, catalizzatore di fusione. Nessun nome di risorsa è ripreso da altri giochi. Vedi [Health and Resources](systems/health-and-resources.md). |
| "The Binding of Isaac" / "Isaac" (o qualunque altro titolo di gioco esistente) | (nessuna citazione) | Non deve comparire in nessun documento, nemmeno nel README, nemmeno come riferimento di genere. |
| "devil room" | (ri-tematizzato in modo originale) | Vedi l'archetipo "scambio ad alto rischio" in [Special Rooms](systems/special-rooms.md), da nominare in modo originale, non come citazione diretta. |

## Casi limite

- Un documento tecnico o di supporto (es. note di ricerca, riferimenti di settore) cita un
  termine vietato solo per spiegare *perché* è vietato: è ammesso esclusivamente dentro
  questa tabella, con etichetta esplicita "termine esterno da non usare", mai come
  vocabolario attivo del progetto.
- Un contenuto generato dall'IA produce per errore un nome che coincide con un termine
  vietato o un titolo esistente: viene trattato come contenuto respinto nella validazione
  (vedi [Generated Content Validation](systems/generated-content-validation.md)).

## Non-obiettivi

- Questa lista non vieta i riferimenti di genere generali (es. "action roguelite a
  stanze"), solo nomi, meccaniche e set di risorse riconoscibili e specifici di opere
  esistenti.

## Domande aperte residue

- Nessuna sul naming: il titolo definitivo è **Worldsmelt** (DEC-071) e la
  nomenclatura inglese in-game è fissata da DEC-072 (mappa completa nel
  [glossario](governance/glossary.md)).

## Scenari

- **Dato** che un documento della knowledge base usa ancora la parola "trinket", **quando**
  viene revisionato, **allora** il termine viene sostituito con "Innesto" e il documento
  rimanda a [Innesti](systems/grafts.md).
- **Dato** che un contenuto generato dall'IA propone un nemico potenziato, **quando** viene
  presentato al giocatore, **allora** è chiamato "Veterano", mai "élite".
- **Dato** che un documento descrive le risorse iniziali del giocatore, **quando** le
  elenca, **allora** usa i nomi per funzione (salute, valuta principale, strumento di
  breccia, strumento di apertura, catalizzatore di fusione) e non il set
  cuori/monete/bombe/chiavi.
- **Dato** che qualcuno propone di citare "The Binding of Isaac" per spiegare il genere del
  gioco, **quando** la proposta viene revisionata secondo questa guardrail, **allora** viene
  riformulata come descrizione astratta del genere, senza il nome del titolo.
