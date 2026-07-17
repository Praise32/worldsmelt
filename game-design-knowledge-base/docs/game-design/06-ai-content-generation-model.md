---
id: gd-ai-content-model
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Ruolo concettuale dell’IA locale nella creazione dei contenuti."
---

# AI Content Generation Model

## Principio

L'IA locale propone contenuti entro un contratto. Il gioco decide se tali contenuti sono validi, coerenti e utilizzabili.

## Tipi di contenuto

- Varianti di oggetti.
- Nemici e boss.
- Pattern di attacco.
- Aspetto e composizione degli sprite.
- Sinergie tra effetti.
- Temi di piano.
- Stanze e ricompense.
- Nomi e descrizioni coerenti con la tassonomia.

## Livelli di generazione

1. **Selezione:** scegliere e combinare elementi già validati.
2. **Composizione:** creare una nuova combinazione da moduli conosciuti.
3. **Variazione:** modificare parametri entro intervalli sicuri.
4. **Nuovo archetipo:** introdurre una regola nuova; richiede validazione più severa.

## Vincoli obbligatori

Ogni contenuto deve avere:

- identità univoca nella run;
- categoria e tag;
- budget di potenza o pericolo;
- segnali visivi e audio;
- descrizione comprensibile;
- dipendenze dichiarate;
- condizioni di esclusione;
- fallback;
- test di giocabilità.

## Trasparenza al giocatore

Il gioco può comunicare che la run è stata generata, ma non deve mostrare prompt, errori interni o dettagli tecnici durante l'esperienza normale.

## Limite

L'IA non può modificare arbitrariamente regole fondamentali come input, condizioni di vittoria, significato delle risorse o segnali di pericolo senza una modalità esplicitamente dedicata.
