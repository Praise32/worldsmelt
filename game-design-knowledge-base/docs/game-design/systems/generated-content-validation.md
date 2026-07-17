---
id: gd-system-content-validation
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Criteri per accettare o rifiutare contenuti generati."
---

# Generated Content Validation

## Stati del contenuto

- proposed;
- structurally-valid;
- simulated;
- approved-for-run;
- rejected;
- fallback-used.

## Controlli minimi

- schema completo;
- valori entro limiti;
- risorse esistenti;
- nessuna dipendenza circolare proibita;
- leggibilità visiva;
- budget di potenza o pericolo;
- compatibilità con il piano;
- condizioni di completamento raggiungibili;
- descrizione coerente con l'effetto;
- originalità.

## Regola

Un contenuto rifiutato non deve diventare visibile al giocatore nella modalità standard.

## Fallback

Ogni categoria generabile possiede un pool curato sufficiente a completare la run senza generazione nuova.

## Telemetria di design

Registrare motivo del rifiuto, fallback usato e impatto sulla run, senza esporre dati tecnici nell'interfaccia normale.
