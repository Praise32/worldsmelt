---
id: gd-system-synergies
title: Synergies
domain: design
status: approved
authority: canonical
owner: design
summary: "Doppio binario delle sinergie: implicite/automatiche tra oggetti compatibili e fusione esplicita nella stanza dedicata. Conflitti senza priorità esplicita si risolvono con RNG derivato dal seed di run (DEC-161, riproducibilità ancora requisito e non stato attuale finché vale DEC-141); il risultato di sinergie e fusioni ha un budget di potenza dedicato, più alto del singolo (DEC-162)."
last_reviewed: 2026-07-27
last_verified_commit: 8210480
topics: [sinergie, fusione, build, visual-language, priorità, DEC-161, DEC-162]
related: []
supersedes: []
source_files: [src/gameplay/synergies.c]
---

# Synergies

## Intento per il giocatore

Una sinergia deve far sentire la build come più della somma delle sue parti: due o più
componenti compatibili producono un risultato più specifico, riconoscibile sia nel
comportamento sia a colpo d'occhio.

## Definizione

Una sinergia esiste quando due o più componenti producono un risultato più specifico
della semplice somma delle statistiche.

## Doppio binario

Le sinergie in questo progetto seguono due binari distinti e complementari.

### (a) Implicite / automatiche

Si attivano automaticamente quando due oggetti compatibili convivono nella build: nessuna
azione dedicata del giocatore, nessuna stanza speciale, nessun consumo di risorse. La
combinazione stessa, una volta presente, produce l'effetto.

Le sinergie implicite attualmente definite nel codice (`src/gameplay/synergies.c`, righe
55–106) sono gli esempi canonici concreti di questo binario:

| Nome | Segnali combinati | Descrizione |
|---|---|---|
| Volo Infilzante | inseguimento/homing + perforazione | i colpi curvano e attraversano la fila |
| Rimbalzo Instabile | rimbalzo + esplosione | rimbalzi più lunghi, impatti più cattivi |
| Sciame | divisione + cadenza rapida | un colpo in più a ogni sparo |
| Gelo Perpetuo | rallentamento + cadenza rapida | tanti colpi, nemici sempre lenti |
| Morso Vorace | furto di vita + colpi giganti | colpi enormi che rubano vita più spesso |
| Arco Voltaico | tipo di colpo che salta bersaglio + rallentamento | la scarica salta a un nemico in più |

Nota di design: **Arco Voltaico** dimostra che le sinergie implicite non sono solo
coppie fisse di tratto+tratto. Anche il tipo di colpo *nuovo*, inventato dal modello per
il piano, può portare un segnale che partecipa a una coppia di sinergia — non solo gli
oggetti dell'inventario.

Queste 6 sono gli esempi canonici oggi presenti; il set di sinergie implicite può
crescere con nuove coppie generate, purché rispettino la stessa regola generale (vedi
sotto).

### (b) Fusione esplicita

La meccanica-firma del progetto: il giocatore, nella stanza di fusione, consuma
deliberatamente due oggetti più un catalizzatore di fusione e ottiene un oggetto nuovo
generato dall'IA che eredita comportamento e presentazione da entrambi. I dettagli della
stanza, del consumo e dell'oggetto generato risultante vivono in
[item-fusion.md](item-fusion.md) e non sono riformulati qui.

## Regola generale (vale per entrambi i binari)

Ogni sinergia o fusione deve cambiare **sia** il comportamento **sia** la presentazione
visiva. Un cambiamento solo meccanico o solo estetico non è una sinergia valida.

## Livelli di una sinergia

1. **Statistica:** modifica valori.
2. **Comportamentale:** cambia traiettoria, cadenza, bersaglio o interazione.
3. **Visiva:** fonde aspetti di proiettili, personaggio ed effetti.
4. **Sistemica:** interagisce con risorse, stanze o nemici.

## Regola di composizione visiva

La presentazione visiva di una sinergia o di una fusione si costruisce componendo gli
**strati** definiti in [Visual Language](../content/visual-language.md): silhouette,
materiale, colore funzionale, particelle, animazione, segnale d'impatto, indicatore sul
personaggio. Nessun vocabolario diverso va introdotto qui.

Esempio (stessa idea del vecchio esempio astratto "tratto appuntito + tratto
incendiario", riscritta nei termini dei 7 strati): un oggetto che porta un tratto di
perforazione e uno di combustione compone così la trasformazione visiva del proiettile e
del personaggio:

- **silhouette:** più affusolata, coerente con la perforazione;
- **materiale:** superficie che richiama calore/combustione;
- **colore funzionale:** tinta associata all'effetto di fuoco;
- **particelle:** scintille o braci lungo la traiettoria;
- **animazione:** movimento del proiettile che comunica velocità/penetrazione;
- **segnale d'impatto:** flash o scia coerente con perforazione + combustione;
- **indicatore sul personaggio:** un accenno visivo coerente con entrambi i tratti,
  senza duplicare l'intera trasformazione sul personaggio.

## Priorità

Quando più effetti competono sulla stessa proprietà:

1. trasformazioni esplicite (fusione);
2. regole di sinergia implicita definite;
3. **conflitto senza priorità esplicita (DEC-161):** quando nessuna delle due regole sopra
   risolve il conflitto, non esiste un ordine fisso di "priorità di categoria" — l'esito si
   decide con un numero pseudo-casuale derivato dal **seed della run**, stabile per tutta
   quella run: la stessa coppia di effetti in conflitto produce sempre lo stesso esito
   nella stessa run (riproducibile con lo stesso seed), ma l'esito può differire da run a
   run. Il prerequisito di DEC-141 (RNG di gameplay derivato dal seed di run, non più
   `time(NULL)`) è **soddisfatto**: la garanzia di riproducibilità è oggi mantenuta dal
   codice (vedi "Stato di implementazione" sotto);
4. fallback visivo e meccanico sicuro.

### Stato di implementazione — conflitto senza priorità (DEC-161)

`SynergyConflictAPrevails(runSeed, keyA, keyB)` (`src/gameplay/synergies.h`/`.c`) è la
funzione che decide un conflitto senza priorità esplicita: uno splitmix64 puro, seminato da
`Game.runSeed` (DEC-141) più una chiave di coppia ordinata (min/max, così l'esito non dipende
da quale dei due argomenti viene passato per primo) e una costante di dominio propria.
Deliberatamente **non** consuma `Game.rng` (lo stream che avanza a ogni sparo/estrazione):
farlo darebbe un esito diverso ad ogni interrogazione nella STESSA run, il contrario della
stabilità richiesta. Applicazione concreta oggi: quando più oggetti posseduti portano lo
stesso segnale di sinergia (es. tre oggetti col trait rallentamento), non esiste una regola
che dica quale dei due "conta" per la coppia — prima di DEC-161 vinceva semplicemente il
primo trovato nell'ordine dell'inventario (una priorità di fatto, mai dichiarata da nessun
documento); ora la scelta passa da `SynergyConflictAPrevails`, stessa run → stesso
candidato, run diverse → può differire. Non cambia mai **se** la sinergia si forma (basta un
candidato qualunque), solo quale oggetto ne detta la rarità (`SynergyRarityScale`). Le 6
regole canoniche della tavola sopra non competono mai fra loro sulla stessa proprietà (i
loro contributi si sommano/moltiplicano, non si sovrascrivono): questo meccanismo è quindi
oggi esercitato solo dal caso "più candidati per lo stesso segnale", ma resta la base pronta
per quando una sinergia implicita generata introdurrà un vero conflitto proprietà-contro-
proprietà. Verificato da `--script-items-test` (`make test-script`, test AU): stabilità a
stesso `runSeed` su 100 ricalcoli/2 costruzioni indipendenti, esito diverso su almeno uno fra
40 seed diversi, sinergia sempre accesa indipendentemente da quale candidato vince.

## Limiti di leggibilità

Il limite al numero di strati/trasformazioni visive che una sinergia o fusione può
introdurre è definito una sola volta in
[Combat and Projectiles — Budget di leggibilità](combat-and-projectiles.md#budget-di-leggibilità):
non riformulato qui. Una sinergia può aumentare spettacolarità ed effetti, ma non può mai
nascondere posizione del personaggio, proiettili nemici, hitbox percepita, direzione
dell'attacco o causa del danno.

## Budget di potenza del risultato (DEC-162)

Oltre al budget di leggibilità (visivo, sopra), sinergia implicita e fusione esplicita
condividono anche un **budget di potenza** dedicato al risultato: più alto del `budget di
potenza` di un singolo oggetto (vedi campo obbligatorio in
[Items, Pools and Rarity](items-pools-and-rarity.md)), perché il risultato deve valere
meccanicamente la combinazione (o il costo, per la fusione) che lo produce. Questo budget
dedicato è verificato nella fase di validazione dei contenuti generati (stato `simulato`,
vedi [Generated Content Validation](generated-content-validation.md)) come ogni altro
contenuto. Il suo valore esatto resta `draft`, da definire col playtest (stile DEC-019); i
dettagli specifici della fusione esplicita vivono in [item-fusion.md](item-fusion.md).

### Stato di implementazione — budget dedicato lato gioco (DEC-162)

Il canale statistico (`SynergiesStatBonus`, canale A) passa ora anche da un tetto dedicato,
`ScriptItemsClampSynergyResultDelta` (`src/script/script_items.c`), applicato SOLO quando
almeno una sinergia è attiva, DOPO l'applicazione dei moltiplicatori di sinergia e PRIMA del
tetto globale (`ScriptItemsClampStats`, invariato, gira comunque sempre dopo) e della curva
di rendimenti decrescenti sul danno. Riusa la stessa meccanica del tetto per-oggetto (delta
rispetto al valore pre-sinergia, clampato a una frazione della statistica di base), con
`SCRIPT_ITEMS_SYNERGY_RESULT_DELTA_FRACTION = 1.20` — il doppio del tetto per-oggetto più
largo (0.60, leggendario) — **default proposto dall'implementazione (stile DEC-019)**: il
documento fissa solo che il tetto esiste ed è più alto di quello per-oggetto, non il numero
esatto. Log su stderr quando il clamp riduce davvero qualcosa ("log chiaro" richiesto dal
task che ha introdotto questo controllo): degradazione silenziosa per il giocatore, non per
chi guarda i log. Mai un crash: il tetto globale e la curva restano comunque le ultime reti
di sicurezza, indipendentemente da questo budget dedicato. Verificato da `--script-items-test`
(`make test-script`, test AV): tre coppie leggendarie simultanee (sei oggetti) su un
giocatore già gonfiato da cinque stat-up leggendari vengono clampate a un valore misurabilmente
più basso di quello che si otterrebbe senza il budget dedicato, restando comunque dentro la
banda globale e finito (mai NaN).

### Stato di implementazione — il risultato a tempo di generazione (DEC-162)

Il documento chiede che questo budget sia «verificato nella fase di validazione dei
contenuti generati come ogni altro contenuto» (sopra). Il controllo che gira lì è in
`tools/melting-gen` (`gen_validate.c`, `NormalizeShot`) e riguarda il **risultato**, non il
singolo oggetto:

- **Quando si applica.** Solo dove il contenuto generato dichiara da sé una sinergia. La
  tavola delle sinergie condiziona su *segnali*, e i soli segnali che un contenuto generato
  può dichiarare per conto proprio sono quelli del tipo di colpo del piano: `chain > 0`
  (Arco Voltaico) e `pierce > 0` (segnale previsto dal vocabolario, nessuna regola lo usa
  ancora — coperto lo stesso, perché il controllo è conservativo e una regola futura non
  deve passare in silenzio). Un tipo di colpo che dichiara uno di questi segnali è, per sua
  stessa dichiarazione, metà di una coppia canonica: l'altra metà è un qualunque oggetto del
  pool, che il pool contiene per costruzione. **Default proposto dall'implementazione
  (stile DEC-019)**: limitare il controllo al contenuto che *dichiara* il segnale, invece di
  applicarlo a ogni tipo di colpo, è una scelta dell'implementazione — il documento non dice
  quale contenuto sia "una sinergia dichiarata". Da confermare col playtest.
- **Cosa verifica.** Che il risultato stia nel budget di **leggibilità**, con la stessa
  soglia del singolo (DEC-146, proxy di copertura schermo in
  [Combat and Projectiles](combat-and-projectiles.md#budget-di-leggibilità)): quel budget non
  si allarga mai per le sinergie (vedi "Limiti di leggibilità" sopra), mentre è il budget di
  *potenza* a essere dedicato e più alto, ed è garantito a runtime da
  `ScriptItemsClampSynergyResultDelta`. Il risultato stimato è il tipo di colpo **più** i
  bonus di canale B che la coppia aggiunge ai proiettili del giocatore: in pratica il
  pallettone in più di `SynergiesExtraPellets` (l'unico bonus discreto che cambia quanti
  segnali stanno a schermo insieme; `pierce`/`bounce`/`chain` sono potenza, non ingombro).
- **Perché serve un controllo dedicato.** Questi bonus non passano da alcun tetto di
  leggibilità a runtime: `CombatFireShot` (`src/gameplay/combat.c`) somma il pallettone di
  sinergia ai pallettoni del tipo di colpo e taglia solo a 5, ben oltre il massimo del tipo
  di colpo. Un tipo di colpo può quindi stare sotto la soglia da solo e sforarla appena si
  accende la coppia che esso stesso ha dichiarato — e a runtime è tardi: la coppia si è già
  formata. Conseguenza dello sforamento: la normale catena di fallback (l'intero tipo di
  colpo generato viene scartato e sostituito con quello procedurale del piano), mai una
  riparazione sul posto, esattamente come per il controllo DEC-146 del singolo.
- **Verificato da** `make test-script` (test AW): la costante del generatore combacia con
  la tavola vera di `synergies.c`, i tre tipi procedurali su cui si ricade superano sia il
  controllo del singolo sia quello del risultato (la catena di fallback non può rimbalzare
  su sé stessa) ed esiste un tipo di colpo, dentro le bande, che passa il primo controllo e
  viene scartato dal secondo — cioè il controllo non è inerte. E da `make test-gen`, che lo
  esercita end-to-end su un manifest vero (`tests/melting-gen/bad/shot-over-budget.json`).

### Tentativo scartato — allargare le bande della mini-VM (2026-07-27)

Una revisione di `gen_validate.c` aveva invece aggiunto, per il caso di un **singolo oggetto
generato** che dichiara già da solo i due trait di una delle 5 coppie canoniche trait+trait,
un allargamento dedicato del tetto dei campi `a`/`b` della sua mini-VM. Tolto nella stessa
revisione, perché **inerte su due fronti**: il motore (`src/gameplay/script_vm.c`) clampa
comunque quei campi alle bande base in esecuzione (un manifest con `a`/`b` allargati si
comporta esattamente come uno senza, dichiarando però valori che il gioco taglia in
silenzio); e soprattutto una sinergia non si accende **mai** da un oggetto solo
(`SignalPresent`/`excludeItem` in `src/gameplay/synergies.c`: «una sinergia è fra DUE
oggetti diversi»), quindi quel caso non produce nemmeno il risultato di cui allargava il
budget. Se un domani questo budget dovrà avere un effetto misurabile sui campi mini-VM, la
banda base del motore va alzata per prima: un allargamento lato generatore che la precede
resta inerte per costruzione.

## Informazione al giocatore

La schermata build deve spiegare quali componenti hanno prodotto la sinergia (implicita
o da fusione), senza necessariamente rivelare formule interne o dettagli tecnici di
generazione (vedi anche la regola generale sul non mostrare dettagli tecnici, definita
in `06-ai-content-generation-model.md`).

## Interazioni

- Con i campi obbligatori dell'oggetto: il campo `valore di sinergia` (vedi
  [Items, Pools and Rarity](items-pools-and-rarity.md)) segnala se un oggetto è un buon
  candidato per sinergie implicite e/o per la fusione esplicita.
- Con gli attivi e i passivi: entrambe le categorie possono portare segnali che
  partecipano a sinergie implicite (vedi [Active Items](active-items.md) e
  [Passive Items](passive-items.md)); gli stat-up, essendo modifiche dirette senza
  comportamento nuovo, tipicamente non portano segnali di sinergia comportamentale.

## Regole per contenuti generati

Una sinergia implicita generata (nuova coppia di segnali oltre alle 6 canoniche) deve
comunque rispettare: la regola generale (comportamento + presentazione), i livelli
sopra elencati, il budget di leggibilità e gli strati unici del vocabolario visivo. Una
fusione esplicita produce sempre un oggetto che passa dagli stati di validazione
descritti in [Generated Content Validation](generated-content-validation.md).

## Casi limite

- Due oggetti che porterebbero segnali di sinergia ma sono dichiarati incompatibili tra
  loro: l'incompatibilità (vedi [Items, Pools and Rarity](items-pools-and-rarity.md))
  prevale, nessuna sinergia si forma.
- Una sinergia implicita e una fusione insistono sulla stessa proprietà: si applica la
  priorità sopra (trasformazioni esplicite/fusione prima delle sinergie implicite).
- Il tipo di colpo generato dal modello per il piano cambia o scade prima che una
  sinergia implicita basata su di esso possa formarsi: nessuna sinergia si applica finché
  il segnale non è di nuovo presente.

## Fallback

Se una sinergia o una fusione generata non supera la validazione, si applica la regola
unica descritta in [Generated Content Validation](generated-content-validation.md): non
riformulata qui.

## Non-obiettivi

- Questo documento non definisce i numeri di bilanciamento delle sinergie (moltiplicatori,
  bonus) né i dettagli della stanza di fusione: vivono altrove ([item-fusion.md](item-fusion.md)).
- Non introduce un vocabolario visivo alternativo a quello di
  [Visual Language](../content/visual-language.md).

## Domande aperte residue

- Quante sinergie implicite generate (oltre alle 6 canoniche) è ragionevole avere attive
  contemporaneamente prima che il budget di leggibilità imponga una semplificazione.
- Come la schermata build presenta una sinergia nata da un tipo di colpo generato (non
  un oggetto dell'inventario) accanto a quelle nate da coppie di oggetti.
- Valore esatto del budget di potenza dedicato al risultato di sinergie/fusioni (DEC-162
  fissa solo che esiste ed è più alto del budget del singolo oggetto, non il numero).

## Scenari verificabili

### Scenario 1 — sinergia implicita canonica

Given un giocatore con un oggetto che porta il tratto di rimbalzo e un oggetto che porta
il tratto di esplosione,  
When entrambi sono presenti nella build,  
Then si forma automaticamente "Rimbalzo Instabile", cambiando sia il comportamento del
proiettile sia la sua presentazione visiva, senza alcuna azione dedicata del giocatore.

### Scenario 2 — sinergia da tipo di colpo generato

Given un piano il cui tipo di colpo generato dal modello salta di nemico in nemico, e un
giocatore con un oggetto che rallenta i nemici colpiti,  
When il tipo di colpo è attivo e l'oggetto è nella build,  
Then si forma "Arco Voltaico": la scarica salta a un nemico in più e lascia i nemici
colpiti rallentati, con un segnale visivo coerente su colpo e personaggio.

### Scenario 3 — fusione esplicita

Given un giocatore nella stanza di fusione con almeno due oggetti nella build e un
catalizzatore di fusione disponibile,  
When sceglie due oggetti da fondere e conferma,  
Then i due oggetti vengono consumati insieme al catalizzatore e il giocatore riceve un
nuovo oggetto generato che eredita comportamento e presentazione visiva da entrambi
(dettagli completi in [item-fusion.md](item-fusion.md)).

### Scenario 4 — priorità in conflitto

Given un oggetto risultante da fusione esplicita e una sinergia implicita che
modificherebbero la stessa proprietà del proiettile,  
When entrambe le condizioni sono presenti nella build,  
Then prevale la trasformazione esplicita della fusione, secondo l'ordine di priorità
dichiarato.

### Scenario 5 — conflitto senza priorità esplicita risolto dal seed di run

Given più oggetti posseduti portano lo stesso segnale di sinergia, senza che nessuna regola
di design dichiari quale dei candidati "conta" per la coppia,  
When il sistema deve decidere quale oggetto prevale,  
Then l'esito è determinato da un numero pseudo-casuale derivato dal seed della run
(DEC-161, `SynergyConflictAPrevails`): rigiocando la stessa run con lo stesso seed si ottiene
sempre lo stesso esito, ma run diverse (seed diversi) possono risolvere lo stesso conflitto
in modo diverso, senza mai cambiare SE la sinergia si forma. Scenario **verificabile sul
codice** (vedi "Stato di implementazione" sopra): il prerequisito di DEC-141 (RNG di
gameplay derivato dal seed di run) è soddisfatto — verificato da `--script-items-test`
(`make test-script`, test AU).

### Scenario 6 — budget di potenza dedicato al risultato di una sinergia

Given una sinergia implicita generata combina due tratti in un effetto più potente della
somma dei singoli budget di potenza degli oggetti sorgente,  
When il contenuto passa per la validazione,  
Then il controllo verifica il risultato contro il budget di potenza dedicato, più alto del
budget di un singolo oggetto (DEC-162), e non contro il budget di un singolo oggetto.
