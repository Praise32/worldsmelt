<!-- GENERATED: make docs-index -- non modificare a mano -->

# Indice `docs/engineering/`

- [Engineering di Worldsmelt — stato tecnico reale](README.md) — Punto d'ingresso del dominio engineering: cosa contiene la cartella, il principio "stato tecnico verificato contro il codice" e come si mantiene allineata con gli strumenti docs-check/docs-index/docs-audit. `[approved/canonical]`
- [Architettura del codice](architecture.md) — Mappa verificata dei moduli C, dei processi esterni (melting-gen, melting-sprites) e dei confini di sicurezza (sandbox Lua, mini-VM di ripiego) del motore di Worldsmelt. `[approved/canonical]`
- [Benchmark melting-gen e melting-sprites — macchina di riferimento](benchmarks.md) — Misure di velocità di generazione (testo e sprite) sulla macchina di riferimento, contesto della misura, disambiguazione tra i due meccanismi di benchmark del repo e la storia del meccanismo di tier automatico, rimosso da DEC-110 (il gioco non legge più logs/benchmark.txt). La tabella testuale è congelata come misura storica del 13/07 (DEC-149); il modello di testo attivo oggi è citato con la formula neutra di DEC-151 (Gemma-3-4B-IT Q4, DEC-140). `[implemented/supporting]`
- [Dipendenze vendorizzate e riferimenti locali](dependencies.md) — Librerie native in deps/ (versioni pinnate da scripts/setup-deps.sh) con ruolo e binario che le linka, piu' i repository locali di sola consultazione e i modelli GGUF/SD scaricati da scripts/download-models.sh. Elenco modelli di testo e opzioni --light/--with-7b allineati al modello attivo (DEC-140/151) e verificati contro lo script reale. `[implemented/supporting]`
- [Espressività dei tipi di colpo — cosa può inventare davvero il modello oggi](espressivita-colpi.md) — Verifica puntuale (file:riga) di cosa il vocabolario attuale dei colpi permette al modello: variazioni sullo stesso scheletro fisico sì (catena inclusa, già nativa), laser/stazionari/orbite no. Base fattuale di DEC-138 (mechanics-lab). `[approved/canonical]`
- [Registro dei difetti e limiti noti](known-issues.md) — Difetti e limiti tecnici NOTI e verificati nel codice reale, con sintomo, evidenza (file:riga) e stato attuale; non e' un elenco di idee o backlog di design. `[approved/supporting]`
- [Multiplayer su Steamworks — direzione proposta](multiplayer-steam.md) — Nota tecnica non decisionale: valuta Steamworks (API C, leaderboard, achievement, Workshop, cloud save) per il multiplayer asincrono già approvato (DEC-016, DEC-021). Il tema distribuzione appartiene ad ai-production (DEC-158): questo documento vi rimanda senza duplicarlo. `[proposed/supporting]`

## adr/

- [ADR-001: backend GPU Vulkan obbligatorio, mai ROCm, mai flash-attention su RDNA1](adr/ADR-001-backend-vulkan-only.md) — llama.cpp e stable-diffusion.cpp usano sempre il backend Vulkan (mai ROCm) sulla RX 5600 XT di riferimento, con flash-attention disattivata; versioni di raylib/llama.cpp/Lua pinnate in scripts/setup-deps.sh. `[approved/canonical]`
- [ADR-002: i generatori girano come processi esterni, mai linkati nel gioco](adr/ADR-002-generatori-processi-esterni.md) — Il binario del gioco non linka mai llama.cpp/stable-diffusion.cpp/cJSON; melting-gen e melting-sprites sono due eseguibili separati (ggml incompatibili, VRAM condivisa) e il gioco legge solo file validati in generated/. `[approved/canonical]`
- [ADR-003: script degli oggetti generati in Lua 5.5 sandboxato, niente DSL tipizzata](adr/ADR-003-lua-sandbox-non-dsl.md) — Gli script dei contenuti generati sono Lua 5.5 sandboxato (allowlist _ENV, tetto memoria, budget istruzioni, solo caricamento testuale), non una DSL tipizzata dedicata; la mini-VM CSV resta come fallback e melting-gen fa un dry-run nella stessa sandbox prima che il gioco veda lo script. `[approved/canonical]`

## specs/

- [Design del ciclo build Linux + LLM locale (fase 1)](specs/2026-07-13-local-llm-linux-design.md) — Spec della fase 1: build Linux, melting-gen come processo esterno llama.cpp/Vulkan con GBNF+validatore+fallback deterministico; la roadmap delle fasi vive qui. `[implemented/canonical]`
- [Design degli sprite locali (fase 2)](specs/2026-07-13-local-sprites-design.md) — Spec della fase 2: melting-sprites su stable-diffusion.cpp, due eseguibili separati per i due ggml incompatibili, post-processing e controlli qualita. `[implemented/supporting]`
- [Design della sandbox Lua (fase 3a)](specs/2026-07-13-lua-sandbox-design.md) — Spec di sicurezza della sandbox Lua 5.5: allowlist _ENV, tetto memoria, budget istruzioni, vie di fuga chiuse e limiti di determinismo; citata da AGENTS.md, test e codice. `[implemented/canonical]`
- [Step 3b: nemici e boss inventati dal modello](specs/2026-07-14-step-3b-enemies.md) — Vocabolario parametrico dei nemici (form/move/fire), EnemyTypeBalance e budget di difficolta della stanza: il motore da mattoni e garanzie, il modello compone. `[implemented/canonical]`
- [Step C: tipi di colpo inventati e bilanciati](specs/2026-07-14-step-c-shottype-balance.md) — Tipi di colpo senza enum fisso: ShotForm+manopole inventati dal modello, ShotTypeBalance riporta ogni tipo a potenza ~1.0 (sidegrade mai dud), curve alla Isaac e fortuna. `[implemented/canonical]`

_3 documenti historical esclusi dall'indice._
