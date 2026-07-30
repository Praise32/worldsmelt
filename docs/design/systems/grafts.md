---
id: gd-system-grafts
title: Grafts
domain: design
status: approved
authority: canonical
owner: design
summary: "Innesto: categoria di oggetto piccolo, situazionale e sostituibile, con slot iniziale singolo ed espandibile. Doppia natura per rarità (DEC-034, DEC-107): comuni/non-comuni/rari potenti ma dentro le regole, solo la rarità leggendaria come piega-regole di una singola regola del gioco. Lo sgancio volontario lascia l'Innesto a terra, nella stanza in cui è stato sganciato, recuperabile per TUTTA LA RUN (DEC-183, supera parzialmente DEC-160: cade la clausola \"perso uscendo dalla stanza\")."
last_reviewed: 2026-07-30
last_verified_commit: 8210480
topics: [Innesto, Graft, rarità, piega-regole, DEC-034, DEC-107, DEC-160, DEC-183]
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

## Drop e persistenza (DEC-115, DEC-116, DEC-183)

- Un Innesto equipaggiato può essere **sganciato volontariamente**: lo slot si libera
  (DEC-115). L'Innesto sganciato resta **a terra, nella stanza in cui è stato sganciato**,
  e può essere **recuperato tornandoci in qualsiasi momento della run** (DEC-183): non è
  mai perso, ed equipaggiarlo di nuovo richiede semplicemente di raccoglierlo da terra come
  ogni altro Innesto. **Nota di supersessione parziale (30/07):** DEC-183 supera la
  clausola di DEC-160 «uscendo dalla stanza si perde» — l'Innesto sganciato non scompare
  più all'uscita, resta a terra nella sua stanza per **tutta la run**. Restano invariate le
  altre clausole di DEC-160: a terra, recuperabile, mai distrutto dallo sgancio.
- Gli Innesti raccolti **persistono per tutta la run**, come ogni altro oggetto della
  build (DEC-116): nessun legame col piano in cui li trovi.

## Stato di implementazione (2026-07-27, persistenza DEC-183 il 30/07)

Il motore implementa la categoria, lo slot e il ciclo raccolta/sostituzione/sgancio;
`tools/melting-gen` ora produce anche Innesti (aggiornamento dello stesso 2026-07-27,
dopo la prima stesura di questa sezione) — i piega-regole restano assenti (vedi sotto).
Il 30/07 (fix del primo playtest) il motore ha chiuso il gap di persistenza registrato da
DEC-183: vedi il punto sullo sgancio sotto e `src/tests/script_items_tests.c` (test AX).

**Implementato**

- `ITEM_GRAFT` è una delle 4 categorie di `ItemKind` (`src/core/game_types.h`), con slot
  Innesto singolo ed espandibile derivato dall'inventario (`src/gameplay/item_slots.{h,c}`).
- Sostituzione a slot pieno con la stessa meccanica del piedistallo degli attivi
  (DEC-117): il vecchio Innesto resta a terra dove hai preso il nuovo, e riprenderlo
  riscambia — il "rifiutare la raccolta" dei casi limite si ottiene semplicemente non
  toccando l'oggetto.
- Sgancio volontario (DEC-115) con l'Innesto lasciato **a terra nella stanza**,
  recuperabile **per tutta la run** (DEC-183, implementato 30/07, corretto un difetto
  bloccante il 30/07 sera — vedi sotto): la persistenza vive in una lista GLOBALE al
  piano, `Game.droppedGrafts` (`src/core/game_types.h`, cap `MAX_DROPPED_GRAFTS=32`),
  non in un campo singolo per stanza — **in una stessa stanza possono coesistere PIÙ
  Innesti a terra insieme** (uno sganciato più uno lasciato da uno scambio successivo, o
  due sganci consecutivi prima di riprenderne nessuno), e un campo singolo li
  sovrascriverebbe silenziosamente uno con l'altro (il difetto bloccante della prima
  stesura di questo fix, corretto prima del commit). Ogni record porta un riferimento
  alla cella di stato a cui appartiene; sopravvive all'azzeramento dei pickup ad ogni
  ingresso in stanza (`EntitiesClear`): `WorldSpawnRoomContents` (`src/world/world.c`)
  ri-materializza OGNI record della stanza corrente a ogni visita (bloccando il pickup se
  nasce sotto/vicino ai piedi del giocatore, stessa guardia dello sgancio) finché non
  viene ripreso davvero (`CombatPickup`, guardato dal marcatore
  `Pickup.isPersistedGraft`/`droppedGraftSlot` per non confondere l'Innesto sganciato con
  un Innesto qualunque offerto da tesoro/negozio, e per sapere QUALE record aggiornare
  quando ce n'è più di uno nella stessa stanza). Uno scambio a slot pieni sullo stesso
  Innesto persistente aggiorna il SUO record invece di azzerarlo: resta a terra, ora con
  l'oggetto appena tolto dallo slot. Riprendere il proprio Innesto persistente (con o
  senza scambio) non tocca mai `rewardTaken`/la valuta di completamento della stanza:
  quel ramo resta riservato a un oggetto offerto DAVVERO da tesoro/negozio, altrimenti
  sganciare in una stanza tesoro non ancora aperta e poi rientrare per riprenderselo
  incasserebbe la valuta di un tesoro mai aperto (DEC-167).
- **Asimmetria dichiarata:** un Innesto lasciato a terra da uno SCAMBIO su un piedistallo
  di tesoro/negozio (il "vecchio Innesto resta a terra dove hai preso il nuovo" descritto
  sopra, quando il piedistallo offre un Innesto qualunque e non un Innesto persistente
  già a terra) **continua a svanire uscendo dalla stanza**, come da DEC-160/167: quel
  pickup non porta il marcatore `isPersistedGraft` e nessuno stato di `Game.droppedGrafts`
  lo tiene in vita. Solo un Innesto sganciato *volontariamente* (o un Innesto persistente
  già a terra che viene toccato di nuovo su un piedistallo, aggiornando il SUO stesso
  record) resta per tutta la run — a schermo i due pickup sono identici, quindi
  l'asimmetria non è visibile al giocatore finché non esce e rientra. Lettera di DEC-183
  rispettata (parla esplicitamente di "Innesto sganciato volontariamente"), ma resta una
  scelta implicita non ancora aperta come domanda al proprietario.
- Stacking dentro i clamp del motore (DEC-122): gli Innesti passano dal ricalcolo da zero
  di `ScriptItemsRecomputeStats` come ogni altro oggetto, senza contabilità nuova. Il
  budget per-oggetto scalato per rarità — finora riservato agli stat-up — si applica
  anche agli Innesti: è il "budget del motore" a cui DEC-122 rimanda.
- I trait di un Innesto si **spengono** allo sgancio: il motore ricalcola da zero anche
  la maschera di trait del giocatore, che prima era un OR monotono mai reversibile.
- HUD: riga permanente distinta da quella dell'attivo, con nome, slot occupati/disponibili
  e tasto di sgancio.

**Non ancora implementato**

- **Piega-regole (DEC-034/DEC-107/DEC-126): assenti.** Nessun campo dichiara quale regola
  un leggendario altera, nessun gancio nei quattro bersagli ammessi, nessuna validazione
  che respinga chi ne tocca più di una.
- Innesti "sensore" per le super-segrete (DEC-127): **assenti**. Aggiornamento WP8
  (30/07): le stanze super-segrete ORA esistono nel motore (`ROOM_SECRET` con
  `RoomState.secretSuper`, vedi [special-rooms.md](./special-rooms.md) e
  [secrets-and-obstacles.md](./secrets-and-obstacles.md)), quindi la dipendenza che
  bloccava questa voce è caduta — ma nessun Innesto sensore è stato aggiunto insieme a
  loro, per scelta dichiarata: il contenuto curato di ripiego non contiene oggi **nessun**
  Innesto, e introdurne la prima categoria dentro il lavoro di una stanza avrebbe toccato
  garanzie (ledger e test del contenuto curato, DEC-171) che con i segreti non c'entrano.
  La super-segreta resta comunque trovabile per intuizione estrema, l'altra via che
  DEC-025 ammette. Il punto di innesto lato mondo è già pronto e pubblico:
  `WorldRoomHiddenOnMap` / `WorldSecretClueVisible` (`src/world/world.h`) sono i due soli
  predicati che decidono cosa si vede di una segreta. Vedi
  `docs/engineering/known-issues.md`, voce 14.
- Nessuna fonte di slot Innesto aggiuntivi esiste in gioco.

### Default proposti dall'implementazione

Stile DEC-019: scelte del codice dove il documento non decide, da confermare.

| Scelta | Default adottato |
|---|---|
| Input di sgancio | tasto **G** |
| Quale Innesto si sgancia con più slot occupati | l'**ultimo equipaggiato** (nessuna UI di selezione degli slot) |
| Quale Innesto viene sostituito a slot pieni | l'**ultimo equipaggiato**, stessa regola |
| Budget dell'effetto | il tetto per-oggetto scalato per rarità già usato dagli stat-up |
| L'Innesto persistente (DEC-183) segue il giocatore al piano successivo? | **No**: resta sul piano dov'è stato lasciato. Lo stato persistente vive nella stessa griglia di stanze del piano corrente, azzerata a ogni nuovo piano come il resto del layout — coerente col fatto che i piani di questa demo si attraversano in un solo verso (nessun modo di tornare a rivisitare la stanza) |

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
- Then può raccogliere di nuovo l'Innesto da terra ed equipaggiarlo (DEC-160, DEC-183)

**Scenario: Innesto sganciato recuperabile anche tornando nella stanza più tardi nella run**
- Given il giocatore ha sganciato volontariamente un Innesto in una stanza, lasciandolo a
  terra, ed è uscito da quella stanza
- When torna in quella stessa stanza in un momento qualsiasi successivo della run
- Then trova ancora l'Innesto a terra e può raccoglierlo di nuovo ed equipaggiarlo: non è
  mai stato perso (DEC-183, supera la clausola "perso uscendo" di DEC-160)

**Scenario: l'effetto di un Innesto si spegne allo sgancio**
- Given un Innesto equipaggiato che modifica una statistica o il comportamento dei colpi
- When il giocatore lo sgancia
- Then l'effetto sparisce immediatamente e completamente, e riprendendo l'Innesto da terra
  torna identico a prima: nessun residuo, nessun accumulo (DEC-122 dentro i clamp del
  motore)
