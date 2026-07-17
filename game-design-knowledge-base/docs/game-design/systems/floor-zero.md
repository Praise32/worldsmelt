---
id: gd-system-floor-zero
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Piano 0: hub ibrido di rifugio e arene opzionali, dove si sceglie tema e personaggio mentre la run si prepara."
---

# Floor Zero

## Intento per il giocatore

Il Piano 0 è un luogo sicuro, sempre disponibile, in cui il giocatore prepara la run
successiva senza fretta e senza rischio di perdita. Deve dare la sensazione di un rifugio
personale che cresce con le run passate, non di una schermata di attesa.

## Condizioni di ingresso

- Si entra nel Piano 0 dallo stato `RunSetup`, come descritto in
  [Game States and Flow](../05-game-states-and-flow.md).
- Si rientra nel Piano 0 anche al termine di una run (vittoria, sconfitta o uscita
  volontaria dai piani extra), prima di tornare al menu principale o avviarne un'altra.
- Il Piano 0 non richiede che alcun contenuto della run futura sia già pronto: è sempre
  giocabile, per costruzione (vedi "Regole per contenuti generati").

## Input/azioni

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Carte tema (2-3 proposte) | Sempre, all'ingresso nel Piano 0 | Finché il tema della run non è stato scelto | Selezionare una carta tema | Il tema guida la generazione dei piani 1-5 e la sua evoluzione/degenerazione fino al boss del piano 5 | Evidenziazione della carta scelta, anteprima del tema |
| Selettore personaggio | Sempre | Sempre | Scegliere il personaggio base o l'alternativa generata per la run, oppure rifiutare l'alternativa | Il personaggio scelto definisce statistiche e trait per l'intera run (vedi [Characters](characters.md)) | Scheda personaggio con statistiche e trait in evidenza |
| Ingresso arena di sfida | Quando esistono contenuti "best-of" già validati disponibili per un'arena | Quando il giocatore non è già impegnato in un'altra attività del Piano 0 | Entrare nell'arena opzionale | Sfida autonoma locale al Piano 0, con ricompense proprie; non modifica la run in preparazione | Segnale d'ingresso dedicato, esito mostrato a fine sfida |
| Museo delle creazioni | Sempre | Sempre | Sfogliare le migliori creazioni delle run passate | Nessun effetto meccanico sulla run corrente, solo consultazione | Galleria consultabile, nessuna modifica allo stato di gioco |
| Indicatore di generazione | Sempre, da quando inizia la preparazione dei piani successivi | Sola lettura, non interagibile | Nessuna (informativo) | Comunica lo stato di preparazione dei piani | Messaggio descrittivo stabile, vedi [Generation Status](../ui/generation-status.md) |
| Uscita verso il piano 1 | Sempre visibile nel Piano 0 | Quando il piano 1 è pronto (validato o in fallback) | Attraversare l'uscita | Avvio della run: il piano 1 viene caricato | L'uscita si apre visibilmente solo quando diventa abilitata |

## Risultato

Al termine della preparazione nel Piano 0, il giocatore entra nella run con: un tema
scelto tra quelli proposti, un personaggio scelto, e un piano 1 pronto. Nessuno di questi
tre elementi può restare indefinito quando si attraversa l'uscita.

## Feedback

- Il tema scelto e il personaggio scelto restano visibili in un riepilogo mentre si
  esplora il resto del Piano 0, così il giocatore non perde la propria decisione.
- L'apertura dell'uscita verso il piano 1 è un evento visibile e distinto (non un
  semplice cambio di stato silenzioso), perché segna la fine dell'attesa.
- L'indicatore di generazione non usa percentuali finte né promesse di tempo, in linea con
  [Generation Status](../ui/generation-status.md).

## Interazioni

- [Characters](characters.md): la scelta del personaggio avviene qui, nel Piano 0.
- [Rooms and Floor Generation](rooms-and-floor-generation.md): il piano 1 che si apre
  dall'uscita segue le regole di struttura dei piani lì definite.
- [Run Manifest and Reproducibility](run-manifest-and-reproducibility.md): il tema e il
  personaggio scelti nel Piano 0 entrano nel manifest della run.
- [Save and Meta Progression](save-and-meta-progression.md): il museo delle creazioni
  migliori e il catalogo dei contenuti generati sono meta-progressione persistente.
- [Special Rooms](special-rooms.md): le arene di sfida del Piano 0 riusano contenuti e
  archetipi già presenti nel gioco, ri-tematizzati come attività opzionali locali.

## Regole per contenuti generati

- Le arene di sfida usano solo contenuti "best-of" già validati nelle run passate: non
  generano nulla di nuovo sul momento.
- I 2-3 temi proposti nella scelta del tema sono generati dall'IA per quella sessione nel
  Piano 0 (vedi [Characters](characters.md) per il meccanismo analogo applicato al
  personaggio alternativo).
- Il gioco è sempre avviabile. Finché non esistono asset dedicati generati per il Piano 0,
  una versione statica curata del Piano 0 fa da sala d'attesa/fallback della primissima
  run: vedi [Generated Content Validation](generated-content-validation.md) per la regola
  generale di fallback.

## Casi limite

- Il giocatore raggiunge l'uscita prima che il piano 1 sia pronto: l'uscita resta chiusa e
  il giocatore può continuare a usare il resto del Piano 0 (museo, arene) senza essere
  bloccato in un'attesa passiva.
- Nessuna delle 2-3 proposte di tema generate supera la validazione: si applica il
  fallback (vedi sotto); il giocatore non deve mai vedersi presentare meno di un'opzione
  di tema selezionabile.
- Il giocatore entra in un'arena di sfida e la abbandona a metà: il ritorno al resto del
  Piano 0 deve restare disponibile senza penalità sulla run in preparazione.

## Fallback

Se un tema, il personaggio alternativo o gli asset del Piano 0 non sono disponibili o non
superano la validazione, si applica la regola di fallback unica definita in
[Generated Content Validation](generated-content-validation.md).

## Non-obiettivi

- Il Piano 0 non è una stanza di combattimento obbligatoria: le arene sono opzionali.
- Il Piano 0 non applica potenziamenti permanenti al personaggio tra una run e l'altra.
- Il Piano 0 non sostituisce il menu principale (`MainMenu`): resta uno stato successivo a
  `RunSetup`, come da [Game States and Flow](../05-game-states-and-flow.md).

## Domande aperte residue

- Le arene di sfida hanno una loro economia di ricompense separata da quella della run, o
  premiano solo meta-progressione (punti, sblocchi)?
- Il riepilogo di tema/personaggio scelto è modificabile prima di attraversare l'uscita, o
  la scelta è definitiva non appena confermata?
- Quanti "best-of" minimi servono perché un'arena di sfida compaia come disponibile?

## Scenari

**Scenario: il piano 1 non è ancora pronto**
- Given il giocatore è nel Piano 0 e ha già scelto tema e personaggio
- When il piano 1 non ha ancora superato la validazione
- Then l'uscita verso il piano 1 resta chiusa e il giocatore può continuare a usare museo
  e arene senza essere bloccato

**Scenario: scelta del tema tra le proposte generate**
- Given il giocatore è entrato nel Piano 0 per la prima volta in questa sessione
- When vengono mostrate 2-3 carte tema generate dall'IA
- Then il giocatore ne seleziona una e il riepilogo del Piano 0 mostra il tema scelto

**Scenario: prima run in assoluto, senza asset dedicati generati**
- Given non esistono ancora asset dedicati generati per il Piano 0
- When il giocatore avvia il gioco per la prima volta
- Then il Piano 0 mostra la versione statica curata di fallback e resta comunque
  interamente giocabile

**Scenario: uscita verso il piano 1 abilitata**
- Given tema e personaggio sono stati scelti e il piano 1 ha superato la validazione
- When il giocatore raggiunge l'uscita
- Then l'uscita si apre e attraversarla avvia la run sul piano 1
