---
id: gd-system-grafts
title: Grafts
domain: design
status: approved
authority: canonical
owner: design
summary: "Innesto: categoria di oggetto piccolo, situazionale e sostituibile, con slot iniziale singolo ed espandibile. Doppia natura per rarità (DEC-034, DEC-107): comuni/non-comuni/rari potenti ma dentro le regole, solo la rarità leggendaria come piega-regole di una singola regola del gioco. Lo sgancio volontario lascia l'Innesto a terra, recuperabile finché si resta nella stanza (DEC-160)."
last_reviewed: 2026-07-27
last_verified_commit: 0ec60d0
topics: [Innesto, Graft, rarità, piega-regole, DEC-034, DEC-107, DEC-160]
related: []
supersedes: []
source_files: []
---

# Grafts

## Nota terminologica

"Innesto" è il termine di lavoro italiano di questa categoria di oggetto; il nome
in-game è **Graft** (DEC-072). Sostituisce un termine esterno precedentemente usato
nella prima stesura della knowledge base, non più impiegato qui.

## Intento per il giocatore

L'Innesto è un oggetto piccolo che offre un vantaggio situazionale, economico o di
supporto. Deve essere facile da valutare e da sostituire, per creare scelte rapide di
adattamento senza impegnare le decisioni più pesanti riservate ad attivi e passivi. Alla
rarità più alta, un Innesto può diventare qualcosa di più di un semplice modificatore: una
piccola alterazione a una regola del gioco stesso (vedi "Doppia natura per rarità" sotto).

## Doppia natura per rarità (DEC-034, DEC-107)

La rarità di un Innesto non cambia solo la sua potenza numerica, ma la **natura** del suo
effetto:

- **Innesti comuni:** piccoli modificatori situazionali passivi, pensati per essere
  scambiati al volo senza costo (coerente con la sostituzione immediata già descritta
  sopra).
- **Innesti non-comuni e rari:** modificatori più potenti dei comuni, ma restano **dentro
  le regole** del gioco: nessuna alterazione di regola (DEC-107).
- **Innesti leggendari:** "piega-regole" — alterano in piccola misura **una sola** regola del
  gioco (esempi concreti: le offerte disponibili nel negozio, il comportamento delle stanze
  segrete). Un piega-regole tocca sempre e solo una regola alla volta: non ne cambia più di
  una contemporaneamente. Soglia di rarità fissata da DEC-107: solo la rarità leggendaria
  (peso 3 su {55, 30, 12, 3}, DEC-019) piega le regole; piegare le regole del gioco è il
  massimo effetto possibile e resta un evento raro e memorabile.

La rarità dell'Innesto decide quindi la profondità del suo effetto, non solo la sua potenza.

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
- Un Innesto piega-regole (rarità leggendaria, DEC-034, DEC-107) generato deve dichiarare
  esplicitamente quale singola regola altera; la validazione lo respinge se il comportamento
  proposto tocca più di una regola contemporaneamente (vedi
  [Generated Content Validation](generated-content-validation.md) per il processo generale,
  non riformulato qui).

## Casi limite

- Il giocatore ha tutti gli slot Innesto occupati e ne trova uno nuovo: deve poter
  scegliere quale sostituire, o rifiutare la raccolta, senza perdite forzate non
  comunicate.
- Uno slot Innesto aggiuntivo viene concesso da un evento raro ma nessun Innesto è ancora
  posseduto: lo slot resta vuoto e visibile, in attesa.
- Un Innesto piega-regole generato tenta di alterare più di una regola contemporaneamente:
  viene respinto in validazione, secondo la regola generale di
  [Generated Content Validation](generated-content-validation.md) (DEC-034, DEC-107).

## Fallback

Se un Innesto generato non è disponibile o non è valido, si applica la regola di fallback
unica definita in
[Generated Content Validation](generated-content-validation.md).

## Non-obiettivi

- L'Innesto non sostituisce il ruolo degli oggetti attivi o passivi nella build.
- L'Innesto non è pensato per essere il fulcro di una build: resta un elemento di rifinitura.

## Stacking, slot e meta (DEC-122, DEC-123)

- Gli effetti di più Innesti equipaggiati **coesistono e si combinano**, dentro i clamp e
  i budget del motore (DEC-122); il bilanciamento fine è materia di playtest.
- Gli slot Innesto aggiuntivi ottenuti da eventi rari valgono **solo per quella run**
  (DEC-123): la meta-progressione resta sblocco di contenuti, mai di potenza
  (DEC-015/DEC-027).

## Drop e persistenza (DEC-115, DEC-116)

- Un Innesto equipaggiato può essere **sganciato volontariamente**: lo slot si libera
  (DEC-115). L'Innesto sganciato resta **a terra, nella stanza in cui è stato sganciato**,
  e può essere **recuperato** finché il giocatore resta in quella stanza (DEC-160): non è
  perso, ed equipaggiarlo di nuovo richiede semplicemente di raccoglierlo da terra come
  ogni altro Innesto. Lasciando la stanza, l'Innesto sganciato non è più recuperabile.
- Gli Innesti raccolti **persistono per tutta la run**, come ogni altro oggetto della
  build (DEC-116): nessun legame col piano in cui li trovi.

## Domande aperte residue



- ~~Quali regole sono bersagli ammissibili per un piega-regole~~: risolto da DEC-126 —
  lista chiusa curata: **economia** (offerte, prezzi, valute), **stanze** (segrete,
  tesoro, ricompense), **drop e rarità** (pesi dei pool), **risorse** (cap e ricariche);
  esclusioni dure: mai difficoltà (DEC-038), competitivo, validazione o fallback.

Nota: la domanda sulla soglia esatta di rarità (rara, leggendaria, o entrambe) a cui un
Innesto diventa piega-regole è risolta da DEC-107 — solo la rarità leggendaria — e non è più
aperta qui.

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

**Scenario: Innesto comune come modificatore situazionale**
- Given un Innesto di rarità comune, equipaggiato nello slot Innesto
- When il giocatore lo consulta o lo usa in un contesto adatto al suo effetto
- Then l'Innesto applica un piccolo modificatore passivo e situazionale, senza alterare
  alcuna regola del gioco (DEC-034)

**Scenario: Innesto leggendario come piega-regole**
- Given un Innesto di rarità leggendaria equipaggiato, dichiarato come piega-regole delle
  offerte del negozio
- When il giocatore entra in una stanza di negozio
- Then le offerte del negozio riflettono l'alterazione dichiarata da quell'unico Innesto,
  e nessun'altra regola del gioco risulta modificata (DEC-034, DEC-107)

**Scenario: Innesto raro potente ma dentro le regole**
- Given un Innesto di rarità rara, equipaggiato nello slot Innesto
- When il giocatore lo consulta o lo usa in un contesto adatto al suo effetto
- Then l'Innesto applica un effetto meccanico più forte di un Innesto comune, senza
  piegare alcuna regola del gioco (DEC-107)

**Scenario: Innesto sganciato recuperabile nella stanza**
- Given il giocatore ha un Innesto equipaggiato e lo sgancia volontariamente in una
  stanza
- When resta nella stessa stanza
- Then può raccogliere di nuovo l'Innesto da terra ed equipaggiarlo (DEC-160)

**Scenario: Innesto sganciato non più recuperabile fuori stanza**
- Given il giocatore ha sganciato volontariamente un Innesto in una stanza, lasciandolo a
  terra
- When lascia quella stanza
- Then l'Innesto sganciato non è più recuperabile (DEC-160)
