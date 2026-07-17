---
id: gd-ui-generation-status
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Indicatore di generazione dentro il Piano 0, non una schermata a sé."
---

# Generation Status

## Intento

Comunicare al giocatore, dentro il Piano 0, solo le informazioni utili sulla preparazione
dei contenuti, senza ansia inutile e senza dettagli tecnici.

## Non è una schermata

Questo documento non descrive uno stato di navigazione. Non esiste più uno stato
"GeneratingRun"/"GenerationStatus" separato: l'indicatore vive dentro `FloorZero` mentre il
giocatore esplora l'hub, sceglie il tema o il personaggio, o visita il museo (DEC-002,
DEC-004). Vedi `ui/navigation-map.md` per gli stati canonici e `systems/floor-zero.md` per
la spec completa del Piano 0.

## Condizioni di ingresso

L'indicatore è visibile per tutta la durata in cui almeno un contenuto della run non è
ancora `approvato-per-run` (vedi stati di validazione in `governance/glossary.md`).

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Indicatore di stato | Un contenuto della run è ancora in preparazione | Sempre, se visibile | Nessuna (sola lettura) | — | Messaggio descrittivo stabile |
| Uscita verso il piano 1 | Sempre | Solo quando il piano 1 è pronto e validato | Lascia `FloorZero` | Entra in `Gameplay` | L'uscita cambia aspetto quando si abilita |

## Stati visibili al giocatore

- preparazione iniziale;
- primo piano pronto (l'uscita verso il piano 1 si abilita);
- contenuti dei piani successivi in preparazione, in background;
- recupero tramite fallback (silenzioso, vedi sotto).

Questi sono stati di comunicazione al giocatore, distinti dagli stati tecnici di
validazione del contenuto (`proposto, strutturalmente-valido, simulato,
approvato-per-run, respinto, fallback-usato`) che restano interni: non vanno mostrati
nell'interfaccia normale.

## Regole

- L'uscita verso il piano 1 si abilita quando il piano 1 e i requisiti minimi sono pronti (DEC-004).
- Non mostrare percentuali false né stime di tempo non affidabili.
- Preferire messaggi descrittivi stabili a barre di progresso ingannevoli.
- Non interrompere l'esplorazione del Piano 0 per comunicare la generazione in background.
- Non mostrare dettagli tecnici, prompt o errori interni: fonte unica delle regole di trasparenza `06-ai-content-generation-model.md`.

## Fallback silenzioso

Se un contenuto non supera la validazione, il gioco sostituisce silenziosamente con un
fallback curato senza allarmare il giocatore e senza bloccare l'uscita verso il piano 1
quando il fallback stesso è pronto. Fonte unica delle regole di fallback:
`systems/generated-content-validation.md`; questo documento non le riformula.

## Errori

Un errore non recuperabile non è uno stato di navigazione ("Error Recovery" non esiste,
vedi `ui/navigation-map.md`): si risolve sempre con un fallback, mai con un blocco della
partita (DEC-020).

## Non-obiettivi

- Non descrive l'architettura tecnica della generazione (vedi `06-ai-content-generation-model.md`).
- Non introduce un nuovo stato di navigazione.

## Domande aperte residue

- Nessuna specifica; il modello di generazione resta approved per principio (DEC-020), i dettagli tecnici sono fuori scope di design.

## Scenari verificabili

1. **Given** il giocatore entra nel Piano 0 subito dopo l'avvio, **when** il piano 1 non è ancora pronto, **then** l'indicatore mostra "preparazione iniziale" e l'uscita verso il piano 1 resta disabilitata.
2. **Given** il piano 1 diventa pronto e validato, **when** l'indicatore si aggiorna, **then** l'uscita verso il piano 1 si abilita senza richiedere un'azione aggiuntiva del giocatore.
3. **Given** un contenuto del piano 2 fallisce la validazione mentre il giocatore è ancora nel Piano 0, **when** il sistema applica il fallback curato, **then** nessun messaggio d'allarme o dettaglio tecnico viene mostrato al giocatore.
4. **Given** il giocatore è nel Piano 0 a scegliere il personaggio, **when** la generazione dei piani successivi procede in background, **then** l'esplorazione dell'hub non viene interrotta.
