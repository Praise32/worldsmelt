# Spec: build Linux + generazione testo con LLM locale

Data: 2026-07-13
Stato: approvata a voce sezione per sezione, in attesa di revisione finale
Ciclo: 1 di ~5 (vedi roadmap)

## 1. Contesto e obiettivo

Melting Run oggi genera i contenuti della run (temi, boss, oggetti, script per
la mini-VM) tramite OpenAI, con un sidecar Node che scrive
`generated/current_run.txt`; il gioco C legge solo quel manifest. La build
funziona solo su Windows (MinGW, percorsi hardcoded nei `.bat`).

Obiettivo di questo ciclo, nell'ordine:

1. Il gioco compila e gira su Ubuntu (macchina di sviluppo attuale).
2. La generazione testuale funziona in locale, senza rete e senza OpenAI,
   tramite llama.cpp e un modello Qwen2.5-Coder in GGUF quantizzato.
3. Il gioco sa lanciare la generazione da solo, con schermata di caricamento.

Il "cosa" viene generato non cambia in questo ciclo: stesso schema di
contenuti, stessa mini-VM, stesso manifest. Cambia solo il "chi" genera.

### Visione di lungo periodo (fuori scope qui, registrata perché guida il design)

L'obiettivo finale del progetto è che a inizio run l'IA generi oggetti davvero
unici, sinergie inventate da zero per ogni combinazione, pattern di movimento e
attacco dei nemici e dei boss. Questo richiede un livello di script più
espressivo della mini-VM attuale (4 operazioni): è la fase "sandbox Lua"
(ispirata alle callback della modding API di Isaac documentate da IsaacDocs).
melting-gen viene quindi progettato predisposto: prompt in file separati e una
grammatica per tipo di contenuto — aggiungere tipi di contenuto non dovrà
richiedere di rifare l'impianto. (Il formato del manifest resta invariato in
questo ciclo; una chiave di versione verrà introdotta solo quando arriveranno
nuovi tipi di contenuto.)

### Roadmap complessiva (ogni fase avrà la sua spec)

- **Fase 0+1 — questa spec:** build Linux + LLM testuale locale.
- Fase 2: sprite locali con stable-diffusion.cpp + modello pixel-art SD1.5,
  downscale nearest-neighbor + quantizzazione palette in C.
- Fase 3: sandbox Lua per oggetti/nemici/boss unici (supera la mini-VM).
- Fase 4: UI con raygui (menu, impostazioni, schermate di caricamento).
- Fase 5: benchmark al primo avvio e scelta automatica del modello.

## 2. Vincoli hardware e decisioni verificate

Macchina di riferimento: Ubuntu 26.04, Ryzen 5 3600, 15 GiB RAM,
**RX 5600 XT 6 GB (RDNA1/gfx1010), driver Mesa RADV**.

- **Backend GPU: Vulkan, mai ROCm.** ROCm non ha mai supportato gfx1010 e i
  workaround sono instabili; il backend Vulkan di llama.cpp è maturo, gira sul
  driver Mesa di serie ed è pari o più veloce di ROCm su RDNA1
  (fonte: scoreboard Vulkan llama.cpp, discussione ggml-org #10879).
- **Niente flash-attention su RDNA1** (crolla le prestazioni, stessa fonte).
- Riferimento prestazioni (5700 XT, 7B Q4_0): ~70 token/s generazione; la
  5600 XT ha meno banda → attesi ~40–50 t/s a pieno offload, meno con offload
  parziale. Numeri definitivi da `make test-llm` sulla macchina reale.
- **VRAM 6 GB = il vincolo che ha deciso l'architettura:** il 7B Q4_K_M
  (~4,7 GB file, ~5,2–5,7 GB con contesto) è al limite; LLM e (in fase 2)
  Stable Diffusion non potranno mai coesistere in VRAM. Da qui la scelta del
  generatore come processo separato che esce e libera tutto (§3).
- Budget di attesa accettato dall'utente: **1–2 minuti** a inizio run, con
  evoluzione futura verso generazione in background dei piani successivi.
- CPU fallback (Ryzen 5 3600): ~6–9 t/s (7B), ~25–40 t/s (1.5B) — lento ma
  utilizzabile entro il budget.

## 3. Architettura

Approccio scelto: **generatore separato** (alternativa scartata: llama.cpp
linkato nel gioco — binario fragile, un OOM di inferenza ucciderebbe il gioco,
e contraddice la regola di AGENTS.md "il motore C resta indipendente dai
modelli AI"; scartato anche llama-server+Node: manterrebbe Node per sempre).

```text
INIZIO RUN (schermata di caricamento)

  gioco (C + raylib)
     └─ lancia ─> melting-gen (C99 + libllama, processo figlio)
                    ├─ carica Qwen GGUF (Vulkan, n_gpu_layers config)
                    ├─ genera JSON (grammatica GBNF committata)
                    ├─ valida/normalizza → generated/current_run.json + .txt
                    ├─ progresso → generated/gen_progress.txt
                    └─ esce → VRAM interamente liberata

  gioco legge il manifest ──> gameplay a 60 FPS
```

Invarianti che questo ciclo NON tocca: formato manifest, mini-VM, struttura
`src/`, percorso Node/OpenAI (resta funzionante come alternativa), file `.bat`.

## 4. Struttura repo e build Linux

```text
melting-run-gpu/
├── Makefile                  ← gioco + melting-gen (Linux)
├── scripts/
│   ├── setup-deps.sh         ← una tantum: dipendenze in deps/
│   └── download-models.sh    ← GGUF in models/, con SHA256
├── deps/                     ← ignorata da git
│   ├── raylib/               ← clone a tag fissato, build statica
│   └── llama.cpp/            ← clone a tag fissato, -DGGML_VULKAN=ON
├── models/                   ← ignorata da git (+ models/README.md generato)
├── tools/melting-gen/        ← generatore: main.c, cJSON vendorato,
│   ├── prompts/              ←   prompt/few-shot come file di testo
│   └── run.gbnf              ←   grammatica committata
└── src/gen/                  ← gen_runner.h/.c (spawn+monitor, lato gioco)
```

- `setup-deps.sh`: installa i pacchetti apt necessari (`cmake`,
  `libvulkan-dev`, `glslc`, dev X11/Wayland; richiede password — avvisare
  prima), poi clona e compila raylib e llama.cpp **a tag precisi** scritti nel
  file. Niente submodule: script leggibile e rilanciabile.
- `Makefile`: target `all`, `run`, `run-gen` (gioco con `--generate`),
  `test`, `test-gen`, `test-llm`, `clean`.
- llama.cpp è C++ solo al link di melting-gen (`-lstdc++`); il binario del
  gioco resta C puro senza dipendenze nuove.
- llama.cpp viene fissato a un tag di release: l'API C di llama.h cambia tra
  release, l'aggiornamento del tag è una modifica deliberata da testare.

## 5. melting-gen (tools/melting-gen/)

Eseguibile C99. Sostituto locale, a parità di output, dello script Node:
scrive gli stessi `generated/current_run.json` e `generated/current_run.txt`.

- **Modello default: Qwen2.5-Coder-7B-Instruct Q4_K_M** (~4,7 GB, Apache 2.0),
  offload parziale su 6 GB; stima 20–60 s per run completa, entro budget.
  **Riserva: 1.5B-Instruct Q4_K_M** (~1,1 GB, Apache 2.0), tutto in GPU,
  pochi secondi. Ripiego automatico 7B→1.5B se il file manca. Il 3B è
  escluso (licenza Qwen Research, non commerciale). I default definitivi
  (modello e `n_gpu_layers`) si fissano coi numeri di `make test-llm`.
- **Output vincolato:** lo schema JSON della run è fisso, quindi la grammatica
  GBNF è scritta a mano una volta e committata (`run.gbnf`), applicata via
  `llama_sampler_init_grammar`. Il modello non può produrre JSON malformato
  né trait/slot/trigger fuori enum. La grammatica vincola ma non spiega: il
  prompt (file in `prompts/`, modificabili senza ricompilare) descrive lo
  schema e include un esempio completo.
- **Validazione:** parsing con cJSON (singolo .c/.h, MIT, vendorato), poi le
  stesse normalizzazioni del Node attuale (es. `on_fire:projectile` →
  combinazione valida; clamp dei parametri). La grammatica garantisce la
  forma, il validatore la semantica: entrambi obbligatori.
- **Retry e fallback:** output troncato/incoerente → fino a 2 retry. Retry
  esauriti, modello assente o errore di caricamento → generazione
  deterministica con seed (portata in C dal percorso `--fallback` del Node).
  In ogni esito il manifest scritto è valido.
- **Progresso:** righe `fase|percentuale|messaggio` su
  `generated/gen_progress.txt` (scrittura atomica: file temporaneo + rename).
  Exit code: 0 = manifest pronto; ≠0 = il chiamante usa il fallback.
- **Diagnostica:** `logs/melting-gen.log` con modello usato, tempi di
  caricamento, token/s, retry — base dati per la fase 5 (benchmark).
- **Flag:** `--model PATH`, `--ngl N`, `--seed N`, `--temp X`, `--ctx N`,
  `--fallback`, `--out DIR`.

## 6. Integrazione nel gioco (src/gen/)

Nuovo modulo `gen_runner.h/.c`, unica responsabilità: ciclo di vita del
processo figlio (avvio POSIX, lettura progresso, esito, kill).

- Il gioco lanciato con `--generate`: alla nuova run (menu o tasto `R`) entra
  nello stato `APP_GENERATING`: schermata di caricamento col renderer attuale
  (barra, fase, seed), non bloccante, un poll del file di progresso per frame.
- `ESC` annulla → kill del figlio → menu.
- Esito: exit 0 + manifest valido → si gioca. Crash, timeout (default 180 s,
  configurabile) o manifest invalido → fallback + messaggio a schermo col
  sistema messaggi esistente. Mai crash, mai schermo nero.
- Senza `--generate` il comportamento attuale è identico (legge il manifest
  esistente).
- Windows: `src/gen/` compila una versione vuota dietro guard di piattaforma
  ("generazione esterna non disponibile"): la repo continua a compilare con i
  `.bat` come oggi.

## 7. Gestione errori (riepilogo)

| Evento | Esito |
|---|---|
| Modello 7B mancante | melting-gen ripiega su 1.5B |
| Anche 1.5B mancante / load fallito | melting-gen `--fallback` (seeded, deterministico) |
| JSON troncato o semanticamente rotto | fino a 2 retry, poi fallback |
| melting-gen crasha o va in timeout | il gioco lo rileva, fallback + messaggio a schermo |
| ESC durante la generazione | kill del processo, ritorno al menu |
| Manifest scritto ma invalido | il validatore lato gioco rifiuta → fallback |

Regola unica: **qualunque percorso termina con un manifest valido e il gioco
avviabile.**

## 8. Test

1. **Senza modello (`make test` + `make test-gen`):** i 4 test esistenti
   (`--script-test`, `--portal-test`, `--smoke-test`, `--screenshot-test`)
   portati su Linux; determinismo del fallback (stesso seed → manifest
   identico byte per byte); validatore contro una raccolta di JSON rotti e
   maliziosi (mai passare output crudo al gioco).
2. **Stato di caricamento senza modello:** finto generatore (script che
   scrive progresso lento e poi un manifest) per collaudare `src/gen/`:
   annullamento, timeout, crash del figlio, exit code sporchi.
3. **Con modello (`make test-llm`):** una run completa reale, validazione del
   manifest, stampa di tempi e token/s. Fa anche da benchmark: i suoi numeri
   fissano i default di §5.

## 9. Documentazione e AGENTS.md (mandato: pieno potere di miglioramento)

- **AGENTS.md** diventa dual-platform: verifiche Linux (`make test`) come via
  principale, sezione Windows conservata; responsabilità dei nuovi moduli
  (`src/gen/`, `tools/melting-gen/`); regola nuova: chi modifica grammatica o
  prompt riesegue `make test-gen` (e `make test-llm` se disponibile un modello).
- **docs/LOCAL_REFERENCES.md** aggiornato alla realtà Linux (raylib/llama.cpp
  in `deps/` pinnati; percorsi `C:\Users\maria\...` rimossi).
- **docs/README.md**: indice aggiornato con questa spec.
- **README.md** radice: sezione "Avvio rapido su Linux" a implementazione
  conclusa (documentare solo cose vere).
- APPUNTI.md e DESIGN_NOTES.md restano intatti (appunti di visione
  dell'autore); questa spec li integra, non li sostituisce.

## 10. Criteri di successo del ciclo

1. `scripts/setup-deps.sh && make && make run` su Ubuntu pulita → il gioco
   parte e si gioca (parità col comportamento Windows attuale).
2. `scripts/download-models.sh && make run-gen` → una run interamente
   generata in locale, senza rete, entro ~2 minuti sulla macchina di
   riferimento, con schermata di caricamento fluida e annullabile.
3. `make test` e `make test-gen` verdi; `make test-llm` produce manifest
   valido e riporta i token/s.
4. Al termine della generazione nessun residuo: processo uscito, VRAM libera.
5. La repo compila ancora su Windows coi `.bat` esistenti (verifica differita
   al prossimo avvio in dual-boot; il codice nuovo è dietro guard).

## 11. Fuori scope (fasi successive)

Generazione immagini locale (fase 2), sandbox Lua e contenuti unici
(fase 3), raygui (fase 4), benchmark al primo avvio con auto-selezione del
modello (fase 5), fine-tuning del modello (non necessario ora; se mai servirà
si farà su GPU cloud, mai su RDNA1), pubblicazione/distribuzione.
