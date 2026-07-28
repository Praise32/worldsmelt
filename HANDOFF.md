# HANDOFF

Stato sintetico del lavoro. La cronologia completa delle sessioni passate è in
`docs/archive/handoffs/` (integrale precedente: `docs/archive/handoffs/HANDOFF-2026-07-19.md`).

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
  - Prove Piano 0: il piano contiene arene opzionali (prove best-of di run passate) che
    richiedono la logica di simulazione ad hoc (generazione senza economia, nessun danno
    permanente). Struttura pronta (DEC-092/093), contenuti non ancora aggiunti al Piano 0.
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
