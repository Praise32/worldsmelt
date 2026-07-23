---
id: ref-generazione-piani-stanze
title: Generazione dei piani e delle stanze (algoritmo, per la fase 3c)
domain: references
status: approved
authority: supporting
owner: design
summary: >-
  Algoritmo esatto di generazione piani di Isaac (griglia 9x8, formula stanze, BFS con 4 regole, boss=ultimo vicolo cieco, segreta a punteggio) mappato su un piano di modifica per WorldGenerateFloorMap.
last_reviewed: 2026-07-22
last_verified_commit: fe27f6d
topics: [generazione-piani, bfs, vicoli-ciechi, boss-placement, stanza-segreta, world-c, isaac-reference]
related: []
supersedes: []
source_files: []
---

# Generazione dei piani e delle stanze (algoritmo, per la fase 3c)

Documento di riferimento. Descrive **l'algoritmo esatto** con cui *The Binding of
Isaac* costruisce la pianta di un piano (griglia, conteggio stanze, crescita a
coda/BFS, vicoli ciechi, piazzamento delle stanze speciali, forme multi-cella) e
lo **mappa concretamente** sul modello di mondo di Melting Run
(`src/world/world.c`, `src/core/game_types.h`). L'obiettivo di fase 3c e' far
"sentire" i nostri piani come quelli di un roguelike a stanze, tenendo separato
cio' che genera il **C** (la pianta) da cio' che descrive il **LLM** (il
contenuto delle stanze, in JSON, validato per raggiungibilita').

Gli algoritmi, le regole e le formule qui sotto sono **fatti** e sono citati
dalle fonti. I nomi di gioco, i testi e le tabelle-ID protetti **non** vengono
copiati: vedi la nota "Confini di IP" in fondo.

---

## 1. La griglia del piano (fatto)

Un piano e' una **griglia rettangolare di celle**, ciascuna cella = una stanza
1x1 (le stanze grandi occupano piu' celle, vedi §7).

- Dimensioni: **9 celle in larghezza x 8 celle in altezza** (9x8).
- Indirizzamento a due cifre usato dal motore: **cifra delle unita' = x**, **cifra
  delle decine = y**. Es. angolo alto-sinistra = `01`, basso-destra = `79`.
- La stanza di partenza sta al **centro**, cella `35` (x=5, y=3).
- I quattro vicini di una cella `c` si raggiungono con gli offset **`+10`, `-10`
  (giu'/su in y) e `+1`, `-1` (destra/sinistra in x)**.

Fonte: BorisTheBrave, *Dungeon Generation in Binding of Isaac*.

---

## 2. Numero di stanze del piano (formula, fatto)

Due formule circolano, coerenti fra loro a meno di arrotondamenti:

**(a) Forma "classica" (BorisTheBrave), quella richiesta da questo task:**

```
numRooms = random(2) + 5 + level * 2.6
```

dove `random(2)` restituisce 0 o 1 e `level` e' la profondita' del piano (1-based).
Al livello 1 da' ~7-8 stanze, e cresce di ~2-3 stanze per livello.

**(b) Forma equivalente della wiki (Rebirth):**

```
numRooms = 3.33 * floorDepth + (5 o 6)         (max 20)
XL:       numRooms = 1.8 * (3.33 * floorDepth + 5..6)   (max 45)
```

con varianti additive per modalita' (Hard mode +2..3, alcune maledizioni +4). Le
due formule concordano sull'ordine di grandezza; usiamo **(a)** come riferimento.

Il conteggio e' un **bersaglio**: la crescita (§3) si ferma appena lo raggiunge, e
poi la pianta viene **validata** (§4). Se non e' valida si **ricomincia da capo**.

Fonti: BorisTheBrave; wiki.gg *Level Generation*.

---

## 3. Crescita della pianta: BFS a coda con regole di accettazione (algoritmo, fatto)

Il cuore dell'algoritmo e' una **visita in ampiezza (BFS)** a partire dalla stanza
di partenza. Ogni cella "espansa" tenta di generare stanze nelle 4 direzioni
cardinali; ogni candidato passa una serie di **filtri**. Pseudocodice fedele:

```
queue      = [35]                 # coda BFS, parte dalla stanza iniziale
placed     = 1                    # stanze gia' piazzate (la start conta)
target     = random(2) + 5 + level * 2.6
endRooms   = []                   # vicoli ciechi (celle senza figli)

while queue non vuota:
    cur = queue.dequeue()
    haFigli = false

    for dir in [ +10, -10, +1, -1 ]:      # giu', su, destra, sinistra
        n = cur + dir

        # --- REGOLE DI ACCETTAZIONE / RIFIUTO ---
        if n e' gia' occupata:            continue      # (1) niente sovrapposizioni
        if n ha 2+ vicini gia' pieni:     continue      # (2) evita "grumi" / box 2x2
        if placed >= target:              continue      # (3) quota raggiunta
        if random() < 0.5:                continue      # (4) salta col 50%

        # --- ACCETTATO ---
        segna n come occupata (kind provvisorio = stanza normale)
        placed += 1
        queue.enqueue(n)
        haFigli = true

    if not haFigli:
        endRooms.append(cur)          # cur e' un vicolo cieco
```

Le quattro regole, nell'ordine in cui contano:

1. **Cella libera.** Non si sovrascrive mai una cella gia' assegnata.
2. **Meno di 2 vicini gia' pieni.** Se il candidato toccherebbe **2 o piu'**
   stanze esistenti viene scartato. E' la regola che evita i **blocchi 2x2** e
   tiene la pianta ramificata "ad albero" invece che a macchia compatta.
3. **Quota non superata.** Appena `placed` raggiunge `target`, nessun nuovo
   candidato viene accettato (la coda si svuota senza crescere).
4. **Lancio di moneta al 50%.** Anche un candidato per il resto valido viene
   saltato con probabilita' 1/2. E' la sorgente principale di varieta': rende le
   piante irregolari invece che a griglia piena.

Una cella che **non genera alcun figlio** (`haFigli == false`) e' un **vicolo
cieco** (`endRoom`). L'ordine in cui gli endRoom entrano nella lista e' l'ordine
BFS, quindi cresce grosso modo per **distanza dalla partenza**: gli ultimi
elementi della lista sono i piu' lontani.

Fonte: BorisTheBrave.

---

## 4. Validazione e retry (fatto)

Finita la crescita, la pianta e' **controllata**:

- deve avere **esattamente** il numero di stanze bersaglio;
- la **stanza boss non puo' essere adiacente alla stanza di partenza**.

Se un controllo fallisce, si **butta via tutto e si rigenera da zero**. Questo
"generate-and-test" e' piu' semplice di un algoritmo che garantisce la validita'
per costruzione, e in pratica converge in pochi tentativi.

Fonte: BorisTheBrave.

---

## 5. Piazzamento del boss e delle stanze speciali (algoritmo + ordine, fatto)

Le stanze speciali **non nascono durante la BFS**: vengono innestate **dopo**, nei
**vicoli ciechi** (`endRooms`), con un ordine preciso.

- **Regola generale:** le speciali occupano i vicoli ciechi, assegnati **dal piu'
  lontano dalla partenza al piu' vicino**. Se i vicoli ciechi finiscono, non si
  generano altre speciali.
- **Numero minimo di vicoli ciechi** per garantire spazio: **5 di base**, +1 sui
  piani non-primi, +1 sui piani XL (numeri Rebirth).

**Ordine di piazzamento** (dalla wiki, semplificato ai tipi che ci interessano):

1. **Boss** — sempre presente; piazzato leggendo **l'ultimo elemento della lista
   `endRooms`**, cioe' il vicolo cieco piu' lontano dalla partenza.
2. **Negozio** — garantito nei primi capitoli.
3. **Tesoro** — garantito nei primi capitoli.
4. Altre (sacrificio, maledizione, mini-boss, biblioteca, sfida, arcade...) — con
   probabilita' varie, ciascuna in un endRoom casuale ancora libero. Es. la
   stanza sacrificio compare ~1 volta su 7 (o ~1 su 3 se sei a vita piena).

**Stanza segreta** (regola a parte, non usa gli endRoom): si sceglie una **cella
vuota** e la si valuta con un **punteggio**:

- peso di partenza **10-14**;
- **-3** se ha solo **2** stanze vicine;
- **-6** se ha solo **1** stanza vicina;
- viene scelta la **cella col punteggio piu' alto**.

In pratica la segreta finisce quasi sempre **circondata da 3+ stanze** (massimizza
il punteggio ed e' "nascosta" dietro pareti bombabili), e **non** viene messa
accanto a boss, altra segreta o super-segreta. La descrizione "storica"
equivalente e': cerca a caso una cella vuota adiacente ad **almeno 3 stanze** e non
adiacente ad alcun vicolo cieco; se dopo **300** tentativi non la trova allenta i
criteri, e dopo **600** li allenta ancora.

Fonti: BorisTheBrave (boss = ultimo endRoom; segreta 300/600); wiki.gg (ordine
delle speciali; pesi 10/-3/-6; min. vicoli ciechi).

---

## 6. Porte e connessioni (fatto)

Due stanze **adiacenti** sulla griglia sono collegate da una **porta**. Il numero
di porte di una stanza = numero di vicini occupati. I vicoli ciechi hanno **una
sola** porta (per questo ospitano le speciali: un solo ingresso e' facile da
"chiudere"/tematizzare). Le stanze grandi (§7) hanno porte su piu' bordi.

---

## 7. Forme multi-cella (fatto)

Nelle versioni Rebirth/DLC esistono **stanze grandi** che occupano piu' celle:

- **2x1** e **1x2** (doppie orizzontali/verticali),
- **2x2** (quadrata grande),
- **forme a L** (in tutte le rotazioni),
- **corridoi** stretti.

Regole chiave:

- Una stanza grande **conta come una sola stanza** ai fini della quota e della
  regola "niente box 2x2": e' per questo che capita di vedere due stanze normali
  sullo stesso lato di una grande.
- La generazione **non** produce **blocchi 2x2 di stanze 1x1** (la regola (2) di
  §3 lo impedisce fra le normali; le grandi 2x2 sono oggetti singoli espliciti).
- Con le stanze grandi la crescita cicla **su tutte le uscite** della forma, non
  solo sulle 4 direzioni di una singola cella; una grande viene tentata a caso e
  **rifiutata se non c'e' spazio**.

Fonti: BorisTheBrave (aggiunte Rebirth); wiki.gg *Rooms*/*Level Generation*.

---

## 8. Mappatura su Melting Run (cosa cambiamo)

### 8.1 Stato attuale (`src/world/world.c`)

Oggi il nostro `WorldGenerateFloorMap` e' una **passeggiata casuale (random walk)**,
non una BFS:

- griglia **`GRID_SIZE x GRID_SIZE` = 5x5** (`game_types.h`), non 9x8;
- `targetRooms = 7 + game->floor` (lineare +1 per piano), non `random(2)+5+level*2.6`;
- un puntatore `(x,y)` fa passi casuali; ogni cella nuova diventa `ROOM_COMBAT`;
- il **boss** e' l'**ultima cella scritta** dalla walk (`lastX/lastY`) — un
  concetto vicino a "vicolo cieco piu' lontano" ma **non garantito**: puo' finire
  adiacente alla partenza, e non c'e' validazione/retry;
- `WorldPlaceSpecialRoom` piazza **tesoro** e **negozio** su una cella **vuota
  adiacente a una stanza esistente** (prima adiacenza trovata, 120 tentativi) —
  quindi le nostre speciali oggi **non** stanno in un vicolo cieco, ma spuntano
  dove capita;
- `WorldLinkRooms` apre le `doors[4]` fra celle adiacenti esistenti — questo e'
  gia' corretto e va tenuto;
- niente stanze grandi, niente segreta, niente vicoli ciechi espliciti.

`RoomState` (`game_types.h`) ha gia' `exists/visited/cleared/rewardTaken/kind/
doors[4]`: e' abbastanza per una pianta alla Isaac, non serve cambiarne la forma
per la struttura di base.

### 8.2 Modifiche proposte per "sentirlo" alla Isaac

1. **Griglia piu' grande.** Portare `GRID_SIZE` a una griglia rettangolare stile
   9x8 (o almeno 8x8 se vogliamo restare quadrati). Con 5x5 le piante sono troppo
   piccole per avere veri vicoli ciechi e ramificazioni. Sostituire i due usi di
   `GRID_SIZE/2` per la partenza con il centro reale della nuova griglia.
2. **Conteggio stanze con la formula.** Rimpiazzare `7 + game->floor` con
   `GameRngRange(&rng,0,1) + 5 + (int)roundf(game->floor * 2.6f)`. Tenere un
   clamp massimo (es. 20) come Rebirth.
3. **BFS al posto della random walk.** Riscrivere `WorldGenerateFloorMap` con la
   coda di §3: start in coda, per ogni cella estratta prova le 4 direzioni con le
   regole **(1) libera, (2) <2 vicini pieni, (3) quota, (4) 50%**; le celle senza
   figli vanno in una lista `endRooms`. La regola (2) e' quella che ci regala la
   ramificazione "ad albero" che oggi ci manca.
4. **Validazione + retry.** Avvolgere la generazione in un loop: se `placed !=
   target` **oppure** il boss finirebbe adiacente alla start, rigenera. Mettere un
   guard massimo di tentativi (es. 50) e un fallback deterministico all'ultimo
   giro, cosi' non blocchiamo mai il gioco.
5. **Boss = ultimo vicolo cieco.** Assegnare `ROOM_BOSS` all'**ultimo** elemento
   di `endRooms` (il piu' lontano dalla partenza), non all'ultima cella della
   walk. Questo rende deterministica la regola "boss lontano dalla start" che oggi
   e' solo casuale.
6. **Speciali nei vicoli ciechi.** Riscrivere `WorldPlaceSpecialRoom` perche'
   peschi da `endRooms` (dal piu' lontano al piu' vicino) invece che da una cella
   vuota qualsiasi: tesoro e negozio in due vicoli ciechi distinti dal boss.
   L'attuale logica "cella vuota adiacente" resta utile solo per la **segreta**.
7. **(Opzionale) Stanza segreta.** Aggiungere `ROOM_SECRET` a `RoomKind` e
   piazzarla con il punteggio di §5 (peso 10-14, -3 con 2 vicini, -6 con 1),
   scegliendo la cella vuota col punteggio massimo e non adiacente al boss/start.
   Richiede una chiave/bomba per entrare (abbiamo gia' `player.keys`/`bombs`).
8. **(Opzionale, piu' avanti) Stanze grandi.** Servirebbe che una `RoomState`
   sappia di appartenere a una forma multi-cella (un `id` di stanza condiviso fra
   celle). E' la modifica piu' invasiva: consigliata **dopo** che la BFS di base
   e' stabile. Per la fase 3c basta 1x1 + vicoli ciechi + speciali ordinate.

Nessuna di queste tocca la parte gia' buona: `WorldLinkRooms`, la logica porte in
`WorldTryEnterRoom`/`WorldHandleTransitions`, il lock delle porte
(`GameRoomIsLocked`) e lo spawn contenuti (`WorldSpawnRoomContents`).

---

## 9. Divisione dei compiti: cosa genera il C, cosa descrive il LLM

Principio: **il C possiede la topologia** (quali celle esistono, come sono
collegate, dove sta il boss). **Il LLM possiede il contenuto** (cosa c'e' *dentro*
una stanza), espresso in **JSON** e **validato** prima dell'uso.

**Genera il C (deterministico, verificabile):**

- la pianta del piano: celle, `kind` strutturale (START/BOSS/TREASURE/SHOP/...),
  vicoli ciechi, porte (`doors[4]`), validazione di raggiungibilita';
- la garanzia che **start -> boss** sia raggiungibile (per costruzione BFS: ogni
  cella nasce da un vicino gia' collegato, quindi il grafo e' connesso);
- gli spawn "meccanici" gia' presenti in `WorldSpawnRoomContents` (conteggio
  nemici che scala col piano, ricompense, uscita) come **default/fallback**.

**Descrive il LLM (contenuto, in JSON):**

- per ogni stanza di combattimento: **quali** `EnemyKind` e quanti, layout
  suggerito, eventuale mini-tema; per il boss: composizione delle **parti** (vedi
  il "boss part system" in `docs/APPUNTI.md`);
- tema/estetica della stanza coerente col `Theme` del piano
  (`content.floors[floor-1].theme`);
- eventuale testo di ambientazione (breve), tenuto separato dalla meccanica.

**Il validatore (C) rifiuta o corregge il JSON del LLM se:**

- cita una cella/porta che non esiste nella pianta generata dal C;
- mette nemici in una stanza non di combattimento, o un boss fuori dalla stanza
  boss;
- supera i limiti di conteggio (`MAX_ENEMIES`, ecc.) o usa `EnemyKind` sconosciuti;
- rompe la **raggiungibilita'**: la stanza deve restare completabile (nemici
  spawnabili entro i bordi giocabili `ROOM_X..ROOM_RIGHT`, nessuna stanza chiusa
  senza modo di ripulirla e sbloccare le porte).

In caso di JSON invalido si **ripiega sui default C** di `WorldSpawnRoomContents`,
esattamente come la pipeline oggi ripiega sulla mini-VM quando il Lua e' assente
(stesso principio di robustezza gia' adottato in fase 3a). Cosi' un LLM che
"allucina" non puo' mai produrre un piano irraggiungibile o un crash: al peggio
otteniamo il piano generato-proceduralmente puro.

---

## 10. Riepilogo delle costanti (fatti citati)

| Cosa | Valore | Fonte |
|---|---|---|
| Griglia | 9 x 8 celle | BorisTheBrave |
| Stanza iniziale | cella 35 (centro) | BorisTheBrave |
| Offset vicini | +10, -10, +1, -1 | BorisTheBrave |
| N. stanze | `random(2) + 5 + level*2.6` | BorisTheBrave |
| N. stanze (Rebirth) | `3.33*depth + 5..6`, max 20 | wiki.gg |
| Rifiuto candidato | occupato / 2+ vicini pieni / quota / 50% | BorisTheBrave |
| Vicolo cieco | cella senza figli -> lista endRooms | BorisTheBrave |
| Validazione | conteggio esatto; boss non adiacente a start; altrimenti retry | BorisTheBrave |
| Boss | ultimo endRoom (piu' lontano) | BorisTheBrave |
| Ordine speciali | dal vicolo cieco piu' lontano al piu' vicino | wiki.gg |
| Min. vicoli ciechi | 5 (+1 non-primo, +1 XL) | wiki.gg |
| Segreta (punteggio) | peso 10-14; -3 con 2 vicini; -6 con 1 | wiki.gg |
| Segreta (300/600) | allenta criteri dopo 300 e 600 tentativi | BorisTheBrave |
| Forme grandi | 2x1, 1x2, 2x2, L, corridoi; contano come 1 stanza | wiki.gg |

---

## Confini di IP

Questo documento riusa **algoritmi, regole, formule e costanti numeriche** di
generazione dei livelli: sono **fatti** e possiamo reimplementarli liberamente
(griglia 9x8, `random(2)+5+level*2.6`, la BFS con le sue 4 regole di
accettazione, boss nell'ultimo vicolo cieco, pesi 10/-3/-6 della segreta, forme
2x2/L). Le fonti sono citate.

**Non** riusiamo e **non** copiamo: i **nomi** protetti di stanze/nemici/oggetti/
personaggi di *The Binding of Isaac*, i suoi **testi** (flavour, prosa della
wiki copiata alla lettera), gli **sprite/asset grafici**, e le **tabelle-ID**
prese di peso. Nel nostro codice usiamo nomi **generici e originali**
(`ROOM_COMBAT`, `ROOM_TREASURE`, `ROOM_SECRET`, `ENEMY_CHASER`, ...) e descriviamo
i **pattern** ("un nemico che insegue il giocatore"), mai l'asset di marca. Il
gioco puo' quindi essere venduto: la struttura del piano e' una funzione
matematica reimplementata in proprio, non una copia di contenuti protetti.
