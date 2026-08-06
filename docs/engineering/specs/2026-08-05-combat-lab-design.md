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
last_reviewed: 2026-08-06
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
