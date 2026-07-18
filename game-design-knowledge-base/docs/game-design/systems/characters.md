---
id: gd-system-characters
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Piccola rosa di 2-3 personaggi base fissi con ruoli distinti, più un personaggio alternativo generato per ogni run che si aggiunge alla rosa nella scelta del Piano 0 (DEC-030); il trait unico del personaggio generato è un comportamento Lua validato in sandbox (DEC-037)."
---

# Characters

## Intento per il giocatore

Ogni run offre una decisione d'identità semplice: scegliere uno dei personaggi curati della
rosa base — pochi, distinti, pensati per ruoli di gioco diversi — oppure accettare
un'alternativa generata apposta per quella run, con un trait unico e statistiche diverse.
Per l'alternativa resta una scelta "prendere o lasciare"; per la rosa base è una vera scelta
tra opzioni curate.

## Condizioni di ingresso

- La scelta avviene nel Piano 0, prima di attraversare l'uscita verso il piano 1 (vedi
  [Floor Zero](floor-zero.md)).
- La rosa dei personaggi base è sempre disponibile, in ogni run, senza condizioni (fatti
  salvi gli sblocchi previsti da DEC-030, vedi sotto).
- Il personaggio alternativo generato per la run si **aggiunge** alla rosa base nella scelta
  del Piano 0: esiste solo se la generazione per quella run lo ha prodotto e validato; in
  caso contrario non compare come opzione (vedi "Fallback").

## Rosa di personaggi base (DEC-030)

I personaggi base non sono un singolo personaggio ma una **piccola rosa fissa e curata di
2-3 personaggi**, ciascuno con un ruolo distinto (indicativamente: un ruolo offensivo, uno
difensivo, uno da esploratore). Nomi e dettagli esatti dei personaggi della rosa restano da
definire (vedi Domande aperte residue). I personaggi della rosa sono **sbloccabili presto**:
non tutti devono essere disponibili fin dal primo avvio (dettagli dello sblocco non
definiti, vedi Domande aperte residue).

Il ruolo di ciascun personaggio della rosa base si riflette anche nel proprio **tetto di
salute base** (DEC-033): un ruolo difensivo può avere un tetto alto ("personaggio-roccia"),
un ruolo più aggressivo o mobile un tetto più basso ("personaggio-vetro"), come parte
curata delle sue statistiche — non un valore unico condiviso da tutta la rosa. Il dettaglio
del meccanismo del tetto vive in
[Health and Resources](health-and-resources.md) (rimando, non riformulato qui).

Il personaggio alternativo generato per run (vedi sotto) non sostituisce la rosa base: la
scelta nel Piano 0 avviene tra i personaggi della rosa (quelli già sbloccati) più
l'eventuale alternativa generata per quella run.

## Input/azioni

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Schede personaggi della rosa base | Sempre, nel Piano 0 | Per ciascuna scheda: quando quel personaggio della rosa è già sbloccato | Selezionare uno dei personaggi della rosa | Il personaggio scelto, col suo ruolo distinto, diventa il personaggio della run | Evidenziazione della scheda selezionata, ruolo messo in risalto |
| Scheda personaggio alternativo | Quando la generazione per la run ha prodotto un'alternativa valida | Sempre, se visibile | Selezionare il personaggio alternativo | Il personaggio alternativo, con il suo trait unico, diventa il personaggio della run | Evidenziazione della scheda, trait unico messo in risalto |
| Rifiuto dell'alternativa | Quando è presente una scheda alternativa | Sempre, se visibile | Non selezionare l'alternativa (lasciare) | Un personaggio della rosa base resta quello attivo | Nessun cambiamento visibile oltre alla non-selezione |

## Risultato

Il personaggio scelto nel Piano 0 (uno della rosa base o l'alternativo generato) è quello
con cui si gioca l'intera run, dal piano 1 fino alla fine. La scelta non cambia durante la
run.

## Feedback

- La scheda del personaggio alternativo mostra il trait unico e le statistiche in modo
  leggibile prima della scelta, non dopo.
- Le statistiche del personaggio alternativo sono presentate come valori entro una banda
  garantita, non come numeri arbitrari: il giocatore deve poter capire che non ci sono
  sorprese fuori controllo.
- Ogni scheda della rosa base comunica chiaramente il proprio ruolo (offensivo, difensivo,
  esploratore o equivalente), così la scelta tra i personaggi curati resta leggibile quanto
  quella verso l'alternativa generata.
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
  vedi domande aperte). Tra queste statistiche c'è anche il proprio tetto di salute base
  (DEC-033), generato entro bande min/max di default da playtest, come i valori di DEC-019:
  il personaggio generato può quindi risultare più "vetro" o più "roccia" di un personaggio
  della rosa base, ma sempre dentro limiti garantiti, mai arbitrario.
- **Trait unico come comportamento Lua (DEC-037):** il trait unico del personaggio
  alternativo generato è un comportamento scritto dall'IA e validato in sandbox, con la
  stessa pipeline usata per i comportamenti degli oggetti (vedi
  [AI Content Generation Model](../06-ai-content-generation-model.md) e
  [Generated Content Validation](generated-content-validation.md), rimando, non riformulare
  qui). Deve inoltre rispettare gli stessi vincoli di leggibilità di qualunque altro
  contenuto generato.
- Il personaggio alternativo dichiara la propria origine come `nuovo` o `variato`, secondo
  la tassonomia unica di origine del contenuto.
- I personaggi della rosa base (DEC-030) sono `curato`: non sono generati, restano fissi tra
  una run e l'altra fino a un eventuale aggiornamento curato del gioco.

## Casi limite

- La generazione del personaggio alternativo per la run non produce un risultato valido:
  nel Piano 0 compare solo la rosa dei personaggi base già sbloccati, senza errore visibile
  al giocatore.
- Il giocatore rifiuta l'alternativa: non viene proposta una seconda alternativa nella
  stessa run (vedi domande aperte per il caso di rigenerazione).
- Il trait Lua generato per il personaggio alternativo non supera la validazione in sandbox:
  si applica il fallback, vedi sotto.
- Un personaggio della rosa base non è ancora sbloccato: la sua scheda non è selezionabile
  nel Piano 0 (dettagli dello sblocco da definire, vedi domande aperte).

## Fallback

Se il personaggio alternativo generato per la run non è disponibile o non è valido, si
applica la regola di fallback unica definita in
[Generated Content Validation](generated-content-validation.md).

## Non-obiettivi

- Non è un sistema di creazione o personalizzazione libera del personaggio.
- Non esistono potenziamenti permanenti del personaggio tra una run e l'altra: la
  meta-progressione riguarda contenuti sbloccati nei pool, non il personaggio stesso (vedi
  [Save and Meta Progression](save-and-meta-progression.md)).
- Non è un roster ampio o liberamente configurabile: resta una piccola rosa fissa e curata
  di 2-3 personaggi (DEC-030) più l'unica alternativa generata per quella run.

## Domande aperte residue

- Quali sono i valori esatti delle bande garantite per le statistiche casuali del
  personaggio alternativo, incluse le bande min/max del suo tetto di salute base (DEC-033)?
- Il giocatore può rifiutare l'alternativa e poi tornare a valutarla più tardi nello stesso
  Piano 0, o il rifiuto è definitivo per quella run?
- Il trait unico del personaggio alternativo può ripetersi tra run diverse, o è garantita
  varietà rispetto alle run precedenti (relazione con il catalogo di
  [Save and Meta Progression](save-and-meta-progression.md))?
- Come cambia, se cambia, la scelta del personaggio nelle modalità competitive asincrone
  (vedi vincoli generali in [Multiplayer and Competition](../08-multiplayer-and-competition.md))?
- Composizione esatta della rosa dei personaggi base (DEC-030): nomi, ruoli precisi oltre
  alle indicazioni offensivo/difensivo/esploratore, e condizioni esatte di sblocco di
  ciascuno (vedi `../governance/open-questions.md`).

## Scenari

**Scenario: scelta di un personaggio della rosa base**
- Given il giocatore è nel Piano 0, con la rosa dei personaggi base sbloccati visibile e
  un'alternativa generata disponibile
- When il giocatore seleziona uno dei personaggi della rosa base (es. il ruolo offensivo)
- Then quel personaggio, col suo ruolo distinto, diventa il personaggio della run e
  l'alternativa non ha alcun effetto

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
- Then compare solo la rosa dei personaggi base già sbloccati, senza alcun errore visibile

**Scenario: rifiuto dell'alternativa**
- Given è disponibile un personaggio alternativo generato per la run
- When il giocatore lo rifiuta e conferma un personaggio della rosa base
- Then la run prosegue con quel personaggio della rosa base e l'alternativa generata non
  viene più riproposta in quella run

**Scenario: trait Lua generato per l'alternativa non supera la validazione**
- Given il trait unico del personaggio alternativo è stato scritto dall'IA come
  comportamento Lua
- When il comportamento non supera la validazione in sandbox
- Then si applica il fallback definito in
  [Generated Content Validation](generated-content-validation.md) e nel Piano 0 compare solo
  la rosa dei personaggi base già sbloccati
