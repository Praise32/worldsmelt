# HANDOFF

Stato sintetico del lavoro. La cronologia completa delle sessioni passate è in
`docs/archive/handoffs/` (integrale precedente: `docs/archive/handoffs/HANDOFF-2026-07-19.md`).

## Stato al 2026-07-31 — W10/W11: gap-closure completa dai docs, demo pronta al playtest

- **Branch**: `main`, tutto committato e pushato (19 commit della sessione 30-31/07,
  `20f5c16..73500d6`). Working tree: SOLO le modifiche audio pre-esistenti in attesa del
  verdetto SFX del proprietario (`assets/audio/*`, `scripts/audio-pack.py`) — non toccate.
- **Il mandato**: analisi di TUTTI i docs contro il codice (8 lettori in parallelo, 97 gap
  classificati) e implementazione dei gap per il playtest completo (DEC-171: la demo copre
  tutti i sistemi documentati). Scala CLAUDE.md rispettata su ogni WP: implementer→verifier
  con mutation-test, escalation a Fable sui gradini 3.
- **Sistemi entrati** (ognuno con test in `make test`, docs aggiornati nello stesso lavoro,
  default proposti registrati in open-questions):
  - Timer di run (DEC-051) + tempo in RunResults/catalogo; salute temporanea Crust
    (DEC-008) dal negozio con contatore HUD; ostacoli distruttibili + pericoli telegrafati
    + budget condiviso (DEC-013/128/043).
  - I **cinque archetipi speciali** tutti fisici nel motore: ROOM_FUSION (crogiolo),
    ROOM_TIMED (clessidra, dal piano 3), ROOM_ARENA (conferma X, foglia per costruzione,
    budget x1.5), ROOM_POURHOUSE (puntata dal seed nel budget di equità, DEC-044, modulo
    `world/pourhouse`), ROOM_SECRET a due livelli (varco murato + breccia, DEC-025).
  - Prove della run (DEC-042, modulo `game/trials`, card di presentazione al varco);
    arene del Piano 0 (DEC-092-095, modulo `world/floor_zero_arena`: simulazioni a rischio
    zero con snapshot integrale, best-of dal catalogo, tutorial DEC-047, HUD via
    `floorZeroTrialActive`); abbandono→RunResults con causa (DEC-082/089); reroll con
    conferma dal PauseMenu (DEC-114, R resta reset stesso-seed); sospensione + Continua
    (DEC-050, modulo `game/run_suspend`, `suspend/current.txt` schema 1); ExitConfirm
    modale leggero dal MainMenu + riga Modalità in RunSetup + focus build sull'ultimo
    acquisito (DEC-090).
  - **Asset demo prodotti in autonomia** (autorizzazione del proprietario 30/07, aseprite-mcp,
    stile S1/palette Fucina, sorgenti in `assets/art-src/`): crogiolo, clessidra, spuntoni,
    pickup cuore/bomba/chiave, sheet Ashblade e Bulwark (rosa distinguibile in silhouette),
    crepa-segreta, tag-veterano, font esteso con accentate+parentesi (glyphs_ext + decoder
    UTF-8). Tutto agganciato al motore (WP-INT). Provvisori: fuori dal dataset LoRA.
- **Incidenti di sessione, risolti e con guardrail**: quota disco esaurita da copie del repo
  negli scratchpad dei mutation-test (pulita; ora vietato copiare il repo, mutazioni in
  place con ripristino via Edit); un commit parziale che aveva rotto main (riparato in
  `4c098f1`); due implementer che si sono cancellati il lavoro con `git checkout` durante i
  mutation-test (ricostruito e verificato; ora vietato git checkout sul tree condiviso).
- **Test**: `make test` completo (35 marker ok, incluse le suite nuove: run-timer,
  temp-health, obstacles, trials, arena-hub, suspend, exit-confirm-light-modal,
  run-setup-mode-line), `make test-script`, `make test-gen`, `make test-sprites`,
  `make docs-index && make docs-check` — tutti verdi alla chiusura (2026-07-31).
- **Differiti dichiarati** (non persi: motivati in `scratchpad`-backlog e nei doc):
  museo del Piano 0 con Reliquie/preferiti (DEC-063/085/069/045), meta-progressione
  Embers/sblocchi rosa (DEC-015/027/100 — nessun salvataggio meta esiste ancora),
  punteggio composito completo (DEC-060), codice breve di condivisione (DEC-066/077),
  danno da contatto per forma + knockback (DEC-061/134), boss due fasi (DEC-028/106),
  Veterani (asset tag pronto), multi-attivi e piega-regole, card scoperta estesa a
  oggetti/fusioni, Options a 6 categorie/rimappatura (DEC-058), scelta binaria primo
  avvio (DEC-070/086), quick-win gen-side (guardrail originalità nel validatore, limiti
  dark DEC-119). Out-of-scope demo (DEC-171/172): stadio 2 fusione, multiplayer online,
  gen-progress UI. Asset-gated: famiglia audio Piano 0 (DEC-121). Il **bilanciamento**
  fine resta legato al feedback del playtest round 2 (mai arrivato): valori attuali =
  default proposti DEC-019 + tabelle di equità/ricompensa nuove, tutti in open-questions.
- **Prove pipeline sprite (31/07 sera)**: la ricerca esterna del proprietario su pixel
  art/LoRA/sinergie è stata confrontata col canone (esito: ~70% già coincide; 32px di
  DEC-177 confermato contro il 64x64 proposto; zip di sprite commerciali = solo studio,
  mai nel repo né nel training). Prodotti su sua richiesta ("facciamo delle prove"):
  protocollo **silhouette-first dimostrato** end-to-end (torretta-di-crogiolo: 3 candidate
  → scelta B "colonna con crogiolo" → materiali 11 colori Fucina → floor test
  chiaro/scuro), template a livelli `assets/art-src/templates/enemy_32_template.aseprite`,
  mockup **overlay sinergie** sulla Fonditrice (corona su slot testa + mutazione crepe
  emissive + combinata, DEC-049) in `assets/art-src/experiments/`. Candidati DEC per il
  facilitator: protocollo+template come standard di produzione; mappatura 6 slot visivi ↔
  slot funzionali; riconciliazione celle atlas SD 128px ↔ scala nativa 32px alla Style
  LoRA. Provvisori: fuori dal dataset LoRA (DEC-201).
- **Art library + Fase A distillation (31/07 sera, sessione art)**: mandato del
  proprietario "definiamo lo stile e rifacciamo tutta la grafica della demo".
  Costruita `assets/art-library/` (struttura della sua ricerca reference):
  9 pack CC0 con sorgenti .aseprite scaricati da itch.io (licenze snapshot
  accanto a ogni pack), **22 reference card** prodotte da 5 agenti Fase A
  (`20_reference_cards/` + INDEX.md per il giro di voti), bozza
  `assets/art-library/30_visual_language/visual-language-v2-DRAFT.md` +
  `assets/art-library/30_visual_language/negative_rules.md`.
  Regole fissate dal proprietario: personaggi **senza volto** e **senza armi**
  (armi = oggetti/overlay), formato piccolo (~26px in cella 32, stile masse
  alla RoR Returns — misurati 22-34px dai suoi sheet), coerente con 640×360 di
  DEC-200. Prove in `assets/art-src/experiments/` (trio mini 32 approvato dal
  gusto, craft pass, A/B 32/64/128). **CHIUSO (31/07, sessione remota):** voto
  card accettato in blocco
  (`assets/art-library/10_references/approved-ai-reference/APPROVED.md`),
  outline deciso — **opzione B, masse senza outline** — e batch DEC-205..210
  registrato dal decision-facilitator: DEC-205 (masse senza outline, supera
  DEC-176(a)), DEC-206 (senza volto), DEC-207 (senza armi in mano, precisa
  DEC-049), DEC-208 (scala 32px riconfermata, DEC-177 invariata, 64px
  scartato), DEC-209 (reference distillation come canone di produzione),
  DEC-210 (onda 1 = personaggi+nemici). Propagato in
  `docs/design/content/visual-language.md`. Prossimo: produzione dell'onda 1
  (asset/codice, fuori da questo lavoro di decisione).
- **Storia W8** (asset art consumati dal motore: art_atlas/art_draw, priorità immagini
  DEC-175, nove schermate rivestite): committata il 30/07 prima di questa sessione; i
  "buchi dichiarati" di allora (font, sheet personaggi, prop cuore/bomba/chiave, salute
  temporanea, timer) sono TUTTI chiusi da questa sessione — vedi known-issues voce 10
  aggiornata.

## Stato al 2026-07-28 — Chiusura maratona implementativa della demo

- **Branch**: `main` (tutto committato e pushato; policy: ogni cambiamento verificato va
  subito su main).
- **Ultimo lavoro**: **maratona implementativa della demo completa** (96578b0..0726b4c,
  ~30 commit in 36 ore). Engine raylib + sessione artistica parallela (CP1→CP3c) unificati.
  Batch finale: DEC-170..178 già registrati nel decision-log la sera del 2026-07-27 e
  implementati nella notte. Punti salienti:
  - Engine: stanze multi-taglia stile Isaac (1x1/1x2/2x1/2x2/L) con telecamera a zoom fisso
    (DEC-170); RNG di gameplay derivato dal seed di run, abilita Classificata a seed
    identico (DEC-141); 4 categorie oggetti (passivo/attivo/Innesto/stat-up) con tassonomia
    e slot di equipaggiamento (DEC-115/117); pool a rarità con correzione di fortuna N basata
    su Fortuna (DEC-144/145); economia da stanze completate per archetipo (DEC-167);
    sinergie seedate deterministiche (DEC-161); fusione stadio 1 (DEC-022/101/102/143/162);
    UI DEC-152 (card scartate), DEC-169 (HUD Piano 0), DEC-159 (causa sconfitta);
    modulo audio standalone (DEC-172) + pacchetto pre-generato (Stable Audio 3 musica +
    rFXGen SFX uscente da pipeline, DEC-109/172/178).
  - Contenuto curato: 189 asset CC0 (85 items, 49 nemici, 15 boss, 40 props) validati
    programmaticamente contro ledger (DEC-171); catalogo in `assets/curated/` +
    `assets/curated/manifest.json` con layer di indirezione contenuto→image-id (DEC-175).
    Catalogo generato con Gemma-3-4B per nomi/descrizioni SFX (default test, DEC-140).
  - Art direction: paletta ufficiale «Fucina di Worldsmelt» 31 colori (DEC-173); stile
    pixel-art S1 con outline nero, scala base sprite 32px (DEC-176/177); tileset 5 temi
    fallback a contratto con layer di indirezione ruolo→tile; HUD reale e prop (piedistallo,
    porta, pickup, cassa); prototipo full character (Fonditrice: walk 4 dir, idle, hit,
    death) + creature non umanoide (ragno con zampe snodate, goblin animato); SFX generati
    offline in attesa di verdetto audio del proprietario (assets/audio/sfx/ non committato).
  - Build: alias `make run-demo` per la demo senza generazione; toolchain riproduttibile.

- **Test**: in verifica finale.
  - `make test-script`, `test-gen`, `test-sprites` — status: verificati oggi, attesa risultati
    full.
  - `make test` — `--states-test` con catalog vuoto (policy: demo curata isolata da artefatti
    di run locali); `test-llm` con Gemma default (DEC-140).
  - Nessun nuovo difetto identificato dalla maratona; gap aperti sono *dich iarati* come
    known-issues, vedi sotto.

- **WIP / blocchi**: nessuno sulla main (git status clean). Nessuna decisione in stato
  `draft` nel decision-log (DEC-001..DEC-178).

- **Gap dichiarati** (non regressioni, intentional skip per scope della demo):
  - known-issues.md #8: pool curato minimo di melting-gen (15 posizioni) NON rispetta ancora
    il floor di rarità DEC-144 al tempo di generazione (contenuto di ripiego lato motore sì,
    generato no). Versione attuale: estraiesti rarità per-oggetto senza garanzia di
    copertura. → richiede modifica gen_fallback.c + coordinamento con manifest
    generato di melting-gen.
  - known-issues.md #9: audio Piano 0 assente finché non arrivano due SFX dedicati
    (AppConfirmThemeChoice/AppOpenFloorZeroExit restano muti; DEC-121 non può restare
    senza hook). → attesa asset curato dal dominio audio.
  - ~~Prove Piano 0: arene opzionali best-of con simulazione senza economia~~ — **CHIUSO
    dal WP15a (30/07)**: `src/world/floor_zero_arena.{h,c}`, tre piazzole nel crogiolo,
    snapshot/ripristino integrale del Player (DEC-092), nessuna economia (DEC-093), morte
    che non e' mai un game over (DEC-055), contenuti best-of dal catalogo con ripiego
    curato (DEC-087/094) e tutorial integrato alla prima visita (DEC-047). Verificato da
    `--arena-hub-test`. Restano tre limiti DICHIARATI in known-issues.md voce 16: la dote
    di DEC-029 non e' implementata, il "tutorial gia' visto" non e' persistito su disco, il
    criterio best-of e' un default proposto (open question 50).
  - Slider volumi in Options: il modulo audio espone AudioSetMasterVolume/MusicVolume/SfxVolume
    (default 1.0), ma APP_OPTIONS resta minima (una voce "Indietro") — aggiungere la UI è
    blocco fuori da scope "modulo audio" della demo; vedi docs/engineering/known-issues.md voce 9.
  - Stanze speciali: ROOM_FUSION non esiste yet (DEC-023/243). La fusione innesca da
    BuildScreen fino all'arrivo della stanza (registrato in src/gameplay/fusion.h,
    FUSION_STAGE_2_HOOK e ROOM_FUSION_TODO). Comportamento demo: BuildScreen innesca e
    esegue davvero, coerente con lo stato temporaneo della demo (DEC-171).
  - Stadio 2 fusione: DEC-023 prevede rifinitura IA in sottofondo; demo non la contiene
    (nessun modello immagine a runtime, DEC-171). Contenuto composto resta "composto",
    non sale a "nuovo". → hook pronto in fusion.h, processo mai avviato.

- **Revisione finale (2026-07-28 notte)**: accumulo di 10 domande da implementazione
  (scratchpad/questions-night.md) verificato contro decision-log + open-questions.md:
  **tutte e 10 già risolte da DEC-141, DEC-170×5, DEC-167×2, DEC-161, DEC-162.** → 0 nuove
  domande aperte. Open questions rimangono 21 voci (numerate 1-11, 13-22; la 12 chiusa da
  DEC-176). La coda è pubblica in `docs/design/governance/open-questions.md`.

## Rifacimento UI (mandato confermato dal proprietario, 02/08)

Flusso concordato: mostro com'è → propongo → conferma → implemento, con screenshot
reale di ogni schermata appena rivestita. Direzione APPROVATA (v2 del 02/08):
**risoluzione interna 640×360** (esegue DEC-200), **palette 100% Fucina** (via il
neon rosa/ciano/arancio), **pannelli tonali** (fill slag-scuro, bevel 2 toni
slag-caldo sopra / slag-nero sotto, MAI cornici colorate 1px), **focus** = barra
fiamma 4px + testo oro-fuso su riga slag-caldo, **font di gioco in due taglie**
(5px base, ×2 per titoli/voci di menu/numeri HUD/riga PIANO — correzione
esplicita del proprietario: "scritte tagliate e tutto troppo piccolo" riferita
alla v1 a taglia singola). Mock di riferimento in scratchpad sessione
(mock-mainmenu.png, mock-gameplay.png). Ordine: fondamenta+token → MainMenu →
HUD → BuildScreen → RunResults → le altre. Scala agenti: WP-UI-0 (trasversale) a
opus giudicato da Fable; reskin per-schermata a sonnet giudicati da opus.
Screenshot: `bin/melting_run_gpu --art-screens-screenshot-test` → logs/worldsmelt-w8-*.png.
PERSONAGGI CONGELATI (il proprietario non approva il metodo sprite attuale;
charrig resta ma non si aggancia nulla senza suo ok). SFX bocciati, in coda.

## Prossimi passi

1. **Playtest completo del proprietario** (`make run-demo`): i 5 archetipi speciali, prove
   della run, arene del Piano 0, sospensione/Continua, Crust, timer, i 3 personaggi
   distinti. Resta sua la sessione CP4 GUI (con mandato 640×360 da DEC-200).
   **Verdetto SFX ARRIVATO (31/07 sera, ascolto remoto): BOCCIATI** — "meccanici ed
   elettrici", troppo aggressivi ("earrape") in alcuni punti, e sospetti hook senza
   suono dopo l'implementazione completa. IN CODA (dopo lo stile grafico): rifare la
   famiglia SFX via pipeline DEC-109 + censimento degli hook audio scoperti; le
   modifiche audio nel working tree restano non committate in attesa del remake.
2. **Feedback → bilanciamento**: i valori sono default proposti (DEC-019 + open-questions
   3/28-58); il primo giro di tuning parte dal feedback del playtest.
3. **Batch decisionale**: le open questions sono arrivate a ~58 voci, molte con default
   proposto implementato — una sessione con worldsmelt-decision-facilitator può chiuderne
   parecchie in un colpo (candidate: 22-27, 31-58).
4. **Differiti della sessione** (vedi sopra): museo Piano 0 e meta-progressione Embers sono
   i due lavori grossi successivi; i quick-win gen-side (guardrail originalità) sono brevi.
5. **Training Style LoRA su Kaggle** (DEC-148/168) e **mechanics-lab** (DEC-165) restano i
   binari di medio periodo, invariati dalla sessione precedente.

## Orientarsi

- **Avvio**:
  - `make run-demo` — demo curata, nessun modello, contenuti di ripiego; esegue in `build/demo`.
  - `make run` — usa `generated/` della repo (ultimo manifest generato o fallback); esegue in `build/worldsmelt`.
  - `make run-gen` / `make run-gen-fast` — pipeline completa con generazione di contenuti.
  - Vedi README.md e doc di ogni target make.
- **Implementazione**: CLAUDE.md (scala agenti) + AGENTS.md (regole moduli per C/Lua/Python).
- **Documentazione/design**: docs/CLAUDE.md + docs/design/README.md (percorso curato).
- **Indice generale**: docs/INDEX.md (rigenera con `make docs-index` dopo ogni commit su docs/).

## Night run 02→03/08 — mandato notturno del proprietario

"Approvo tutto" sulla review UI (11/11 schermate + le 3 scelte segnalate:
pausa con ABBANDONA in fiamma, risorse a stringa ambra, MONDO: riempito nel
Piano 0 — la riga vuota attuale è un bug da correggere). In più: usare tutti
gli sprite/animazioni dai materiali esistenti e GENERARE i mancanti; piena
autonomia fino a demo pronta. Piano notte: WP-UI-0 fondamenta (opus, in
volo) → WP-UI-1 HUD → WP-UI-2 Build+Fusione → WP-UI-3 famiglia menu
(setup/pausa/uscita/opzioni) → WP-UI-4 risultati+Piano0+catalogo — tutti
SEQUENZIALI (stessi file renderer), sonnet+verifica opus, commit a verdetto.
In parallelo: censimento entità senza sprite → produzione dei mancanti con
le pipeline esistenti. Personaggi giocabili: restano gli sheet attuali (il
redo è congelato finché il proprietario non approva il metodo).
