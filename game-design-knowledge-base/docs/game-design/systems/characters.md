---
id: gd-system-characters
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Personaggio base sempre disponibile e un personaggio alternativo generato per ogni run, scelto o rifiutato nel Piano 0."
---

# Characters

## Intento per il giocatore

Ogni run offre una decisione d'identità semplice: giocare con il personaggio base,
affidabile e conosciuto, oppure accettare un'alternativa generata apposta per quella run,
con un trait unico e statistiche diverse. È una scelta "prendere o lasciare", non una
creazione di personaggio.

## Condizioni di ingresso

- La scelta avviene nel Piano 0, prima di attraversare l'uscita verso il piano 1 (vedi
  [Floor Zero](floor-zero.md)).
- Il personaggio base è sempre disponibile, in ogni run, senza condizioni.
- Il personaggio alternativo esiste solo se la generazione per quella run lo ha prodotto e
  validato; in caso contrario non compare come opzione (vedi "Fallback").

## Input/azioni

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Scheda personaggio base | Sempre, nel Piano 0 | Sempre | Selezionare il personaggio base | Il personaggio base diventa il personaggio della run | Evidenziazione della scheda selezionata |
| Scheda personaggio alternativo | Quando la generazione per la run ha prodotto un'alternativa valida | Sempre, se visibile | Selezionare il personaggio alternativo | Il personaggio alternativo, con il suo trait unico, diventa il personaggio della run | Evidenziazione della scheda, trait unico messo in risalto |
| Rifiuto dell'alternativa | Quando è presente una scheda alternativa | Sempre, se visibile | Non selezionare l'alternativa (lasciare) | Il personaggio base resta quello attivo | Nessun cambiamento visibile oltre alla non-selezione |

## Risultato

Il personaggio scelto nel Piano 0 (base o alternativo) è quello con cui si gioca l'intera
run, dal piano 1 fino alla fine (piano 5 o piani extra). La scelta non cambia durante la
run.

## Feedback

- La scheda del personaggio alternativo mostra il trait unico e le statistiche in modo
  leggibile prima della scelta, non dopo.
- Le statistiche del personaggio alternativo sono presentate come valori entro una banda
  garantita, non come numeri arbitrari: il giocatore deve poter capire che non ci sono
  sorprese fuori controllo.
- Il personaggio scelto resta visibile nel riepilogo del Piano 0, insieme al tema della run
  (vedi [Floor Zero](floor-zero.md)).

## Interazioni

- [Player](player.md): responsabilità e statistiche base condivise da ogni personaggio,
  base o alternativo.
- [Floor Zero](floor-zero.md): la scelta del personaggio avviene lì, insieme alla scelta
  del tema della run.
- [Run Manifest and Reproducibility](run-manifest-and-reproducibility.md): il personaggio
  scelto entra nel manifest della run.

## Regole per contenuti generati

- Il personaggio alternativo è generato una sola volta per run: un trait unico più
  statistiche casuali entro bande garantite (i valori esatti delle bande sono da definire,
  vedi domande aperte).
- Il trait unico deve rispettare gli stessi vincoli di leggibilità e validazione di
  qualunque altro contenuto generato (vedi [Generated Content Validation](generated-content-validation.md)).
- Il personaggio alternativo dichiara la propria origine come `nuovo` o `variato`, secondo
  la tassonomia unica di origine del contenuto.

## Casi limite

- La generazione del personaggio alternativo per la run non produce un risultato valido:
  nel Piano 0 compare solo il personaggio base, senza errore visibile al giocatore.
- Il giocatore rifiuta l'alternativa: non viene proposta una seconda alternativa nella
  stessa run (vedi domande aperte per il caso di rigenerazione).

## Fallback

Se il personaggio alternativo generato per la run non è disponibile o non è valido, si
applica la regola di fallback unica definita in
[Generated Content Validation](generated-content-validation.md).

## Non-obiettivi

- Non è un sistema di creazione o personalizzazione libera del personaggio.
- Non esistono potenziamenti permanenti del personaggio tra una run e l'altra: la
  meta-progressione riguarda contenuti sbloccati nei pool, non il personaggio stesso (vedi
  [Save and Meta Progression](save-and-meta-progression.md)).
- Non è previsto un roster di personaggi multipli selezionabili liberamente: solo base e
  l'unica alternativa generata per quella run.

## Domande aperte residue

- Quali sono i valori esatti delle bande garantite per le statistiche casuali del
  personaggio alternativo?
- Il giocatore può rifiutare l'alternativa e poi tornare a valutarla più tardi nello stesso
  Piano 0, o il rifiuto è definitivo per quella run?
- Il trait unico del personaggio alternativo può ripetersi tra run diverse, o è garantita
  varietà rispetto alle run precedenti (relazione con il catalogo di
  [Save and Meta Progression](save-and-meta-progression.md))?
- Come cambia, se cambia, la scelta del personaggio nelle modalità competitive asincrone
  (vedi vincoli generali in [Multiplayer and Competition](../08-multiplayer-and-competition.md))?

## Scenari

**Scenario: scelta del personaggio base**
- Given il giocatore è nel Piano 0 con un'alternativa generata disponibile
- When il giocatore seleziona il personaggio base
- Then il personaggio base diventa il personaggio della run e l'alternativa non ha alcun
  effetto

**Scenario: scelta del personaggio alternativo**
- Given il giocatore è nel Piano 0 e viene proposta un'alternativa generata con trait
  unico e statistiche entro banda garantita
- When il giocatore seleziona l'alternativa
- Then il personaggio alternativo, con il suo trait unico, diventa il personaggio della
  run fino alla fine

**Scenario: generazione dell'alternativa non disponibile**
- Given la generazione del personaggio alternativo per questa run non ha prodotto un
  risultato valido
- When il giocatore entra nel Piano 0
- Then compare solo il personaggio base, senza alcun errore visibile

**Scenario: rifiuto dell'alternativa**
- Given è disponibile un personaggio alternativo generato per la run
- When il giocatore lo rifiuta e conferma il personaggio base
- Then la run prosegue con il personaggio base e l'alternativa generata non viene più
  riproposta in quella run
