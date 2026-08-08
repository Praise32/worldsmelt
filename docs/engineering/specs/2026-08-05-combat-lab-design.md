---
id: eng-spec-combat-lab
title: "Combat Lab: demo di debug del combattimento con attacchi generati da Gemma"
domain: engineering
status: approved
authority: canonical
owner: engineering
summary: >-
  Spec della demo giocabile di debug (tools/procedural-combat-demo): arena raylib
  con la ScriptSandbox vera, attacchi nemici e armi del player generati live da
  Gemma via melting-gen --attacks, pool su disco con hot-reload, resa smooth.
last_reviewed: 2026-08-08
last_verified_commit: e0bf48f
topics: [spec, combat, lua, sandbox, generazione, demo, debug]
related: [eng-adr-002, eng-adr-003, eng-spec-local-llm-linux]
supersedes: []
source_files:
  - tools/procedural-combat-demo/main.c
  - tools/procedural-combat-demo/demo_script_api.c
  - src/script/script_sandbox.c
---

# Spec: Combat Lab — combattimento con attacchi generati da Gemma

Data: 2026-08-05
Stato: approvata dal proprietario in chat (sessione 05/08), decisioni Q&A registrate sotto
Origine: `procedural-combat-demo` (prova fatta sul PC Windows senza GPU, comportamento
del modello simulato con 5 script scritti a mano; zip esterno, ora portata nel repo)

## 1. Contesto e obiettivo

La prova `procedural-combat-demo` ha dimostrato il layer corretto per attacchi e
"animazioni di gameplay" procedurali: Lua (nella **ScriptSandbox vera** del gioco)
compone un alfabeto di verbi geometrici (`telegraph_arc`, `emit_ring`, `emit_orbit`,
`emit_beam`, `melee_sweep`, `capture_radius`, `release_echoes`, …) e il C valida,
clampa, applica collisioni e disegna. Nella prova gli script erano scritti a mano
"come li scriverebbe Gemma"; su questa macchina (GPU, modelli locali presenti) il
modello li genera per davvero.

Obiettivo: un **ambiente di debug giocabile** dove il proprietario prova il
combattimento (movimento, mira, danno, pattern nemici, armi) con contenuti
inventati live da Gemma. Da questa demo nascerà la base del gameplay; stanze,
bilanciamento, oggetti e sinergie arriveranno dopo, una cosa alla volta.

Decisioni del proprietario (chat 05/08):

- **Generazione live in demo**: un tasto chiede nuovi pattern mentre si gioca; se
  un attacco non piace, si rigenera al volo. Lo script corrente resta attivo
  finché il nuovo non è validato.
- **Scope**: Gemma genera sia i pattern dei **nemici** sia le **armi del player**.
- **Input**: seed casuale + brief opzionale (`generated/combat-lab/brief.txt`,
  se non vuoto entra nel prompt).
- **Resa**: modalità **smooth** di default (né pixel-perfect né ibrida); visuale e
  camera della prova invariate. Vincoli asset del gioco (32×32, palette Fucina)
  esplicitamente NON applicati: ordine del proprietario, questa demo ne è esente.
- Gli sprite CC0 della prova (OpenGameArt, provenienza in
  `tools/procedural-combat-demo/assets/ASSET_PROVENANCE.md`) restano quelli.

## 2. Architettura (ADR-002 rispettata)

Tre componenti, nessuna modifica al runtime del gioco:

1. **Demo** `tools/procedural-combat-demo/` — eseguibile raylib autonomo, target
   `make combat-lab`, compila `main.c + demo_script_api.c +
   src/script/script_sandbox.c + src/core/game_math.c` contro le librerie Linux
   già usate dal gioco. Non linka mai llama.cpp: chiede la generazione a
   `melting-gen` come **processo figlio** e legge solo file validati.
2. **Generatore** `melting-gen --attacks` — nuova modalità in
   `tools/melting-gen/gen_attacks.{c,h}`: prompt → Gemma (sessione llama.cpp già
   esistente) → validazione nella sandbox vera con l'API della demo →
   ritenta (max 3, errore rimandato al modello) → scrive su disco solo ciò che
   passa. Un'invocazione genera un **lotto** (default 3 script) per ammortizzare
   il caricamento del modello.
3. **Protocollo file** `generated/combat-lab/` — `enemy/*.lua` e `weapon/*.lua`
   (nome `NNN_seedS.lua`, header commentato con seed/brief/data), `brief.txt`
   opzionale. La demo fa polling della cartella (~1 s) e integra a caldo i file
   nuovi; l'HUD mostra lo stato del processo figlio (attivo/esito/errore).

## 3. Gameplay dell'arena

Arena continua (stessa stanza e camera della prova, player ~34 px su 720p):

- **Player**: WASD/frecce per muovere, mira col mouse, click sinistro = arma
  base sempre disponibile (colpo semplice, rate fisso). Arma **generata**
  equipaggiabile dal pool; gira in una sandbox propria (player-owned).
- **Nemico**: un attore alla volta, HP visibile, colpibile dai proiettili del
  player; alla morte respawn con il pattern successivo del pool. Pattern
  corrente in una sandbox propria (enemy-owned). I 5 script della prova restano
  nel repo come pool curato di fallback/riferimento (`scripts/curated/`).
- **Due sandbox attive insieme** (nemico + arma player), ognuna col proprio
  `DemoScriptApiState`, quote e kill switch indipendenti (già per-sandbox).
- Danno, collisioni, i-frame, particelle: la logica della prova, riusata.

Estensione API (solo sandbox player-owned, documentata nel cheat-sheet):
`fire_held()` e `special_pressed()` — letture dell'input reale, così un'arma
generata risponde al mouse invece di auto-sparare. Per le sandbox nemiche le due
funzioni esistono ma ritornano sempre false (nessun ramo speciale nel prompt).

## 4. Controlli

| Tasto | Azione |
|---|---|
| WASD/frecce + mouse | movimento, mira, fuoco (click sx) |
| G / H | genera lotto nuovo: attacchi nemico / armi player |
| N / M | cicla pool: nemico / arma (istantaneo, da disco) |
| B | attiva/disattiva il brief E rilegge `brief.txt` (stato in HUD; usato dal prossimo G/H) |
| R | reset arena (stessi script) |
| 1 / 2 / 3 | pixel / smooth / ibrido (default smooth) |
| Spazio | pausa |

## 5. Generazione e validazione

- Prompt: `tools/melting-gen/prompts/attack_system.txt` (cheat-sheet dell'
  alfabeto: firme, conteggio degli argomenti, chiamate d'esempio corrette,
  quote 48/64 per tick, coordinate arena, "telegraph prima del danno") +
  `attack_user.txt` (kind enemy/weapon, seed, brief se presente, forma
  d'attacco principale che ruota col seed). Inglese, come da DEC-052. Budget
  prompt sorvegliato con lo stesso meccanismo di `GEN_LUA_PROMPT_BYTE_CEILING`.
- **Niente few-shot completi** (correzione dal primo giro reale, 05-06/08): il
  cheat-sheet portava i due script curati `spider_arc`/`squid_reload` per
  intero e Gemma-3-4B li ricopiava alla lettera — 8 script su 8, su semi
  diversi, erano `spider_arc.lua` byte per byte. Al loro posto: due
  **scheletri** commentati (la macchina a fasi senza contenuto) più l'elenco
  in una riga delle cinque idee già nel pool curato, così il modello sa cosa è
  già preso senza avere il codice da copiare. Con gli scheletri le copie sono
  sparite del tutto. Rete di sicurezza in C: un gate anti-copia fra
  validazione e scrittura (`gen_attacks.c`) respinge uno script che, tolti
  commenti e spazi, sta dentro il cheat-sheet o ricalca un file già presente
  in `generated/combat-lab/<kind>/`, e rimanda l'errore al modello.
- `n_predict` dedicato (768: gli `on_tick` con macchina a stati sono più lunghi
  degli script oggetto).
- Validazione (`GenAttackValidate`, senza modello, riusabile da `--attack-check`):
  carica nella sandbox vera con l'API demo VERA (`demo_script_api.c`, compilata
  dentro melting-gen); deve definire `on_tick`; 120 tick di prova con stato
  plausibile senza disabilitare la sandbox; almeno un comando d'attacco entro i
  120 tick. Niente validatori di dominio (corridoio sicuro, telegraph minimo,
  budget danno, "un'arma deve leggere `fire_held()`") in questa fase:
  arriveranno quando il combattimento soddisfa il proprietario (restano
  elencati nel README della demo).
- Errore di rifiuto **azionabile**: è l'unico testo che il ritento rimanda al
  modello, quindi porta con sé il tick, il numero di riga, la riga di codice
  colpevole e la firma esatta della funzione che sbaglia. Il messaggio Lua vero
  si recupera rigiocando il solo tick fallito in una sandbox nuova
  (`ScriptSandboxCallVoid` conserva solo la classificazione "errore a
  runtime"). Senza questo, il ritento ripeteva lo stesso errore tre volte.
- Fallimento dopo i ritenti: il lotto scrive gli script riusciti; se zero, la
  demo lo segnala in HUD e il pool resta com'è. Mai un file non validato su disco.

## 6. Test e chiusura

- `--attack-check <file>`: valida un file senza modello; i 5 script curati
  passano (corpus in `make test-gen` o target dedicato).
- `make test` resta verde (incluso il guard `nm` di ADR-002 in test-script.sh).
- Un giro reale con Gemma (`make combat-lab` + G/H) documentato in HANDOFF.
- Fuori scope, rimandato: validatori di dominio, integrazione nel runtime del
  gioco (`on_enemy_update`), metadati pivot/tip/muzzle per sprite arbitrari,
  audio degli attacchi.

## 7. v3 — Budget di bilanciamento e UI debug (playtest 08/08)

Feedback del proprietario dopo il primo giro reale con Gemma: (1) nemici e armi
generati "lanciano tantissimi proiettili" — sbilanciati rispetto ai 5 pattern
curati; (2) non si capiva quando si stava guardando contenuto **curato** o
**generato**, ne' quando cambiava nemico/arma; (3) a volte un'entita' "sembrava
non vedersi". Tre correzioni, tutte host-side in `tools/procedural-combat-demo/
main.c` (l'API v2 di `demo_script_api.h` resta **congelata**: firme, costanti
pubbliche e semantica invariate — i tetti vivono nel consumo dei comandi, non
nella sandbox).

Una revisione adversariale dell'08/08 ha bocciato la prima stesura di questa
sezione e le correzioni sono state riassorbite qui dentro, dove la decisione
cambiava: telegrafi **esentati** da ogni budget (7.1), debug UI **fuori**
dall'arena (7.2), sprite per **indice** di pool (7.2), floor di luminanza
**reale** (7.3), proprietario corretto per `release_echoes` (7.1), troncamento
uniforme dei burst circolari (7.1), annuncio del lotto legato all'esito (7.2).

### 7.1 Budget di bilanciamento

Tre meccanismi indipendenti, tutti in `DemoConsumeCommands`/`DemoNewProjectile`,
tutti a **troncamento silenzioso** (mai un errore Lua, mai una sandbox disabilitata):

| Meccanismo | Nemico | Arma player | Note |
|---|---|---|---|
| Cap colpi ATTIVI (proprietario) | 56 | 40 | Somma su `DemoProjectile` con lo stesso flag `hostile`; oltre il tetto la mezzaluna di EMIT_ARC/RELEASE_ECHOES si ferma a meta' burst, mentre EMIT_RING/EMIT_ORBIT chiedono prima quanti colpi passano (`DemoShotAllowance`) e li ridistribuiscono sull'intero giro. |
| Token bucket proiettili/s | 30/s | 25/s | Capacita' = rate, ricarica continua, si parte a **meta'** capacita' (mai un burst pieno al primo tick). |
| Cooldown per categoria | emit_beam 0.35s · melee_sweep 0.25s · capture_radius 0.8s · ring/orbit/echoes 0.12s | idem | Per entita' (nemico/arma), non per singolo tipo di comando: l'orbit conta come ring. |

**I TELEGRAFI NON PAGANO NULLA** (decisione 08/08, correzione di una v3 che li
metteva nella stessa categoria della `emit_*` corrispondente): `telegraph_arc` e
`telegraph_beam` non spawnano niente e non fanno danno, quindi sono esclusi da
ogni gate — ne' bucket ne' cooldown — e restano soggetti alle sole quote
per-tick dell'API congelata (64 comandi visuali). Condividere la categoria beam
fra `telegraph_beam` e `emit_beam` rendeva **inerte** ogni pattern sano con un
windup piu' corto di 0.35s fra l'annuncio e il colpo: il telegrafo prendeva il
cooldown, l'`emit_beam` lo trovava chiuso e il raggio che fa danno non nasceva
mai (misurato: script con windup 0.30s, 30s simulati, 18 cicli, 18 `emit_beam`
scartati, zero danno al player). Peggiorava con il telegrafo ridisegnato ad ogni
tick, pattern che Gemma scrive volentieri, perche' ricaricava il cooldown
all'infinito. Il cooldown vive ora solo fra emissioni che spawnano davvero.

Il gate del cooldown confronta col timestamp dell'**ultima emissione accettata**
della categoria, aggiornato **una sola volta a fine tick** (`DemoThrottleCommitTick`)
e non ad ogni comando: cosi' piu' emissioni della stessa categoria nello STESSO
tick (es. `emit_ring`+`emit_orbit` nella stessa transizione di fase) passano
tutte — solo lo spam FRA un tick e l'altro viene tagliato.

**Proprietario di un `release_echoes`**: i colpi ereditano l'`hostile` della
sandbox che ha emesso il comando, non piu' `false` fisso (correzione 08/08: un
nemico sparava colpi del player e li faceva pagare al budget dell'ARMA, che
vedeva salire "scartati" per colpa del nemico). La riserva di echi resta una
risorsa del player: la riempie solo un `capture_radius` dell'arma e la consuma
solo un `release_echoes` dell'arma. Un nemico non ha riserva, quindi il suo
`release_echoes` vale per il ventaglio che lo script dichiara (`count`, gia'
clampato a 24 dall'API): farlo cadere a un colpo solo lo avrebbe reso un dud
silenzioso — i prompt elencano `release_echoes` fra gli attacchi veri di un
nemico e il validatore lo accetta, esattamente il difetto del cooldown beam
condiviso col telegrafo.

Un contatore "scartati" per proprietario somma **tutti e tre** i meccanismi
(cap, bucket, cooldown): il proprietario deve vedere "Gemma sta sforando", non
distinguere quale dei tre tetti ha fermato un colpo.

**Taratura**: i 5 script curati (`scripts/curated/`) sono stati analizzati
staticamente (rate di emissione per ciclo di fase) e fatti girare via
`--capture` (450 frame, ~30s simulati, 2 cicli completi dei 3 pattern nemico
curati). Nessuno tocca il tetto: il picco piu' alto e' la mezzaluna di
`spider_arc` (13 proiettili in un colpo solo, media ~6.9/s sul ciclo) e il
ring+orbit di `glass_moth` (13 colpi, media ~6/s) — entrambi sotto meta' del
bucket nemico (30/s) e lontanissimi dal cap (56). Un test manuale con uno
script deliberatamente abusivo (`emit_ring` di 20 colpi ad ogni tick, ~1200
colpi/s tentati) conferma che il sistema interviene davvero: "scartati" sale a
centinaia al secondo e i colpi vivi smettono di crescere, mentre i pattern
curati restano a "scartati: 0/s" per tutta la loro durata. Il numero di colpi
vivi a regime **non e' una proprieta' del sistema**: e' l'equilibrio fra il rate
del bucket e la vita dei colpi di QUEL pattern (misurato ~30-32 con quello
script, ~53 su 56 con uno che spara colpi a vita lunga). L'unica garanzia e' il
cap; i budget contengono gli eccessi di Gemma, non castrano il contenuto sano.

### 7.2 UI debug

**L'arena e' intoccabile.** `DEMO_ROOM` (58,88 → 1222,654) e' la visuale di
gioco originale e nessun elemento di debug puo' entrarci. Il debug vive in due
bande fuori dal rettangolo (`DEMO_HUD_TOP_HEIGHT`/`DEMO_HUD_BOTTOM_Y`/
`DEMO_HUD_BANNER_Y` in `main.c`):

- **Banda alta, 0..88px** (esattamente `DEMO_ROOM.y`): i due pannelli NEMICO
  (sinistra) e ARMA (destra), su righe compattate che chiudono a y=85. La
  prima v3 usava una barra opaca da 130px: copriva i primi 42px dell'arena,
  cioe' tagliava lo sprite del nemico appena comparso (bordo alto y≈103) e
  nascondeva **del tutto** la sua barra vita (y≈91). Chi aggiunge una riga
  accorcia le altre, non la banda.
- **Banda bassa, 658..720px** (sotto il bordo inferiore dell'arena, y=654):
  vita del player e contatori globali, poi la riga GEN **da sola** (e' la piu'
  lunga: fino a 192 byte di stato piu' il brief — prima si sovrapponeva
  all'avviso asset), poi la legenda dei tasti.
- L'**avviso asset** e' abbreviato e allineato a destra sulla prima riga della
  banda alta; il messaggio per esteso resta su stderr all'avvio.

Contenuto dei pannelli: nome script (senza `.lua`), tag colorato `[CURATO]`
(azzurro) o `[GEMMA]` (arancio) o `[BASE]` (pistola, ne' l'uno ne' l'altro),
seed se generato (letto dal nome file `NNN_seed<seed>.lua`, convenzione di
`gen_attacks.c`), posizione nel pool, stato Lua (`ON`/`KO`/`FALLBACK`), colpi
attivi/cap e "scartati: N/s".

- **Banner grande temporaneo** (2.5s, dissolvenza negli ultimi 0.4s): striscia
  a tutta larghezza nella **banda bassa**, che copre la legenda dei tasti
  finche' e' viva (l'unica informazione fissa che si puo' perdere per 2.5s
  senza danno). Nella prima v3 stava a y=140, cioe' esattamente sopra il nemico
  appena comparso: copriva la cosa che annunciava. Si accende al cambio
  nemico/arma (N/M/SHIFT+N/SHIFT+M) e quando un lotto G/H **finisce bene**,
  es. `"NEMICO -> GEMMA: 003_seed4160589630 (seed 4160589630)"`. Colore coerente
  col tag dell'evento. Non si accende sul respawn automatico dopo la morte (per
  non spammare in un fight lungo) ne' in `--capture` (nessun tasto/lotto li', il
  PNG dello smoke test resta deterministico).
- L'annuncio del lotto segue l'**esito del processo figlio**, non il delta di
  conteggio del pool: con il pool gia' a `DEMO_POOL_MAX_ENTRIES` il delta e' 0
  anche per una generazione perfettamente riuscita, e il lotto atterrava in
  silenzio. Tre messaggi: voci nuove (`+N`), pool pieno (con l'invito a
  riavviare), nessuno script nuovo sul disco. Un lotto fallito non fa banner:
  resta sulla riga GEN, che riporta anche l'exit code.
- **Indicatore generazione in corso**: la riga GEN pulsa in arancio e mostra uno
  spinner testuale (`| / - \`) mentre `gen.pid != 0`; sempre spento in
  `--capture` (nessun figlio parte mai li').
- **SHIFT+N/SHIFT+M**: scorrono il pool all'indietro (nuovo in v3, prima solo
  avanti). Legenda tasti in basso aggiornata di conseguenza.
- **Sprite nemico per indice**: `spriteKind = enemyPool->current & 3`
  (`DemoEnemyBeginPattern`), non piu' ciclato ad ogni respawn e non piu' un hash
  del nome file — `djb2(fileName) & 3` mandava `spider_arc`,
  `snail_calligrapher` e `squid_reload` sullo stesso sprite, cioe' due nemici
  curati su tre con la stessa faccia e il cambio con N impercettibile. L'indice
  garantisce sprite **diversi a voci adiacenti**, che e' la proprieta' utile
  mentre si scorre il pool, e resta stabile per script perche' l'ordine del pool
  e' deterministico (curati nell'ordine cablato, generati appesi in coda in
  ordine alfabetico e mai riordinati).

### 7.3 Visibilita' garantita

- Raggio di **disegno** minimo `max(3.0, radius)` e alpha minimo 0.55 mentre
  il colpo e' vivo (mai sulla hitbox fisica, solo su cio' che finisce a
  schermo) in `DemoDrawProjectile`.
- `DemoEnsureVisibleColor`: floor di luminanza **vero** (canale massimo portato
  esattamente a 150) applicato a `DemoVisualColor` — rete di sicurezza per un
  `VIS_*` futuro con una tinta scura; i 6 colori curati attuali sono gia' tutti
  chiari, la funzione non li tocca. I tre canali si scalano dello stesso
  fattore, cosi' la tinta resta identica; il nero puro, che non ha tinta da
  preservare, diventa il grigio 150. La prima v3 mixava verso il bianco con un
  margine di 0.6 e ritornava il colore **invariato** per `maxChannel <= 0`: non
  raggiungeva mai la soglia dichiarata (149 → 149.4) e saltava proprio il caso
  peggiore. Codice, commento e questa riga dicono ora la stessa cosa.
- **Fallback texture**: `DemoDrawTextureOrPlaceholder` sostituisce un asset
  stock non valido con un rettangolo magenta bordato (mai un'entita'
  invisibile) e alza `assetWarningShown` (riga sticky in HUD, non un
  messaggio a tempo). `DemoLoadAssets` non e' piu' fatale: un asset singolo
  rotto non spegne piu' l'intera demo (avviso su stderr all'avvio, poi
  ripiego per-texture a runtime).
- **Ordine di disegno**: nemico e player si disegnano **dopo** proiettili/
  archi/raggi/particelle (prima erano sotto): non spariscono mai sotto un
  accumulo di colpi. Il nemico morto mantiene un contorno ad alpha minima
  (0.30) per tutto il `corpseFade`, non solo il tint che sfuma col resto dello
  sprite.

### 7.4 Prompt Gemma

`tools/melting-gen/prompts/attack_system.txt` dichiara i budget v3 (cap colpi
attivi, colpi/s, cooldown per tipo) come vincoli di design espliciti in un
nuovo paragrafo "BALANCE BUDGETS", con un richiamo nella lista "Rules": il
modello deve progettare **dentro** i limiti, non scoprirli da un valore di
ritorno (i comandi oltre budget sono accettati dalla sandbox e producono
silenziosamente nulla a schermo). Nessun esempio di codice Lua aggiunto (regola
anti-copia, DEC storico su questo stesso file): solo prosa vincolante, come il
resto del cheat-sheet. Non toccata la grammatica GBNF (i budget non riguardano
JSON strutturato).

Il paragrafo dichiara anche i due punti che nessun controllo automatico copre
(08/08): (1) il cooldown vale sulle sole chiamate che fanno danno, i
`telegraph_*` sono gratuiti; (2) il **validatore gira sulla sandbox sola** — il
suo "almeno un attacco vero entro 120 tick" e' un pavimento, non la promessa che
il pattern atterri a schermo nell'arena, perche' i budget vivono in `main.c` e
il validatore non li simula. Con i telegrafi esentati il caso peggiore e'
chiuso: la primissima emissione di ogni categoria passa sempre (sentinella
`DEMO_COOLDOWN_NEVER`), il bucket parte a meta' capacita' (≥15 colpi per il
nemico) e il cap parte da zero, quindi un primo attacco onesto non puo' finire a
zero effetti. Resta **un solo buco residuo**, dichiarato nel prompt come vincolo
di design e non chiuso nel codice: telegrafi e forme reali condividono lo stesso
pool di slot (`DEMO_MAX_BEAMS` 48, `DEMO_MAX_ARCS` 96), quindi un telegrafo
lungo ridisegnato ad ogni tick puo' saturarlo e togliere posto all'attacco vero.
Serve un pattern patologico (durata lunga × ridisegno ogni tick): se dovesse
comparire davvero nel generato, la risposta e' far cedere per primo un
telegrafo, non alzare i pool.
