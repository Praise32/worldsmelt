# Piano: Roguelike generato da IA locale (clone di Isaac)

*Redatto il 16 luglio 2026 — basato su ricerca web verificata contro fonti primarie (policy Steamworks, testi delle licenze, Steam Hardware Survey giugno 2026) e su una revisione architetturale a tre lenti (latenza/UX, sicurezza sandbox, shippability).*

---

## 1. Verdetto sul concept

**L'idea è valida e il momento è giusto, ma il punto esatto in cui ti collochi è la parte difficile e non dimostrata.**

- Il precedente più vicino, **AI Roguelite** (dev solista, Steam 2023), è *Very Positive* (83%, ~583 recensioni, stimati 100-200k copie) — la nicchia esiste ed è redditizia per un solista. Ma la sua variante d'azione, **AI Roguelite 2D** (marzo 2024), è ferma a *Mixed 59% con 27 recensioni*: la generazione live innestata su gameplay d'azione è la parte non risolta. In un gioco testuale l'incoerenza si tollera; in un arcade ogni comportamento generato è meccanicamente portante. Questo è il tuo vero rischio e la tua vera opportunità.
- Hai **due vantaggi strutturali** rispetto a quasi tutti i giochi AI-native falliti: (1) tutto locale = **costo marginale di inferenza zero** (il costo per sessione è ciò che ha ucciso o mutilato Retail Mage, Suck Up!, Whispers from the Star); (2) generazione batch a inizio/durante la run = la latenza conversazionale non ti riguarda.
- Precedente tecnico diretto: il sample **NVIDIA NVIGI "Code Agent: Lua Dungeon Crawler"** fa esattamente la tua architettura (Qwen3-8B locale che scrive Lua eseguito in sandbox indurita con retry su errore). E **inZOI** (Krafton) già spedisce un modello on-device dietro requisiti hardware — il tuo pattern "benchmark + tier" è validato commercialmente.

**Due convinzioni da correggere subito** (entrambe architetturali, entrambe emerse da tutte e tre le revisioni):

1. **Il determinismo cross-macchina non esiste.** llama.cpp e Stable Diffusion NON sono bit-riproducibili tra GPU, backend (CUDA/Vulkan/Metal/CPU) e versioni driver. "Stesso seed + stesso modello = stessa run" vale solo sulla stessa macchina con la stessa build. Daily challenge e condivisione run vanno costruite distribuendo il **contenuto generato**, non il seed (vedi RunBundle, §2).
2. **"Genero tutto a inizio run" non è UX sostenibile su nessun hardware.** Numeri concreti su RTX 3060: ~55 snippet Lua × ~400 token a 35-45 tok/s ≈ 9 minuti; ~125 sprite SD1.5 a 512px ≈ 5-6 minuti. Ottimizzando tutto (batching, speculative decoding, LCM) scendi a 3-6 minuti — comunque fatale per un arcade dove morte→retry deve stare sotto i 10 secondi. La soluzione non è un modello più veloce: è **generare in anticipo** (§2).

---

## 2. Architettura: cosa tenere, cosa cambiare

### Da tenere (scelte giuste)

- **C + raylib + sandbox Lua embedded**: combinazione snella e spedibile da solista. **Lua è la tua decisione tecnica più forte**: l'API di modding di Isaac (Repentance) è Lua, più i corpora Roblox/LÖVE/PICO-8/WoW — Qwen ha visto un volume enorme di *esattamente questo tipo* di codice gameplay Lua. Un sandbox WASM butterebbe via il vantaggio di corpus e aggiungerebbe una toolchain di compilazione al loop di runtime. Niente Godot (attrito GDExtension + nessun vantaggio); LÖVE avrebbe senso solo se l'engine C fosse ancora embrionale.
- **Qwen2.5-Coder-7B-Instruct** come baseline: a metà 2026 è ancora considerato il miglior 7B "puro codice" (~69 tok/s Q4_K_M su 3060 12GB). Costruisci però lo slot modello come **GGUF intercambiabile** e fai A/B test sui *tuoi* prompt Lua con **Qwen3.5-4B** (~77 tok/s, sta in 6GB) e **Qwen3.5-9B** (LiveCodeBench 65.6, ~6.8GB a Q4/32k ctx, `enable_thinking=false`) — tutti Apache 2.0. Attenzione: un benchmark indipendente di maggio 2026 mostra Qwen3.5 sotto Qwen2.5-Coder su task di codice in llama.cpp (supporto immaturo dell'architettura ibrida) — non fidarti dei benchmark pubblici, testa sui tuoi prompt.
- **llama.cpp + stable-diffusion.cpp linkati in-process** (entrambi MIT, entrambi API C, entrambi ggml — allinea le versioni ggml o linka staticamente ciascuno con la propria). stable-diffusion.cpp è maturo e attivissimo (release luglio 2026; supporta SD1.x, FLUX.2-klein, Z-Image, LoRA, LCM-LoRA, TAESD; backend CPU/CUDA/Vulkan/Metal). Il sidecar server è un fallback per crash-isolation, ma in un gioco Steam un processo unico evita prompt del firewall, conflitti di porta e processi orfani.
- **Il benchmark al primo avvio**: idea giusta, va resa *misurata* e non sintetica (§4).

### Cambiamento n.1 — Architettura "RunBundle" (il pivot più importante)

Separa il gioco in due fasi:

- **Fase di generazione** → produce un **bundle autocontenuto e versionato**: sorgenti Lua, PNG degli sprite, JSON della mappa, metadata `{hash SHA-256 del GGUF, quantizzazione, versione dei prompt template, commit llama.cpp, config sampler}`. Peso: ~200KB-2MB compresso zstd.
- **Fase di simulazione** → engine **completamente deterministico** a timestep fisso (un solo PRNG seedato PCG32, fixed-point dove serve) che *consuma* bundle.

Cosa ti compra questo singolo cambiamento:
- **Daily challenge**: generi tu la run sul tuo PC, la validi, pubblichi il bundle (Workshop o CDN). Tutti giocano la *stessa* run — cosa impossibile via seed.
- **Condivisione run** tra giocatori: mandi 2MB, non un seed che desincronizza.
- **QA su contenuto infinito**: il bottone "segnala oggetto rotto" allega il bundle → ogni bug dei giocatori è riproducibile al 100% sulla tua macchina. Questa è *l'intera* strategia QA.
- **Cache/banking**: i bundle si generano in anticipo e si accumulano su disco.

### Cambiamento n.2 — Mai generare in modo sincrono

- **Piano 1 autoriale + primo boss + ~30 oggetti/nemici starter fatti a mano.** Il contenuto generato entra dal piano 2 (es. 50% al piano 2, 100% dal 3). Questo: (a) garantisce i primi 10 minuti — la finestra rimborso Steam di 2 ore decide il destino del gioco; (b) dà al tutorial contenuto fisso insegnabile; (c) **maschera la generazione**: "Nuova Run" → giocabile in <5s mentre i piani 2+ si generano in background durante il piano 1.
- **Daemon di banking**: tieni sempre 1-2 run future complete già generate (durante la run corrente — che dura 20-40 min contro 3-6 min di GPU necessari — nei menu, sulla schermata di morte). Dalla seconda run in poi, morte→nuova run ≈ 1-2 secondi.
- **Pipeline per piano**: il piano N+1 deve essere pronto prima che muoia il boss del piano N (p95); se non lo è, inserisci una stanza autoriale "interstiziale" (negozio/riposo) che compra 30-60s. Gate alla botola, mai a inizio run.
- Budget di latenza da rispettare: nuova run giocabile <5s su TUTTI i tier; generazione completa di una run <90s su desktop di fascia alta.

### Cambiamento n.3 — Ottimizzazioni di generazione (moltiplicano a ~5-8x)

LLM: **grammatiche GBNF di llama.cpp** ristrette al tuo subset di API Lua (quasi elimina la tassa del 10-30% di Lua invalido — è ciò che ha separato Retail Mage dal jank); prefix/KV cache del system prompt condiviso; speculative decoding con Qwen2.5-Coder-0.5B come draft (1.5-2.5x); continuous batching 4-8 slot. Target: 55 snippet in 90-150s su 3060.

Sprite: **mai generare nativamente a 64px** (la U-Net di SD1.5 collassa sotto ~256px). Genera a 512 con **LCM-LoRA (4-8 step, licenza openrail++, ok commerciale)** + TAESD per il decode VAE, poi downscale nearest-neighbor a 64-128px + **quantizzazione a palette di 16 colori generata una volta per run** (k-means) — forza coerenza visiva interna alla run e fa leggere gli sprite come direzione artistica, non come "AI slop". Usa una LoRA pixel-art (es. PixelArtRedmond per SD1.5, commercial-ok). Valuta griglie sprite-sheet 2x2/3x3 per ammortizzare. Target: 125 sprite in 60-120s su 3060.

**Trappola di licenza**: NON usare SD-Turbo/SDXL-Turbo — sono sotto Stability AI Community License (termina sopra $1M di fatturato annuo + obbligo "Powered by Stability AI"). Percorsi veloci license-clean: LCM-LoRA su SD1.5; come tier alto **FLUX.2-klein-4B** (Apache 2.0, 4 step, ~13GB bf16 quindi GGUF quantizzato; esiste una LoRA pixel-art Apache-2.0/CC0 fatta apposta per sprite di gioco con sfondo trasparente: Limbicnation/pixel-art-lora) oppure **Z-Image-Turbo** (Apache 2.0, 6B, ~6GB in GGUF).

VRAM: 7B Q4 (~5-6GB con KV) + SD1.5 fp16 (~2-3.4GB) **coesistono solo su 12GB+**. Su 8GB: carica/scarica sequenziale (2-4s da NVMe) — combacia naturalmente con la tua fase di generazione. Cap del contesto a 4-8k per risparmiare ~1GB. Durante il gameplay attivo, throttola la SD in background (step singoli interleaved o solo nelle transizioni stanza/porta) per non mangiare frame time.

### Cambiamento n.4 — raygui solo per tool di debug

raygui va benissimo per i tool interni, ma non è una UI di gioco spedibile. Budget 2-3 settimane per una UI immediate-mode custom su raylib (menu, HUD, tooltip oggetti). È normale e costa meno che combattere l'estetica spartana di raygui.

---

## 3. Sandbox e sicurezza (checklist condensata)

**Modello di minaccia corretto**: i modelli girano in locale → i giocatori *sostituiranno* i pesi, editeranno gli snippet in cache e condivideranno "model pack" modificati. La sandbox deve reggere contro codice **deliberatamente ostile**, non solo contro output difettoso del 7B. Un escape = il tuo gioco diventa vettore di malware via community.

1. **PUC Lua 5.4.x, NON LuaJIT** (i debug hook non scattano nelle trace JIT; FFI non è sandboxabile). Pinna le patch di sicurezza 5.4 (ci sono stati CVE GC/stack).
2. **Solo chunk testuali**: un unico choke-point `luaL_loadbufferx(..., "t")`. Lua ≥5.2 non ha più il verificatore di bytecode: caricare bytecode ostile = lettura/scrittura arbitraria della memoria di processo. Check in CI che non esistano altri path di load.
3. **Environment whitelist-only** per snippet (`_ENV` fresco): consenti `pairs/ipairs/select/type/tonumber/tostring/error/pcall` (wrappato), `math.*` (con `math.random` = PRNG seedato del gioco), `string.*` con `rep` cappato a 4KB, `table.*` con `concat` cappato. Nega: `io, os, debug, package, coroutine, load*, dofile, require, collectgarbage, raw*, get/setmetatable, _G`. Sanifica anche la metatable condivisa delle stringhe (`('x'):rep` bypassa i globali).
4. **Terminazione**: `lua_sethook` LUA_MASKCOUNT; budget ~100k istruzioni per `on_update`, 500k per generazione one-shot; kill a 2ms wall-clock per callback; budget totale Lua ≤2ms per frame. **Al trip, ri-arma l'hook con count=1** così l'errore si ri-solleva dentro qualunque `pcall` con cui lo script provi a inghiottirlo. `coroutine` negato (gli hook sono per-thread).
5. **Memoria**: `lua_Alloc` custom, cap rigido 64MB per l'intero stato, GC d'emergenza a 48MB.
6. **Niente reentrancy**: le API di gioco chiamate da Lua accodano comandi in una **command queue** drenata dopo il ritorno dello script. Quote: max 32 `spawn_projectile` per callback, con clamp su velocità/danno/durata. Riferimenti a entità = handle opachi (indice + generation counter), mai puntatori; handle stantio = no-op, mai crash.
7. **Pipeline di validazione**: GBNF → compile check → lint statico (identificatori proibiti, difesa in profondità) → **dry-run headless** (seed fisso, player finto + 3 nemici dummy, 600 frame a dt=1/60, `on_hit`×10, `on_death`) → check semantici (tutti i numeri finiti, DPS ≤3x baseline del piano, almeno un comando osservabile — rigetta i no-op). **Max 3 retry** rimandando al modello l'errore esatto; poi fallback a **template parametrizzati** (30-50 comportamenti scritti a mano di cui l'LLM riempie solo un blocco JSON schema-validato e clampato). Aspettati 5-15% di fallimento first-pass; target <2% che cade a template, allarme sopra 5%.
8. **HMAC sulla cache**: snippet validati persistiti su disco firmati HMAC-SHA256 con chiave random per-installazione, riverificati al load — altrimenti editare il save bypassa l'intera pipeline e la validazione è decorativa.
9. **Recovery a scale**: ogni chiamata C→Lua via `lua_pcall` con message handler. Strike 1-2 → disabilita solo la callback fallita, sostituisci comportamento archetipo (nemico: insegui+danno da contatto); strike 3 → ritira lo snippet per la run. Errore di memoria → rebuild completo del `lua_State` rieseguendo gli snippet HMAC-verificati (devono essere pure/stateless by design). Dopo ogni callback: clamp di posizioni/velocità/HP, sostituzione NaN/inf. Autosave a ingresso stanza. **La run non finisce mai per colpa di uno script.**
10. **Fuzzing**: harness libFuzzer su ogni binding C; CI con ASan su un corpus di 10k snippet (generati + avversariali scritti a mano: pcall-loop, rep-bomb, ricorsione profonda, probing di metatable).

### Content safety (requisito Steam, non optional)

- **Lockdown dei prompt**: nessuna stringa controllata dal giocatore entra mai nei prompt (né nome profilo né save). Tutti i prompt sono template tuoi parametrizzati dallo stato di gioco. *Questa è la tua guardrail più forte da dichiarare a Valve.*
- **Testi generati** (nomi/descrizioni oggetti): normalizzazione (casefold, leet-map, strip zero-width) → blocklist (liste LDNOOBW, ~28 lingue) → classificatore locale **Detoxify** (Apache 2.0, ~110M, <15ms su CPU via ONNX); >0.5 → rigenera max 3, poi generatore procedurale aggettivo+sostantivo. Nomi ≤32 char, ASCII+latino base.
- **Sprite**: negative prompt fisso hardcoded + classificatore NSFW locale (**Falconsai/nsfw_image_detection**, ViT Apache 2.0, o rilevatore CLIP LAION MIT); >0.7 → rigenera con nuovo seed max 2, poi sprite procedurale di fallback. Il safety checker stock di SD1.5 è tarato sul fotorealismo, sui pixel-art è debole: usalo solo come terza opinione. Il downscale a 64px tronca ulteriormente il rischio residuo.
- **Log locale di ogni generazione** (template, seed, hash output, verdetto filtro): l'overlay Steam ha un bottone dedicato per segnalare contenuto AI illegale — devi poter riprodurre e spiegare qualunque segnalazione.
- **Anti-IP-lookalike**: niente nomi di franchise/personaggi/artisti nei template. Il caso Allumeria (feb 2026: delisting da bot copyright Microsoft per *somiglianza* a Minecraft, senza asset Minecraft) mostra che il vettore di delisting realistico per un generatore di sprite è la somiglianza, non la policy AI.

### Anti-ripetizione (difende la USP)

Un 7B lasciato libero converge in poche run su comportamenti templati ("spara 3 proiettili a ventaglio"). Contromisure: inietta in ogni prompt **assi di vincolo ortogonali** campionati per run (elemento, archetipo di movimento, geometria proiettili, budget statistiche); tieni un check di similarità n-gram/embedding contro le ultime ~20 run del giocatore e rigenera i quasi-duplicati.

---

## 4. Il benchmark al primo avvio

Tienilo, ma **misurato, non sintetico** (~30-90s): una generazione reale da 512 token con il GGUF esatto + una immagine SD a 4 step. Misura: tok/s prefill+decode, secondi/immagine, VRAM (DXGI/NVML), RAM, tempo di load da disco, spazio libero (~10-12GB per modelli+cache). Persisti il risultato; ri-esegui se cambia l'hardware. **Tier per throughput misurato, MAI per nome GPU**: la GPU n.1 su Steam è ora la RTX 4060 *Laptop* (3.81%) — stesso nome del desktop, ~2x più lenta.

| Tier | Soglie (esempio) | Configurazione |
|---|---|---|
| **S — Full** | ≥10GB VRAM, ≥25 tok/s, ≤2.5s/img | 7B + SD co-residenti, banking in background, zero attese |
| **A — Sequenziale** | 6-8GB VRAM, ≥12 tok/s, ≤8s/img | Stessi contenuti, load/unload sequenziale, prima run mascherata dal tutorial |
| **B — Logic-only** | ≥6 tok/s, niente SD viabile | Lua/oggetti/sinergie ancora generativi (col banking il tempo c'è); sprite da libreria di parti pre-generate ricomposte + palette-swap |
| **C — Non supportato** | <5 tok/s o <12GB RAM | Verdetto onesto pre-acquisto + pool di ~500 bundle pre-generati e validati (comunque "nuovi per questo giocatore") — mai una modalità degradata silenziosa |

**Metti il benchmark nella demo gratuita** (Next Fest): il giocatore sa *prima di comprare* se la sua macchina regge — altrimenti i rimborsi diventano un problema di business.

Dati di mercato (Steam Hardware Survey, giugno 2026, verificati):
- Min spec **8GB VRAM + 16GB RAM ≈ 75%** degli utenti Steam (≥16GB RAM è ~87.7%). Richiedere 12GB VRAM (necessari per co-residenza) **dimezza il mercato a ~47%** → ecco perché il tier sequenziale è obbligatorio, non opzionale.
- **Steam Deck**: 8 dei 20 giochi più giocati su Deck nel 2025 sono roguelite (Balatro #2, Isaac #19) — non puoi ignorarlo. La RAM basta (7B Q4 + SD + gioco ≈ 9.4GB di 16 unificati) ma la banda (~88GB/s) no: 7B a 8-15 tok/s, SD1.5 ~90s/immagine. Tier Deck dedicato: modello 3B, LCM a pochi step a 256px, generazione in background spinta; 7B come opzione "quality" opt-in. Testa su Deck vero, presto.

### USP e onestà sui tier

La promessa "nessun pool di contenuti" **sopravvive al tier B** se la definisci come "nessun pool fisso di oggetti/nemici/comportamenti" — il Lua generato resta genuinamente nuovo per run anche su CPU, dato il lead time del banking; solo la novità *visiva* degrada a combinatoria. Dichiara i tier esplicitamente in-game e sullo store, e calibra i claim del marketing su ciò che offre il tier B, perché **il tier B sarà l'esperienza del giocatore mediano**. Niente "sprite infiniti" nei trailer se la maggioranza gira in tier B.

---

## 5. Licenze e legale (verificato sui testi delle licenze)

| Componente | Licenza | Obblighi |
|---|---|---|
| Qwen2.5-Coder-7B-Instruct (e Qwen3.5) | Apache 2.0 | Testo licenza + NOTICE ("Copyright 2024 Alibaba Cloud"). Bundling in gioco a pagamento: OK senza restrizioni |
| llama.cpp / stable-diffusion.cpp | MIT | Solo notice nei crediti |
| SD 1.5 | CreativeML OpenRAIL-M | Commerciale OK, irrevocabile, nessun cap di fatturato. MA: (a) spedire il testo della licenza; (b) **la tua EULA deve incorporare le 11 restrizioni d'uso dell'Attachment A come clausole vincolanti per il giocatore** (Sez. III, §4.a/4.b — è boilerplate standard, non un blocco); (c) NON è open source secondo OSI |
| LCM-LoRA SD1.5 | openrail++ | Come sopra (la distillazione non toglie gli obblighi RAIL) |
| SD-Turbo / SDXL-Turbo | Stability Community License | ⚠️ termina sopra $1M/anno + attribution obbligatoria — **evitare** |
| FLUX.2-klein-**4B** / Z-Image-Turbo | Apache 2.0 | Puliti (attenzione: klein-**9B** è non-commerciale) |

- Il vincolo "solo modelli open source": SD1.5 lo viola in senso stretto (RAIL non è OSI). O lo ammorbidisci in "modelli con pesi liberamente ridistribuibili per uso commerciale", o il tier alto Apache-2.0 (FLUX.2-klein-4B) diventa il tuo percorso "veramente open".
- **Vendorizza i pesi**: il repo originale runwayml/stable-diffusion-v1-5 è stato cancellato (agosto 2024, oggi 401); il canonico è un mirror comunitario non affiliato. Il tuo diritto di ridistribuzione è irrevocabile una volta ottenuta una copia: pinna commit hash + checksum e tieni i pesi nella tua pipeline di build.
- **Copyright degli output**: USCO Part 2 (gen 2025) + Thaler v. Perlmutter (cert. negata mar 2026) — l'output puramente AI non ha copyright USA; posizione UE equivalente. Gli asset generati sulla macchina del giocatore sono di fatto pubblico dominio. **Non impedisce di vendere il gioco**: la tua IP difendibile è engine, pipeline, prompt template, trademark e asset autoriali. Conseguenza pratica: usa arte umana per capsule e materiali marketing chiave (hanno anche copyright pieno, e Steam chiede disclosure pure sul marketing AI).

---

## 6. Steam: approvazione e packaging

**Il gioco è approvabile.** Regole (policy gennaio 2024, chiarita gennaio 2026, verificate sulla doc Steamworks):

- Nel Content Survey spunti **Live-Generated AI**: descrivi l'implementazione in dettaglio e "what kind of guardrails you're putting on your AI to ensure it's not generating illegal content". Il testo appare *verbatim* sullo store. Spunta anche Pre-Generated se spedisci asset AI baked (tier C, capsule).
- La tua disclosure ideale, già pronta dai §3: modelli nominati, tutto locale, **nessun input libero del giocatore raggiunge i generatori**, sandbox Lua capability-restricted, filtro NSFW su ogni sprite, filtri sui testi, log locale.
- Unica categoria vietata: contenuto sessuale Adult-Only live-generated ("we don't want to ship... at this time") → il filtro NSFW è hard-requirement, mai un toggle "uncensored".
- I giocatori hanno un bottone nell'overlay per segnalare contenuto AI illegale: le guardrail devono *funzionare*, non solo essere descritte. (Nota: la voce "rimozione dallo store per guardrail inefficaci" circola sui blog ma NON è nella doc primaria Valve; resta il potere generale di rimozione del Distribution Agreement.)
- Precedenti approvati e in vendita con generazione locale runtime: AIdventure (2022), AI Roguelite (2023). Nel 2025 ~1 release su 5 ha dichiarato genAI: la disclosure non è più una lettera scarlatta.

**Packaging dei pesi (norma Valve verificata)**: la doc Steamworks dice esplicitamente di *non* far scaricare contenuto dentro il gioco dopo il lancio → **niente downloader HTTP al primo avvio**. Meccanismo corretto: **depot DLC gratuiti** con "Disable Steam automatically downloading this DLC", installati da codice via `ISteamApps::InstallDLC` dopo che il benchmark ha scelto il tier (è il pattern standard dei texture pack HD). Struttura: base game ~1.5GB (tier più piccolo incluso), "Standard models" ~7GB (7B Q4 + SD1.5) come DLC gratuito, tier low-spec separato. **Un depot per versione di modello**: un model update diventa un depot nuovo opzionale, non un re-download forzato da 5GB (il delta patcher di Steam lavora malissimo sui blob di pesi). Lo spazio disco non è un problema (83% degli utenti ha ≥100GB liberi).

**Matrice di build**: spedisci **Vulkan + CPU** su Windows/Linux (un binario copre NVIDIA/AMD/Intel e il Deck, ed elimini ~500MB di redistributable CUDA e una colonna di QA), Metal su Apple Silicon, macOS x86 fuori o tier C. Tre target QA invece di sette.

---

## 7. Piano commerciale

- **Prezzo: $14.99** — la fascia provata degli arcade roguelike premium (Isaac: Rebirth 6M+ copie, Balatro 5M+, e AI Roguelite sta esattamente lì). L'IA *non* giustifica un premium. Sconto lancio sì, prezzo di listino più basso no ($5 stile Vampire Survivors/Megabonk funziona solo con scope minuscolo).
- **La tassa dello stigma AI è reale e va messa a budget**: studio causale Game Oracle su 9.879 giochi 2025 — i giochi con disclosure AI ricevono **-52.6% di recensioni** (mediane 4 vs 7 nel primo mese, 84.6% vs 88.3% positive), e la penalità è *massima* per i giochi ad alto potenziale ben marketati.
- **Contromisura n.1 — framing**: vendi il gameplay, mai l'IA come headline. Ogni "primo gioco fatto dall'IA" è stato massacrato ("a milestone for slop" — PC Gamer su Codex Mortis; "dementia Minecraft" su Oasis); ogni successo (Infinite Craft, AI2U 90% positive, Suck Up!) ha venduto l'esperienza. Il tuo pitch: **"Un roguelike infinito: nessun pool di oggetti, nessuna run già vista da qualcun altro. Tutto sul tuo PC, niente cloud, niente abbonamenti."** L'IA sta nella disclosure e nelle FAQ, con orgoglio tecnico ma non in copertina. Il local-only è anche un angle privacy/anti-subscription che il pubblico Steam premia.
- **Contromisura n.2 — demo gratuita + streamer**: l'imprevedibilità emergente è materiale da clip naturale (è come sono cresciuti Suck Up! e AI2U). La demo col benchmark integrato abbatte insieme stigma, dubbio hardware e rischio rimborsi. Steam Next Fest come momento chiave.
- **Retention oltre la novelty**: i giochi AI-native collassano quando l'effetto "è vivo" svanisce (Whispers from the Star: 964→21 CCU). Il loop roguelike deve essere divertente *anche con contenuto mediocre*: controlli tight, proiettili leggibili, meta-progressione. Le run col contenuto "meh" arriveranno comunque — è il pavimento di gameplay a salvarle.
- **Lezione AI Roguelite (da copiare/evitare)**: copia l'economia local-first e l'onestà della disclosure; evita l'attrito di setup (5-15 min di LMStudio citati in ogni recensione negativa — i tuoi pesi bundled + benchmark invisibile sono il vantaggio); evita l'incoerenza non validata (le sue critiche top — testo incoerente, progressione nonsense, incontri "derpy" — sono tutte assenze di validazione, e sono il tuo soffitto di qualità se non costruisci la pipeline del §3).
- **Studia le recensioni negative di AI Roguelite 2D** prima di congelare lo scope di generazione: è il tuo canarino — stessa premessa, Mixed 59%.

---

## 8. Roadmap

**Fase 0 — Fondamenta (subito, ~3-6 settimane)**
Pivot RunBundle; sandbox indurita completa (§3.1-3.6); pipeline di validazione con GBNF + dry-run headless + fallback a template; HMAC su cache. CI con golden corpus di ~1.000 bundle archiviati ri-validati a ogni modifica di prompt/modello.

**Fase 1 — Vertical slice**
Piano 1 autoriale + piano 2 generato al 100%; daemon di banking; benchmark misurato con tier S/A/B/C; UI di gioco custom (raygui → solo debug tools). Milestone: nuova run <5s, piano N+1 pronto p95, <2% fallback a template.

**Fase 2 — Safety + presenza Steam**
Filtri contenuti (testo+sprite) e log; pagina Steam con disclosure Live-Generated scritta bene; EULA con pass-through OpenRAIL-M; THIRD_PARTY_NOTICES; demo gratuita con benchmark; Next Fest.

**Fase 3 — Early Access a $14.99**
Depot DLC per i modelli; telemetria opt-in (failure rate di generazione, istogramma retry, tok/s per tier); bottone "segnala oggetto rotto" con allegato bundle; build Vulkan Win/Linux; verifica Steam Deck (tier dedicato).

**Fase 4 — Post-lancio**
Daily challenge via bundle pubblicati; condivisione run/Workshop; tier modello alto (FLUX.2-klein-4B / Z-Image-Turbo, Qwen3.5 dopo A/B); 1.0 quando retention e failure rate reggono.

---

## Fonti principali (verificate contro fonti primarie il 16/07/2026)

- Policy AI Steam: https://partner.steamgames.com/doc/gettingstarted/contentsurvey · annuncio gen 2024: https://store.steampowered.com/news/group/4145017/view/3862463747997849618
- Updates/depot: https://partner.steamgames.com/doc/store/updates · DLC: https://partner.steamgames.com/doc/store/application/dlc
- Licenza Qwen: https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct/blob/main/LICENSE · OpenRAIL-M: https://huggingface.co/spaces/CompVis/stable-diffusion-license/raw/main/license.txt · RAIL FAQ: https://www.licenses.ai/faq-2
- SD1.5 mirror: https://huggingface.co/stable-diffusion-v1-5/stable-diffusion-v1-5 · stable-diffusion.cpp: https://github.com/leejet/stable-diffusion.cpp
- USCO Part 2: https://www.copyright.gov/ai/Copyright-and-Artificial-Intelligence-Part-2-Copyrightability-Report.pdf
- Steam Hardware Survey: https://store.steampowered.com/hwsurvey/Steam-Hardware-Software-Survey-Welcome-to-Steam
- AI Roguelite: https://store.steampowered.com/app/1889620/AI_Roguelite/ · AI Roguelite 2D: https://store.steampowered.com/app/2800150/AI_Roguelite_2D/
- Studio stigma AI: https://www.game-oracle.com/blog/ai-part2
- NVIGI Lua Code Agent: https://docs.nvidia.com/nvigi-sdk/1.5.0/docs/SamplesCodeAgentLua.html
- Decoding vincolato per Lua di gioco: https://arxiv.org/pdf/2508.15866
