---
id: gd-system-items-pools
title: Items, Pools and Rarity
domain: design
status: approved
authority: canonical
owner: design
summary: "Fonte unica dei campi obbligatori di un oggetto, tassonomia a 4 categorie con oggetti ibridi comportamento+statistiche (DEC-035), pool, rarità (pesi in stato draft, con garanzia di almeno un oggetto per rarità nel pool minimo, DEC-144), densità di 3-5 oggetti per piano (DEC-032) e correzione di fortuna con soglia N esplicita, ridotta dalla Fortuna, su tutti i pool (DEC-145)."
last_reviewed: 2026-07-28
last_verified_commit: cfeaa47
topics: [oggetti, pool, rarità, slot, correzione-di-fortuna, tassonomia, DEC-144, DEC-145]
related: []
supersedes: []
source_files: []
---

# Items, Pools and Rarity

Questo documento è la fonte **unica** dei campi obbligatori di un oggetto. Ogni altro
documento (inclusi [Active Items](active-items.md), [Passive Items](passive-items.md),
[Grafts](grafts.md) e `templates/item-spec-template.md`) deve rimandare qui invece di
ridefinire l'elenco.

## Intento per il giocatore

Ogni ricompensa deve sembrare una decisione, non un tiro di dado isolato: la rarità
comunica frequenza attesa, non necessariamente potenza; il peso e i prerequisiti fanno
sì che i pool offerti restino pertinenti al momento della run; la correzione di fortuna
evita sequenze percepite come ingiuste senza eliminare la varianza.

## Categorie (tassonomia completa)

Un oggetto appartiene a esattamente una delle 4 categorie:

- **attivo** — si attiva volontariamente (vedi [Active Items](active-items.md));
- **passivo** — effetto sempre presente una volta ottenuto (vedi [Passive Items](passive-items.md));
- **stat-up** — modifica diretta e minima di una statistica, senza comportamento nuovo;
- **Innesto** — oggetto piccolo, situazionale, sostituibile (vedi [Grafts](grafts.md) per
  i dettagli di questa categoria; non riformulati qui).

Il termine "trinket" è un riferimento esterno e non va usato: la categoria equivalente
in questo progetto si chiama **Innesto**.

### Oggetti ibridi comportamento+statistiche (DEC-035)

Un oggetto attivo o passivo può includere, oltre al proprio comportamento, modifiche dirette
a una o più statistiche — positive o negative, coerenti con l'effetto dichiarato. Questo non
introduce una quinta categoria: l'oggetto resta classificato come `attivo` o `passivo` nel
campo `categoria`; le modifiche di statistica si sommano al comportamento e vanno dichiarate
insieme ad esso. Questa regola non rompe la tassonomia a 4 categorie di DEC-011: la aggira
componendo, non aggiungendo categorie.

Gli stat-up, inoltre, compaiono anche nei pool normali (tesoro, negozio), non solo come
ricompensa boss: non sono riservati a un contesto specifico.

### Slot

- Si parte con **1 slot attivo** + **1 slot Innesto**.
- Oggetti o eventi rari durante la run possono aggiungere slot aggiuntivi (attivo e/o
  Innesto): gli slot sono quindi espandibili, non fissi per l'intera run.
- I **passivi** e gli **stat-up** non hanno limite di slot: si accumulano tutti quelli
  raccolti.

### Stato di implementazione della tassonomia (2026-07-27)

Le 4 categorie esistono nel motore (`ItemKind`, `src/core/game_types.h`) con semantiche
distinte: fino a questa data ne esistevano **due**, e la prima si chiamava "attivo" pur
significando **passivo** (effetto sempre presente). La rinomina è stata fatta senza
rompere il contenuto già su disco.

**Compatibilità del contenuto già generato o curato.** Ogni manifest e ogni record di
catalogo scritto finora usa `kind=active` per quelli che questo documento chiama
**passivi**. Il lettore del gioco non traduce quella parola alla lettera: si appoggia al
contratto di [Active Items](active-items.md) — "ogni attivo dichiara uno tra cariche e
cooldown" — e considera attivo solo un record che, oltre a dire `active`, dichiara anche
una delle due. Nessun contenuto scritto prima di oggi lo fa, quindi:

| Testo nel manifest | Dichiara cariche o cooldown | Categoria letta |
|---|---|---|
| `active` | no | **passivo** (tutto il contenuto storico) |
| `active` | sì | **attivo** |
| `passive` | — | passivo |
| `statup` | — | stat-up |
| `graft` | — | Innesto |
| assente, vuoto, sconosciuto | — | passivo |

Il ripiego lato gioco (contenuto di riserva quando nessun manifest esiste) produce solo
passivi e stat-up: attivi e Innesti hanno slot e contratti che un contenuto procedurale di
riserva non può dichiarare in modo sensato, e arriveranno dal generatore.

**Aggiornamento 2026-07-27 (stesso giorno, dopo la prima stesura della sezione sopra):**
`tools/melting-gen` genera ora un **mix delle 4 categorie** sulle 15 posizioni normali
(tesoro/negozio) dell'intera run, con garanzia di copertura in stile DEC-144 (ciascuna
delle 4 compare almeno una volta): il ripiego lato gioco resta solo passivi/stat-up (vedi
sopra, non riformulato), ma una run generata da melting-gen offre ora anche attivi e
Innesti. `pool` continua a non esistere come campo esplicito (il pool di un oggetto resta
la sua posizione, vedi §Pool); prerequisiti, incompatibilità, peso nel pool e valore di
sinergia restano campi del documento senza corrispondente in codice.

Pesi del mix di categoria — **default proposto dall'implementazione** (stile DEC-019, da
confermare col playtest, vedi la domanda aperta annotata durante questa fase):

| Categoria | Peso |
|---|---|
| Passivo | 50 |
| Attivo | 20 |
| Innesto | 15 |
| Stat-up | 15 |

Passivo maggioritario (nessun limite di slot, coerente con l'essere stato finora l'unico
contenuto reale); attivo e Innesto più rari perché hanno slot limitati (trovarne uno è un
evento, non la norma); lo stat-up compare anche nel pool normale (DEC-035, sopra) accanto
al bossItem garantito a parte. Un attivo generato dichiara sempre esattamente cariche
**oppure** cooldown (mai entrambi, mai nessuno dei due): valori derivati dalla rarità
dell'oggetto (più raro → più cariche o cooldown più corto), dentro le bande di sicurezza
di [Active Items](active-items.md#default-proposti-dallimplementazione). Un Innesto
generato non aggiunge campi divergenti (vedi [Grafts](grafts.md), "Regole per contenuti
generati", non riformulato qui): stessi campi di ogni altro oggetto.

## Campi obbligatori di un oggetto

Ogni oggetto, curato o generato, deve compilare questi campi. L'elenco è esaustivo per
quanto riguarda i *nomi* dei campi; i documenti di sottotipo possono solo aggiungere
dettagli, non nomi di campo incompatibili.

| Campo | Valori / significato |
|---|---|
| `id` | identificatore stabile e univoco dell'oggetto |
| `nome` | nome presentato al giocatore |
| `categoria` | `attivo` \| `passivo` \| `stat-up` \| `Innesto` |
| `origine` | `curato` \| `composto` \| `variato` \| `nuovo` — vedi [glossario](../governance/glossary.md) per la tassonomia dell'origine del contenuto |
| `rarità` | `comune` \| `non-comune` \| `rara` \| `leggendaria` |
| `peso nel pool` | peso relativo usato nell'estrazione pesata |
| `budget di potenza` | quantità massima di impatto meccanico consentita a quella rarità/contesto |
| `tag` | proprietà semantiche usate da regole, generazione e presentazione |
| `prerequisiti` | condizioni che devono essere vere perché l'oggetto possa comparire in un pool |
| `incompatibilità` | oggetti o stati con cui l'oggetto non può coesistere nella build |
| `valore di sinergia` | partecipazione a sinergie implicite e/o alla fusione esplicita (vedi [Synergies](synergies.md)) |
| `costo o rischio` | quando presente: cosa il giocatore paga o rischia per ottenerlo/usarlo |
| `pool di appartenenza` | uno o più pool tematici o funzionali a cui l'oggetto è candidato |

Nota terminologica: il concetto che una versione precedente di questo documento chiamava
"esclusioni" si chiama sempre **incompatibilità**.

## Pool

Un oggetto appartiene a uno o più pool tematici o funzionali. Esempi astratti:

- ricompense standard;
- tesoro;
- rischio elevato;
- protezione;
- mobilità;
- trasformazione dei proiettili;
- economia;
- boss;
- segreto.

## Densità di oggetti per piano (DEC-032)

In media un piano offre **3-5 oggetti** al giocatore; una run completa (5 piani) ne offre
indicativamente circa **20** in totale. Questo valore risolve la domanda aperta sul numero
medio di oggetti per piano.

## Condizioni di ingresso

Un pool viene interrogato quando il gioco deve offrire o assegnare un oggetto: stanza
tesoro, drop di boss, negozio, stanza di fusione (l'oggetto risultante segue comunque
questi stessi campi, vedi [item-fusion.md](item-fusion.md)), eventi generati.

## Input/azioni

1. Selezione del pool (o dei pool) pertinente al contesto.
2. Filtro per prerequisiti non soddisfatti e per incompatibilità con la build corrente.
3. Estrazione pesata secondo `peso nel pool` e rarità.
4. Applicazione della correzione di fortuna se le condizioni sono soddisfatte.

## Risultato

Un oggetto con tutti i campi obbligatori compilati, assegnato allo slot o alla build del
giocatore secondo la sua categoria.

## Feedback

Icona/silhouette, nome, rarità e una descrizione breve dell'effetto sono sempre visibili
al momento dell'ottenimento e nella schermata build.

## Rarità e peso (default proposto — stato draft)

I seguenti pesi sono i **default attuali dell'implementazione**, non una decisione di
design definitiva: vanno validati col playtest e possono cambiare. Il resto di questo
documento (tassonomia, slot, campi, correzione di fortuna) è `approved`; solo questa
sezione numerica resta `draft`.

Pool standard:

| Rarità | Peso |
|---|---|
| comune | 55 |
| non-comune | 30 |
| rara | 12 |
| leggendaria | 3 |

Pool boss:

| Rarità | Peso |
|---|---|
| comune | 0 |
| non-comune | 0 |
| rara | 70 |
| leggendaria | 30 |

### Pool curato minimo: almeno un oggetto per rarità (DEC-144)

Il pool curato minimo di 20 oggetti (fonte unica della tabella per categoria in
[Generated Content Validation](generated-content-validation.md)) garantisce **almeno un
oggetto per ciascuna delle 4 rarità**: nessuna rarità può restare vuota nel pool minimo,
nemmeno quando l'applicazione pura dei pesi standard sopra la escluderebbe per
arrotondamento. L'eventuale eccedenza necessaria a rispettare la garanzia **si sottrae
alle rarità più comuni** — a partire dalla comune, la fascia più popolosa e quindi quella
che assorbe l'aggiustamento senza alterare la gerarchia percepita delle rarità — e **non
si aggiunge al totale**: il pool minimo resta di 20 oggetti (DEC-087).

Esempio numerico **derivato**, marcato come tale e non normativo — i valori restano da
confermare col playtest (stile DEC-019), qui serve solo a mostrare come si applica la
regola: applicando i pesi standard (comune 55, non-comune 30, rara 12, leggendaria 3) a un
pool di 20 oggetti si ottiene comune 11, non-comune 6, rara 2,4, leggendaria 0,6; troncando
per difetto restano 11 + 6 + 2 = 19 oggetti e la rarità leggendaria resta vuota. Il vincolo
pretende almeno 1 leggendario, e l'unità che serve arriva dal residuo di arrotondamento
senza toccare il totale, così: **11 comuni / 6 non-comuni / 2 rari / 1 leggendario = 20**.
Se il residuo non bastasse, l'unità mancante si sottrarrebbe alle rarità più comuni, perché
il totale resta 20.

## Controllo della run

Il sistema deve evitare:

- troppe ricompense della stessa categoria;
- build senza possibilità di scalare;
- offerte inutilizzabili senza alternativa;
- ripetizioni eccessive;
- combinazioni dichiarate incompatibili.

## Correzione di fortuna

Sostituisce il concetto informale di "pity": è la garanzia che, dopo N estrazioni
sfortunate consecutive, la qualità minima della successiva estrazione salga, **senza**
garantire sempre la soluzione perfetta per la build corrente. È un correttore invisibile
al giocatore nei suoi meccanismi interni, non nei suoi effetti (il giocatore percepisce
un'offerta migliore, non la formula).

### Soglia N e statistica Fortuna (DEC-145)

La soglia **N** — il numero di estrazioni consecutive di rarità comune prima che la
correzione scatti — è un parametro esplicito del sistema, non un dettaglio implicito
lasciato all'implementazione: il suo valore esatto resta da confermare col playtest (vedi
Domande aperte residue), ma la sua esistenza, il suo nome e il suo ruolo sono parte del
contratto di questo documento.

La statistica **Fortuna** del personaggio (vedi [player.md](player.md)) **riduce N**: più
Fortuna possiede il personaggio, meno estrazioni sfortunate consecutive servono perché la
correzione scatti. La riduzione non elimina mai del tutto la soglia (non esiste Fortuna
che garantisca sempre la rarità massima), coerente con la regola generale sopra.

**Scope: tutti i pool.** La correzione di fortuna, con la sua soglia N eventualmente
ridotta dalla Fortuna, si applica a ogni pool di oggetti (standard, tesoro, negozio,
ecc.), senza eccezioni dichiarate. Nota sul **pool boss**: quel pool ha già una garanzia
strutturalmente superiore (pesi 0/0/70/30, vedi tabella sopra — nessun peso su comune o
non-comune), quindi la condizione che attiva la correzione di fortuna (estrazioni
consecutive di rarità comune) non può verificarsi in un pool che non contiene comuni: la
correzione di fortuna resta definita anche per il pool boss, ma vi si applica solo in
teoria, perché la garanzia strutturale del pool la rende già superflua in pratica.

### Stato di implementazione (2026-07-27)

DEC-144 e DEC-145 vivono lato motore in `src/gameplay/item_pool.h`/`.c` (modulo puro,
prefisso `ItemPool`), usato sia dal contenuto di ripiego sia dall'estrazione a runtime:

- `ItemPoolMinimumCounts(poolSize, weights, outCounts)` implementa la garanzia di
  copertura DEC-144: applica i pesi per proporzione (arrotondamento per difetto), poi
  porta a 1 ogni rarità a peso > 0 rimasta a zero, prelevando l'unità mancante prima dal
  residuo di arrotondamento e poi dalle rarità più comuni capienti. Verificata
  esattamente contro l'esempio normativo di questo documento (pool di 20, pesi standard
  → 11/6/2/1).
- Il pool a cui questa funzione si applica **davvero** nel contenuto di ripiego
  (`GenerateFallbackContent`, `src/content/run_content.c`) sono le **15 posizioni normali
  dell'intera run** (`FLOOR_COUNT * 3` — i 5 piani x le 3 posizioni tesoro/negozio
  ciascuno, `bossItem` escluso): `ItemPoolMinimumCounts(15, pesi standard, ...)` garantisce
  almeno una copia di ogni rarità mantenendo la comune come fascia maggioritaria. Le 15
  rarità così calcolate vengono rimescolate **fra tutte le posizioni della run** (Fisher-
  Yates sull'RNG di gameplay) e poi assegnate 3 alla volta a ciascun piano. **Non** si
  applica più alle 3 posizioni di un singolo piano: un pool di sole 3 posizioni non può
  ospitare le 4 rarità senza azzerare del tutto la comune
  (`ItemPoolMinimumCounts(3, ...)` → `{0, 1, 1, 1}`), il che va contro la motivazione di
  DEC-144 ("senza alterare la gerarchia percepita delle rarità") e rende la correzione di
  fortuna DEC-145 strutturalmente inerte su quel piano (senza comuni lo streak di
  estrazioni sfortunate non può mai crescere) — regressione misurata e corretta in questa
  stessa fase. Il `bossItem` di ripiego, per ciascun piano, tira invece dai **pesi del pool
  boss** (`ItemPoolRollRarity` con `{0,0,70,30}`) invece di un valore fisso: "il boss non
  delude mai" resta vero per costruzione (mai comune/non-comune), non per un caso speciale
  in più.
- L'estrazione a runtime (`WorldSpawnRoomContents`, stanza tesoro/negozio) pesca in modo
  **uniforme** fra i 3 candidati del piano (più la restrizione della correzione di
  fortuna), non più pesata per rarità al momento della scelta: i candidati arrivano **già**
  pesati secondo DEC-019 da chi li ha generati (melting-gen per una run vera,
  `ItemPoolMinimumCounts` per il ripiego, entrambi sopra), quindi pesare di nuovo in
  `ItemPoolDrawIndex` applicherebbe la tabella una seconda volta, elevando la distribuzione
  al quadrato invece di rispettarla — regressione misurata su 2.000.000 di estrazioni
  reali (rara dimezzata, leggendaria quasi azzerata) e corretta in questa stessa fase. Una
  scelta uniforme fra candidati già distribuiti secondo i pesi standard riproduce quei pesi
  **esattamente** sul candidato estratto: è così che DEC-019 resta rispettato "salvo
  intervento della correzione di fortuna" (Scenario 1). Se la correzione è attiva e nessun
  candidato del pool è non-comune, l'estrazione degrada a quella uniforme normale (promessa
  dichiarata sopra: "senza garantire sempre la soluzione perfetta"). Con il contenuto di
  ripiego questo caso PUÒ presentarsi su un singolo piano (la garanzia di copertura DEC-144
  vale sulle 15 posizioni dell'intera run, non piano per piano: un piano può ricevere, per
  come cade il rimescolo, 3 posizioni tutte comuni) — è esattamente lo scenario per cui
  serve la correzione di fortuna DEC-145, che qui resta viva perché la comune è ancora
  presente nel pool.
- Soglia N (DEC-145): **N base = 4**, ridotta di 1 ogni 4 punti di Fortuna
  (`ItemPoolLuckThreshold`), clampata a un minimo di 1 (richiesto dal documento) e a un
  massimo pari alla base (scelta implementativa: una Fortuna negativa non peggiora la
  sequenza oltre il default, solo una Fortuna positiva la accorcia). **Default proposto
  dall'implementazione (stile DEC-019)**: N base, divisore e clamp superiore restano da
  confermare col playtest — vedi la domanda aperta annotata durante questa fase.
  Contatori indipendenti per pool tesoro/negozio (`Game.treasureLuckStreak`/
  `shopLuckStreak`, azzerati a ogni reset di run); nessun contatore per il pool boss
  (superfluo per costruzione, vedi sopra).
- Tutto deterministico dall'RNG di gameplay (`game->rng`, derivato dal seed di run
  DEC-141): nessuna funzione del modulo usa `time()`/`rand()` globali. Verificato da
  `--item-pool-test` (`make test`): garanzia numerica, soglia N, correzione avversariale
  (pesi sbilanciati apposta), statistica su 200 semi che la Fortuna alta limita la
  sequenza sfortunata massima osservata, copertura del contenuto di ripiego su 60 semi,
  determinismo end-to-end.

### Stato di implementazione del pool curato (W5b, DEC-153, 2026-07-28)

Il **formato e il loader** del pool curato minimo (tabella sopra e in
[Generated Content Validation](generated-content-validation.md#pool-curato-minimo-dec-087))
esistono lato motore in `src/content/curated_catalog.h`/`.c`: un file per categoria
(`items.txt`/`enemies.txt`/`bosses.txt` dentro `assets/curated-content/`), stesso stile
riga-chiave=valore già usato dal manifest di una run generata — riusa `ItemKindFromText`/
`RarityFromText` (`src/content/run_content.h`) e `EnemyFormFromText`/`EnemyMoveFromText`/
`EnemyFireFromText`/`EnemyTypeBalance` (`src/core/enemy_type.h`), nessun secondo
vocabolario testo↔enum. `CuratedCatalogValidateFloor` verifica al caricamento che il floor
DEC-144 sia rispettato (un log chiaro su stderr se una rarità a peso positivo risulta
scoperta, mai bloccante).

`RunContentLoad` (`src/content/run_content.c`) applica il pool curato come **stato base**
(DEC-153) sopra il contenuto di ripiego puramente procedurale descritto sopra e sotto una
generazione vera: precedenza `curated-content -> fallback deterministico -> generazione
reale`. La stessa distribuzione di rarità sui 15 slot normali della run (sopra) sceglie la
rarità **target** per slot; `CuratedCatalogPickItem` pesca dal pool curato una voce di
quella rarità esatta (RNG indipendente dal fallback procedurale, salt proprio) — uno slot
la cui rarità non ha corrispondenza nel pool resta quello procedurale, mai vuoto. I due
nemici e il boss del piano (se il pool curato ne dichiara) sono pescati con lo stesso RNG
derivato dal seed (non un indice fisso per piano): run diverse vedono nemici/boss curati
diversi, coerente con DEC-141. Un `assets/curated-content/` assente o incompleto
(checkout senza contenuto ancora scritto) lascia il contenuto esattamente come il ripiego
procedurale l'ha già prodotto: nessuna differenza di comportamento rispetto a prima di
questo lavoro.

`bossItem` (la ricompensa del boss del piano) **resta sempre procedurale**
(`MakeFallbackBossItem`): il pool curato di questo lavoro copre solo i 15 slot normali
della run, non la ricompensa del boss. Scelta deliberata (coerente con la sezione sopra,
che tratta `bossItem` a parte dai "15 slot normali" anche nel conteggio del floor
DEC-144), non un buco: con un catalogo curato completo il giocatore non vedrà mai
contenuto curato come ricompensa di un boss.

Invariante di tassonomia mantenuta anche sotto l'overlay curato: un oggetto curato con
`kind=statup` non porta mai il tipo di colpo del piano. Se il pool curato assegna uno
stat-up esattamente allo slot che il ripiego procedurale aveva scelto come portatore del
tipo di colpo, `ApplyCuratedCatalog` lo **sposta** su un'altra posizione non-statup dello
stesso piano (stesso algoritmo di `tools/melting-gen/gen_fallback.c`: "si sceglie fra le
posizioni non-statup"), azzerandolo solo nel caso degenere in cui tutti e tre gli slot del
piano risultino statup. Verificato su 25 semi con un pool minimo (comune/non-comune
statup, raro/leggendario passivi) da `--curated-content-test`.

Il **layer di indirezione immagini** (`content-id -> image-id -> file`, DEC-175(b) esteso
da W5b al pool di testi/parametri, `src/content/curated_image_map.h` +
`CuratedImagesFindById` in `src/content/curated_images.h`, file
`assets/curated-content/image-map.txt`) e lo script di auto-mapping riproducibile per tag
(`scripts/curated-map.py`, nessuna curation estetica, con de-duplicazione: a parità di
punteggio un content-id senza tag in comune non finisce più sempre sulla stessa prima
immagine della categoria, salvo che le immagini disponibili siano meno dei content-id)
sono anch'essi implementati; un id assente in una qualunque delle due tappe lascia
l'oggetto senza immagine (resa geometrica di sempre), mai un crash. Un'immagine curata
risolta come stato base viene marcata subito in `Game.curatedImageUsed`
(`GameResetRunWithSeed`, `src/game/game.c`), cosa che DEC-171 richiede per non farla
ripescare da `FusionPerform` come se fosse ancora libera.

**Non ancora fatto** (passo successivo dichiarato di questo lavoro): il contenuto curato
vero e proprio (le voci reali di `assets/curated-content/`) non esiste ancora — solo
fixture minime di test in `src/tests/curated_content_tests.c`. Verificato da
`--curated-content-test` (`make test`): floor rispettato/violato, indirezione risolta, id
immagine mancante, catalogo assente, l'integrazione end-to-end dentro `RunContentLoad`
con una fixture isolata (`CuratedCatalogSetTestDir`, mai `assets/curated-content/` reale),
e l'invariante statup/tipo-di-colpo su piu' semi. I test che caricano `RunContentLoad`
mettono da parte `generated/current_run.txt` (se presente) prima di caricare e lo
ripristinano subito dopo — stesso schema di `TestFallbackBossItemIsRare`
(`src/tests/script_items_tests.c`) e `GameItemPoolFallbackCoverageTest`
(`src/tests/game_tests.c`): senza questa guardia un manifest reale presente in
`generated/` (stato normale dopo `make gen` o dopo una run giocata) sovrascriverebbe il
contenuto curato prima delle verifiche.

## Interazioni

- Con le **sinergie**: `valore di sinergia` segnala se l'oggetto partecipa a coppie
  implicite e/o è un buon candidato per la fusione esplicita — vedi [Synergies](synergies.md).
- Con il **budget di leggibilità**: quando un oggetto introduce trasformazioni visive
  (in particolare passivi e Innesti con effetti persistenti), si applica il limite
  descritto in [Combat and Projectiles](combat-and-projectiles.md#budget-di-leggibilità)
  senza riformularlo qui.
- Con l'**economia**: `costo o rischio` collega l'oggetto alle risorse definite altrove
  nella KB (valuta principale, catalizzatore di fusione, ecc.).

## Regole per contenuti generati

Un oggetto con `origine: composto | variato | nuovo` deve comunque compilare tutti i
campi obbligatori prima di poter comparire in un pool. La generazione non sceglie da un
menu: inventa parametri dentro le bande di garanzia e i budget dichiarati (budget di
potenza in primis), e il contenuto proposto passa dagli stati di validazione descritti
in [Generated Content Validation](generated-content-validation.md) prima di diventare
`approvato-per-run`.

## Casi limite

- Pool vuoto dopo i filtri di prerequisiti/incompatibilità: si applica il fallback.
- Tutti gli oggetti candidati risultano incompatibili tra loro nella stessa offerta: se
  ne rimuove il numero minimo necessario per rendere l'offerta valida.
- Slot Innesto o attivo pieno: l'oggetto può comunque essere offerto se il gioco prevede
  sostituzione volontaria dello slot; altrimenti resta escluso dall'offerta in quel
  momento.

## Fallback

Quando un pool non può produrre un'estrazione valida (pool vuoto, oggetto generato non
validato in tempo), si applica la regola unica descritta in
[Generated Content Validation](generated-content-validation.md): non riformulata qui.

## Non-obiettivi

- Questo documento non stabilisce il bilanciamento numerico finale (i pesi sono draft).
- Non è un sistema di monetizzazione: nessuna valuta reale è coinvolta nell'estrazione.
- Non descrive i dettagli della categoria Innesto o della fusione esplicita: vivono
  rispettivamente in [Grafts](grafts.md) e [item-fusion.md](item-fusion.md).

## Domande aperte residue

- Validazione col playtest dei pesi di rarità (pool standard e pool boss), del valore
  esatto della soglia N di estrazioni sfortunate che attiva la correzione di fortuna e di
  quanto la statistica Fortuna la riduce (DEC-145 fissa che N esiste, che è ridotta dalla
  Fortuna e che si applica a tutti i pool, non i numeri esatti).
- Numero massimo di slot attivo/Innesto ottenibili in una run.

## Scenari verificabili

### Scenario 1 — estrazione pesata standard

Given un pool di ricompense standard con i pesi di rarità default (comune 55,
non-comune 30, rara 12, leggendaria 3),  
When il giocatore apre una ricompensa da quel pool,  
Then l'oggetto estratto rispetta i pesi configurati, salvo intervento della correzione
di fortuna.

### Scenario 2 — prerequisito non soddisfatto

Given un oggetto con un prerequisito non soddisfatto dallo stato attuale della run,  
When il generatore compone il pool per una stanza,  
Then l'oggetto è escluso da quel pool per quella run.

### Scenario 3 — correzione di fortuna

Given un giocatore che ha ricevuto N estrazioni consecutive di rarità comune,  
When si attiva la correzione di fortuna,  
Then la prossima estrazione garantisce almeno rarità non-comune, senza garantire la
rarità massima.

### Scenario 4 — incompatibilità tra oggetti offerti

Given due oggetti dichiarati incompatibili tra loro,  
When entrambi risulterebbero candidati per la stessa offerta,  
Then il sistema ne rimuove uno secondo la regola di priorità dichiarata, mantenendo
l'offerta valida.

### Scenario 5 — stat-up in un pool standard

Given un pool di negozio o di tesoro che include oggetti di categoria stat-up (DEC-035),  
When il gioco compone l'offerta per quella stanza,  
Then uno stat-up può comparire nell'offerta senza che l'unica fonte prevista sia una
ricompensa boss.

### Scenario 6 — oggetto ibrido con modifica di statistica negativa

Given un oggetto passivo generato con comportamento e una modifica negativa a una
statistica, coerente con il proprio effetto (DEC-035),  
When l'oggetto viene offerto al giocatore,  
Then la scheda dell'oggetto mostra sia il comportamento sia la modifica di statistica,
positiva o negativa, come parte dello stesso oggetto, senza introdurre una categoria
diversa da `passivo`.

### Scenario 7 — la Fortuna riduce la soglia N su un pool qualsiasi

Given due giocatori con statistica Fortuna diversa (uno più alta dell'altro), entrambi
in una sequenza di estrazioni sfortunate consecutive dallo stesso tipo di pool (non
boss),  
When si conta quante estrazioni servono prima che scatti la correzione di fortuna per
ciascuno,  
Then il giocatore con Fortuna più alta raggiunge la correzione dopo meno estrazioni
(soglia N più bassa), secondo la regola di DEC-145; il pool boss non è mai in questa
condizione perché non contiene rarità comuni da cui partire.
