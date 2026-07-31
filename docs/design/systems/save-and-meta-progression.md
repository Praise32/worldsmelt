---
id: gd-system-save-meta
title: Save and Meta Progression
domain: design
status: approved
authority: canonical
owner: design
summary: "Cosa persiste tra le run (DEC-015): catalogo contenuti, museo del Piano 0, punti singleplayer per sblocchi a doppio canale (DEC-027, dettaglio in rewards-and-economy.md); niente potenziamenti permanenti del personaggio. Il Catalogo del menu principale ha tre funzioni: enciclopedia — sette categorie canoniche, Oggetti/Nemici/Boss/Personaggi/Mondi/Layout/Colpi (DEC-083), con anche i contenuti curati incontrati marcati origine `curato` (DEC-103) —, preferiti, spesa punti (DEC-045). Il museo è curato in modo misto: metriche più preferiti del Catalogo, che hanno la precedenza e non escono mai (DEC-063). Le medaglie/cornici cosmetiche della Classificata giornaliera persistono nel profilo, fuori dall'economia dei punti (DEC-064). La run può essere sospesa in ogni momento: al rientro la stanza corrente riparte dall'ingresso con i nemici ripristinati, il resto della run riprende esattamente com'era (DEC-050). A ogni aggiornamento del gioco il catalogo viene riconvalidato: ciò che fallisce diventa una Reliquia, consultabile ma non più giocabile né sbloccabile; nel museo una Reliquia resta esposta solo se era lì come preferito, altrimenti ne esce automaticamente (DEC-069, DEC-085)."
last_reviewed: 2026-07-31
last_verified_commit: a5cc3a3
topics: [save, meta-progressione, catalogo, museo, reliquie, sospensione-run, DEC-050, WP17]
related: []
supersedes: []
source_files: [src/content/run_catalog.c, src/game/run_suspend.c, src/game/run_suspend.h, src/tests/suspend_tests.c]
---

# Save and Meta Progression

## Intento per il giocatore

Perdere una run non deve mai corrompere il profilo del giocatore. La progressione persistente
amplia le possibilità di gioco (più contenuti nei pool), ma non deve mai rendere irrilevante
la competenza del giocatore dentro la singola run.

## Condizioni di ingresso

Si applica a ogni run completata o interrotta in modalità singleplayer; le modalità
competitive interagiscono con questo sistema solo in lettura limitata (vedi sotto).

## Input/azioni

- una run singleplayer termina (vittoria, permadeath, o abbandono);
- un contenuto generato durante la run raggiunge lo stato approvato-per-run (vedi
  `generated-content-validation.md`);
- il giocatore guadagna punti in una run singleplayer;
- il giocatore spende punti per sbloccare contenuti generati nei pool delle run future.

## Cosa persiste tra le run (DEC-015)

- **Il catalogo di tutti i contenuti generati**: ogni contenuto approvato-per-run in qualunque
  run singleplayer entra nel catalogo permanente del profilo.
- **Il museo del Piano 0**, che raccoglie i migliori contenuti tra quelli catalogati con un
  criterio di curatela misto — promozione automatica per metriche più preferiti del
  giocatore, che hanno la precedenza e non escono mai dal museo (DEC-063, fonte unica in
  `floor-zero.md`, rimando, non riformulato qui).
- **I punti guadagnati solo in singleplayer**, spendibili per sbloccare contenuti generati nei
  pool delle run future. Il meccanismo di guadagno è a doppio canale — punti base dal
  risultato della run più bonus da prove specifiche (DEC-027) — descritto in
  [rewards-and-economy.md](rewards-and-economy.md) come fonte unica; questo documento non lo
  ripete, registra solo che quei punti persistono.

## Abbandono e reroll contano come sconfitta per i punti (DEC-082)

Ai fini dei punti sblocco, l'abbandono volontario di una run in corso (`ExitConfirm` da
`PauseMenu`) e il reroll da `Gameplay` contano entrambi come **sconfitta**: punti ridotti
standard su quanto maturato fino a quel momento, nessuna categoria intermedia. L'abbandono
della sola preparazione nel Piano 0 (`ExitConfirm` da `FloorZero`, DEC-074) avviene prima
che la run giocata cominci: non è una sconfitta e resta fuori da questa contabilità. Il catalogo e le statistiche si
aggiornano comunque con quanto incontrato — è la stessa regola già valida per ogni run
interrotta a metà (vedi "Casi limite" sotto). La distinzione vittoria/sconfitta e l'esatta
riduzione dei punti sono fonte unica in
`ui/results-and-leaderboards.md` (DEC-041); questo documento registra solo che abbandono e
reroll rientrano nel bucket "sconfitta", senza una categoria propria.

Il **flusso di uscita** differisce però tra i due: l'abbandono passa da `RunResults` come una
sconfitta, con i punti ridotti visibili lì; il reroll salta i risultati e accredita i punti
ridotti **in silenzio**, restando comunque consultabili nel Catalogo. Fonte unica del flusso:
`ui/results-and-leaderboards.md` (DEC-089); questo documento registra solo che il conteggio ai
fini punti resta identico per entrambi (rimando, non riformulato qui).

## Il Catalogo: tre funzioni (DEC-045)

Il Catalogo, raggiungibile dal menu principale (vedi `ui/main-menu.md`), è la schermata che
espone la meta-progressione persistente sopra descritta attraverso tre funzioni:

- **enciclopedia consultabile** di tutto il contenuto generato incontrato dal giocatore,
  organizzata nelle sette categorie canoniche elencate sotto (DEC-083): ogni scheda mostra
  nome, sprite, storia e statistiche d'uso;
- **preferiti**: il giocatore può segnare contenuti come preferiti; i preferiti pesano
  leggermente sulle proposte future dell'IA nelle run successive, senza garantirne la
  comparsa (restano soggette alle stesse regole di generazione e di peso nel pool); i
  preferiti hanno inoltre la precedenza nel museo del Piano 0 e non ne escono mai (DEC-063,
  rimando a `floor-zero.md`);
- **spesa dei punti sblocco**: è il luogo dove si spendono i punti guadagnati in
  singleplayer (DEC-015, DEC-027) per sbloccare contenuti generati nei pool delle run future.

Le **sette categorie consultabili** dell'enciclopedia sono canone (DEC-083; questo documento
è fonte unica del catalogo): ognuna ha una scheda propria, nessuna resta un semplice record
interno.

| Categoria | Cosa include |
|---|---|
| Oggetti | oggetti generati raccolti durante le run |
| Nemici | nemici generati incontrati |
| Boss | boss generati incontrati, con l'esito sconfitto/non sconfitto |
| Personaggi | personaggi generati scelti in una run |
| Mondi | temi dei piani generati raggiunti |
| Layout | layout stanza generati incontrati |
| Colpi | tipi di colpo generati adottati |

Il Catalogo è distinto dal museo del Piano 0: il museo è una galleria curata delle sole
creazioni migliori, provabile in loco (DEC-040); il Catalogo è l'enciclopedia completa più
preferiti e spesa punti. Idea futura (lista DEC-018): portare le funzioni del Catalogo anche
dentro il Piano 0/museo.

## Migrazione del catalogo tra versioni: le Reliquie (DEC-069)

A ogni aggiornamento del gioco, il catalogo persistente (vedi "Cosa persiste tra le run"
sopra) viene **riconvalidato** con la stessa pipeline di validazione usata in run (vedi
[Generated Content Validation](generated-content-validation.md), fonte unica del processo e
degli stati; questo documento non li ripete):

- ciò che **supera** la riconvalida resta giocabile e sbloccabile come prima, senza alcuna
  perdita;
- ciò che **fallisce** la riconvalida viene spostato in una sezione dedicata del Catalogo
  chiamata **Reliquie**: resta consultabile — scheda visibile con nome, sprite e storia — ma
  non è più giocabile né sbloccabile nei pool delle run future.

La memoria del giocatore non si perde mai: un contenuto diventato Reliquia resta
nell'enciclopedia personale del giocatore, semplicemente non più attivo in run. Il termine
"Reliquie" è coerente con la cornice del crogiolo (DEC-067, fonte unica in
[Narrative Tone](../content/narrative-tone.md)): ciò che il crogiolo ha sciolto lascia
comunque una traccia consultabile.

### Reliquie nel museo del Piano 0 (DEC-085)

Un contenuto esposto nel museo del Piano 0 (DEC-063) che diventa una Reliquia non esce
sempre allo stesso modo:

- se era esposto **come preferito del giocatore**, resta esposto — la curatela per
  preferiti non esce mai dal museo finché marcata (DEC-063) — con la scheda museale che
  segnala che si tratta ormai di una Reliquia, e senza più la prova in arena (DEC-040),
  perché le Reliquie non sono giocabili (DEC-069);
- se era esposto **solo per promozione automatica per metriche**, esce automaticamente dal
  museo: la promozione per metriche riguarda solo contenuti ancora in circolazione, non le
  Reliquie.

I criteri di curatela del museo (soglie, precedenza dei preferiti) restano descritti in
`floor-zero.md`, fonte unica; questo documento registra solo l'effetto sulle Reliquie.

## Sospensione della run e ripresa (DEC-050)

A differenza delle sezioni sopra — che descrivono cosa persiste **tra** una run e l'altra —
questa sezione descrive la sospensione di una run **in corso**: il giocatore può sospendere
in qualunque momento, non solo nei punti di salvataggio tradizionali.

Al rientro:

- la **stanza corrente riparte dall'ingresso**, con i nemici ripristinati: non esiste uno
  snapshot di metà combattimento;
- il **resto della run** (piani già completati, build, oggetti, risorse, tema scelto)
  riprende esattamente com'era al momento della sospensione.

Questo comportamento è coerente con "Continua" da `MainMenu`, che rientra direttamente nello
stato salvato (`Gameplay` nel caso tipico, `FloorZero` se la sospensione è avvenuta nel
Piano 0): fonte unica dei nomi di stato e della transizione è
[Game States and Flow](../05-game-states-and-flow.md) e
[Navigation Map](../ui/navigation-map.md); questo documento non li ripete, registra solo la
regola di ripristino della stanza corrente.

> **Nota di implementazione (WP17, 2026-07-31, gap G6 di `piano-zero-e-meta` e G2 di
> `ui-cornice`):** la sospensione esiste ora nel motore (`src/game/run_suspend.c`,
> prefisso `RunSuspend*`). Fino a questo lavoro il documento descriveva DEC-050 come
> comportamento attuale mentre nel codice non esisteva alcuna forma di salvataggio dello
> stato di una run: chiudere il gioco perdeva sempre la run in corso.
>
> **Quando.** La sospensione è **esplicita**: la voce "Sospendi e esci" di `PauseMenu`
> (vedi [Pause Menu](../ui/pause-menu.md)) scrive il file e torna a `MainMenu`, dove
> "Continua" prende il focus. Nessuna conferma: è l'unica uscita di quel menu che non
> perde nulla. **Non esiste un salvataggio automatico** alla chiusura della finestra:
> limite dichiarato in `docs/engineering/known-issues.md`, non un requisito scartato — il
> ciclo applicativo esce da `WindowShouldClose`/`ExitConfirm` senza un gancio di
> terminazione, e aggiungerne uno che scriva a ogni chiusura (compresa quella dopo una
> morte) è una decisione di comportamento che questo documento non fissa.
>
> **Formato** (`suspend/current.txt`, accanto a `catalog/`, mai versionato, dati del
> giocatore): file di testo chiave=valore nello stile di `run_catalog.c`, scritto con
> tmp+rename atomico e con la stessa **disciplina zero-default** (una chiave assente vale
> sempre il significato più innocuo). La prima riga è `suspendSchema=1`, il campo
> **versione**: un file senza quella riga, con un numero diverso, troncato o con piano e
> stanza fuori banda viene **ignorato per intero** — voce "Continua" assente, nessun
> crash, stesso pattern con cui il Catalogo salta un record corrotto.
>
> **Cosa si salva**: seed di run; personaggio scelto (indice, più la definizione completa
> quando è quello generato per la run, che vive in `generated/` e potrebbe non esserci più
> al rientro); piano e stanza correnti; stato del giocatore (salute, Crust, risorse,
> statistiche di base, slot funzionali) e **inventario intero, sorgente Lua compresa** —
> non "per nome dal manifest": un oggetto **fuso** (DEC-023) non esiste in nessun manifest,
> e ricostruirlo per nome lo perderebbe in silenzio; stanze visitate/ripulite/premiate,
> arene accettate, segrete aperte, distruttibili distrutti; Innesti lasciati a terra
> (DEC-183); la puntata della Pourhouse per intero, offerta compresa, più la firma di run
> che impedisce due puntate identiche (DEC-044); le prove della run con il loro stato
> (DEC-042); il numero di fusioni; il tempo di run; i contatori di correzione di fortuna;
> i tipi di nemico/boss già incontrati.
>
> **Cosa NON si salva**, per la regola stessa di DEC-050: la posizione del giocatore
> dentro la stanza, i nemici vivi, colpi e particelle. E nessuno stato del Piano 0.
>
> **Come si ricostruisce.** Il mondo non è serializzato: si rigenera dal seed di run
> (`GameResetRunWithSeed` → `WorldStartFloor`) e lo stato salvato si applica **sopra**
> (visitate/ripulite/aperte/distrutte). Perché la mappa del piano corrente torni identica
> servono due stati di RNG, non uno: `Game.floorEntryRng` (il valore di `game->rng`
> catturato da `WorldStartFloor` prima di generare la mappa) e `game->rng` al momento della
> sospensione — vedi i default proposti qui sotto.
>
> Alla ripresa la **sospensione si consuma**: il file viene cancellato appena la
> ricostruzione riesce, così morire dopo il rientro non riporta al punto di salvataggio
> (coerente con il permadeath descritto sopra). Anche l'**abbandono** (DEC-082/DEC-089), il
> **reroll** (DEC-114) e "Nuova run" con una sospensione attiva la cancellano.
>
> **Limite dichiarato:** il **Piano 0 non è sospendibile** in questa fetta — la voce
> "Sospendi e esci" esiste solo in una run vera (`game->floor >= 1`, la stessa soglia di
> WP19). Il documento prevede anche `FloorZero` come stato salvato: quello stato comprende
> la generazione in corso e le carte-proposta, che un file di run non ricostruisce da solo.
> Registrato in `docs/engineering/known-issues.md`, non requisito scartato.
>
> Verificato da `--suspend-test` (`GameSuspendTest`, `src/tests/suspend_tests.c`, in
> `make test`): andata e ritorno su una run ricca al piano 3 con confronto **campo per
> campo**, mappa del piano rigenerata identica, stanza che riparte dall'ingresso coi
> nemici ripristinati, determinismo di due riprese dallo stesso file, file corrotto o di
> versione diversa che non produce mai una voce "Continua" né un crash, sospensione
> consumata, e le tre vie che la cancellano.

### Default proposti dall'implementazione (WP17, non canone)

Registrati anche in [open-questions.md](../governance/open-questions.md), voce 58; il
proprietario resta libero di deciderli diversamente. Nessuno di questi è promosso a canone.

- **Percorso e forma del file**: `suspend/current.txt`, **una sola** sospensione per
  profilo (è quello che "Continua" sa esprimere: una voce, non una lista di salvataggi).
  Il documento non fissa né percorso né molteplicità. Nota: questo è un **salvataggio di
  run**, dominio di questo documento, non il file di preferenze della domanda aperta 24,
  che resta aperta e distinta.
- **Si salva lo stato dell'RNG di gioco** (`game->rng` al momento della sospensione, più
  `Game.floorEntryRng`, il valore d'ingresso nel piano): la sequenza di gioco riprende
  identica invece di divergere. L'alternativa — accettare la divergenza e non salvare
  nulla — sarebbe stata più semplice ma avrebbe reso una run ripresa non più confrontabile
  con la stessa run giocata di fila.
- **"L'ingresso" della stanza è il suo baricentro** (`WorldRoomCenter`), lo stesso punto in
  cui il giocatore compare entrando in un piano. La porta da cui era entrato non è salvata,
  ed è l'unica cosa che permetterebbe di scegliere uno dei quattro punti d'ingresso laterali.
- **Nessun salvataggio automatico** alla chiusura della finestra (vedi la nota sopra).
- **I semi delle sandbox Lua dei singoli oggetti non si salvano**: si ri-estraggono dallo
  stream di ricostruzione, quindi una funzione Lua che chiami `rng()` riparte da un punto
  diverso della propria sequenza. Resta deterministico dal file — due riprese dello stesso
  file danno gli stessi semi — ma non identico alla run originale.

## Ricompense cosmetiche della Classificata giornaliera (DEC-064)

Oltre al catalogo, al museo e ai punti sblocco (DEC-015), il profilo persistente conserva
anche le **medaglie e cornici** guadagnate nella Classificata giornaliera pubblica ("Daily",
DEC-062): ricompense cosmetiche legate ai piazzamenti e alle streak di partecipazione,
visibili nel profilo e nel museo del Piano 0. Fonte unica della Daily e delle sue regole:
[08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md#ricompense-della-daily-cosmetici-dec-064)
(rimando, non riformulato qui).

Le medaglie e le cornici **non assegnano punti sblocco** e non toccano in alcun modo
l'economia dei punti di meta-progressione (DEC-015, DEC-027): sono un canale di persistenza
separato, puramente cosmetico.

## Cosa NON persiste

Nessun potenziamento permanente del personaggio: non esiste una progressione di potenza
persistente. Ogni run parte dallo stesso punto di partenza in termini di forza del
personaggio; ciò che cambia tra run è l'ampiezza dei pool di contenuti generati disponibili,
non la potenza di base del personaggio.

## Risultato

Un profilo che accumula un catalogo di contenuti e punti spendibili, senza mai alterare
l'equilibrio di potenza di partenza di una run.

## Feedback

Le rappresentazioni concrete del catalogo, del museo e degli sblocchi (schermate, elenchi,
notifiche) restano fuori scope qui e vivono nei documenti di `ui/`; questo documento descrive
solo cosa persiste e con quali regole.

## Interazioni

- `floor-zero.md`: il museo del Piano 0 espone i migliori contenuti del catalogo persistente;
  la sezione "Reliquie nel museo del Piano 0" sopra registra cosa succede quando un contenuto
  lì esposto diventa una Reliquia (DEC-085).
- `run-manifest-and-reproducibility.md`: contesto sulle gare asincrone per cui gli sblocchi
  sono disattivati.
- `generated-content-validation.md`: i contenuti approvati-per-run entrano nel catalogo; i
  contenuti curati incontrati, incluso l'uso come fallback, entrano anch'essi marcati con
  origine `curato` (DEC-103, vedi "Regole per contenuti generati" sopra).
- `rewards-and-economy.md`: fonte del doppio canale di guadagno dei punti sblocco (DEC-027:
  punti base dal risultato della run più bonus da prove specifiche); questo documento
  descrive solo cosa persiste, non come si guadagna.
- `ui/main-menu.md`: la voce Catalogo del menu principale apre la schermata a tre funzioni
  descritta qui (DEC-045).
- `ui/results-and-leaderboards.md`: fonte unica della distinzione vittoria/sconfitta e della
  riduzione dei punti alla sconfitta (DEC-041); questo documento registra solo che abbandono
  e reroll rientrano nel bucket sconfitta ai fini dei punti (DEC-082) e che il flusso differisce
  — RunResults per l'abbandono, accredito silenzioso per il reroll (DEC-089).
- `05-game-states-and-flow.md` e `ui/navigation-map.md`: la regola "Continua" rientra nello
  stato salvato; questo documento aggiunge solo il dettaglio del ripristino della stanza
  corrente (DEC-050).
- `08-multiplayer-and-competition.md`: fonte unica delle ricompense cosmetiche della Daily
  (medaglie/cornici) che persistono qui nel profilo (DEC-064).
- `../content/narrative-tone.md`: la cornice del crogiolo (DEC-067) dà senso al termine
  "Reliquie" usato qui per i contenuti che non superano la riconvalida (DEC-069).

## Regole per contenuti generati

- Un contenuto **generato** entra nel catalogo permanente solo se ha raggiunto lo stato
  approvato-per-run in una run singleplayer.
- I **contenuti curati** incontrati durante una run (inclusi quelli usati come fallback)
  entrano anch'essi nel Catalogo, marcati con la loro origine `curato` (DEC-103) — tassonomia
  a quattro valori fonte unica in [Content Taxonomy](../content/content-taxonomy.md), rimando,
  non riformulato qui.
- Gli sblocchi acquistati con i punti ampliano i pool disponibili nelle run future, ma non
  garantiscono la comparsa di un contenuto specifico in una run specifica (restano soggetti
  alle stesse regole di generazione e di peso nel pool).

## Sblocchi disattivati nelle modalità competitive

Gli sblocchi acquisiti con i punti di meta-progressione sono **disattivati** nelle modalità
competitive: una gara asincrona su un manifest condiviso usa i pool di base, non i pool
ampliati dagli sblocchi personali di un giocatore (vedi `run-manifest-and-reproducibility.md`
per il contesto delle gare asincrone).

## Casi limite

- Un giocatore con un catalogo molto ampio non ottiene alcun vantaggio di potenza: solo più
  varietà potenziale nei pool delle run future singleplayer.
- Una run interrotta a metà (abbandono volontario da `ExitConfirm` o reroll da `Gameplay`)
  non deve corrompere il profilo: i contenuti già approvati-per-run fino a quel momento
  restano acquisiti nel catalogo secondo le stesse regole di una run completata; ai fini dei
  punti sblocco, abbandono e reroll contano entrambi come sconfitta, con la riduzione
  standard su quanto maturato (DEC-082).
- Il giocatore sospende la run a metà di un combattimento in una stanza: al rientro la
  stanza riparte dall'ingresso con i nemici ripristinati, non dal punto esatto di
  sospensione.
- Il giocatore sospende la run nel Piano 0: al rientro rientra direttamente nel Piano 0,
  coerente con `FloorZero` come stato salvato (vedi [Navigation Map](../ui/navigation-map.md)).
- Un aggiornamento del gioco riconvalida il catalogo e un contenuto già approvato-per-run in
  passato non supera più la riconvalida: diventa una Reliquia, resta consultabile nel
  Catalogo ma esce dai pool sbloccabili delle run future (DEC-069).
- Un contenuto esposto nel museo del Piano 0 diventa una Reliquia dopo un aggiornamento: se
  era lì come preferito del giocatore, resta esposto con la scheda che segnala la Reliquia e
  senza prova in arena; se era lì solo per metriche, esce automaticamente dal museo (DEC-085).

## Fallback

Questo sistema non definisce la regola di fallback per i contenuti generati: vedi
`generated-content-validation.md`.

## Stato di implementazione (M7, 2026-07-19; M8, 2026-07-19)

Nota di stato tecnica, non una regola di design: il **substrato del catalogo** è
implementato (`src/content/run_catalog.c`) — a fine run (vittoria, sconfitta o
abbandono, incluso il reroll da `Gameplay`) il gioco registra in un file locale
(`catalog/`, mai versionato, dati del giocatore) i contenuti generati che il
giocatore ha davvero incontrato in quella run: temi dei piani raggiunti, layout
stanza, oggetti presi, tipi di colpo adottati, nemici/boss incontrati, il
personaggio generato se scelto. Il record è autosufficiente (porta la
definizione completa, incluso il sorgente Lua dove esiste), pensato per essere
la fonte della riconvalida (DEC-069) senza dipendere da `generated/`, che resta
effimero. Se abbandono e reroll dovessero contare come sconfitta ai fini dei
punti sblocco, e non solo come casi da registrare nel catalogo, restava una
domanda aperta a questo punto dell'implementazione: è ora risolta da DEC-082
(entrambi contano come sconfitta, punti ridotti standard, vedi sopra).

M8 aggiunge la **prima fetta consultabile**: la voce "Catalogo" del menu
principale (`ui/main-menu.md`, DEC-015) apre ora un'enciclopedia v1 che
aggrega ON-DEMAND tutti i file di `catalog/` (mai per-frame) per le sette
categorie canoniche — Mondi, Layout, Oggetti, Colpi, Nemici, Boss, Personaggi
— con conteggio incontri/run e, per i boss, l'esito sconfitto/non sconfitto.
Al momento di questa implementazione era ancora una scelta aperta se layout
stanza e tipi di colpo meritassero una scheda propria o restassero solo
record interni: DEC-083 la scioglie sancendo le sette categorie come canone
(vedi tabella sopra). Consultazione soltanto: nessuna azione sulle voci,
nessuna scrittura.

Restano **gap di implementazione espliciti**, non requisiti scartati: i
**preferiti**, i **punti sblocco e la loro spesa** (DEC-027), il **museo del
Piano 0**, gli **sprite nelle schede** del Catalogo, e la **riconvalida vera**
che sposta un contenuto tra le Reliquie (DEC-069) — il file su disco è
completo, ma nessuno di questi cinque punti lo consuma ancora.

La domanda "i contenuti fallback-usati contano?" è ora risolta a livello di
design da **DEC-103**: sì, i contenuti curati incontrati (incluso il fallback)
entrano nel Catalogo, marcati con origine `curato`. Il default v1 di questa
fetta implementativa — una run interamente su contenuto di ripiego (nessuna
generazione, o generazione mai avviata) non scrive alcun record — resta però
il comportamento effettivo di `run_catalog.c`: **gap di implementazione
esplicito**, non un requisito scartato, tra la decisione di design (DEC-103) e
il substrato attuale, che non registra ancora né l'origine dei contenuti né i
contenuti incontrati in run interamente di fallback.

## Non-obiettivi

- Nessuna forma di potenziamento permanente del personaggio (statistiche, vite extra,
  vantaggi di potere che persistono tra run): esplicitamente escluso.
- Questo documento non definisce l'interfaccia di catalogo/museo/sblocchi (vedi `ui/`).
- Non definisce il tasso di conversione punti-sblocco (valore non deciso qui).
- Non ridefinisce i nomi di stato o le transizioni di navigazione (vedi
  [Game States and Flow](../05-game-states-and-flow.md),
  [Navigation Map](../ui/navigation-map.md)).
- Non ridefinisce i criteri esatti di piazzamento/streak che assegnano le medaglie della
  Daily: fonte unica in `08-multiplayer-and-competition.md`.

## Domande aperte residue

- Il tasso esatto di guadagno (punti base e bonus, DEC-027) e di spesa dei punti di
  meta-progressione (non deciso).
- Peso esatto che i preferiti aggiungono alle proposte future dell'IA (DEC-045 fissa solo
  che il peso è "leggero", non il valore).

Nota: la domanda su cosa mostri il museo del Piano 0 (intero catalogo o sottoinsieme curato)
è risolta da DEC-063 — criterio misto, metriche più preferiti — e non è più aperta qui; la
soglia esatta delle metriche resta una domanda aperta specifica, registrata in
`floor-zero.md`. La domanda su cosa succeda a un contenuto del museo che diventa una
Reliquia è risolta da DEC-085 (vedi "Reliquie nel museo del Piano 0" sopra) e non è più
aperta qui. La domanda se i contenuti fallback-usati entrano comunque nel catalogo è
risolta da DEC-103 — sì, i contenuti curati incontrati entrano marcati con origine `curato`
— e non è più aperta qui; resta come gap di implementazione esplicito, non come domanda di
design (vedi "Stato di implementazione" sopra).

## Idee future (experimental)

Sezione dedicata a idee parcheggiate, non requisiti attuali.

- **DEC-018 — Lobby online custom con contenuti sbloccati**: l'idea di lobby online
  personalizzate che usano i contenuti sbloccati dal catalogo di un giocatore è un'idea
  futura, non un requisito attuale. Non è stata progettata nel dettaglio e non deve essere
  trattata come funzionalità pianificata.
- **DEC-018/DEC-045 — Funzioni del Catalogo anche nel Piano 0/museo**: portare l'enciclopedia
  consultabile, i preferiti e la spesa punti anche dentro il museo del Piano 0, non solo nel
  Catalogo del menu principale, è un'idea futura, non un requisito attuale.

## Scenari

**Scenario: un contenuto approvato entra nel catalogo permanente**
- Given un giocatore sta giocando una run singleplayer,
- When un oggetto generato durante quella run raggiunge lo stato approvato-per-run,
- Then quel contenuto entra nel catalogo permanente del profilo, disponibile per essere
  esposto nel museo del Piano 0 e nei pool di run future.

**Scenario: permadeath non corrompe il profilo**
- Given un giocatore perde una run singleplayer (salute a zero, permadeath),
- When la run termina,
- Then il catalogo dei contenuti già approvati durante quella run e i punti già guadagnati
  restano acquisiti nel profilo, senza alcuna perdita del profilo stesso.

**Scenario: nessun potenziamento permanente altera la run successiva**
- Given un giocatore ha accumulato un catalogo ampio e molti punti spesi in sblocchi,
- When avvia una nuova run,
- Then il personaggio parte con la stessa potenza di base di qualunque altra run: cambia solo
  l'ampiezza dei pool di contenuti generati disponibili, non la forza di partenza.

**Scenario: sblocchi disattivati in una gara asincrona**
- Given un giocatore ha sbloccato contenuti extra spendendo punti guadagnati in singleplayer,
- When entra in una gara asincrona su un manifest di run condiviso,
- Then quegli sblocchi non si applicano: la run competitiva usa i pool di base, non ampliati.

**Scenario: punti da entrambi i canali persistono insieme**
- Given un giocatore ha guadagnato punti base dal risultato di una run e bonus da prove
  specifiche completate nella stessa run (DEC-027),
- When la run termina,
- Then entrambi i canali confluiscono nello stesso totale di punti sblocco persistente nel
  profilo, senza distinzione nel saldo spendibile.

**Scenario: le tre funzioni del Catalogo (DEC-045)**
- Given un giocatore con un profilo che contiene contenuti catalogati, alcuni segnati come
  preferiti, e punti sblocco disponibili,
- When apre il Catalogo dal menu principale,
- Then trova insieme l'enciclopedia consultabile dei contenuti incontrati, i preferiti
  segnati e la possibilità di spendere i punti per sbloccare contenuti nei pool delle run
  future.

**Scenario: i preferiti pesano leggermente sulle proposte future**
- Given un giocatore ha segnato alcuni contenuti come preferiti nel Catalogo,
- When l'IA genera le proposte per una run futura,
- Then quei contenuti preferiti ricevono un peso leggermente maggiore nel pool, senza che la
  loro comparsa sia garantita.

**Scenario: sospensione a metà combattimento**
- Given un giocatore in una stanza di combattimento con nemici parzialmente sconfitti,
- When sospende la run e poi la riprende con "Continua",
- Then la stanza riparte dall'ingresso con tutti i nemici ripristinati, mentre il resto
  della run (piani completati, build, risorse) resta esattamente come al momento della
  sospensione (DEC-050).

**Scenario: sospensione nel Piano 0**
- Given un giocatore nel Piano 0 che ha già scelto tema e personaggio,
- When sospende la run e poi la riprende,
- Then rientra direttamente nel Piano 0 con le stesse scelte già fatte, coerente con lo
  stato salvato `FloorZero` (DEC-050). *(Gap di implementazione esplicito, WP17: oggi la
  sospensione è disponibile solo in una run vera; vedi la nota sopra e
  `docs/engineering/known-issues.md`.)*

**Scenario: la sospensione si consuma alla ripresa (WP17, DEC-050)**
- Given un giocatore ha sospeso una run e la riprende con "Continua",
- When muore poco dopo il rientro,
- Then la run si chiude come qualunque altra sconfitta: il menu principale non offre più
  "Continua", perché la sospensione è stata consumata al momento della ripresa e non esiste
  più alcun punto di salvataggio a cui tornare.

**Scenario: una sospensione incompatibile viene ignorata (WP17)**
- Given sul disco esiste un file di sospensione scritto da una versione diversa del gioco,
  o troncato,
- When il giocatore apre il menu principale,
- Then la voce "Continua" non compare, il file viene ignorato e il menu resta interamente
  utilizzabile, senza alcun errore tecnico mostrato.

**Scenario: abbandonare cancella la sospensione (WP17, DEC-089)**
- Given un giocatore ha una run sospesa,
- When abbandona una run in corso, effettua un reroll, o sceglie "Nuova run" e conferma,
- Then la sospensione viene cancellata: nessuna delle tre lascia in piedi un rientro in una
  run che il giocatore ha già chiuso.

**Scenario: un preferito ha precedenza nel museo**
- Given un giocatore ha segnato un contenuto come preferito nel Catalogo (DEC-045),
- When le metriche di quel contenuto scendono sotto la soglia di promozione automatica,
- Then il contenuto resta comunque esposto nel museo del Piano 0, perché il preferito ha la
  precedenza sulla promozione per metriche (DEC-063, fonte unica in `floor-zero.md`).

**Scenario: medaglie della Daily persistono nel profilo**
- Given un giocatore partecipa alla Classificata giornaliera pubblica e ottiene un
  piazzamento,
- When la sessione termina,
- Then una medaglia o cornice cosmetica entra nel profilo persistente, visibile nel profilo e
  nel museo, senza alcun punto sblocco aggiuntivo (DEC-064, fonte unica in
  `08-multiplayer-and-competition.md`).

**Scenario: un contenuto supera la riconvalida dopo un aggiornamento**
- Given un aggiornamento del gioco riconvalida l'intero catalogo persistente,
- When un contenuto già approvato-per-run in passato supera di nuovo la pipeline di
  validazione,
- Then resta giocabile e sbloccabile come prima, senza spostarsi tra le Reliquie (DEC-069).

**Scenario: un contenuto diventa una Reliquia dopo un aggiornamento**
- Given un aggiornamento del gioco riconvalida l'intero catalogo persistente,
- When un contenuto già approvato-per-run in passato non supera più la pipeline di
  validazione,
- Then il contenuto si sposta nella sezione Reliquie del Catalogo: resta consultabile con
  nome, sprite e storia, ma non è più giocabile né sbloccabile nei pool delle run future
  (DEC-069).

**Scenario: abbandono e reroll contano come sconfitta per i punti (DEC-082)**
- Given un giocatore ha maturato punti sblocco durante una run in corso,
- When abbandona volontariamente la run da `ExitConfirm` oppure effettua un reroll da
  `Gameplay`,
- Then i punti sblocco maturati fino a quel momento vengono conteggiati con la stessa
  riduzione standard della sconfitta (DEC-041), senza una categoria intermedia, e il
  catalogo e le statistiche si aggiornano comunque con quanto incontrato.

**Scenario: le sette categorie del Catalogo sono tutte consultabili (DEC-083)**
- Given un giocatore ha incontrato almeno un layout stanza generato e un tipo di colpo
  generato durante le sue run,
- When apre l'enciclopedia del Catalogo,
- Then trova anche Layout e Colpi tra le sette categorie consultabili (Oggetti, Nemici,
  Boss, Personaggi, Mondi, Layout, Colpi), ciascuna con scheda propria come per le altre
  categorie, non solo record interni.

**Scenario: una Reliquia preferita resta esposta nel museo (DEC-085)**
- Given un contenuto è esposto nel museo del Piano 0 come preferito del giocatore,
- When un aggiornamento del gioco lo trasforma in una Reliquia,
- Then il contenuto resta esposto nel museo, con la scheda che segnala che è una Reliquia e
  senza più la possibilità di provarlo in arena.

**Scenario: una Reliquia promossa solo per metriche esce dal museo (DEC-085)**
- Given un contenuto è esposto nel museo del Piano 0 solo per promozione automatica da
  metriche, senza essere un preferito del giocatore,
- When un aggiornamento del gioco lo trasforma in una Reliquia,
- Then il contenuto esce automaticamente dal museo, pur restando consultabile nel Catalogo
  come Reliquia.

**Scenario: l'abbandono passa da RunResults con i punti visibili (DEC-089)**
- Given un giocatore ha maturato punti sblocco durante una run in corso,
- When abbandona volontariamente la run da `ExitConfirm`,
- Then la run si chiude passando da `RunResults` come una sconfitta, con i punti ridotti
  visibili lì.

**Scenario: il reroll accredita in silenzio e resta consultabile nel Catalogo (DEC-089)**
- Given un giocatore ha maturato punti sblocco durante una run in corso,
- When effettua un reroll da `Gameplay`,
- Then i risultati vengono saltati, i punti ridotti si accreditano in silenzio senza passare
  da `RunResults`, e restano comunque consultabili nel Catalogo.

**Scenario: un contenuto curato incontrato entra nel Catalogo marcato come tale (DEC-103)**
- Given un giocatore incontra durante una run un contenuto curato, ad esempio impiegato come
  fallback,
- When quel contenuto viene registrato nel catalogo persistente,
- Then compare nell'enciclopedia del Catalogo marcato con origine `curato` (tassonomia in
  `content/content-taxonomy.md`), accanto ai contenuti composto/variato/nuovo già presenti.
