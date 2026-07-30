# HANDOFF

Stato sintetico del lavoro. La cronologia completa delle sessioni passate è in
`docs/archive/handoffs/` (integrale precedente: `docs/archive/handoffs/HANDOFF-2026-07-19.md`).

## Stato al 2026-07-30 — W8: il motore consuma gli asset artistici veri

- **Branch**: `main`. **NON committato**: in attesa del giudizio di Fable (task di gradino 3).
- **Il problema che chiudeva**: `make run-demo` mostrava ancora la grafica vecchia. Le 73
  coppie spritesheet+manifest della sessione artistica erano su disco e nessuna riga di C le
  leggeva; il commit B3 ("aggancio completo al layer image-id") aveva agganciato solo il
  ramo OGGETTI attraverso il ponte CC0 statico — nemici, boss, tileset, HUD e personaggio no.
- **Moduli nuovi** (confini documentati in `docs/engineering/architecture.md`):
  - `src/assets/art_atlas.{h,c}` (`ArtAtlas*`): carica e POSSIEDE gli asset di `assets/art/`.
    Scanner sequenziale minimale per i tre sapori di manifest (spritesheet + `slice` per i
    9-patch, tileset, font bitmap) — il gioco non linka cJSON; ogni chiave sconosciuta viene
    saltata, così un contratto esteso domani si carica oggi. Animatore PURO e deterministico
    (`ArtAnimFrameAt`/`ArtAnimDone`, nessun `GetTime` dentro). Registro statico al modulo e
    non in `Game`: gli asset non sono contenuto di run e `Game` viene azzerato più volte per
    partita. Caricamento pigro, voci negative in cache, `ArtAtlasShutdown` accanto ad
    `AudioShutdown`.
  - `src/render/art_draw.{h,c}` (`ArtDraw*`): DISEGNA. Fotogramma ancorato con specchiatura,
    tile con ritaglio proporzionale del sorgente, 9-patch a bordi ripetuti (non stirati),
    font pixel, icone. `ArtUiReady()` è l'interruttore unico fra veste pixel art e ripiego.
- **Cosa si vede ora**: tileset dei 5 temi (pavimento con varianti deterministiche, cornice,
  angoli, porte a 3 stati, `l_block`, ostacoli per famiglia `ROOM_LAYOUT_*`, vuoto, variante
  di escalation `_deg` dal piano 3); personaggio animato (4 camminate/idle/hit) al posto
  dello stickman, coi layer degli oggetti sopra invariati; nemici e boss animati con flip e
  `hit`; animazione di MORTE via `Game.artFx` (effetti solo grafici che sopravvivono
  all'entità — un nemico esce di scena nell'istante in cui muore, e cambiare quella
  semantica avrebbe toccato ogni `if (e->active)` del motore); colpi per forma; HUD in
  pixel art nel canvas 960×640 col layout V3 approvato al CP2, numero per numero (DEC-174);
  **tutte le nove schermate** rivestite coi componenti di sistema; tre slider di volume in
  `Options`.
- **Priorità delle immagini** (DEC-175(b)): `Item.imageId`/`EnemyTypeDef.imageId`/
  `DiscoveryCard.imageId` nuovi accanto a `imagePath`, scritti dagli stessi punti che già
  scrivevano il percorso. Ordine: originale in `assets/art` → ponte CC0 di `assets/curated`
  → cella d'atlas → primitiva. Il seed non cambia di un bit: la pesca della fusione è la
  stessa, si conserva anche l'id della voce pescata.
- **Test**: `make test` (con `--art-atlas-test` nuovo), `make test-script`, `make docs-check`
  verdi. `--atlas-fallback-test` ora punta di proposito il pacchetto artistico su una
  cartella inesistente, così continua a proteggere il gradino più basso della priorità.
  Screenshot di verifica: `logs/worldsmelt-w8-<schermata>.png` (9 scatti).
- **Buchi dichiarati** (asset mancanti, non codice): `docs/engineering/known-issues.md`
  voce 10 — font senza accentate né parentesi tonde, un solo sheet di personaggio per tre
  personaggi, cuore/bomba/chiave senza prop, salute temporanea e timer di run assenti dal
  motore. Domande aperte 23-26 in `docs/design/governance/open-questions.md`.

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

## Prossimi passi

1. **Validazione finale**: `make docs-index && make docs-check` (eseguiti al termine della
   maratona, nessun docstring work-in-progress su docs/).
2. **Playtest**: con la demo giocabile e la lista di gap dichiarati, inizio raccolta feedback
   su economia, bilanciamento, leggibilità visiva (leggibilità formula DEC-146).
3. **Risposta alle open questions**: alcuni gap della demo dipendono da risposte alle 21 OQ
   (es. #9 per audio Piano 0, #13 per design tool UI, #18 per body plan e animazione).
   Richiede sessione decisionale dedicata.
4. **Training Style LoRA su Kaggle**: con DEC-148/168, il proprietario prepara i dataset
   definitivi per il training (~30 ore gratuite di Kaggle); demo intanto gira con asset CC0
   bridge (DEC-171).
5. **Aggancio asset art**: CP3c prototipo è un campione di produzione; il passo successivo è
   scalare la pipeline Aseprite a tutti i personaggi base (3 roster + generato-per-run) e
   boss/nemici principali. Timeline: attesa della revisione art dal proprietario + coordinate
   con il training LoRA.
6. **Mechanics-lab sbloccato** (DEC-165): pronto per esperimenti su game feel, danno,
   movimento, knockback, VFX. Richiede investigazione aggiuntiva su DEC-146 e DEC-163
   (proxy di leggibilità, template di contenuto).

## Orientarsi

- **Avvio**:
  - `make run-demo` — demo curata, nessun modello, contenuti di ripiego; esegue in `build/demo`.
  - `make run` — usa `generated/` della repo (ultimo manifest generato o fallback); esegue in `build/worldsmelt`.
  - `make run-gen` / `make run-gen-fast` — pipeline completa con generazione di contenuti.
  - Vedi README.md e doc di ogni target make.
- **Implementazione**: CLAUDE.md (scala agenti) + AGENTS.md (regole moduli per C/Lua/Python).
- **Documentazione/design**: docs/CLAUDE.md + docs/design/README.md (percorso curato).
- **Indice generale**: docs/INDEX.md (rigenera con `make docs-index` dopo ogni commit su docs/).
