---
id: gd-system-save-meta
status: approved
owner: design
last_reviewed: 2026-07-19
summary: "Cosa persiste tra le run (DEC-015): catalogo contenuti, museo del Piano 0, punti singleplayer per sblocchi a doppio canale (DEC-027, dettaglio in rewards-and-economy.md); niente potenziamenti permanenti del personaggio. Il Catalogo del menu principale ha tre funzioni: enciclopedia — sette categorie canoniche, Oggetti/Nemici/Boss/Personaggi/Mondi/Layout/Colpi (DEC-083) —, preferiti, spesa punti (DEC-045). Il museo è curato in modo misto: metriche più preferiti del Catalogo, che hanno la precedenza e non escono mai (DEC-063). Le medaglie/cornici cosmetiche della Classificata giornaliera persistono nel profilo, fuori dall'economia dei punti (DEC-064). La run può essere sospesa in ogni momento: al rientro la stanza corrente riparte dall'ingresso con i nemici ripristinati, il resto della run riprende esattamente com'era (DEC-050). A ogni aggiornamento del gioco il catalogo viene riconvalidato: ciò che fallisce diventa una Reliquia, consultabile ma non più giocabile né sbloccabile; nel museo una Reliquia resta esposta solo se era lì come preferito, altrimenti ne esce automaticamente (DEC-069, DEC-085)."
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
- `generated-content-validation.md`: solo i contenuti approvati-per-run entrano nel catalogo
  (i fallback-usati restano un caso da chiarire, vedi Domande aperte).
- `rewards-and-economy.md`: fonte del doppio canale di guadagno dei punti sblocco (DEC-027:
  punti base dal risultato della run più bonus da prove specifiche); questo documento
  descrive solo cosa persiste, non come si guadagna.
- `ui/main-menu.md`: la voce Catalogo del menu principale apre la schermata a tre funzioni
  descritta qui (DEC-045).
- `ui/results-and-leaderboards.md`: fonte unica della distinzione vittoria/sconfitta e della
  riduzione dei punti alla sconfitta (DEC-041); questo documento registra solo che abbandono
  e reroll rientrano nel bucket sconfitta ai fini dei punti (DEC-082).
- `05-game-states-and-flow.md` e `ui/navigation-map.md`: la regola "Continua" rientra nello
  stato salvato; questo documento aggiunge solo il dettaglio del ripristino della stanza
  corrente (DEC-050).
- `08-multiplayer-and-competition.md`: fonte unica delle ricompense cosmetiche della Daily
  (medaglie/cornici) che persistono qui nel profilo (DEC-064).
- `../content/narrative-tone.md`: la cornice del crogiolo (DEC-067) dà senso al termine
  "Reliquie" usato qui per i contenuti che non superano la riconvalida (DEC-069).

## Regole per contenuti generati

- Un contenuto entra nel catalogo permanente solo se ha raggiunto lo stato approvato-per-run
  in una run singleplayer.
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

Default v1 sulla domanda aperta "i contenuti fallback-usati contano?" (vedi
"Domande aperte residue" sotto): **no** — una run interamente su contenuto di
ripiego (nessuna generazione, o generazione mai avviata) non scrive alcun
record. La domanda resta aperta per una futura decisione di design; questo è
solo il comportamento scelto per la prima fetta implementativa.

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

- Se i contenuti fallback-usati durante una run singleplayer entrano comunque nel catalogo, o
  solo i contenuti approvati-per-run (vedi anche `generated-content-validation.md`).
- Il tasso esatto di guadagno (punti base e bonus, DEC-027) e di spesa dei punti di
  meta-progressione (non deciso).
- Peso esatto che i preferiti aggiungono alle proposte future dell'IA (DEC-045 fissa solo
  che il peso è "leggero", non il valore).

Nota: la domanda su cosa mostri il museo del Piano 0 (intero catalogo o sottoinsieme curato)
è risolta da DEC-063 — criterio misto, metriche più preferiti — e non è più aperta qui; la
soglia esatta delle metriche resta una domanda aperta specifica, registrata in
`floor-zero.md`. La domanda su cosa succeda a un contenuto del museo che diventa una
Reliquia è risolta da DEC-085 (vedi "Reliquie nel museo del Piano 0" sopra) e non è più
aperta qui.

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
  stato salvato `FloorZero` (DEC-050).

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
