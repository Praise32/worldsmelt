---
id: gd-system-synergies
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Doppio binario delle sinergie: implicite/automatiche tra oggetti compatibili e fusione esplicita nella stanza dedicata."
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
3. priorità di categoria;
4. fallback visivo e meccanico sicuro.

## Limiti di leggibilità

Il limite al numero di strati/trasformazioni visive che una sinergia o fusione può
introdurre è definito una sola volta in
[Combat and Projectiles — Budget di leggibilità](combat-and-projectiles.md#budget-di-leggibilità):
non riformulato qui. Una sinergia può aumentare spettacolarità ed effetti, ma non può mai
nascondere posizione del personaggio, proiettili nemici, hitbox percepita, direzione
dell'attacco o causa del danno.

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
