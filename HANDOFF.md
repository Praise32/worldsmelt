# Cosa fare quando torni

Scritto da Claude mentre eri via. **Prima la sezione 0-quater** (il 18 luglio, la piu' recente), il resto con calma.

---

## 0-quater. Il 18 luglio: fase 1 di Worldsmelt — la KB comincia a diventare gioco

Quattro milestone implementate, verificate e pushate su `main` (come chiesto: la KB e' la
fonte, la scala di agenti implementa e giudica, commit solo dopo verdetto APPROVA):

| Commit | Cosa |
|---|---|
| `f019cd2` | **M1a — Macchina a stati canonica.** I 4 vecchi AppMode diventano i 9 stati della KB (MainMenu, RunSetup, FloorZero, Gameplay, PauseMenu, Options, BuildScreen, RunResults, ExitConfirm). Il seed lo scegli in RunSetup e la generazione usa DAVVERO quello. Ogni azione distruttiva passa da ExitConfirm. Titolo: WORLDSMELT. |
| `18ec4da` | **M1b — Il Piano 0 e' la sala d'attesa giocabile** (DEC-002/004). Cammini nella stanza hub mentre i modelli generano in sottofondo; l'uscita verso il piano 1 si apre da sola quando il piano 1 e' pronto (messaggio + varco luminoso + particelle). Niente piu' overlay bloccante con la barra: l'indicatore e' una riga discreta, senza percentuali (come vuole `ui/generation-status.md`). |
| `8f751e5` | **M2 — Stanze di numero e grandezza variabili** (DEC-009). Griglia fissa, 6+piano+(0..3) stanze, taglie tutte diverse nello stesso piano (minimo garantito 556x298, boss sempre alla massima). Valori registrati in KB come default proposti stile DEC-019: le domande aperte restano aperte. Il test nuovo ha trovato un bug vero: la forma SCATTER collassava a zero ostacoli alla taglia minima — corretto. |
| `cdb4310` | **M3 — Generazione inglese-first** (DEC-052). Prompt, esempi few-shot, ispirazioni e TUTTI e tre i pool di fallback in inglese; guardia di lingua in `make test-llm`. Campione vero dal modello: *Ice Vault, Crystal Condo, Lava Ledge; Frozen Sentinels, Crystal Guardians; Honey Sphere, Lava Beam*. Varieta' 5/5. |

### Provalo

```bash
make run-gen
```

Il flusso nuovo: menu **WORLDSMELT** → *Nuova run* → scegli/rerolla il **seed** → entri nel
**Piano 0** e ti muovi mentre la generazione lavora → l'uscita in alto **si apre da sola** →
la attraversi e sei nel piano 1. ESC nel Piano 0 = abbandono con conferma (la generazione
continua finche' non confermi). Screenshot dell'hub: `logs/worldsmelt-floorzero-screen.png`.

### Cose da sapere

- **Il corpus QLoRA si ripulisce da solo:** i `manifest_pairs` italiani vecchi vengono
  scartati automaticamente al prossimo `corpus_to_dataset.py build` (confronto mtime coi
  prompt). Gli script Lua del corpus restano validi (sono codice, non prosa).
- **Il novelty ledger** (`logs/novelty-ledger.txt`) smaltisce le parole italiane da solo
  entro ~20 run: non serve azzerarlo.
- **Suite nuove dentro `make test`:** `--states-test` (transizioni della mappa canonica),
  `--floor-zero-test` (uscita chiusa/aperta con generatore finto), `--rooms-test` (DEC-009).
- **Backlog noto (non urgente):** l'RNG di gioco e' ancora seedato con `time(NULL)` in
  GameResetRun — i contenuti sono deterministici dal seed, il gameplay no; va agganciato al
  seed per le gare asincrone (DEC-016/062/066). E `gen_progress_lazy.txt` non viene mai
  scritto dai processi reali: mai costruirci UI sopra senza un `--progress-path` in melting-gen.
- **Domanda aperta #14 in KB:** l'arco Piano 0 → MainMenu (ESC) non e' nella mappa canonica;
  l'ho implementato via ExitConfirm e registrato come domanda, decidi tu se sancirlo.
- Il Piano 0 per ora e' la versione statica curata prevista dalla KB come sala d'attesa:
  scelta tema con anteprime (DEC-005/039), personaggi (DEC-014/030), museo e arene sono le
  prossime milestone naturali.

---

## 0-ter. La notte del 17 luglio: dal piano approvato al lavoro fatto

Hai approvato il piano (`/home/meri/.claude/plans/ti-ho-messo-una-gleaming-riddle.md`,
basato sui tuoi appunti `roguelike-ai-appunti/` + ricerca verificata), poi mi hai detto
di proseguire da solo. Tutto quello che segue e' **gia' su `main` e pushato su GitHub**
(come hai chiesto: da ora ogni cambiamento va diretto su main). Ho lavorato con lo
schema che hai voluto: io orchestro, i task li implementano sottoagenti Sonnet dedicati
(`.claude/agents/`), uno scettico Sonnet prova a demolirli prima di ogni commit, Opus
interviene sui bocciati, io solo sui casi estremi. Lo schema ha funzionato per davvero:
la verifica ha trovato e fatto correggere una vulnerabilita' vera (symlink nell'import
dei bundle) e un bug vero (hash dipendente dalla locale).

### Il gioco base (settimana 1 del piano)
- **Simulazione a passo fisso 60 Hz** (`4cb4232`): il gameplay non dipende piu' dal
  framerate; gli input a evento (bomba, reset) sono latchati per frame, mai persi ne'
  raddoppiati. Prerequisito del determinismo/replay futuri.
- **Resa pixel-perfect** (`c144e6d`): canvas campionato POINT + scala a passi di 1/8 —
  gli sprite pixel-art non sono piu' sfocati dal filtro bilineare.

### La macchina della misura (il pezzo piu' importante della notte)
- **Corpus delle generazioni** (`8946e61`): ogni passaggio del modello (tentativi JSON,
  tentativi Lua con le coppie errore→correzione, ripieghi) finisce in
  `logs/gen-corpus/*.jsonl`. E' telemetria OGGI e il dataset del QLoRA DOMANI, gratis.
- **`make gen-metrics`**: 3 generazioni vere su seed fissi → validita' Lua, varieta'
  fra run, campione d'italiano. Quattro misure fatte stanotte (stessi seed, confrontabili):

| Misura | Lua 1° colpo | Senza comportamento | Nomi unici | Note |
|---|---|---|---|---|
| A. Baseline | 78,3% | 20,0% | temi 14/15, nemici 29/30 | vocabolario convergente (cattedrale/caverna/deserto ovunque), «Bosco Uriante» |
| B. + Semi d'ispirazione | 85,0% | 15,0% | tutto 15/15 tranne stanze | unico dup: «Colonnato Sacro» = l'esempio fisso del prompt, copiato |
| C. + Esempi rotanti | 85,0% | 13,3% | **100% ovunque** | vocabolario fresco a ogni run |
| D. Due modelli (Instruct per JSON) | 80,0% | 20,0% | 100% | italiano NON migliore («Fratteio», «Etterno»), +15 s/run |

- **Decisioni prese sui numeri**: semi d'ispirazione (`3787033`) ed esempi rotanti
  (`fcb510e`) restano; il due-modelli (`4e90f98`) resta come flag `--model-text` ma NON
  e' il default (non paga). Per l'italiano la leva vera sara' il QLoRA quando il corpus
  sara' maturo — e il corpus si sta gia' riempiendo da solo.

### RunBundle v1 (`ad42389`)
`generated/` ora porta la sua provenienza (`provenance.txt`: seed, modelli, hash dei
prompt) e si esporta/importa con verifica: `scripts/bundle-export.sh` →
`bundles/melting-bundle-seed<seed>-<hash8>.tar.gz`, `scripts/bundle-import.sh` verifica
ogni sha256 + hash aggregato e sostituisce atomicamente. Rifiuta symlink/hardlink e
path traversal (testato con tar forgiati ad arte). E' il mattone di: condivisione run,
bug report riproducibili, daily challenge.

### Dataset e sprite (preparazione della campagna LoRA, settimana 2)
- **Registro di provenienza** (`11ff49a`): `docs/dataset/README.md` (regole dalla tua
  nota 05) + `scripts/dataset_ledger.py` (add/check/stats su `ledger.jsonl`).
- **Baseline sprite «Esperimento 0»**: 15 coppie tema/stile CONGELATE in
  `docs/dataset/baseline-prompts.txt` + `make sprite-baseline` → atlanti in
  `logs/sprite-baseline/<timestamp>/` (30 atlanti, ~40 min di GPU, lanciata stanotte:
  guarda `index.txt` li' dentro). Ogni LoRA futura si confronta ALLA CIECA con questi.

### Cosa aspetta TE (non potevo farlo io)
1. **Campagna Style LoRA su cloud** (~€40 per eccesso dei tuoi ~€200): apri un account
   RunPod (o Vast), il piano e la checklist sono nella tua nota 06 + il piano approvato.
   Il dataset va curato con i tuoi occhi: le fonti CC0 candidate sono in
   `docs/dataset/README.md`, registrale con `scripts/dataset_ledger.py`.
2. **Retro Diffusion**: se vuoi davvero i loro output come dataset, serve il permesso
   scritto (ToS §6 «Competing with us» + tracciabilita' server-side: possono sempre
   risalire). La mail da mandare: support@retrodiffusion.com. Altrimenti: via CC0.
3. **Guarda a occhio**: la resa POINT del canvas (gusto tuo), i temi delle run nuove
   (`make run-gen`), e gli atlanti della baseline sprite.

### Aggiunte della mattina (mi hai detto di continuare mentre eri fuori)
- **Campagna LoRA pronta al 100%** (`84ba041`): 2.768 immagini CC0 scaricate in
  `dataset-raw/` (5 pack Kenney + superpowers-asset-packs, che include gia'
  Ninja Adventure; Urizen va preso a mano da itch.io), 3.158 voci nel ledger,
  e **`docs/dataset/TRAINING-RUNBOOK.md`**: il passo-passo RunPod+kohya per chi
  non ha mai noleggiato una GPU. Ti serve solo l'account.
- **Preparazione QLoRA** (`4181e97`): `scripts/corpus_to_dataset.py` distilla il
  corpus in dataset (oggi: 104 script Lua validati, 6 coppie manifest, 2 coppie
  errore→correzione — crescera' da solo giocando). E `gen_metrics.py` ora misura
  anche le **parole ricorrenti fra run**: la baseline nascondeva 8/19 parole
  condivise («caverna» e «deserto» in 3 run su 3) dietro nomi "unici"; l'ultima
  misura e' a 2/31.
- **Preset `--low-spec`** (`fcab206`): il gioco avviato con questo flag usa il
  1.5B per il testo e sprite generati a 256px (`--gen-size` in melting-sprites).
  Misurato sulla tua scheda: sprite 49s invece di 80s, testo 127s invece di
  ~155s. E' il mattone del futuro sistema a tier per Deck/portatili. La verifica
  adversariale ha beccato (e fatto correggere a Opus) la ripresa B2 che avrebbe
  caricato il 7B a meta' run anche in low-spec.

### Seconda sessione fuori-casa (mi hai detto di continuare ancora)
- **Benchmark misurato + tier automatico** (`70a9d06`): `make benchmark` misura la
  macchina coi processi figli (`--bench`, isolato: non tocca mai `generated/`) e
  scrive `logs/benchmark.txt`; il gioco lo legge a `--generate` e applica il preset
  low-spec da solo (override: `--full-spec`; i flag espliciti vincono sempre; mai
  bloccato il gioco, nemmeno su `unsupported`). Taratura trovata col benchmark VERO:
  la prima immagine paga il warmup Vulkan (14,9 s!) e mandava la tua 5600 XT in
  lowspec; ora si misura la seconda, a regime (5,6 s) → la tua scheda esce `full`.
- **Novelty ledger fra run** (`01e2ca6`): il generatore ricorda le parole-contenuto
  delle ultime 20 run (`logs/novelty-ledger.txt`, mai avvelenato da fallback/resume)
  e inietta nel prompt le parole viste in ≥2 run come blocco «EVITA». È la difesa
  della varietà sul lungo periodo, misurabile con la nuova metrica delle parole.
  Qui la scala di verifica ha reso il massimo: due bocciature (buffer che troncava
  l'ultima parola; parser che non faceva mai convergere le parole oltre i 31 char),
  due correzioni, test di regressione per entrambe.

### Sistema
- Sospensione automatica: disattivata/ripristinata a ogni sessione di lavoro.
  Stato finale: `suspend` (verifica: `gsettings get
  org.gnome.settings-daemon.plugins.power sleep-inactive-ac-type`).
- Scaricato `models/qwen2.5-7b-instruct-q4_k_m.gguf` (4,7 GB) per l'esperimento D.
- Nessun tocco a GRUB/partizioni/root. Le suite (`make test`, `test-gen`, `test-script`,
  `test-sprites`) sono verdi su ogni commit della notte.

---

## 0. La notte del 14 luglio: i quattro passi della roadmap sono fatti

Ho eseguito la roadmap che avevi lasciato (`docs/superpowers/specs/2026-07-14-feedback-roadmap.md`)
nell'ordine che avevi chiesto. Quattro commit, tutti su `local-sprites`, `make test` +
`make test-gen` + `make test-script` verdi.

**C — Bilanciamento alla Isaac + tipi di colpo inventati dall'AI** (`1f924e1`)
Il tuo feedback a metà lavoro («i tipi di colpo devono SEMPRE crearli i modelli AI, i tre
che hai fatto sono solo esempi») ha riscritto questo passo, ed è la parte di cui vado più
fiero. Il motore **non ha un menu di tipi di colpo**: espone un vocabolario parametrico
(5 forme di resa × 7 manopole) e **il modello inventa i tipi a ogni run** — nome, forma e
numeri — nel JSON, uno per piano, attaccato all'oggetto che sceglie lui. Chiodi/raggio/scarica
sopravvivono solo come *esempi nel prompt* e come ripiego procedurale.
La promessa che il motore fa al modello è `ShotTypeBalance()`: qualunque cosa scriva, il tipo
viene riportato a un budget di potenza ~1.0. Quindi un tipo di colpo è sempre un **sidegrade**
(chiodi veloci e deboli ≈ raggio che perfora ≈ scarica che salta), mai un dud e mai rotto.
Un test prova 768 combinazioni estreme: nessuna esce dalla banda.
Curve alla Isaac: cadenza con pavimento pratico a 0.10s, rendimenti decrescenti sul danno
sopra 2× base, nuova statistica **fortuna**.
*Bug vero trovato dai test nuovi*: la perforazione non perforava — ricolpiva lo **stesso**
nemico ogni frame mentre lo attraversava, bruciando tutti i passaggi sul primo. Ora un colpo
non colpisce mai due volte lo stesso nemico.

**D — Sinergie implicite** (`59a881f`)
Sei coppie canoniche in una tavola dichiarativa (aggiungerne una = una riga). Due canali:
statistico dentro il ricalcolo-da-zero (idempotente e clampato per costruzione) e
comportamentale sui trait del colpo. La potenza scala sulla **rarità minima** della coppia.
**E si vedono**, che era il punto: anello pulsante sui colpi sinergici, elenco delle coppie
attive nel pannello in basso, messaggio + particelle quando ne sblocchi una.

**E — Resa 2.5D** (`3415b64`)
Come avevi chiesto di valutare: il 2.5D alla Isaac è quasi tutto **rendering**, quindi costa
**zero secondi di generazione**. Ombre ellittiche sotto ogni entità, ordinamento per
profondità (chi è più in basso è disegnato davanti), pavimento in prospettiva, muri con
spessore (le porte sono passaggi *nel* muro), vignettatura. I prompt degli sprite ora
chiedono una vista a ¾ dall'alto, coerente con le ombre. Campo di gioco invariato: nessuna
collisione è cambiata.

**B2 — Generazione pigra dei piani** (`5e483dc`)
Il passo bloccante scrive il Lua del **solo piano 1** (4 script invece di 20); gli altri 16
li scrive un secondo processo che parte quando comincia la partita e lavora **mentre giochi**,
pubblicando il manifest piano per piano. Il gioco raccoglie gli script di un piano quando ci
entra. Misurato col modello piccolo: **49s bloccanti invece di ~90s**, e i quattro piani
restanti completati in 42s di gioco.

**Da guardare a occhio quando torni** (sono gusto tuo, non correttezza):
`logs/melting-run-shotforms-screen.png` — le cinque forme di colpo, la stanza in 2.5D, la
sinergia attiva nel pannello LOG e l'anello attorno al colpo sparato.

## 0-bis. Poi ho continuato: nemici, stanze, GUI — tutto generato dall'AI

Dopo la roadmap ho proseguito, sempre con lo stesso principio (il motore dà mattoni parametrici
+ garanzie, il modello compone), e con una **review a freddo dopo ogni fase** (agenti in
parallelo che si demoliscono i risultati a vicenda). Ogni review ha trovato bug veri — quasi
tutti scoperti *guardando* o *provando*, non nei test. Commit in coda su `local-sprites`:

- **Fix anti-fotocopia del generatore** (`fe1fce9`). Al primo giro col 7B vero ho scoperto che a
  certi seed sputava **cinque piani identici** (manifest perfetto, tutti i test verdi, run
  rovinata). Causa: aggiungere i tipi di colpo aveva allungato il JSON oltre la finestra della
  penalità sulle ripetizioni. Fix: finestra allargata + rete che sostituisce un piano-fotocopia
  col piano procedurale + guardia in `make test-llm`.
- **Review del lavoro notturno** (`71585c0`): 8 bug. Il più grave: il ricalcolo delle statistiche
  **leggeva il proprio output precedente**, così la stessa coppia di oggetti dava o non dava una
  sinergia a seconda dell'*ordine* in cui li raccoglievi. Più: la perforazione ricolpiva lo stesso
  nemico, una generazione nuova adottava gli script Lua di quella vecchia, ecc.
- **Fase 3b — nemici e boss inventati dall'AI** (`607d86d`). Il motore non ha un catalogo di
  nemici: 4 forme, 5 movimenti, 4 modi di sparare, manopole clampate; il modello inventa 2 nemici
  + 1 boss per piano. Due reti: `EnemyTypeBalance` (potenza in banda) e il **budget di difficoltà
  della stanza** (spende punti, non spawna un numero fisso — nemici cattivi = meno nemici). Il 7B
  ha inventato "Guardiano delle Dune", "Scorpiotto Fiorente"… Review 3b (`4231bce`): 2 bug.
- **Fase 3c — stanze con ostacoli inventati dall'AI** (`7281992`). 5 forme di layout
  (open/pillars/corridor/arena/scatter) + densità; le stanze di combattimento hanno ostacoli
  solidi (coperture) con collisione vera. Garanzia: la croce centrale resta sempre libera, quindi
  la stanza è **sempre** giocabile. Il 7B ha inventato "Colonnato Sacro", "Casse Ardenti"…
  Review 3c (`bbed31f`): 2 bug (un ostacolo attaccato al muro poteva spingerti *dentro* il muro).
- **GUI completa con raygui** (`bbed31f`). Restyle di tutti i pannelli + **HP come cuori**,
  **tooltip sugli oggetti** (passa il mouse), **minimappa migliore** (colori, icone T/$/B,
  legenda, stanza corrente), e un **blocco BUILD** che mette in evidenza tipo di colpo + sinergie.

**Non fatto, di proposito:** il benchmark al primo avvio (mi hai detto di saltarlo: la tua
macchina è già tarata). E la fase 3c è l'ultimo contenuto generato: da qui in poi è
bilanciamento fine e contenuti aggiuntivi, non nuovi sistemi.

**Tutte le suite verdi** (`make test`, `make test-gen`, `make test-script`, `make test-sprites`)
e verificato col 7B vero a ogni fase. Il gioco non linka mai llama/cJSON (incluso dopo raygui).

---

## 1. Provalo

```bash
cd ~/progetti/melting-run-gpu
git checkout local-sprites
make run-gen
```

Premi **INVIO** nel menu. Parte una barra di caricamento in due tempi:

1. **Testo** — il Qwen 7B inventa i 5 piani (temi, boss, oggetti, tipi di colpo) e scrive il
   Lua del **solo piano 1** (step B2: gli altri quattro piani li scrive in sottofondo mentre
   giochi, vedi sezione 0).
2. **Sprite** (~85 s) — Stable Diffusion disegna i 12 sprite del gioco, coerenti col tema appena inventato.

L'attesa iniziale è **scesa** rispetto ai 2 min 30 s di prima (il pezzo Lua è passato da 20
script a 4). **ESC** annulla, **R** rigenera. Mentre giochi il piano 1, un secondo processo
scrive gli oggetti dei piani 2-5: li trovi già pronti quando ci arrivi.

Se vuoi solo il testo (85 secondi in meno):

```bash
make run-gen-fast     # equivale a --no-sprites: tiene gli sprite geometrici di prima
```

Guarda anche l'atlas da solo:

```bash
xdg-open generated/current_atlas.png
```

### Se qualcosa non parte

```bash
make                 # ricompila tutto
make test            # test del gioco
make test-gen        # test del generatore di testo (senza modello, veloci)
make test-sprites    # test degli sprite (senza modello: usa --dry-run)
make test-llm        # generazione di testo vera (~1 min)
```

---

## 2. Lo stato del lavoro: due branch, nessuno pushato

Ho lasciato a te la decisione di mergiare: vorrai prima vederlo girare.

| Branch | Cosa | Stato |
|---|---|---|
Tutto il lavoro e' sul branch **`local-sprites`** (contiene anche `linux-local-llm`):
**34 commit, non pushato, non mergiato.** Tre fasi complete, ognuna con la sua review:

| Fase | Cosa fa | Stato |
|---|---|---|
| 1 — testo | Build Linux + i contenuti della run generati dal Qwen 7B in locale | completa, review passata |
| 2 — sprite | Gli sprite del gioco generati da Stable Diffusion in locale | completa, review passata |
| 3a — Lua | **L'IA scrive il comportamento degli oggetti in Lua** (vedi sotto) | completa, review passata |

Per prendere tutto: `git checkout main && git merge local-sprites && git push`.
Per buttare via: `git branch -D linux-local-llm local-sprites` (il tuo `main` e' intatto).

## La cosa grossa: la fase 3a e' fatta

Era il tuo obiettivo dichiarato, e adesso funziona. Fino a ieri un oggetto era una fra
quattro operazioni che il C sapeva gia' fare (`on_fire:burst,3,0.36,split`): l'IA poteva
inventare il *nome*, non *cosa fa*. Ora l'IA **scrive vero codice Lua** per ogni oggetto, e
il gioco lo esegue in una sandbox blindata.

Nell'ultima run di prova, il 7B ha scritto Lua funzionante per **12-15 oggetti su 15 al
primo colpo, zero ripieghi**. Esempio, scritto dal modello senza ritocchi:

```lua
function on_tick(dt)
  if player_hp() < player_max_hp() / 2 then
    spawn_shot(player_x(), player_y(), 0, 1, 75, 3, 4, TRAIT_RAPID)
  end
end
```

(un oggetto che spara da solo quando sei a meta' vita). Il test di riferimento e' un oggetto
che la vecchia mini-VM **non sapeva esprimere**: «ogni terzo colpo si sdoppia e i frammenti
inseguono il nemico piu' vicino, ma solo se hai meno di tre cuori» — gira davvero in gioco.

**La sicurezza e' la parte a cui ho dedicato piu' attenzione**, perche' quel Lua lo scrive
un modello inaffidabile e un giorno potrebbe girare sui computer di chi compra il gioco. La
sandbox chiude undici vie di fuga note (cicli infiniti, bombe di memoria, tentativi di
aprire file, di uscire dalla sandbox). Due bug seri trovati dalle review e corretti: uno
sprite scartato lasciava nemici/boss **invisibili** (ora ripiegano sulla forma geometrica);
e una funzione Lua (`table.move`) permetteva un ciclo in C che bloccava il gioco per sempre
(ora e' rimossa, con un test che lo dimostra). **Qualunque script fallisca — non compila,
va in loop, esaurisce la memoria — viene ucciso e l'oggetto ripiega sulla vecchia mini-VM.
Il giocatore vede al massimo un oggetto scialbo, mai un crash.**

## Fase 3, seconda parte: la tua visione degli oggetti

Dopo la tua descrizione (oggetti semplici ma unici, sinergie, stat-up dal boss,
personaggio a strati) ho scritto il design in
`docs/superpowers/specs/2026-07-13-items-synergy-vision.md` e implementato le parti
non ambigue. Cosa c'e' adesso:

- **Due famiglie di oggetti.** Gli oggetti di tesoro/negozio sono **attivi** (un
  comportamento semplice: un colpo che rimbalza, uno che insegue, uno grande…). Il
  boss lascia cadere un oggetto **stat-up** puro (piu' danno, piu' vita…), bilanciato:
  ogni stat-up puo' spostare una statistica al massimo del 25% della base, quindi non
  puo' rompere il personaggio.
- **Il 7B scrive davvero i comportamenti.** All'inizio scriveva quasi solo scaling di
  numeri (1 comportamento su 15); ho rifatto il prompt e ora ne scrive **11 su 14**.
  Esempi veri, scritti dal modello: un frammento che rimbalza verso il nemico
  all'impatto, uno che lo insegue, un colpo extra grande quando spari. Gli stat-up del
  boss restano puliti (5/5, bilanciati).
- **Il personaggio a strati (la tua "tela vuota").** Il personaggio base ora e' uno
  stickman minimale e fisso, e gli oggetti raccolti si vedono **sopra**, a strati,
  ognuno al suo slot (cappelli impilati con badge "+2" se sono tanti, occhiali, arma in
  mano, mantello dietro le gambe, aura che orbita). C'e' uno screenshot:
  `logs/melting-run-layers-screen.png` — guardalo quando torni. La sorgente di ogni
  strato e' **pluggabile**: adesso e' geometria colorata, domani ci si mette lo sprite
  generato senza rifare niente.

## Pool e rarita' (le tue decisioni di stamattina, gia' costruite)

Hai scelto: sinergie **implicite alla Isaac**, rarita' a **4 livelli** (potenza +
frequenza), il **luogo** determina tipo e rarita', sprite **geometrici** per ora. Ho
scritto il design in `docs/superpowers/specs/2026-07-13-pools-rarity-design.md` e
costruito pool + rarita':

- **Quattro rarita'**: Comune (bianco), Non-comune (verde), Raro (blu), Leggendario (oro).
  La rarita' decide **quanto e' forte** l'oggetto e **quanto e' raro** trovarlo.
- **Bilanciamento vero.** Piu' alta la rarita', piu' grande l'effetto — ma dentro un
  tetto: cinque leggendari sulla stessa statistica ti lasciano a 32 di danno su un tetto
  di 200. Un leggendario e' forte, mai rotto. (Verificato con un test apposta.)
- **I pool per luogo.** Il boss da' **sempre** uno stat-up raro o leggendario; il tesoro
  da' oggetti attivi di rarita' mista; il negozio da' oggetti attivi che paghi in monete,
  e **il costo scala con la rarita'** (un leggendario costa piu' di un comune).
- **Espandibile come volevi.** I pesi della rarita', i tetti di potenza e gli archetipi
  di effetto stanno in tabelle marcate «modifica qui per bilanciare»: aggiungi un
  archetipo o ritocchi un numero senza toccare il motore.
- **Si vede a colpo d'occhio.** Screenshot: `logs/melting-run-rarity-screen.png` (anche
  in `docs/img/rarity-screen.png`). I pickup a terra hanno un anello del colore della
  rarita', e il pannello mostra nome e colore. Il leggendario a terra ha un bagliore che
  pulsa.

Il segnale di rarita' **arriva davvero al modello**: in una run vera, un boss leggendario
ha scritto +10 danno, i rari +1.5/+2; un oggetto attivo comune ha fatto un rimbalzo base.

**Una cosa da sapere per il futuro.** Il prompt che chiede al modello di scrivere il Lua
e' ormai vicino al limite di contesto (n_ctx=4096): oggi ci sta con poco margine. C'e' un
controllo automatico (`make test-gen`) che fallisce se il prompt cresce troppo, cosi' non
puo' piu' capitare che si sfori in silenzio. Ma se un domani vuoi aggiungere molti
archetipi di effetto nel cheat-sheet (`tools/melting-gen/prompts/lua_system.txt`), a un
certo punto dovrai alzare n_ctx (costa un po' piu' di VRAM e tempo) o accorciare il prompt.
Non e' urgente, e' solo un tetto da tenere a mente.

**Le sinergie vere** sono ora **fatte** (step D, vedi sezione 0 in cima): sei coppie
canoniche, due canali di applicazione, potenza scalata sulla rarita' minima della coppia, e
soprattutto *visibili* in gioco. La tavola e' dichiarativa: aggiungere una sinergia e' una
riga in `src/gameplay/synergies.c`.

## Le altre domande ancora aperte (design doc sezione 7)

1. **Il merge/le sinergie.** Il pezzo piu' importante della tua visione — "facendo il
   merge dei due oggetti si creano nuovi oggetti" — non l'ho costruito, perche' ci sono
   tre modi diversi di intenderlo (sinergia implicita alla Isaac / fusione esplicita di
   due oggetti in uno / arma che evolve) e la scelta cambia molto. La mia proposta e' la
   sinergia implicita per iniziare, con la fusione visiva fatta a strati. Dimmi tu.
2. **Gli sprite degli oggetti.** Adesso gli strati sono forme geometriche. Generare uno
   sprite per ogni oggetto raccolto e' il passo dopo, ma aggiunge tempo di generazione:
   vale la pena, o gli strati geometrici bastano per ora?
3. **Il personaggio base**: l'ho fatto fisso e minimale (per agganciare gli oggetti in
   modo affidabile). Confermi, o lo vuoi generato da SD ma semplice?

## Le tue risposte, registrate

- **Vendita**: passo al modello Apache 2.0 per gli sprite → **ma non era adatto** (fa
  spritesheet a 4 pose, non sprite singoli). Rileggendo la licenza, il problema che temevi
  non esiste: il gioco non ridistribuisce mai i pesi (li scarica chi gioca), quindi OpenRAIL-M
  va bene. Tutto in `docs/LICENZE.md`.
- **Attesa**: 2 min 30 s vanno bene → non toccato.
- **Fase 3**: scope «tutto». La **3a (oggetti) e' fatta**, e sopra ci ho costruito la
  tua visione (oggetti attivi + stat-up + personaggio a strati, vedi sopra). Restano
  **il merge/sinergie** (domanda 1 qui sopra), **3b** (nemici e boss) e **3c** (stanze).
  Quelle contengono scelte di design del *tuo* gioco che voglio decidere con te.

`local-sprites` contiene gia' tutto `linux-local-llm`, quindi per prendere tutto:

```bash
git checkout main
git merge local-sprites
git push               # solo se vuoi mandarlo anche su GitHub
```

Per buttare via tutto: `git branch -D linux-local-llm local-sprites`. Il tuo `main` non e' stato toccato.

---

## 3. I numeri veri, misurati sulla tua macchina

| | |
|---|---|
| Testo: Qwen2.5-Coder **7B** | ~50 s, **28 token/s**, 4,5 GB di VRAM |
| Sprite: SD1.5 pixel-art + LCM | ~5,7 s per sprite, **~85 s** per 12, 2,0 GB di VRAM |
| Totale a inizio run | **~2 min 30 s** |

Il 7B ci sta **tutto** nei tuoi 6 GB (era la mia previsione piu' pessimista, sbagliata: bene cosi').
I due modelli non possono stare in VRAM insieme, ma non serve: sono due processi che si
alternano e ognuno libera tutto quando esce. Dettagli in `docs/BENCHMARKS.md` e `docs/SPRITES-SPIKE.md`.

---

## 4. Le due cose che devi guardare tu, perche' sono gusto tuo

### a) La qualita' degli sprite

Personaggi, nemici, boss, cuore, porta: **vengono bene**. Le icone piccole (moneta, chiave,
bomba, colpo) sono un terno al lotto: a volte la chiave e' una chiave, a volte e' un
lucchetto o peggio. Ho gia' fatto due giri di miglioramenti (soggetto in testa al prompt,
negativi mirati, cfg alzato a 1.8 — sotto ignorava il soggetto, sopra sporcava lo sfondo).

Da qui in poi e' **direzione artistica, ed e' tua**. I prompt sono file di testo, uno per
sprite, modificabili senza ricompilare:

```
tools/melting-sprites/prompts/{player,boss,coin,key,bomb,shot,...}.txt
tools/melting-sprites/prompts/negative.txt      <- cosa NON deve disegnare
```

Cambi il testo, rilanci `./bin/melting-sprites --seed 5`, guardi `generated/current_atlas.png`.
Un giro costa 85 secondi. Se una cella esce indecente il gioco **non si rompe**: la lascia
vuota e ridisegna la forma geometrica di prima per quell'entita'.

### b) La qualita' dei testi

Il 7B in italiano e' altalenante a seconda del seed: a volte *"Catacombe Crepuscolari —
Ossamento Scuro"*, a volte *"deserto — Sandstorm"*. Ho tolto il difetto piu' grosso (copiava
pari pari l'esempio dal prompt) e aggiunto la penalita' sulle ripetizioni. I prompt sono in
`tools/melting-gen/prompts/`.

---

## 5. Decisioni che aspettano te (nessuna urgente)

1. **Il gioco resta un hobby o punti a venderlo?** Cambia la scelta del modello per gli sprite.
   Quello che uso ora (All-In-One-Pixel-Model) e' OpenRAIL-M: le immagini generate sono tue e
   vendibili, ma se **ridistribuisci i pesi** insieme al gioco devi propagare le restrizioni
   d'uso della licenza. Esiste un'alternativa Apache 2.0 (`SD_PixelArt_SpriteSheet_Generator`),
   un po' peggiore in resa. I modelli di testo (Qwen) sono Apache 2.0: nessun problema.

2. **Attesa a inizio run.** Adesso sono 2 minuti e mezzo. Alternative: generare gli sprite
   una volta sola e riusarli per piu' run; oppure generarli in sottofondo mentre giochi il
   primo piano. Dimmi cosa preferisci.

3. **Il tuo obiettivo vero: oggetti unici, sinergie inventate, comportamenti dei nemici.**
   E' la **sandbox Lua** (fase 3), il pezzo piu' grosso. La mini-VM di adesso ha 4 operazioni,
   non basta a esprimerli. Vorrei progettarla **con te presente**: le decisioni li' dentro
   (quali callback esporre all'IA, quanto potere darle, cosa resta blindato in C) sono scelte
   di design del *tuo* gioco, non dettagli tecnici che posso decidere io.

---

## 6. Cose che ho cambiato sul tuo sistema

- **Installato `xvfb` e `xdotool`.** Servivano per testare senza di te: i test aprono una
  finestra e, con lo schermo bloccato, su Wayland il gioco si piantava (una finestra non
  visibile non riceve piu' frame). Ora i test girano su uno schermo virtuale. Il Makefile li
  usa: se li togli (`sudo apt remove xvfb xdotool`) i test torneranno a richiedere lo schermo
  sbloccato.
- **Sospensione automatica**: disattivata mentre lavoravo, **ripristinata** com'era (suspend
  dopo 2 ore). Verifica: `gsettings get org.gnome.settings-daemon.plugins.power sleep-inactive-ac-type`
  deve dire `'suspend'`.
- **La password che mi hai passato in chat e' finita nella trascrizione della sessione.**
  L'ho usata solo per installare quei due pacchetti e non l'ho salvata da nessuna parte.
  Valuta di cambiarla.

Non ho toccato GRUB, partizioni, dischi NTFS, ne' il resto della tua configurazione.

---

## 7. Se vuoi capire cosa e' stato fatto

- `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md` — il progetto della fase 1 e la roadmap.
- `docs/superpowers/specs/2026-07-13-local-sprites-design.md` — il progetto della fase 2.
- `docs/SPRITES-SPIKE.md` — le misure con cui ho deciso che gli sprite erano fattibili, e le
  due trappole che hanno cambiato il progetto (perche' **non** si generano su sfondo nero).
- `docs/BENCHMARKS.md` — i numeri del modello di testo.
- `git log --oneline main..local-sprites` — i commit, uno per pezzo.

I tuoi appunti (`docs/APPUNTI.md`, `docs/DESIGN_NOTES.md`) non li ho toccati: sono tuoi.
