---
id: meta-doc-code-drift
title: Registro drift documentazione-codice (DOC-CODE-DRIFT-001..035)
domain: meta
status: approved
authority: supporting
owner: meta
summary: >-
  Divergenze fra cio' che i documenti dichiarano e cio' che il codice mostra,
  verificate con Codebase Memory + conferma diretta (file:riga) e giudice opus. 35 divergenze reali, 43 conferme di allineamento.
  Registro vivo (DEC-150): ogni voce porta oggi un campo Stato (aperta/applicata/superata)
  verificato il 27/07.
last_reviewed: 2026-07-27
last_verified_commit: d30890b
topics: [audit, drift, codice, registro-vivo]
related: [meta-doc-conflicts]
supersedes: []
source_files: []
---

# Registro drift documentazione ↔ codice

Classificazioni: doc-superato, doc-impreciso, codice-avanti (il codice fa piu' di quanto
il doc dichiari), codice-indietro, incerto. Le voci 'allineato' (in coda) dimostrano che
le affermazioni portanti sono state VERIFICATE, non dedotte.

**Registro vivo (DEC-150).** Ogni voce numerata (001-035) porta un campo **Stato** —
*aperta* / *applicata* / *superata* — con la DEC o il commit che l'ha chiusa, verificato
il 27/07/2026 contro lo stato reale del repo. La sezione finale "Affermazioni portanti
confermate allineate" non riceve un campo Stato per riga: sono gia', per costruzione,
conferme di allineamento verificate all'audit del 22/07, non divergenze da chiudere.
Mappa dei percorsi pre-migrazione citati sotto: vedi la stessa tabella in
`docs/_meta/DOC-CONFLICTS.md` (identica per questo repo).

## DOC-CODE-DRIFT-001 — docs/ARCHITECTURE.md:8 [doc-superato]

- **Stato**: applicata — `docs/engineering/architecture.md` (migrato) descrive oggi i 9 stati reali di `AppMode`.
- **Affermazione (DOCUMENTED AS-IS)**: Diagramma: gen/GenRunner opzionale con --generate porta allo stato APP_GENERATING.
- **Osservato (OBSERVED AS-IS)**: L'enum AppMode non ha piu' APP_GENERATING: e' stato assorbito da APP_FLOOR_ZERO, che ospita la generazione come overlay bloccante. Nove stati reali: APP_MAIN_MENU..APP_EXIT_CONFIRM.
- **Evidenza**: src/core/game_types.h:177-187 (enum AppMode, 9 stati) e commento 168-176 ('FloorZero assorbe la vecchia schermata di generazione, mai uno stato a parte')
- **Azione**: Riscrivere il diagramma con i 9 stati reali di AppMode (o rimandare a game-design-knowledge-base/docs/game-design/05-game-states-and-flow.md) prima di spostare il doc in docs/engineering/.

## DOC-CODE-DRIFT-002 — docs/ARCHITECTURE.md:29 [doc-superato]

- **Stato**: applicata — `docs/engineering/architecture.md` tratta oggi `src/script/` come percorso canonico (sandbox Lua), con `script_vm.c`/mini-VM come rete di sicurezza.
- **Affermazione (DOCUMENTED AS-IS)**: gameplay/script_vm e' l'interprete dichiarativo sandboxato del gioco (unico sistema di script citato).
- **Osservato (OBSERVED AS-IS)**: src/gameplay/script_vm.c e' solo la mini-VM CSV a 4 operazioni, oggi rete di sicurezza; il percorso principale e' src/script/ (sandbox Lua 5.5: script_sandbox, script_api, script_items, script_character), assente dal doc.
- **Evidenza**: src/script/{script_sandbox,script_api,script_items,script_character}.{h,c}; Makefile:12,16 (LUA_LIB in GAME_LIBS); AGENTS.md:19,32-45
- **Azione**: Aggiungere src/script come modulo canonico (4 sotto-file, confine 'combat.c non include mai lua.h') prima della migrazione a docs/engineering/.

## DOC-CODE-DRIFT-003 — docs/ARCHITECTURE.md:40 [doc-superato]

- **Stato**: applicata — `docs/engineering/architecture.md` descrive la sandbox Lua come stato attuale, con le suite `script_sandbox_tests.c` ecc. citate.
- **Affermazione (DOCUMENTED AS-IS)**: Un'eventuale sandbox Lua dovra' avere un modulo e test di sicurezza propri (presentato come lavoro futuro).
- **Osservato (OBSERVED AS-IS)**: La sandbox Lua e' gia' implementata in src/script/ con suite dedicata (script_sandbox_tests.c, script_items_tests.c, script_character_tests.c) e target make test-script.
- **Evidenza**: src/tests/script_sandbox_tests.c, script_items_tests.c, script_character_tests.c; Makefile:162-163 'test-script: game'; AGENTS.md:71,87
- **Azione**: Rimuovere il framing 'eventuale/futuro': la sandbox Lua e' lo stato attuale. Riscrivere il paragrafo o archiviare la pagina come superseded ripartendo da AGENTS.md.

## DOC-CODE-DRIFT-004 — docs/ARCHITECTURE.md:40 [doc-superato]

- **Stato**: applicata — corretto in `docs/engineering/architecture.md`: la mini-VM è oggi descritta come rete di sicurezza, il percorso principale è Lua.
- **Affermazione (DOCUMENTED AS-IS)**: La mini-VM rimane il comportamento predefinito.
- **Osservato (OBSERVED AS-IS)**: AGENTS.md descrive la mini-VM (ScriptVm/script_vm.c) come 'la rete di sicurezza': ogni oggetto generato riceve uno script Lua e ricade sulla mini-VM solo in caso di fallita generazione/validazione. Il percorso principale e' Lua, non la mini-VM.
- **Evidenza**: AGENTS.md:19 ('ScriptVm ... la rete di sicurezza'), AGENTS.md:62 ('su fallimento l'oggetto resta sulla mini-VM'); src/script/ come sandbox principale
- **Azione**: Correggere: la mini-VM e' oggi la rete di sicurezza/fallback, non il comportamento predefinito; il percorso principale e' Lua (src/script). Divergenza sfuggita all'analista, stessa riga 40.

## DOC-CODE-DRIFT-005 — docs/ARCHITECTURE.md:40 [doc-superato]

- **Stato**: applicata — `docs/engineering/architecture.md` registra Raygui come già integrata in `src/render/raygui_impl.c`, notando che `src/ui` separato 'non esiste ancora nel repo'.
- **Affermazione (DOCUMENTED AS-IS)**: La futura integrazione Raygui dovra' vivere in un modulo src/ui separato dal rendering del mondo.
- **Osservato (OBSERVED AS-IS)**: Raygui e' gia' integrata (RAYGUI_IMPLEMENTATION) in src/render/raygui_impl.c, dentro render e non in un src/ui separato; la cartella src/ui non esiste.
- **Evidenza**: src/render/raygui_impl.c:1-15 (define RAYGUI_IMPLEMENTATION, vendor deps/raygui/ 4.5.0); assenza di src/ui/ (ls fallisce)
- **Azione**: La 'futura integrazione' e' gia' avvenuta, ma dentro src/render. Verificare con l'engineering se la separazione src/ui e' ancora un obiettivo: se no correggere il claim; se si', spostarlo in docs/plans/active/ invece che in un doc architetturale descrittivo.

## DOC-CODE-DRIFT-006 — docs/ARCHITECTURE.md:26-29 [doc-impreciso]

- **Stato**: applicata — `docs/engineering/architecture.md` cita oggi `gameplay/synergies.c`.
- **Affermazione (DOCUMENTED AS-IS)**: Sotto-moduli di gameplay: combat, entities, item_traits, script_vm (nessun altro).
- **Osservato (OBSERVED AS-IS)**: src/gameplay contiene anche synergies.c/h, responsabilita' distinta (sinergie tra oggetti) non citata dal doc.
- **Evidenza**: src/gameplay/synergies.c, src/gameplay/synergies.h presenti; assenti dal testo del doc
- **Azione**: Aggiungere una riga 'gameplay/synergies: sinergie tra oggetti' all'elenco dei moduli in fase di migrazione.

## DOC-CODE-DRIFT-007 — docs/ARCHITECTURE.md:32 [doc-impreciso]

- **Stato**: applicata — `docs/engineering/architecture.md` elenca oggi le suite reali di `src/tests` (`game_tests.c`, `catalog_tests.c`, `script_sandbox_tests.c`, ecc.).
- **Affermazione (DOCUMENTED AS-IS)**: tests: test del portale e della mini-VM.
- **Osservato (OBSERVED AS-IS)**: src/tests contiene anche catalog_tests.c, script_character_tests.c, script_items_tests.c, script_sandbox_tests.c, game_tests.c: copre soprattutto la sandbox Lua e il catalogo, non solo portale/mini-VM.
- **Evidenza**: ls src/tests: catalog_tests.c, game_tests.c, script_character_tests.c, script_items_tests.c, script_sandbox_tests.c
- **Azione**: Sostituire la descrizione puntuale con un rimando a 'make test'/AGENTS.md, oppure elencare le suite attuali.

## DOC-CODE-DRIFT-008 — docs/OPENAI_SETUP.md (intero file) [doc-impreciso]

- **Stato**: applicata — `docs/OPENAI_SETUP.md` (oggi `docs/archive/superseded/openai-setup.md`) è stato spostato in archivio; README.md dichiara oggi esplicitamente il percorso OpenAI 'storico ... non è stato rimosso' ma non è il riferimento.
- **Affermazione (DOCUMENTED AS-IS)**: Presenta la Responses API + Image API di OpenAI come modalita' di generazione contenuti, senza segnalare che e' un percorso secondario/storico rispetto al motore locale.
- **Osservato (OBSERVED AS-IS)**: Il motore di gioco (src/app/app.c:1124-1125) usa di default bin/melting-gen e bin/melting-sprites; il percorso OpenAI in llm/ non e' collegato ne' menzionato dal codice C. Confermato: grep nel doc per 'storico|deprecat|percorso' non trova alcun banner di framing (solo righe modelli/endpoint).
- **Evidenza**: src/app/app.c:1124-1125 (gen.command="bin/melting-gen", gen.spritesCommand="bin/melting-sprites") verificato; grep 'storico|deprecat|percorso' docs/OPENAI_SETUP.md -> solo righe 18/20/115/116 (modelli+endpoint), nessun banner; README.md:10-12 invece dice 'percorso storico... non e' stato rimosso'.
- **Azione**: Aggiungere in cima al file un banner 'percorso storico, non piu' primario' allineato a README.md, poi spostarlo in docs/archive/legacy-notes/ o docs/references/ secondo la mappa di migrazione (nessun contenuto di design).

## DOC-CODE-DRIFT-009 — llm/, generate_llm_content.bat, start_llm_server.bat, .env.example, docs/OPENAI_SETUP.md (l'intero sottosistema) [incerto]

- **Stato**: applicata (parziale) — README.md marca oggi esplicitamente `llm/` come percorso storico non di riferimento; nessun ADR dedicato è stato scritto e `llm/` resta nel repo senza rimozione, ma il rischio di lettura come design corrente è mitigato dal framing del README.
- **Affermazione (DOCUMENTED AS-IS)**: I file esistono ancora e non sono marcati come deprecati/rimossi: rischiano di essere letti come design/architettura corrente.
- **Osservato (OBSERVED AS-IS)**: Verificato: il decision-log (DEC-001..108) non menziona mai 'OpenAI'; ultimo commit su llm/ e OPENAI_SETUP.md e' b3f8857 (9 luglio 2026), precedente alla Fase 1. Sottosistema vivo (fetch reali, .bat/server funzionanti) ma architetturalmente orfano: nessun modulo in src/ o tools/ lo richiama.
- **Evidenza**: grep 'openai' game-design-knowledge-base/.../decision-log.md -> nessun match; git log -1 -- llm/ e -- docs/OPENAI_SETUP.md -> b3f8857 2026-07-09; nessun caller C.
- **Azione**: Decisione per Fable/PM: se il percorso OpenAI resta alternativa ufficiale, formalizzare un ADR in docs/engineering/adr/ e spostare i doc in docs/engineering/ o docs/references/; se e' residuo di prototipo, spostare tutto in docs/archive/superseded/ e valutare rimozione di .bat/.mjs in task separato. Non cancellare silenziosamente (README dichiara 'non e' stato rimosso').

## DOC-CODE-DRIFT-010 — README.md "Avvio rapido" (righe 18-25, quickstart Windows con build_gpu.bat + run_gpu.bat) [doc-superato]

- **Stato**: applicata — README.md non contiene più il quickstart `build_gpu.bat`/`run_gpu.bat`: il Quick Start unico oggi è 'Avvio rapido (Linux, percorso di riferimento)'.
- **Affermazione (DOCUMENTED AS-IS)**: Per avviare il gioco su Windows basta build_gpu.bat poi run_gpu.bat.
- **Osservato (OBSERVED AS-IS)**: build_gpu.bat globa ricorsivamente tutti gli src/*.c (incluso src/script/script_sandbox.c che #include lua.h/lauxlib.h/lualib.h) ma NON aggiunge -I per deps/lua-5.5.0 ne' -llua: la compilazione fallisce. Il .bat e' fermo a b3f8857 (2026-07-09, non 13/07), precedente al commit 1c592cb che ha aggiunto la sandbox Lua.
- **Evidenza**: build_gpu.bat:25-29 (solo -I src, -I raylib include, -lraylib -lopengl32 -lgdi32 -lwinmm; nessun Lua); src/script/script_sandbox.c:1-7 (#include lua.h/lauxlib.h/lualib.h); Makefile:15-16 (GAME_CFLAGS -I$(LUA_DIR)/src, GAME_LIBS $(LUA_LIB)); git log build_gpu.bat -> b3f8857 2026-07-09; commit 1c592cb 'vendor Lua 5.5.0 and build a locked-down sandbox' successivo.
- **Azione**: Retrocedere il quickstart .bat Windows in docs/archive/legacy-notes/ come percorso storico non funzionante sul codice attuale; promuovere "Avvio rapido su Linux" a unico quickstart nel nuovo README/docs/engineering, con nota che i .bat vanno rigenerati (mancano Lua/raygui) o rimossi.

## DOC-CODE-DRIFT-011 — README.md "Limiti intenzionali" (righe 271-276) e "Pipeline dinamica" (righe 111-124) [doc-superato]

- **Stato**: applicata — README.md non presenta più gli sprite locali come fase futura; il testo attuale cita `tools/melting-sprites` come parte della pipeline di riferimento.
- **Affermazione (DOCUMENTED AS-IS)**: Il percorso locale genera solo testo; gli sprite generati in locale sono una fase futura della roadmap.
- **Osservato (OBSERVED AS-IS)**: bin/melting-sprites (Stable Diffusion via stable-diffusion.cpp) e' gia' cablato come secondo stadio di default: spritesCommand di default 'bin/melting-sprites' e avvio automatico dopo melting-gen.
- **Evidenza**: src/app/app.c:1125 (gen.spritesCommand = "bin/melting-sprites"); src/app/app.c:225-236 (AppStartSpritesGeneration) e 822-832 (inSpritesStage, avvio automatico); Makefile:63-84 (SPRITES_BIN, 'fase 2, S3'); commit c40b54d 'wire melting-sprites into the game as a two-stage generation pipeline' e 344a362.
- **Azione**: Rimuovere la frase su sprite locali come 'fase futura': registrare in docs/design o docs/engineering che la generazione sprite locale via stable-diffusion.cpp e' gia' implementata e di default; l'unico limite residuo e' la qualita' visiva (LoRA da riallenare, nota 18/07).

## DOC-CODE-DRIFT-012 — README.md "Test" (righe 175-205) [doc-impreciso]

- **Stato**: applicata — README.md non elenca più manualmente i singoli flag di test; il testo attuale rimanda a `make test`/`test-gen`/`test-script`/`test-sprites`/`test-llm`.
- **Affermazione (DOCUMENTED AS-IS)**: I test disponibili sono solo --smoke-test, --portal-test, --script-test e --screenshot-test.
- **Osservato (OBSERVED AS-IS)**: Il target `make test` lancia 15 flag; 11 non compaiono nel README (--states-test, --floor-zero-test, --rooms-test, --catalog-test, --catalog-screen-test, --gen-test, --layout-test, --bench-preset-test, --atlas-fallback-test, --layer-test, --shot-forms-screenshot-test).
- **Evidenza**: Makefile:140-154 (15 flag in test:); README.md:179-198 (solo 4 flag).
- **Azione**: Sostituire l'elenco manuale con un rimando a `make test` come fonte unica; se serve il dettaglio flag-per-flag, spostarlo in docs/engineering/testing.md aggiornato insieme al Makefile.

## DOC-CODE-DRIFT-013 — README.md "Script sandboxati" (righe 153-173) [doc-impreciso]

- **Stato**: applicata — README.md sez. 'Script sandboxati' cita oggi esplicitamente Lua, non solo la mini-VM dichiarativa.
- **Affermazione (DOCUMENTED AS-IS)**: L'LLM genera solo istruzioni dichiarative floor1.item1.script=on_fire:... interpretate da una mini VM interna; nessuna menzione di Lua.
- **Osservato (OBSERVED AS-IS)**: La mini-VM esiste come mappa trait->stringa di fallback in run_content.c, ma dalla fase 3a-L3 il generatore scrive anche script Lua per oggetto (WriteItemLua) eseguiti nella sandbox Lua linkata nel gioco (deps/lua-5.5.0); il README non menziona mai Lua.
- **Evidenza**: tools/melting-gen/gen_manifest.c:84,300-302 (WriteItemLua); src/content/run_content.c:208-219 (FallbackScriptForTrait, mappa on_fire/on_hit legacy); src/script/ (script_sandbox/api/character/items linkati nel gioco via GAME_SRC, GAME_LIBS $(LUA_LIB)); commit f416f28 'the LLM writes and validates item Lua scripts' e 1c592cb.
- **Azione**: Riscrivere la sezione scripting per documentare entrambi i livelli (mini-VM legacy per trait + sandbox Lua per script per-oggetto generati), allineandola a 'Lua e' la via principale' (16/07); verificare col decision-log prima di pubblicare.

## DOC-CODE-DRIFT-014 — README.md "Architettura" (righe 240-251, elenco cartelle src/) [doc-impreciso]

- **Stato**: applicata — la mappa dettagliata delle cartelle (incluso `src/script/`) vive oggi in `docs/engineering/architecture.md`, non più solo nel README.
- **Affermazione (DOCUMENTED AS-IS)**: Il codice src/ e' organizzato in app, assets, content, core, game, gameplay, gen, render, tests, world (piu' src/main.c e tools/melting-gen).
- **Osservato (OBSERVED AS-IS)**: L'elenco omette src/script/, cartella reale che ospita la sandbox Lua e l'API di scripting (script_sandbox.c, script_api.c, script_character.c, script_items.c) - centrale nel design attuale. Le cartelle elencate esistono tutte, ma la mappa e' incompleta.
- **Evidenza**: ls src/ -> presente 'script' oltre alle 10 elencate; README.md:240-251 non nomina mai src/script; grep 'src/script' README.md -> nessun risultato.
- **Azione**: Aggiungere src/script/ alla mappa cartelle (sandbox Lua + API di scripting) nella migrazione verso docs/engineering/ARCHITECTURE.md.

## DOC-CODE-DRIFT-015 — docs/SPRITES-SPIKE.md:15 [doc-superato]

- **Stato**: aperta — verificato: `docs/ai-production/experiments/sprites-spike.md` (migrato) riporta ancora `--cfg-scale 1.5`, non aggiornato a 1.8 come nel codice attuale (`tools/melting-sprites/main.c`); nessuna correzione applicata.
- **Affermazione (DOCUMENTED AS-IS)**: Configurazione misurata: 512x512, 8 passi, --sampling-method lcm, --cfg-scale 1.5, --vae-conv-direct.
- **Osservato (OBSERVED AS-IS)**: Il default di produzione e' cfg=1.8, non 1.5: a 1.5 il modello ignorava il soggetto (commento esplicito). Il commento riassuntivo in sprite_sd.c cita gia' 1.8 attribuendolo allo spike, in contraddizione col .md.
- **Evidenza**: tools/melting-sprites/main.c:62-66 (args->cfg = 1.8f; commento 'a 1.5 il modello ignorava il soggetto'); tools/melting-sprites/sprite_sd.c:9 (commento 'cfg 1.8 ... misurati nello spike, vedi docs/SPRITES-SPIKE.md')
- **Azione**: Aggiornare cfg a 1.8 (o annotare che 1.5 era il valore iniziale poi rivisto) quando il contenuto confluisce in docs/ai-production/; il .md descrive solo lo snapshot 13/07.

## DOC-CODE-DRIFT-016 — docs/SPRITES-SPIKE.md:23 [doc-superato]

- **Stato**: aperta — stesso file riporta ancora '12 sprite', non le 13 celle attuali (`SPRITE_CELLS` in `melting_sprites.h`); nessuna nota di scostamento aggiunta.
- **Affermazione (DOCUMENTED AS-IS)**: 12 sprite (quelli che usa il gioco), tempo atlas ~75 s.
- **Osservato (OBSERVED AS-IS)**: SPRITE_CELLS e' oggi 13 (aggiunto enemy_floater in fase 3b), un'entita' in piu' rispetto alla misura originale.
- **Evidenza**: tools/melting-sprites/melting_sprites.h:31 (#define SPRITE_CELLS 13 /* fase 3b: +enemy_floater, aggiunta IN CODA */)
- **Azione**: In migrazione spostare le misure in docs/ai-production/ come dato storico datato (13/07, 12 sprite) senza correggerlo in-place; se serve un numero corrente va rimisurato con 13 celle.

## DOC-CODE-DRIFT-017 — docs/SPRITES-SPIKE.md:15 [codice-avanti]

- **Stato**: aperta — il file non menziona ancora `--gen-size`; il flag resta nel codice come opzione manuale (non più legato al preset lowspec, rimosso da DEC-110).
- **Affermazione (DOCUMENTED AS-IS)**: Implicito: la generazione avviene sempre a 512x512 (nessuna dimensione alternativa).
- **Osservato (OBSERVED AS-IS)**: E' stato aggiunto --gen-size (256 o 512) per il preset --low-spec, parametro runtime assente dallo spike.
- **Evidenza**: tools/melting-sprites/main.c:15-21,46,69,85,101-104; tools/melting-sprites/melting_sprites.h:11-20 (commento 'Dalla fase --gen-size ... roadmap 16/07/2026')
- **Azione**: Non e' errore del doc (feature del 16/07, spike del 13/07): segnalare come nota storica pre-feature, con rimando al doc/ADR che introduce --gen-size.

## DOC-CODE-DRIFT-018 — docs/BENCHMARKS.md [codice-avanti]

- **Stato**: superata (DEC-110, 22/07) — il meccanismo di tier automatico è stato rimosso dal codice; `docs/engineering/benchmarks.md` (migrato) lo documenta oggi come storia, con la disambiguazione fra i due meccanismi.
- **Affermazione (DOCUMENTED AS-IS)**: Presenta la tabella come unico riferimento e parla di una 'futura funzione scegli il modello in base alla macchina' (righe 43-45).
- **Osservato (OBSERVED AS-IS)**: Il rilevamento automatico del tier (full/lowspec/unsupported) via scripts/benchmark.sh + logs/benchmark.txt + AppReadBenchmarkPreset e' gia' implementato 4 giorni dopo, mai menzionato nel doc.
- **Evidenza**: docs/BENCHMARKS.md:43-45 (funzione 'futura'); scripts/benchmark.sh:65-83 (logica tier); src/app/app.c:44-78 (AppReadBenchmarkPreset); git: b836e96 (13/07 crea BENCHMARKS.md) precede 70a9d06 (17/07 'automatic tier selection')
- **Azione**: In migrazione spostare in docs/engineering/; riscrivere la frase sulla 'funzione futura' (implementata come soglia singola tokS/imgS, non uno sweep ngl per-macchina come la tabella lasciava intendere) e linkare scripts/benchmark.sh e AppReadBenchmarkPreset.

## DOC-CODE-DRIFT-019 — docs/BENCHMARKS.md [doc-impreciso]

- **Stato**: applicata — `docs/engineering/benchmarks.md` ha oggi front matter completo e data/commit nel testo.
- **Affermazione (DOCUMENTED AS-IS)**: Nessun front matter; i numeri riportano la macchina ma non una data ne' un commit di riferimento.
- **Osservato (OBSERVED AS-IS)**: DOCUMENT-STANDARDS richiede front matter YAML con status e last_verified_commit (obbligatorio per approved/implemented); BENCHMARKS.md non ne ha; la data reale (13/07/2026, b836e96) e' desumibile solo da git.
- **Evidenza**: docs/BENCHMARKS.md:1-3 (nessun front matter/data); docs/_meta/DOCUMENT-STANDARDS.md:49-77 (front matter e last_verified_commit obbligatori); git b836e96 2026-07-13 09:29
- **Azione**: Aggiungere front matter (status, last_verified_commit, last_reviewed) prima di spostare il file in docs/engineering/. Nota: e' una lacuna di standard piu' che una contraddizione doc-codice.

## DOC-CODE-DRIFT-020 — docs/BENCHMARKS.md [codice-avanti]

- **Stato**: applicata — `docs/engineering/benchmarks.md` distingue oggi esplicitamente i due meccanismi ('Disambiguazione: due "tok/s" diversi nel repo').
- **Affermazione (DOCUMENTED AS-IS)**: logs/benchmark.txt e scripts/benchmark.sh non sono citati; il doc descrive solo lo sweep manuale via make test-llm.
- **Osservato (OBSERVED AS-IS)**: logs/benchmark.txt e' una misura del meccanismo automatico (benchSchema/tokS/imgS/tier), schema e semantica del tutto diversi dalla tabella (load/gen/totale/token/tok/s/VRAM per modello e ngl).
- **Evidenza**: logs/benchmark.txt:1-5 (benchSchema=1, tokS=42.29, imgS=5.62, tier=full); scripts/benchmark.sh:76-83 (formato chiave=valore); docs/BENCHMARKS.md:9-15 (formato tabellare, nessun benchSchema/tier)
- **Azione**: Chiarire i due meccanismi complementari (BENCHMARKS.md = calibrazione una tantum dei default in tools/melting-gen/main.c; benchmark.sh/benchmark.txt = tier per-utente a runtime) con cross-reference. Sovrapposto al finding 1: valuta di consolidarli nel doc migrato.

## DOC-CODE-DRIFT-021 — docs/superpowers/plans/2026-07-13-linux-local-llm.md [doc-superato]

- **Stato**: applicata — spostato in `docs/archive/historical-plans/2026-07-13-linux-local-llm.md`; le checkbox restano non aggiornate (residuo cosmetico, la collocazione segnala comunque lo stato storico).
- **Affermazione (DOCUMENTED AS-IS)**: Tutti gli step dei 12 task sono marcati '- [ ]' non fatti, incluso il 'Commit' finale di ogni task
- **Osservato (OBSERVED AS-IS)**: Piano interamente implementato ma nessuna checkbox aggiornata: 82 righe '- [ ]', zero '- [x]'; tools/melting-gen popolato con 16 file .c/.h nel dir (+2 in vendor)
- **Evidenza**: grep '- [ ]' = 82 occorrenze, grep '- [x]' = 0; ls tools/melting-gen/: main.c 57319B, gen_manifest.c 34863B (18/19-07)
- **Azione**: Archiviare in docs/archive/historical-plans/ (piano completato mai chiuso); non usarlo come tracker di stato attivo

## DOC-CODE-DRIFT-022 — docs/superpowers/plans/2026-07-13-linux-local-llm.md [codice-avanti]

- **Stato**: applicata — stesso spostamento; la Task 10 (APP_GENERATING) resta descritta come storica, lo stato reale (`AppMode` a 9 valori, `APP_FLOOR_ZERO`) è documentato in `docs/engineering/architecture.md`.
- **Affermazione (DOCUMENTED AS-IS)**: Task 10: enum AppMode { APP_MENU, APP_PLAY, APP_PAUSE, APP_GENERATING } con overlay dedicato; passaggio a APP_GENERATING all'avvio generazione
- **Osservato (OBSERVED AS-IS)**: AppMode reale ha 9 valori (APP_MAIN_MENU..APP_EXIT_CONFIRM, M1a) senza APP_GENERATING; generazione dentro APP_FLOOR_ZERO via 4 GenRunner distinti (lazyRunner/proposeRunner/runner/spritesRunner)
- **Evidenza**: src/core/game_types.h:177-187 (enum senza APP_GENERATING) vs piano righe 2656-2693; src/app/app.c:158,221,236,342 (GenRunnerStartWithArgs su 4 runner)
- **Azione**: Archiviare la sezione Task 10 (superata da M1a/M1b); documentare lo stato reale ex novo in docs/engineering/ dal codice

## DOC-CODE-DRIFT-023 — docs/superpowers/plans/2026-07-13-linux-local-llm.md [codice-avanti]

- **Stato**: applicata — stesso spostamento; la firma reale di `RendererDrawApp` è verificabile oggi in `src/render/game_renderer.h`, non più cercata nel piano.
- **Affermazione (DOCUMENTED AS-IS)**: Task 10: firma renderer 'void RendererDrawApp(Game*, RenderTexture2D, AppMode, bool takeScreenshot, const GenProgress*)'
- **Osservato (OBSERVED AS-IS)**: Firma reale 'void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, const AppUi *ui, ...)': niente param GenProgress, passa via AppUi
- **Evidenza**: src/render/game_renderer.h:43 vs piano riga 2650
- **Azione**: Non riportare la firma come riferimento; eventuale API doc va rigenerata da game_renderer.h corrente

## DOC-CODE-DRIFT-024 — docs/superpowers/plans/2026-07-13-linux-local-llm.md [codice-avanti]

- **Stato**: applicata — stesso spostamento; la mappa file aggiornata di `tools/melting-gen/` vive oggi in `docs/engineering/dependencies.md` e `docs/engineering/architecture.md`.
- **Affermazione (DOCUMENTED AS-IS)**: Struttura file (righe 26-54) elenca solo 8 sorgenti per tools/melting-gen e nessun modulo Lua
- **Osservato (OBSERVED AS-IS)**: Dir reale contiene anche gen_lua.c/.h (sandbox Lua script oggetti), gen_inspire, gen_novelty, gen_corpus, character.gbnf, propose.gbnf — moduli di fasi successive mai citati nel piano (0 menzioni)
- **Evidenza**: piano righe 34-44 (8 sorgenti); ls tools/melting-gen/: gen_lua.c 48479B (19/07), gen_inspire/novelty (18/07), gen_corpus (16/07); grep gen_lua|gen_inspire|... nel piano = 0
- **Azione**: Archiviare com'e' (fotografia iniziale); la mappa file aggiornata va in docs/engineering/

## DOC-CODE-DRIFT-025 — docs/superpowers/plans/2026-07-13-linux-local-llm.md [codice-avanti]

- **Stato**: applicata — stesso spostamento; i prompt aggiuntivi (`propose_*`, `lua_*`, `character_*`) sono oggi documentati in `docs/ai-production/`.
- **Affermazione (DOCUMENTED AS-IS)**: Task 5: grammatica GBNF unica run.gbnf + prompts/system.txt e user.txt guidano l'LLM sull'intera run
- **Osservato (OBSERVED AS-IS)**: run.gbnf coerente col piano (root/floor/foe), ma prompts/ ha 10 file (propose_*, lua_*, character_*) per fasi di proposta tema e script Lua non nel piano
- **Evidenza**: tools/melting-gen/run.gbnf:4-9; ls prompts/ = 10 file (lua_system.txt, propose_system.txt, ...) vs i 2 previsti (piano riga 44,1361)
- **Azione**: run.gbnf resta riferimento storico valido; i prompt aggiuntivi vanno in docs/ai-production/

## DOC-CODE-DRIFT-026 — HANDOFF.md / CLAUDE.md (governance implicita: suite verdi ad ogni milestone) [incerto]

- **Stato**: aperta — difetto ancora presente: `docs/engineering/known-issues.md` voce 1 documenta lo stesso comportamento (`make test`/`--states-test` rosso se `catalog/` contiene run locali), non ancora corretto nel codice.
- **Affermazione (DOCUMENTED AS-IS)**: Stato di branch implicito: 'test verdi'.
- **Osservato (OBSERVED AS-IS)**: Riprodotto: `./bin/melting_run_gpu --states-test` esce 15 ('su/giu su categoria vuota ha spostato il focus voce') col file residuo catalog/run-4100422243-sconfitta-p2-1.txt; esce 0 se rimosso. RunCatalogAggregate legge davvero i file da catalog/ (gitignored), falsificando la precondizione 'Catalogo vuoto per costruzione' del test a game_tests.c:274.
- **Evidenza**: esecuzione diretta: exit 15 con file / exit 0 senza; src/tests/game_tests.c:255-274; catalog/run-4100422243-sconfitta-p2-1.txt presente
- **Azione**: Non e' regressione di milestone ma difetto di isolamento del test (dipende da stato ambientale gitignored): pulire/isolare catalog/ nel target test prima di fidarsi del claim 'suite verde'; non toccare il testo storico di HANDOFF, segnalare come azione tecnica aperta.

## DOC-CODE-DRIFT-027 — HANDOFF.md (chiusura 0-quater, riga ~65) [doc-superato]

- **Stato**: applicata — risolto in questo stesso lavoro: `HANDOFF.md` è stato riscritto (pacchetto eng-meta) con lo stato reale più recente (audit docs DEC-141..169, backlog implementativo, training LoRA su Kaggle).
- **Affermazione (DOCUMENTED AS-IS)**: Prossimo lavoro naturale: museo/arene sul Piano 0.
- **Osservato (OBSERVED AS-IS)**: Il commit dopo l'ultimo citato (7a64417/75c8ab2) e' a2d5df7 'docs(meta): struttura a domini...' — inizio della migrazione documentale (docs/_meta/, INDEX.md, scripts/docs/build_knowledge_index.py, target docs-index/docs-check/docs-audit), non feature di gioco. HANDOFF non menziona a2d5df7 ne' la migrazione (grep = 0 hit).
- **Evidenza**: git log: a2d5df7 dopo 75c8ab2; grep migrazione/docs/_meta/a2d5df7 in HANDOFF.md = nessun match; docs/_meta/ e scripts/docs/build_knowledge_index.py presenti
- **Azione**: HANDOFF non riflette il lavoro reale piu' recente: archiviare la sezione in docs/archive/handoffs/ e portare lo stato 'prossimo task' come voce viva in docs/plans/active/.

## DOC-CODE-DRIFT-028 — HANDOFF.md (0-quater, riga M1b, riferimento a ui/generation-status.md) [doc-impreciso]

- **Stato**: applicata — verificato: `HANDOFF.md` attuale non cita più `ui/generation-status.md`; il link segnalato come rotto non è più presente.
- **Affermazione (DOCUMENTED AS-IS)**: Il riferimento relativo 'ui/generation-status.md' punta a un doc raggiungibile.
- **Osservato (OBSERVED AS-IS)**: `make docs-check` segnala 'HANDOFF.md: puntatore rotto: ui/generation-status.md'; il file esiste solo in game-design-knowledge-base/docs/game-design/ui/generation-status.md.
- **Evidenza**: output make docs-check; find: unico match game-design-knowledge-base/docs/game-design/ui/generation-status.md
- **Azione**: Contenuto sostanziale vero; sistemare o rimuovere il link in fase di archiviazione di HANDOFF. Nessun rilavoro di codice.

## DOC-CODE-DRIFT-029 — CLAUDE.md (repo principale, riga 5) — riferimento al decision-log [doc-impreciso]

- **Stato**: applicata (parziale) — il percorso in `CLAUDE.md` root è oggi corretto (`docs/design/governance/decision-log.md`); il conteggio '140 decisioni' risulta però stale rispetto a DEC-169 (169 decisioni oggi), fuori perimetro di questo pacchetto (root `CLAUDE.md` non è fra i file assegnati).
- **Affermazione (DOCUMENTED AS-IS)**: 108 decisioni in `docs/game-design/governance/decision-log.md`.
- **Osservato (OBSERVED AS-IS)**: `make docs-check` segnala 'CLAUDE.md: puntatore rotto: docs/game-design/governance/decision-log.md'; il path reale e' game-design-knowledge-base/docs/game-design/governance/decision-log.md. Il numero 108 e' corretto, il puntatore no. Divergenza sfuggita all'analista.
- **Evidenza**: CLAUDE.md:5; output make docs-check (puntatore rotto); file reale in game-design-knowledge-base/docs/game-design/governance/decision-log.md
- **Azione**: Correggere il path relativo nel puntatore (prefisso game-design-knowledge-base/) nella migrazione; il conteggio 108 resta corretto.

## DOC-CODE-DRIFT-030 — 07-ARCHITETTURA-RUNTIME.md, "Scheduling" (righe 41-54) [doc-impreciso]

- **Stato**: applicata — `docs/ai-production/07-ARCHITETTURA-RUNTIME.md` (migrato) ha oggi front matter `status: proposed`, che demota esplicitamente il contenuto a visione non implementata; la prosa resta al presente ma l'autorità è già declassata dal front matter.
- **Affermazione (DOCUMENTED AS-IS)**: Qwen carica/genera/scarica, poi SD carica/genera/scarica, poi il motore valida e pubblica SpriteBundle, poi il gioco entra nel piano.
- **Osservato (OBSERVED AS-IS)**: La sequenza Qwen->SD e' reale (melting-sprites parte solo a GEN_RUNNER_SUCCEEDED del runner testo), ma non esiste alcuno stage 'motore valida e pubblica SpriteBundle' fra i due passi.
- **Evidenza**: src/app/app.c:822-832 (AppStartSpritesGeneration solo dopo active->state==GEN_RUNNER_SUCCEEDED); grep sprite_bundle/rig_registry sotto src/: zero occorrenze (riprodotto)
- **Azione**: La sequenza Qwen->SD e' intento confermato dal codice; togliere lo step 'SpriteBundle/validazione del motore' dalla Scheduling perche' e' proposta (10-PIANO Fase C), non realta'. Verso design come intento di sequenziamento.

## DOC-CODE-DRIFT-031 — 07-ARCHITETTURA-RUNTIME.md, "Cache" (righe 66-85) [doc-impreciso]

- **Stato**: applicata — stesso file, stessa demozione via front matter `status: proposed` per le chiavi di cache descritte.
- **Affermazione (DOCUMENTED AS-IS)**: Present tense: chiave di cache (pipeline_version, model_sha256, lora_sha256[], prompt_hash, ...), e 'un asset valido gia' in cache non viene rigenerato'.
- **Osservato (OBSERVED AS-IS)**: Nessuno di questi identificatori nel codice; nessuna logica di cache di generazione (l'unica occorrenza 'cache' e' un campo della lib SD in sd_img_gen_params_init). Nessun generation_recipe.json.
- **Evidenza**: grep pipeline_version|model_sha256|lora_sha256|generation_recipe su src/ e tools/: zero (riprodotto); tools/melting-sprites/sprite_sd.c:137 = param di libreria, non cache di run
- **Azione**: Il doc afferma in presente un sottosistema di cache inesistente: spostare in docs/plans/active/ come proposta, mai in engineering/design come architettura corrente.

## DOC-CODE-DRIFT-032 — 07-ARCHITETTURA-RUNTIME.md, "Tier" (8-40) e "Benchmark" (135-147) [doc-impreciso]

- **Stato**: applicata — stesso file, stessa demozione via front matter `status: proposed` per i tier/rig/ControlNet descritti.
- **Affermazione (DOCUMENTED AS-IS)**: Quattro Tier (0..3) con rig raylib e ControlNet; benchmark misura tok/s, sec/img, VRAM, load/unload, memoria, disco e propone un tier.
- **Osservato (OBSERVED AS-IS)**: Il codice conosce solo tre valori di tier (lowspec/full/unsupported) mappati su due preset; benchmark.txt registra solo benchSchema/tokS/imgS/tier/measuredAt; nessun rig raylib ne ControlNet.
- **Evidenza**: src/app/app.c:44-78 (AppReadBenchmarkPreset legge tier fra lowspec/unsupported/full); logs/benchmark.txt (solo 5 campi); grep rig_biped|rig_registry|ControlNet in src/: zero (riprodotto)
- **Azione**: Tier 2/3 con rig e ControlNet -> docs/ai-production/ come visione; in engineering descrivere solo i due preset (lowspec/full) e i campi realmente scritti dal benchmark.

## DOC-CODE-DRIFT-033 — 07-ARCHITETTURA-RUNTIME.md, "Pubblicazione atomica" (87-101) [doc-impreciso]

- **Stato**: applicata — stesso file, stessa demozione via front matter `status: proposed` per la granularità di pubblicazione.
- **Affermazione (DOCUMENTED AS-IS)**: Ogni bundle scritto in generated/tmp/<bundle-id>/ e rinominato in generated/bundles/<bundle-id>/; manifest aggiornato solo dopo la pubblicazione.
- **Osservato (OBSERVED AS-IS)**: Il pattern tmp+rename atomico e' reale, ma a livello di SINGOLO FILE, non di cartella-per-bundle; le directory generated/tmp/<bundle-id>/ e generated/bundles/ non esistono.
- **Evidenza**: tools/melting-sprites/sprite_util.c:63 e sprite_atlas.c:80 (rename(tmp,fin) su file); tools/melting-gen/gen_util.c:143,193; ls generated/ (nessuna dir bundles) (riprodotto)
- **Azione**: Confermare il principio 'scrivi in tmp, pubblica con rename atomico' come allineato (vero a livello file); correggere la granularita' (file, non directory-per-bundle) quando confluisce in engineering.

## DOC-CODE-DRIFT-034 — game-design-knowledge-base/docs/game-design/systems/generated-content-validation.md (DEC-103) [codice-indietro]

- **Stato**: aperta — backlog Catalog v2 non risulta implementato (nessuna DEC del batch 144-169 lo copre); `run_catalog.c` presumibilmente ritorna ancora 0 per `source=fallback` senza campo 'origine'.
- **Affermazione (DOCUMENTED AS-IS)**: Un contenuto in stato fallback-usato entra comunque nel Catalogo persistente, marcato con origine 'curato'.
- **Osservato (OBSERVED AS-IS)**: RunCatalogWriteRun ritorna 0 quando source=fallback; nessun campo 'origine' esiste nel formato del catalogo (che e' run-level, non item-level).
- **Evidenza**: src/content/run_catalog.c:219 (guardia strcmp(source,"fallback")==0 -> return 0); src/content/run_catalog.h:22-23 documenta il comportamento come 'default v1... una run interamente fallback non scrive nulla', cioe' la domanda aperta risolta al CONTRARIO di DEC-103 (approved 2026-07-19, decision-log.md:1052); nessuna occorrenza di 'origine'/'curato' come campo scritto.
- **Azione**: Confermata. Il codice incorpora un default v1 che contraddice una decisione approvata recente (DEC-103). Mantenere DEC-103 come fonte di design; aprire voce backlog Catalog v2 (registrare item fallback-usato con origine=curato, formato catalogo item-level) in docs/plans/active. Nessuna modifica al doc approvato.

## DOC-CODE-DRIFT-035 — game-design-knowledge-base/docs/game-design/systems/run-manifest-and-reproducibility.md (DEC-066/DEC-077, Nota di implementazione) [doc-impreciso]

- **Stato**: aperta — nessun meccanismo di codice breve lato gioco risulta implementato (nessuna DEC del batch lo copre); la nota di implementazione del documento di design resta da correggere.
- **Affermazione (DOCUMENTED AS-IS)**: Nota di implementazione del doc: 'il codice attuale trasporta solo seed piu' versione di gioco; l'estensione a tema e personaggio non e' ancora implementata' (implica che un codice breve seed+versione sia gia' condivisibile).
- **Osservato (OBSERVED AS-IS)**: Nessun meccanismo di codice breve (generazione/importazione) esiste nel gioco: RunSetup ha solo Seed(R rigenera)/Avvia/Indietro, nessun campo per incollare un codice. RunBundle esiste solo lato generatore (tools/melting-gen), non come codice importabile in gioco.
- **Evidenza**: src/render/game_renderer.c DrawRunSetupOverlay (solo righe Seed/Avvia/Indietro); src/app/app.c case APP_RUN_SETUP (stesse 3 opzioni, focus %3); nessuna occorrenza di ShareCode/RunCode in src/; RunBundle solo in tools/melting-gen/{main,gen_util,gen_manifest}.c e melting_gen.h.
- **Azione**: Confermata: la Nota di implementazione fa un'affermazione FALSA sullo stato del codice. Correggere la nota: non manca solo l'estensione tema/personaggio, manca l'INTERO meccanismo di codice breve lato gioco (anche il solo seed+versione). Aggiornare prima della migrazione a docs/design; aprire voce in docs/plans/active.

## Affermazioni portanti confermate allineate

- `docs/ARCHITECTURE.md:6-7`: src/main.c contiene soltanto il punto di ingresso e delega l'esecuzione ad AppRun. — OK (src/main.c:1-6)
- `docs/ARCHITECTURE.md:30`: gen: ciclo di vita del processo esterno di generazione (avvio, sondaggio del progresso, timeout, annullamento); nessuna logica di gioco. — OK (src/gen/gen_runner.c, src/gen/gen_runner.h; AGENTS.md descrizione di src/gen)
- `docs/ARCHITECTURE.md:9`: tools/melting-gen (fuori da src/, ma parte della build) e' un processo figlio separato del binario di gioco. — OK (Makefile:66 ('melting-sprites e melting-gen restano DUE ESEGUIBILI SEPARATI, mai linkati'); Makefile:16 GAME_L)
- `README.md righe 10-12`: 'Il percorso storico con OpenAI + un piccolo sidecar Node resta disponibile come alternativa... e non e' stato rimosso.' — OK (llm/run_content.mjs:408,715; generate_llm_content.bat riga 'node llm\generate_run.mjs'; start_llm_server.bat 1)
- `AGENTS.md riga 20`: 'Mantieni il motore C indipendente da rete, chiavi API e modelli AI... Solo bin/melting-gen linka llama.cpp e cJSON; bin/melting-sprites linka stable-diffusion.cpp.' — OK (grep -rniE 'openai|api[_-]?key' src/ tools/ Makefile -> nessun match; grep 'localhost|127.0.0.1|8787' src/ too)
- `docs/OPENAI_SETUP.md (sezioni modelli e variabili .env)`: OPENAI_MODEL default gpt-5.5, OPENAI_IMAGE_MODEL default gpt-image-2, endpoint /v1/responses e /v1/images/generations. — OK (llm/run_content.mjs:35,37,408,715; .env.example (OPENAI_MODEL=gpt-5.5, OPENAI_IMAGE_MODEL=gpt-image-2); docs/O)
- `README.md "Avvio rapido su Linux" (righe 63-87)`: make compila gioco + melting-gen; make run gioca senza generare; make run-gen genera in locale; make test && make test-gen sono test senza modello; make test-llm fa generazione reale. — OK (Makefile:100 (.PHONY run run-gen test test-gen test-llm ...); Makefile:122-126 (run: game; run-gen: all, --gen)
- `docs/SPRITES-SPIKE.md:63-66`: L'API di stable-diffusion.cpp e' cambiata: i punti d'ingresso sono new_sd_ctx() + generate_image(), CFG in sample_params.guidance.txt_cfg. — OK (tools/melting-sprites/sprite_sd.c:76 (new_sd_ctx), :157 (generate_image), :147 (sample_params.guidance.txt_cfg)
- `docs/SPRITES-SPIKE.md:55-58`: Ritaglio con flood fill che parte dai bordi; un pixel nero dentro lo sprite non e' raggiungibile senza attraversare lo sprite, quindi sopravvive. — OK (tools/melting-sprites/sprite_post.c:64-158 (bg da bordo, flood fill righe ~107-123, due passaggi halo righe ~1)
- `docs/SPRITES-SPIKE.md:59-61`: Dopo la riduzione della palette, ogni colore troppo scuro viene alzato a un minimo (KEY_FLOOR); misurato 0 pixel a rischio. — OK (tools/melting-sprites/melting_sprites.h:40 (#define SPRITE_KEY_FLOOR 16); sprite_post.c (SpritesQuantize, inna)
- `docs/SPRITES-SPIKE.md:68-73`: Librerie di post-processing: stb_image.h/stb_image_write.h ed exoquant, da vendorizzare. — OK (tools/melting-sprites/vendor/ (ls: exoquant.c, exoquant.h, stb_image.h, stb_image_write.h, stb_impl.c); sprite)
- `docs/BENCHMARKS.md`: Default in tools/melting-gen/main.c: modello 7B Q4_K_M, ngl 99. — OK (tools/melting-gen/main.c:79 (model 7B q4_k_m); main.c:82 (ngl = 99))
- `docs/BENCHMARKS.md`: Il 1.5B resta il fallback automatico gia' cablato in main.c. — OK (tools/melting-gen/main.c:80 (modelFallback 1.5b); src/app/app.c:91 (APP_LOW_SPEC_MODEL = 1.5b q4_k_m))
- `docs/BENCHMARKS.md`: Comando MODEL=... NGL=... SEED=42 make test-llm, una riga per corsa. — OK (scripts/test-llm.sh:18-20 (MODEL:-1.5b, NGL:-99, SEED:-31337); Makefile:165-166 (test-llm: all; bash scripts/t)
- `docs/dataset/README.md e docs/dataset/TRAINING-RUNBOOK.md`: I due doc descrivono "il" dataset del progetto (registro CC0 + runbook Style LoRA immagini) — OK (grep -rn 'qlora|corpus_to_dataset|QLoRA' docs/dataset/ -> nessun risultato; HANDOFF.md:141-146 cita scripts/co)
- `docs/dataset/TRAINING-RUNBOOK.md riga 100`: Il registro ha oggi 3158 file da 6 pack, tutti CC0 — OK (wc -l docs/dataset/ledger.jsonl = 3158; dataset_ledger.py stats: CC0 3158; ls dataset-raw/ = 6 cartelle)
- `docs/dataset/TRAINING-RUNBOOK.md riga 22`: Registro verificato con python3 scripts/dataset_ledger.py check (nessun problema) — OK (python3 scripts/dataset_ledger.py check -> 'check OK: 3158 voci, nessun problema')
- `docs/dataset/TRAINING-RUNBOOK.md righe 133-138`: Tutti i prompt fissi in tools/melting-sprites/prompts/*.txt iniziano con pixelsprite, — OK (head -1 dei 13 prompt: tutti iniziano con 'pixelsprite,'; negative.txt inizia con 'scenery,')
- `docs/dataset/TRAINING-RUNBOOK.md righe 276-278`: bin/melting-sprites accetta un solo slot LoRA (--lora, default models/lcm-lora-sdv1-5.safetensors) — OK (tools/melting-sprites/main.c:40,59,80,388,621)
- `docs/dataset/TRAINING-RUNBOOK.md riga 27 e sezione baseline`: Checkpoint base in models/Public-Prompts-Pixel-Model.ckpt; baseline in logs/sprite-baseline/20260717-053243/ (30 atlanti, seed 5 e 17, con index.txt) — OK (ls models/Public-Prompts-Pixel-Model.ckpt = 2132856622 byte; ls logs/sprite-baseline/20260717-053243/ | wc -l )
- `docs/dataset/README.md righe 95-108`: scripts/dataset_ledger.py compila in automatico sha256, dimensions (solo PNG via IHDR) e license_snapshot_date; niente Pillow; add/check/stats — OK (scripts/dataset_ledger.py:10-12 (no Pillow, IHDR), :46 (whitelist), :59 (sha256), :67-82 (png_dimensions), :13)
- `docs/dataset/TRAINING-RUNBOOK.md righe 338-345`: Ninja Adventure gia' presente in dataset-raw/superpowers-asset-packs/ninja-adventure/ — OK (ls dataset-raw/superpowers-asset-packs/ include 'ninja-adventure')
- `docs/dataset/TRAINING-RUNBOOK.md (training LoRA immagine non ancora eseguito)`: Il runbook presenta il training Style LoRA come non ancora avvenuto (account RunPod e training vero da fare) — OK (grep RUNBOOK:32; ls models/*.safetensors = solo lcm-lora + taesd, nessun output kohya)
- `docs/superpowers/plans/2026-07-13-linux-local-llm.md`: Task 4: melting_gen.h definisce GEN_FLOORS 5, GEN_ITEMS 3, GEN_MAX_OPS 3 — OK (tools/melting-gen/melting_gen.h:20-22)
- `docs/superpowers/plans/2026-07-13-linux-local-llm.md`: Task 9: src/gen/gen_runner.{h,c} spawna e monitora melting-gen come processo esterno (fork/exec + waitpid non bloccante) — OK (src/gen/gen_runner.c:100 (fork), :108 (execv), :131 (waitpid WNOHANG), :139 (WIFEXITED). NOTA: la citazione or)
- `docs/superpowers/plans/2026-07-13-linux-local-llm.md`: Il gioco NON linka mai llama.cpp/cJSON; solo bin/melting-gen li linka (Global Constraints, riga 19) — OK (Makefile:16 (GAME_LIBS, non riga 14 come citato), Makefile:54 (GEN_LIBS llama), Makefile:66 (commento); piano )
- `docs/superpowers/plans/2026-07-13-linux-local-llm.md`: Task 4 Step 1: vendorizza cJSON v1.7.19 in tools/melting-gen/vendor/ — OK (ls tools/melting-gen/vendor/: cJSON.c, cJSON.h; piano righe 42,406)
- `HANDOFF.md (0-quater, 'Backlog noto', riga 50-52)`: L'RNG di gioco e' ancora seedato con time(NULL) in GameResetRun. — OK (src/game/game.c:99,103; HANDOFF.md:50-52)
- `HANDOFF.md (0-quater, 'Backlog noto', riga 52)`: gen_progress_lazy.txt non viene mai scritto: serve un --progress-path in melting-gen. — OK (tools/melting-gen/gen_util.c:137; tools/melting-sprites/sprite_util.c:57; src/app/app.c:159; grep progress-pat)
- `HANDOFF.md (0-quater, 'Suite nuove dentro make test')`: Suite nuove: --states-test, --floor-zero-test, --rooms-test. — OK (Makefile (blocco target test, righe ~142-144))
- `CLAUDE.md — 'decision-log.md (108 decisioni)'`: Il decision-log contiene 108 decisioni approvate (DEC-001..DEC-108). — OK (governance/decision-log.md:7 (DEC-000 Titolo), :19 (DEC-001), :1099 (DEC-108); grep -c = 109)
- `HANDOFF.md (0-quater) e open-questions.md`: Dopo il secondo giro del 19/07 restano 11 domande aperte, rinumerate da 1. — OK (game-design-knowledge-base/docs/game-design/governance/open-questions.md, numerazione 1..11 riprodotta)
- `10-PIANO-INTEGRAZIONE-C.md, Fase A "Multi-LoRA" (3-53)`: Il doc PROPONE SpriteLoraConfig/MAX_SPRITE_LORAS 8 e --lora ripetibile. — OK (tools/melting-sprites/main.c:80 (un solo --lora); grep MAX_SPRITE_LORAS/SpriteLoraConfig: zero (riprodotto); d)
- `10-PIANO-INTEGRAZIONE-C.md, Fasi C/D/E/F (78-137)`: Il doc PROPONE nuovi moduli: sprite_bundle, sprite_animator, rig_registry, rig_biped/blob/chain/flying, EnemySpec esteso. — OK (find *sprite_bundle*/*rig_registry*/*sprite_animator*/*rig_biped*: nessun risultato (riprodotto); src/render/g)
- `10-PIANO-INTEGRAZIONE-C.md, Fase H "Scheduling" (149-161)`: src/gen deve orchestrare melting-gen -> validate manifest -> melting-sprites -> validate bundles -> atomic publish; il gioco non linka SD/llama.cpp. — OK (src/gen/gen_runner.c:95-169 (fork/execv, nessuna validazione); tools/melting-gen/gen_validate.c mai richiamato)
- `10-PIANO-INTEGRAZIONE-C.md, Fase G "Loader" (139-147)`: Il loader prova 1. SpriteBundle 2. cella atlas legacy 3. forma geometrica; mai invisibilita'. — OK (src/render/game_renderer.c:1386 (atlasMode: 'Sprite locali... atlas PNG' vs 'Atlas procedurale/fallback BMP');)
- `07-ARCHITETTURA-RUNTIME.md, "Nessuna inferenza in combattimento" (114-133)`: In combattimento vietati SD, Qwen, download, compilazione script, validazione pesante; ammessi lettura cache, animazione, compositing, ecc. — OK (src/gameplay/combat.c: grep GenRunner/StableDiff/Qwen/melting-*: zero (riprodotto); src/app/app.c:118-127 (App)
- `10-PIANO-INTEGRAZIONE-C.md, Fase H (riga 161)`: Il gioco non linka Stable Diffusion o llama.cpp, coerentemente con AGENTS.md. — OK (src/gen/gen_runner.c:99-110 (fork()+execv del comando esterno); ls tools/ = solo melting-gen, melting-sprites )
- `07-ARCHITETTURA-RUNTIME.md, "Scheduling" (riga 54): coesistenza VRAM`: Qwen e SD non devono coesistere in VRAM sulla macchina da 6 GB: prima si scarica Qwen, poi si carica SD. — OK (src/app/app.c:822-832 (sprites solo dopo SUCCEEDED del runner testo); app.c:118-127 (commento esplicito 'due m)
- `game-design-knowledge-base/docs/game-design/systems/floor-zero.md (DEC-004/029/055/092/093/094, arene di sfida e dote)`: Le arene di sfida opzionali del Piano 0 danno una dote iniziale, sono a rischio zero e seminate dal pool curato minimo. — OK (grep arena/dote su src/world/floor_zero.c: nessun risultato; src/core/game_types.h:282-283 (2 pannelli); src/c)
- `game-design-knowledge-base/docs/game-design/systems/floor-zero.md (DEC-040/063/085, museo e Reliquie)`: Il museo del Piano 0 espone le migliori creazioni, permette prove illimitate e gestisce Reliquie/preferiti. — OK (src/content/run_catalog.h:23-24 ('UI del Catalogo, museo, punti, preferiti, riconvalida vera = gap di implemen)
- `game-design-knowledge-base/docs/game-design/systems/floor-zero.md (DEC-070/DEC-086, scelta completo/solo curato al primo avvio)`: Al primo avvio il gioco mostra una schermata dedicata a due carte (completo/solo curato) dopo il benchmark, senza default silenzioso. — OK (src/core/game_types.h:177-186 (9 AppMode, nessuno per la scelta); src/app/app.c:44-79 AppReadBenchmarkPreset ()
- `game-design-knowledge-base/docs/game-design/systems/items-pools-and-rarity.md (Correzione di fortuna, sezione approved)`: Dopo N estrazioni sfortunate consecutive la qualita' minima della prossima estrazione sale (regola approved, non draft). — OK (grep pity/correzione di fortuna/sfortun/streak su src/ e tools/: nessun risultato pertinente; tools/melting-ge)
