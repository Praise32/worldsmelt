---
id: aiprod-exp-image-gen2-2026-07-23
title: Comparison immagini gen-2 — SD3.5, SDXL, Flux su 6 GB (23/07/2026)
domain: ai-production
status: implemented
authority: supporting
owner: ai-production
summary: >-
  Probe empirici dei modelli moderni su RX 5600 XT/Vulkan: nessuno batte la baseline
  SD1.5 (5.6s/img) — Flux.1-schnell Q2K@768 il piu' vicino a ~63s (11x), SDXL@768 74s,
  SD3.5 Medium@768 89s; TAESD-XL sblocca il decode 1024 (1.78s); licenze verificate.
  Artefatti in logs/model-comparison/images-gen2-20260723-202847/.
last_reviewed: 2026-07-23
last_verified_commit: c3f3005
topics: [comparison, immagini, sdxl, flux, sd35, taesd, licenze, gen2]
related: [aiprod-exp-image-comparison-2026-07-23]
supersedes: []
source_files: [docs/plans/active/model-comparison.md]
---

# Estensione gen-2: modelli immagine "moderni" su RX 5600 XT 6 GB (Vulkan)

Task 23-24/07/2026 (notte + mattina, dopo liberazione della GPU confermata dal
coordinatore). Estende `docs/plans/active/model-comparison.md` (sezione immagini, finora
solo SD1.5) ai modelli non-SD1.5 supportati dal build pinnato di
`deps/stable-diffusion.cpp` (`master-775-b5d8120`, commit `b5d8120`): SD3.5 Medium, SDXL,
Flux.1-schnell. Obiettivo: capire quale modello moderno è il più piccolo/veloce/allenabile-
con-LoRA per orientare una futura campagna di training.

Confronto **raw pipeline**: generato con il binario `sd-cli` di `deps/stable-diffusion.cpp`
direttamente (non `bin/melting-sprites`, cablato su SD1.5 — LoRA/prompt/negative/quality-
gate del gioco NON sono in gioco qui). Il giudizio sulla resa visiva finale resta
dell'utente sugli artefatti PNG in questa cartella; questo documento si limita a velocità,
VRAM, fattibilità, licenze.

## 1. Cosa supporta il build pinnato

- `deps/stable-diffusion.cpp` tag `master-775-b5d8120`. Binari: **`sd-cli`, `sd-server`**.
- Architetture immagine (da `README.md`): SD1.x/2.x/Turbo, **SDXL/SDXL-Turbo**,
  **SD3/SD3.5**, **FLUX.1-dev/schnell**, e altre non rilevanti qui.
- Formati pesi: checkpoint PyTorch, safetensors, **GGUF anche per i text encoder**
  (confermato da `docs/wan.md`).
- LoRA (`<lora:nome:peso>` + `--lora-model-dir`), LCM/LCM-LoRA, TAESD/TAESD-XL,
  quantizzazione al volo `--type`, **`--vae-tiling`** (leva chiave scoperta in questa
  sessione per sbloccare SDXL a 1024×1024, vedi §5.2).
- `--backend te=cpu`: tiene i text encoder in RAM — la leva che rende fattibili SD3.5/Flux
  sui 6 GB di riferimento. Usata in OGNI config di questo documento.
- `--list-devices`: `Vulkan0 AMD Radeon RX 5600 XT (RADV NAVI10)`, nessun matrix core
  (RDNA1) — coerente con ADR-001.

## 2. Candidati scaricati (tutti verificati, licenze in §6)

| Candidato | Fonte (non gated) | Dimensione |
|---|---|---:|
| SD3.5 Medium Q4_K_M | `city96/stable-diffusion-3.5-medium-gguf` | 1.79 GB |
| SD3.5 clip_l/clip_g | `Comfy-Org/stable-diffusion-3.5-fp8` | 0.25+1.39 GB |
| SD3.5 VAE | `Shio-Koube/SD-3.5-vae` (SHA256 identico all'ufficiale gated) | 168 MB |
| T5-XXL encoder GGUF Q4_K_M | `city96/t5-v1_1-xxl-encoder-gguf` (condiviso SD3.5+Flux) | 2.90 GB |
| Flux.1-schnell Q2_K + Q3_K | `leejet/FLUX.1-schnell-gguf` | 4.11+5.31 GB |
| Flux clip_l | `comfyanonymous/flux_text_encoders` | 246 MB |
| Flux VAE | `Comfy-Org/Lumina_Image_2.0_Repackaged` (dimensione byte-identica all'ufficiale) | 335 MB |
| SDXL base 1.0 | `stabilityai/stable-diffusion-xl-base-1.0` | 6.94 GB |
| SDXL VAE fp16-fix | `madebyollin/sdxl-vae-fp16-fix` | 335 MB |
| TAESD-XL | `madebyollin/taesdxl` | 9.8 MB |
| LCM-LoRA-SDXL | `latent-consistency/lcm-lora-sdxl` | 394 MB |
| SDXL-Lightning 4-step LoRA | `ByteDance/SDXL-Lightning` | 394 MB |
| pixel-art-xl LoRA di stile | `nerijs/pixel-art-xl` | 170 MB |
| SDXL-Turbo / SD-Turbo | **non scaricato** (esclusione richiesta) | — |

Totale ~25 GB (budget 30-40 GB), sotto `models/gen2-comparison/` (gitignored).

## 3. SD3.5 Medium Q4_K_M

### 3.1 Il VAE mancante
`city96/stable-diffusion-3.5-medium-gguf` è solo il diffusion transformer: senza `--vae`
fallisce con errore esplicito ("VAE tensor ... not in model metadata"). Il repo ufficiale
col VAE è gated (form Stability, dati anagrafici — non completato: un agente non deve
inventare dati anagrafici per conto dell'utente). Usato `Shio-Koube/SD-3.5-vae`, **SHA256
identico byte per byte** (`8f53304a79335b55e13ec50f63e5157fee4deb2f30d5fae0654e2b2653c109dc`)
al file ufficiale (verificato via API HF autenticata, che espone gli hash dei repo gated
senza serve scaricarli).

### 3.2 Risoluzione e tempi

| Config | Esito | Note |
|---|---|---|
| 1024×1024, 20 step, euler, te=cpu | **CRASH** (device-lost dopo 3° step) | mmdit compute 1878 MB + params 2166 MB già oltre margine |
| 768×768, stessi parametri | **OK** — probe isolato pulito: **89.23s/immagine** | mmdit compute 660 MB; ~15.5s encode (T5-XXL CPU) + ~65s sampling (3.25s/step a regime) + ~4.15s decode |

Matrice reale interrotta a **2/12** (209.14s, 885.41s — sporca per contesa GPU con un
verifier concorrente nella prima parte della sessione, prima della liberazione della GPU).
Numero di riferimento: il probe isolato, **89.23s**. Non ripresa dopo la liberazione GPU:
priorità data a Flux/SDXL, esplicitamente richiesti dal coordinatore per completare la
matrice.

## 4. Flux.1-schnell — matrice completa (GPU dedicata, post-liberazione)

| Config | Esito | VRAM (params+compute) |
|---|---|---:|
| Q2_K, 1024×1024, 4 step, cfg 1.0, euler | **CRASH** (device-lost, 1° step) | 3920+3933 = **7853 MB** |
| **Q2_K, 768×768** | **OK — matrice completa 12/12** | 3920+973 = **4893 MB** |
| Q3_K, 768×768 | **CRASH** (device-lost, 1° step) | 5067+973 = **6040 MB**, appena sopra 6 GB |

**Q2_K@768 è il punto di equilibrio**: Q3_K sfora il budget di soli ~40 MB.

**Tempi Q2_K@768, matrice 6×2=12 immagini**: warmup 65.34s, **regime 62.94s/immagine**
(11 immagini, deviazione ±0.7s — GPU dedicata, nessuna contesa). Composizione: ~6.7s
encoding (CPU, T5-XXL) + ~42s sampling (4 step, 8.6s/step a regime dopo warmup shader) +
**15.11s decode VAE** (il VAE 16 canali di Flux ha un compute buffer sproporzionato, 5652
MB anche a 768×768 — il decode è il singolo passo più lento della pipeline Flux).
12/12 riuscite, zero fallimenti. `flux-schnell-q2k-768/` (12 immagini + meta).

## 5. SDXL base 1.0 — matrice a due risoluzioni + 4 accelerazioni (GPU dedicata)

### 5.1 768×768 — non serve `--vae-tiling`
Matrice completa 12/12, 25 step/cfg 7.0/euler_a: warmup 75.40s, **regime 74.28s/immagine**
(deviazione ±0.3s). unet compute 331.51 MB, vae compute 5652.14 MB (stesso ordine di
grandezza del VAE Flux — pattern ricorrente su questa scheda). `sdxl-base-768/` (12
immagini + meta).

### 5.2 1024×1024 — `--vae-tiling` è la scoperta chiave

| Config | Esito |
|---|---|
| senza `--vae-tiling` | **FALLITO — OOM pulito**: `failed to allocate Vulkan0 buffer of size 8541306888` (8.54 GB!) al decode VAE. Il sampling UNet completa senza problemi — il collo di bottiglia è SOLO il decode a piena risoluzione |
| con `--vae-tiling` | **OK**: vae compute crolla da ~8.5 GB a **416 MB**, unet compute 830.86 MB |

`--vae-tiling` non era nel task originale: trovato nell'help della CLI
(`process vae in tiles to reduce memory usage`) e verificato empiricamente come UNICO
sblocco per SDXL a risoluzione piena sui 6 GB di riferimento.

### 5.3 Tempi 1024×1024 — nota importante: throttling variabile osservato

Durante questa sessione la velocità di sampling a 1024×1024 ha oscillato **fino a 6×** fra
misure identiche in momenti diversi:
- **Regime "pulito"** (probe isolati, dopo una pausa fra generazioni): **3.78-3.83s/step**
- **Regime "rallentato"** (osservato nelle matrici lanciate a raffica, senza pause):
  **~23.6s/step**, stabile per decine di step consecutivi

Non isolata la causa con certezza: la GPU non risultava né power-capped né in P-state
basso quando verificato (`pp_dpm_sclk` mostrava 1780MHz*, il massimo; temperatura 64-65°C,
non estrema) — ma il pattern osservato (lento durante uso back-to-back, tornato veloce dopo
una pausa di alcuni minuti spesi a scrivere questo report) è coerente con un qualche tipo
di throttling termico/di sustained-load non riflesso nei contatori letti. **Riportato
onestamente entrambi i regimi**, perché chiunque pianifichi generazioni massive su questa
scheda deve aspettarsi variabilità reale, non solo il numero migliore misurato una volta.

| Config SDXL @1024×1024 | Step | Immagini | Tempo/immagine | Regime |
|---|---:|---:|---:|---|
| base (euler_a, cfg 7.0) + `--vae-tiling` | 25 | 1/12 (interrotta) | 607.98s | rallentato (23.6s/step) |
| + LCM-LoRA (lcm, cfg 1.5) + `--vae-tiling` | 8 | 2/12 (interrotta) | 274.59s, 276.10s | rallentato |
| + Lightning 4-step (euler, cfg 1.0) + `--vae-tiling` | 4 | 2/12 (interrotta) | 119.03s, 130.36s | rallentato |
| + pixel-art-xl(1.2) + LCM-LoRA(1.0), `--vae-tiling` | 8 | 2/12 (interrotta) | 211.97s, 209.74s | rallentato |
| + TAESD-XL (invece del VAE, NO `--vae-tiling` necessario) | 25 | 1 probe isolato | **101.08s** | **pulito** (3.78-3.83s/step) |

Tutte le config 1024 sono state **interrotte prima dei 12/12** dopo aver ottenuto una
lettura di regime stabile (2+ misure coerenti fra loro), per lasciare tempo a coprire
tutte le configurazioni richieste nella sessione. Le cartelle (`sdxl-base-1024-tiled/`,
`sdxl-lcmlora-1024/`, `sdxl-lightning-1024/`, `sdxl-pixelart-lcm-1024/`,
`sdxl-taesdxl-1024/`) contengono le immagini raccolte + `meta.txt` col dettaglio.

**TAESD-XL è l'accelerazione più efficace osservata**: decode VAE in **1.78s** (contro
10-15s del VAE pieno anche tiled) E non richiede `--vae-tiling` (il suo compute buffer,
2784 MB, sta comodamente sotto il limite che fa fallire il VAE pieno non-tiled). Anche
tenendo conto che il suo probe è caduto nel regime "pulito", il vantaggio strutturale
(niente tiling necessario, decode 5-8× più veloce) resta valido indipendentemente dal
regime di sampling.

## 6. Tabella licenze (sintesi — dettaglio in `docs/ai-production/licenze.md`)

| Candidato | Licenza | Commerciale | Allenabile LoRA |
|---|---|---|---|
| SD3.5 Medium (+ VAE/encoder) | Stability AI Community License | sì, sotto 1M$/anno ricavi | sì, `sd-scripts` (`sd3_train_network.py`) |
| SDXL + VAE-fix + TAESD-XL | openrail++ / MIT | sì | sì, `sd-scripts` (`sdxl_train_network.py`, il più maturo) |
| LCM-LoRA-SDXL, SDXL-Lightning | openrail++ | sì | n/a (già acceleratori) |
| pixel-art-xl (LoRA di stile) | CreativeML OpenRAIL-M | sì (stessa famiglia del pixel model di produzione) | n/a |
| Flux.1-schnell (+ ae/clip_l) | **Apache 2.0** | sì, nessuna restrizione | sì, `sd-scripts` (`flux_train_network.py`) |
| T5-v1_1-xxl-encoder GGUF | Apache 2.0 | sì | n/a |
| SDXL-Turbo / SD-Turbo (ESCLUSO) | Stability Community License (**verificato: NON la vecchia licenza non-commerciale assunta** — correzione registrata) | sì, se riconsiderato | n/a |

Tutte verificate all'upstream, non per sentito dire. Nessun peso ridistribuito col gioco.

## 7. Giudizio finale

### Velocità/fattibilità — nessuno dei tre "moderni" batte SD1.5

SD1.5 baseline: **5.63s/immagine** a 512×512, 8 step LCM (dato dalla comparison
precedente). I tre candidati moderni, ai loro punti di equilibrio migliori:

| Modello | Risoluzione | s/immagine (regime pulito) | Fattore vs SD1.5 |
|---|---|---:|---:|
| Flux.1-schnell Q2_K | 768×768 | 62.94s | ~11× |
| SDXL base | 768×768 | 74.28s | ~13× |
| SD3.5 Medium | 768×768 | 89.23s | ~16× |
| SDXL + TAESD-XL | 1024×1024 | 101.08s (regime pulito) | ~18× |

Non sorprende: SD1.5 è un UNet piccolo (860M parametri) con un solo CLIP leggero; le tre
moderne sono tutte DiT/MMDiT (o UNet 2.6B per SDXL) con encoder di testo enormi (T5-XXL) o
VAE dal compute buffer sproporzionato, entrambi costi che SD1.5 non paga.

### Il più piccolo/veloce accettabile: **Flux.1-schnell Q2_K @768**

È il più veloce dei tre moderni (62.94s vs 74.28s SDXL vs 89.23s SD3.5), il file più
piccolo su disco (4.11 GB contro 6.94 GB SDXL e ~4.4 GB combinati SD3.5+encoder dedicati),
**e ha la licenza più pulita** (Apache 2.0 puro, nessuna soglia di ricavi, nessuna
Attachment A). Tre criteri allineati sullo stesso candidato è un segnale forte.

### Chi batte la baseline SD1.5 per lo use-case sprite: **nessuno, su velocità**

Nessun candidato moderno è più veloce di SD1.5 su questa scheda con questo build. Se la
priorità restasse la velocità pura per la pipeline di generazione sprite del gioco (vincolo
1-2 minuti per l'intera run, non solo un'immagine), SD1.5 resta l'unica scelta praticabile
oggi. I moderni diventano interessanti SOLO se il caso d'uso cambia (es. generazione
offline/batch per una libreria di asset curati, non on-demand a inizio run) — decisione
che spetta al design del gioco, non a questo report tecnico.

### Leve di accelerazione per architettura

- **SDXL**: `--vae-tiling` (obbligatorio a 1024), TAESD-XL (la scoperta più utile: decode
  5-8× più veloce E niente tiling necessario), LCM-LoRA (8 step) e Lightning (4 step)
  entrambe riducono gli step ma NON il costo per-step-di-sampling — utili solo se il
  rallentamento "sustained load" osservato in §5.3 non si manifesta nel caso d'uso reale.
- **Flux**: nessuna leva di accelerazione testata oltre alla natura già-distillata di
  schnell (4 step nativi); il collo di bottiglia è il decode VAE (15s su 63s totali, 24%
  del tempo) — un TAEHV/TAE equivalente per Flux (non testato, non scaricato in questa
  sessione) potrebbe avere un impatto simile a TAESD-XL per SDXL.
- **SD3.5**: nessuna leva di accelerazione nella lista candidati di questa sessione
  (LCM-LoRA/Lightning sono specifiche per SDXL, non compatibili architetturalmente con
  SD3.5 MMDiT senza una LoRA dedicata, non scaricata).

### Allenabilità LoRA — tutti e tre pronti, SDXL il più maturo

Tutti e tre hanno script dedicati e documentati in `kohya-ss/sd-scripts` (ramo principale,
non sperimentali): `sdxl_train_network.py` (SDXL, il più maturo — architettura più vecchia
delle tre), `sd3_train_network.py` (SD3.5), `flux_train_network.py` (Flux). Nessun dato
sui requisiti VRAM di training raccolto in questa sessione (fuori scope: solo inferencing
misurato).

### Raccomandazione

Se si deve scegliere UN modello moderno su cui investire in una campagna LoRA:
**Flux.1-schnell Q2_K** per licenza pulita e velocità relativa migliore, con la riserva che
resta comunque ~11× più lento di SD1.5 e quindi non adatto a un uso on-demand nel gioco
senza un cambio di caso d'uso. **SDXL con TAESD-XL** è il secondo candidato più interessante
per la ricchezza dell'ecosistema di accelerazione (LoRA di stile pubbliche disponibili,
inclusa `pixel-art-xl` già verificata in questa sessione) anche se più lento e con licenza
leggermente meno pulita (openrail++ con Attachment A vs Apache 2.0 puro).

**Ricorda**: il giudizio sulla resa visiva (non velocità/fattibilità/licenze) resta
dell'utente sugli atlas PNG in questa cartella.

## Artefatti

- `sd35-medium-q4km-base/` — 2 immagini (contese) + probe isolato pulito + meta.
- `flux-schnell-q2k-768/` — 12 immagini (matrice completa) + meta.
- `sdxl-base-768/` — 12 immagini (matrice completa) + meta.
- `sdxl-base-1024-tiled/` — 1 immagine + meta.
- `sdxl-lcmlora-1024/` — 2 immagini + meta.
- `sdxl-lightning-1024/` — 2 immagini + meta.
- `sdxl-pixelart-lcm-1024/` — 2 immagini + meta.
- `sdxl-taesdxl-1024/` — 1 probe isolato + meta.
- `models/gen2-comparison/` — tutti i pesi scaricati e verificati (non versionato).
