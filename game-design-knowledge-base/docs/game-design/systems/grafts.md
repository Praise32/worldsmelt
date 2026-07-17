---
id: gd-system-grafts
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Innesto: categoria di oggetto piccolo, situazionale e sostituibile, con slot iniziale singolo ed espandibile."
---

# Grafts

## Nota terminologica

"Innesto" è il nome placeholder di questa categoria di oggetto. Sostituisce un termine
esterno precedentemente usato nella prima stesura della knowledge base, non più impiegato
qui. Restano in discussione, come rosa di nomi alternativi al placeholder, anche
**Scaglia**, **Residuo** e **Sigillo** (vedi "Domande aperte residue").

## Intento per il giocatore

L'Innesto è un oggetto piccolo che offre un vantaggio situazionale, economico o di
supporto. Deve essere facile da valutare e da sostituire, per creare scelte rapide di
adattamento senza impegnare le decisioni più pesanti riservate ad attivi e passivi.

## Condizioni di ingresso

- Il giocatore parte ogni run con 1 slot Innesto disponibile (oltre a 1 slot attivo, vedi
  [Items, Pools and Rarity](items-pools-and-rarity.md) per la tassonomia completa degli
  oggetti).
- Ulteriori slot Innesto si ottengono solo tramite oggetti o eventi rari durante la run.
- Un Innesto si ottiene dalle stesse fonti generali di ricompensa oggetto (stanze,
  ricompense, eventi), filtrate per la categoria Innesto.

## Input/azioni

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Slot Innesto vuoto | Il giocatore ha almeno 1 slot Innesto | Il giocatore ha un Innesto disponibile da equipaggiare | Equipaggiare l'Innesto | L'effetto dell'Innesto diventa attivo | Icona dell'Innesto compare nello slot |
| Slot Innesto occupato | Il giocatore ha equipaggiato un Innesto | Il giocatore possiede un nuovo Innesto da assegnare allo stesso slot | Sostituire l'Innesto equipaggiato | Il vecchio Innesto viene rimosso, il nuovo diventa attivo | Transizione visibile tra le due icone |
| Tooltip Innesto | Il puntatore o il focus è sullo slot | Sempre | Consultare l'effetto | Nessun cambiamento di stato | Testo dell'effetto e origine del contenuto |

## Risultato

L'Innesto equipaggiato applica il proprio effetto situazionale finché resta nello slot.
La sostituzione è immediata: non esiste un periodo di transizione in cui nessuno dei due
Innesti è attivo.

## Feedback

- Lo slot Innesto è visivamente distinto dagli slot di attivi e passivi, per segnalare che
  è pensato per essere sostituito spesso.
- L'interfaccia mostra sempre quanti slot Innesto sono disponibili e quanti occupati.
- Un nuovo slot Innesto ottenuto durante la run genera un feedback dedicato, distinto
  dall'ottenimento di un Innesto stesso.

## Interazioni

- [Items, Pools and Rarity](items-pools-and-rarity.md): fonte unica della tassonomia
  completa degli oggetti e dei campi obbligatori di ogni oggetto, Innesto incluso.
- [Active Items](active-items.md) e [Passive Items](passive-items.md): categorie di
  oggetto con un peso maggiore in build, a cui l'Innesto si affianca senza sostituirle.
- [Special Rooms](special-rooms.md) e [Rewards and Economy](rewards-and-economy.md): fonti
  possibili di Innesti e di slot aggiuntivi.

## Regole per contenuti generati

- Ogni Innesto rispetta gli stessi campi obbligatori definiti in
  [Items, Pools and Rarity](items-pools-and-rarity.md) (rarità, peso nel pool, budget di
  potenza, tag, prerequisiti, incompatibilità, valore della sinergia, costo o rischio
  quando presente), senza campi divergenti specifici della categoria.
- L'origine di un Innesto generato si dichiara con uno dei quattro valori canonici:
  `curato | composto | variato | nuovo`.

## Casi limite

- Il giocatore ha tutti gli slot Innesto occupati e ne trova uno nuovo: deve poter
  scegliere quale sostituire, o rifiutare la raccolta, senza perdite forzate non
  comunicate.
- Uno slot Innesto aggiuntivo viene concesso da un evento raro ma nessun Innesto è ancora
  posseduto: lo slot resta vuoto e visibile, in attesa.

## Fallback

Se un Innesto generato non è disponibile o non è valido, si applica la regola di fallback
unica definita in
[Generated Content Validation](generated-content-validation.md).

## Non-obiettivi

- L'Innesto non sostituisce il ruolo degli oggetti attivi o passivi nella build.
- L'Innesto non è pensato per essere il fulcro di una build: resta un elemento di rifinitura.
- Questo documento non fissa il nome definitivo della categoria: resta un placeholder.

## Domande aperte residue

- Quale tra Scaglia, Residuo e Sigillo (o il placeholder Innesto stesso) diventerà il nome
  definitivo della categoria?
- Il drop volontario di un Innesto equipaggiato è permesso?
- Gli Innesti possono avere stacking tra loro?
- Gli Innesti persistono tra un piano e l'altro della stessa run, o sono legati a
  condizioni locali?
- Gli slot Innesto aggiuntivi ottenuti durante la run persistono solo per quella run, o
  contribuiscono in qualche forma alla meta-progressione descritta in
  [Save and Meta Progression](save-and-meta-progression.md)?

## Scenari

**Scenario: equipaggiare il primo Innesto**
- Given il giocatore ha 1 slot Innesto vuoto e nessun Innesto equipaggiato
- When il giocatore raccoglie un Innesto
- Then l'Innesto viene equipaggiato nello slot e il suo effetto diventa attivo

**Scenario: sostituzione di un Innesto**
- Given il giocatore ha già un Innesto equipaggiato nell'unico slot disponibile
- When il giocatore raccoglie un nuovo Innesto e sceglie di sostituire
- Then il vecchio Innesto viene rimosso e il nuovo diventa attivo immediatamente

**Scenario: nuovo slot da evento raro**
- Given il giocatore ha già occupato l'unico slot Innesto iniziale
- When un evento raro concede uno slot Innesto aggiuntivo
- Then compare un secondo slot vuoto, disponibile per un nuovo Innesto senza rimuovere
  quello già equipaggiato

**Scenario: Innesto generato non valido**
- Given una ricompensa dovrebbe offrire un Innesto generato dall'IA
- When quell'Innesto non supera la validazione
- Then si applica il fallback definito in Generated Content Validation e il giocatore
  riceve comunque un Innesto valido
