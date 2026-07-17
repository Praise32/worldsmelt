---
id: gd-run-structure
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Struttura di una run: Piano 0 più cinque piani generati, e prosecuzione post-boss."
---

# Run Structure

## Intento per il giocatore

Il giocatore deve poter avviare il gioco senza attese morte, capire in ogni momento in
quale fase della run si trova, e scegliere consapevolmente se fermarsi al boss del piano 5
o rischiare piani extra sempre più degenerati.

## Struttura canonica (DEC-001)

Una run è composta da **Piano 0** più **cinque piani** generati.

### Piano 0 — hub sempre giocabile (DEC-002, DEC-004)

- Il gioco è sempre avviabile: il Piano 0 fa da spazio di attesa giocabile mentre l'IA
  genera il piano 1. Finché non esistono asset dedicati, una versione statica curata del
  Piano 0 fa da sala d'attesa.
- Contiene: il museo delle creazioni migliori (contenuti "best-of" già validati di run
  passate), la scelta del tema della run tra 2–3 proposte dell'IA (DEC-005), la scelta del
  personaggio (base o alternativo generato, DEC-014), e l'indicatore di generazione (vedi
  [Generation Status](ui/generation-status.md)).
- Il Piano 0 è anche hub: oltre al rifugio sicuro, offre arene di sfida opzionali che
  riusano contenuti già validati del museo.
- **Condizione di ingresso:** avvio di una nuova run o ritorno da una run conclusa.
- **Condizione di uscita verso il piano 1:** il piano 1 è pronto (generato e validato).
- Vedi [Floor Zero](systems/floor-zero.md) per il contratto completo del sistema.

### Piano 1

- Introduce il tema scelto dal giocatore nella sua forma iniziale.
- Usa una base di contenuti curati sufficiente a garantire un'esperienza leggibile fin da
  subito, anche se parti del piano sono generate.

### Piani 2–4 — evoluzione del tema

- Il tema scelto nel Piano 0 evolve o degenera piano dopo piano (DEC-005): varietà,
  difficoltà e interazioni aumentano gradualmente, mantenendo riconoscibile la relazione
  con il tema di partenza.
- Contenuti generati o composti aumentano di proporzione rispetto ai contenuti curati.

### Piano 5 — culmine e chiusura della run ufficiale

- Contiene il boss del piano 5, la forma più evoluta o degenerata del tema della run.
- Sconfiggere questo boss **chiude la run ufficiale**: il risultato è valido per le
  classifiche (DEC-006).

### Prosegui oltre (DEC-006)

- Dopo il boss del piano 5, il giocatore sceglie esplicitamente se fermarsi (risultato
  registrato come run ufficiale conclusa) o proseguire in **piani extra**.
- I piani extra continuano a degenerare il tema oltre il punto raggiunto al piano 5; non
  sono validi per le classifiche della run ufficiale, ma i loro contenuti generati entrano
  comunque nel catalogo persistente (vedi [Core Loop](03-core-loop.md)).
- La run termina quando la salute del giocatore raggiunge zero: permadeath, nessuna
  eccezione.

## Input e azioni

- Nel Piano 0: navigazione libera, selezione del tema, selezione del personaggio, ingresso
  nelle arene di sfida opzionali, uscita verso il piano 1 quando disponibile.
- Nei piani 1–5 ed extra: gli stessi input di gioco definiti in [Player](systems/player.md)
  e [Combat and Projectiles](systems/combat-and-projectiles.md); in più, la decisione di
  proseguire oltre al termine del piano 5.

## Risultato

- Run ufficiale conclusa con vittoria (boss del piano 5 sconfitto) o con sconfitta
  (permadeath in un punto qualsiasi della run, incluse le arene di sfida del Piano 0 se
  applicabile).
- Eventuale prosecuzione in piani extra fino alla sconfitta.

## Feedback

- L'indicatore di generazione nel Piano 0 comunica lo stato di preparazione senza mostrare
  dettagli tecnici (regola unica in
  [AI Content Generation Model](06-ai-content-generation-model.md)).
- Al boss del piano 5, il gioco segnala chiaramente che la run ufficiale sta per chiudersi e
  presenta la scelta di proseguire come decisione esplicita, non come continuazione
  automatica.

## Interazioni

- Con [Item Fusion](systems/item-fusion.md): la stanza di fusione è disponibile in ogni
  piano generato secondo le regole di [Special Rooms](systems/special-rooms.md).
- Con [Difficulty and Progression](07-difficulty-and-progression.md): la crescita di
  difficoltà tra piani segue le curve lì definite.
- Con [Save and Meta Progression](systems/save-and-meta-progression.md): l'esito della run
  (ufficiale o estesa) alimenta il catalogo e i punti sblocco.

## Regole per contenuti generati

- Ogni piano generato deve rispettare i budget di novità: non tutto deve essere nuovo
  contemporaneamente (nuove famiglie di nemici, nuove regole ambientali, nuove categorie di
  ricompensa, nuovi effetti visivi dominanti). Questi budget sono `draft`, da tarare col
  playtest (DEC-019).
- L'evoluzione/degenerazione del tema è un vincolo di generazione: i contenuti dei piani
  2–5 ed extra devono restare riconoscibilmente derivati dal tema scelto nel Piano 0, non
  arbitrari.
- La pre-generazione di una run futura, quando risorse e tempo lo consentono, è opzionale e
  non deve ridurre la stabilità o la leggibilità della run corrente.

## Casi limite

- Il piano 1 non è ancora pronto quando il giocatore prova a uscire dal Piano 0: l'uscita
  resta chiusa e il giocatore riceve feedback che la generazione è in corso, senza blocco
  dell'interazione con il resto del Piano 0.
- Il giocatore muore all'interno di un'arena di sfida opzionale nel Piano 0: da definire se
  equivale a una sconfitta di run o a un evento locale reversibile (vedi Domande aperte).
- Un piano generato (2–5 o extra) non supera la validazione in tempo utile: si applica il
  fallback, vedi sotto.

## Fallback

Per la regola di fallback quando un piano generato non è pronto o non supera la
validazione, fonte unica è
[Generated Content Validation](systems/generated-content-validation.md). Questo documento
non la ripete.

## Non-obiettivi

- La struttura Piano 0 + 5 piani non implica un numero fisso di stanze per piano: il numero
  di stanze è variabile e definito in
  [Rooms and Floor Generation](systems/rooms-and-floor-generation.md).
- Il Piano 0 non è un tutorial obbligatorio con contenuti bloccati: resta un hub navigabile
  liberamente.

## Domande aperte residue

- Cosa succede esattamente alla salute/allo stato del giocatore se muore in un'arena di
  sfida opzionale del Piano 0.
- Numero massimo di piani extra oltre il piano 5, se un limite pratico è necessario.

## Scenari

- **Dato** che il giocatore avvia una nuova run e il piano 1 non è ancora pronto, **quando**
  entra nel Piano 0, **allora** può muoversi liberamente, consultare il museo e scegliere
  tema e personaggio, e l'uscita verso il piano 1 resta chiusa finché la generazione non è
  completata.
- **Dato** che il giocatore sconfigge il boss del piano 5, **quando** il gioco chiude la run
  ufficiale, **allora** il risultato viene registrato come valido per classifiche e al
  giocatore viene chiesto esplicitamente se vuole proseguire in piani extra.
- **Dato** che il giocatore sceglie di proseguire oltre il piano 5, **quando** entra nel
  primo piano extra, **allora** il tema della run continua a degenerare rispetto alla forma
  raggiunta al piano 5, e la run resta non valida per le classifiche ufficiali.
- **Dato** che un piano generato non supera la validazione entro il tempo previsto,
  **quando** il giocatore tenta di accedervi, **allora** il gioco applica il fallback
  descritto in [Generated Content Validation](systems/generated-content-validation.md)
  senza mostrare dettagli tecnici e senza bloccare la run.
