---
id: aiprod-training-runbook
title: Runbook: prima campagna Style LoRA su RunPod
domain: ai-production
status: approved
authority: supporting
owner: ai-production
summary: >-
  Runbook passo-passo per addestrare la prima Style LoRA su RunPod: prerequisiti, scelta pod, dataset, comandi kohya_ss, sweep iperparametri, valutazione cieca.
last_reviewed: 2026-07-22
last_verified_commit: fe27f6d
topics: [LoRA, RunPod, kohya_ss, training, SD1.5, sweep]
related: []
supersedes: []
source_files: [tools/melting-sprites/main.c, logs/sprite-baseline/20260717-053243/]
---
# Runbook: prima campagna Style LoRA su RunPod

Scritto il 17/07/2026 dal content-designer, mentre l'utente dormiva. Tutto
quello che poteva essere preparato in locale (dataset CC0, registro di
provenienza, baseline congelata) è già fatto. Quello che resta è
intrinsecamente suo: aprire un account cloud, pagare, guardare gli sprite e
scegliere. Questo runbook è pensato per essere seguito passo-passo da chi
non ha mai noleggiato una GPU né usato kohya_ss.

Riferimenti: `roguelike-ai-appunti/05-dataset-e-licenze.md`,
`roguelike-ai-appunti/06-training-hardware-costi.md`, il piano approvato
`/home/meri/.claude/plans/ti-ho-messo-una-gleaming-riddle.md` (sezione
"Costi e tempi GPU cloud" e punto 7 della roadmap), `docs/ai-production/dataset/README.md`.

## 0. Stato di partenza (cosa è già pronto)

- **Dataset grezzo** in `dataset-raw/` (non versionato, vedi `.gitignore`):
  5 pack Kenney (CC0, link diretti verificati) + il repo GitHub
  `superpowers-asset-packs` (CC0-1.0, clone superficiale). Dettagli e numeri
  nel punto 4 di questo runbook / nel report di consegna.
- **Registro di provenienza** aggiornato: `docs/ai-production/dataset/ledger.jsonl`,
  verificato con `python3 scripts/dataset_ledger.py check` (nessun problema).
- **Baseline congelata**: `docs/ai-production/dataset/baseline-prompts.txt` (15 coppie
  tema|stile, NON toccare) + `logs/sprite-baseline/20260717-053243/` (30
  atlanti, seed 5 e 17, con `index.txt`). È il metro di paragone cieco per
  questa e per ogni futura LoRA: non rigenerarla, non modificarla.
- **Checkpoint base** già in `models/Public-Prompts-Pixel-Model.ckpt` (SD1.5
  pixel-art) — la Style LoRA si allena SOPRA questo checkpoint, non lo
  sostituisce.
- **Manca**: Ninja Adventure e Urizen scaricati "a mano" (vedi punto 4c —
  in realtà Ninja Adventure è già dentro `dataset-raw/superpowers-asset-packs/`,
  vedi sotto), l'account RunPod, e ovviamente il training vero e proprio.

## a. Prerequisiti

1. **Account RunPod**: [runpod.io](https://www.runpod.io), email + metodo di
   pagamento. RunPod ha un tetto di spesa di default di **80 $/ora su tutto
   l'account** ([documentazione billing](https://docs.runpod.io/references/billing-information))
   — è troppo alto per essere una vera rete di sicurezza per un singolo pod
   da meno di 1 $/ora: la disciplina vera è quella dei punti sotto, non
   questo numero.
2. **Ricarica a piccoli passi**: metti 20-30 € alla volta, non l'intero
   budget. Su Billing → Auto-Payments puoi impostare una soglia e un importo
   di ricarica automatica: tienilo basso (es. soglia 5 €, ricarica 20 €) così
   un pod dimenticato acceso non ti svuota il conto prima che tu te ne
   accorga.
3. **Budget**: questa campagna (Style LoRA, Esperimento 1) è stimata **35-40 €
   di eccesso** sul budget totale del mese di ~200 € (vedi il piano
   approvato, "Costi e tempi GPU cloud"). Il resto del budget è per le LoRA
   di ruolo successive (item, projectile/VFX) e margine errori — non
   spenderlo tutto qui.
4. **Tetto orario personale** (non è una funzione di RunPod, è disciplina
   tua): non lasciare mai un pod acceso senza sorveglianza per più di
   qualche ora. Un timer sul telefono o un controllo ogni 1-2 ore alla
   dashboard Billing basta. Per la primissima sessione (setup ambiente,
   primo run di prova a 100 step) mettiti un tetto morbido di ~2-3 ore GPU
   (~2 €), non di più: se qualcosa non funziona, spegni e ripensaci a mente
   fresca invece di lasciarlo girare a vuoto.

## b. Scelta del pod

| Fase | GPU | Tipo | Prezzo verificato |
|---|---|---|---|
| Prima sessione (setup ambiente + smoke test 100 step) | RTX 4090, 24 GB | **Secure Cloud** | 0,69 $/h |
| Sweep 5-10 run (una volta che l'ambiente funziona) | RTX 4090, 24 GB | Community Cloud, o Vast.ai | 0,34 $/h (Community) / variabile (Vast) |

Fonti: [RunPod RTX 4090](https://www.runpod.io/gpu-models/rtx-4090),
[listino Pods](https://www.runpod.io/pricing),
[Vast.ai pricing](https://docs.vast.ai/guides/instances/pricing). Le tariffe
cambiano: riverificale al momento del noleggio.

**Perché Secure la prima volta**: la prima sessione è quella in cui più cose
vanno storte (driver, versioni Python, path del dataset) — Secure Cloud è
gestito direttamente da RunPod, meno variabilità di hardware/affidabilità
di un host Community. Una volta che hai uno script di setup che funziona,
il costo/ora conta di più della garanzia, e passi a Community o Vast.

**Regole operative** (dal piano approvato, non negoziabili):
- On-demand per le prime sessioni; gli spot (interrompibili) solo se hai
  già checkpoint frequenti che sopravvivono a un'interruzione.
- **Auto-stop** attivo sul pod (RunPod ha un'opzione di idle/max-runtime
  timeout in fase di creazione del pod — impostala sempre).
- **Su Vast.ai: CANCELLA le istanze, non limitarti a fermarle.** Vast
  fattura storage/banda separatamente da RunPod e un'istanza "stopped" può
  continuare a costare.
- **Su RunPod, occhio alla differenza fra "stopped" e "terminated"**:
  secondo la [documentazione ufficiale dei prezzi](https://docs.runpod.io/pods/pricing),
  il *volume disk* di un pod resta a 0,10 $/GB/mese mentre gira, ma
  **raddoppia a 0,20 $/GB/mese quando il pod è solo "stopped"** (non
  terminato) — è ancora lì, ancora fatturato, più caro che running. Un
  *network volume* invece resta fisso a **0,07 $/GB/mese** (sotto 1 TB;
  0,05 $/GB/mese oltre) indipendentemente da pod acceso/spento/terminato.
  Conclusione pratica: metti dataset curato + checkpoint su un **network
  volume** separato (pochi GB, costa centesimi al mese), e a fine sessione
  **termina** il pod di calcolo (non limitarti a fermarlo) dopo aver
  scaricato tutto quello che ti serve in locale.

## c. Preparazione dataset (100-300 immagini curate)

Il registro (`docs/ai-production/dataset/ledger.jsonl`) ha oggi 3158 file da 6 pack, tutti
CC0. Non tutti sono utili: molti sono tilesheet impacchettate, preview,
file `.txt`/`.url`/font/suoni che lo script ha registrato ma che non sono
immagini di training individuali. Per lo Style Core (quello che insegna
tratto, palette, shading — non un soggetto specifico, vedi nota 05):

1. **Scegli le fonti in base alla coerenza stilistica** (nota 05: "il
   nucleo dello stile dovrebbe essere... coerente"). I 5 pack Kenney
   condividono la stessa mano/casa grafica: sono la scelta più sicura per un
   primo Style Core. `superpowers-asset-packs` è più eterogeneo (9
   sotto-pack di generi diversi) — buono come riserva per LoRA di ruolo
   future o per un secondo esperimento di stile, non ideale da mischiare
   ciecamente nel primo.
2. **Filtra via cartelle sbagliate**: escludi `Preview.png`, `Sample*.png`,
   i tilemap "_packed" (sono fogli interi, non sprite singoli), e i file
   non-immagine. Tieni le cartelle `Tiles/` (sprite singoli, già ritagliati)
   come priorità.
3. **Split per pack, MAI per frame** (regola 6 del registro): tieni un
   pack intero fuori dal training come validazione visiva. Esempio
   ragionevole: train = Tiny Dungeon + Micro Roguelike + Pixel Shmup (~600
   file grezzi, scendono a ~200-250 dopo il filtro del punto 2); holdout =
   1-Bit Pack (pack piccolo, stile compatibile, buono per un controllo a
   occhio rapido).
4. **Deduplica**: lo script del registro salta già gli sha256 duplicati fra
   pack (è già successo per qualche tile durante l'`add`, vedi output). Le
   varianti *quasi* identiche (ricolorazioni) non sono ancora rilevate
   automaticamente (perceptual hash non calcolato, vedi README del
   registro) — una scrematura a occhio veloce sulle ~200-250 immagini
   scelte basta, non serve uno strumento nuovo per 200 file.
5. **Caption**: un file `.txt` con lo stesso nome base di ogni immagine.
   Scrivi il CONTENUTO (soggetto, vista), non lo STILE — lo stile è quello
   che la LoRA deve imparare implicitamente, nominarlo nella caption lo
   rende più difficile da richiamare in modo affidabile in inferenza.
   **Usa `pixelsprite` come primo token di ogni caption**: è la parola con
   cui iniziano TUTTI i prompt fissi di `tools/melting-sprites/prompts/*.txt`
   (verificato: bomb/boss/coin/enemy_*/exit/heart/item/key/player/shot
   iniziano tutti con `pixelsprite,`). La valutazione cieca (punto e)
   richiede di NON toccare quei prompt: legando lo stile a un token già
   presente in ogni singolo prompt di baseline, la LoRA si attiva su ogni
   generazione senza bisogno di aggiungere altro. Esempio:
   `pixelsprite, stone wall tile, top-down dungeon floor`.

Struttura cartelle attesa per kohya (vedi punto d):

```
lora-dataset/
  10_meltingrun-style/
    tile_0001.png
    tile_0001.txt
    tile_0002.png
    tile_0002.txt
    ...
```

Il prefisso `10_` nel nome della cartella è la convenzione DreamBooth
classica di kohya: `10` = numero di ripetizioni per epoca (vedi punto d),
`meltingrun-style` è il "class token" — un'etichetta libera, non deve
comparire per forza nelle caption se usi già `pixelsprite` come sopra.

## d. Comandi kohya_ss (sd-scripts) per una Style LoRA SD1.5

Sul pod, dopo aver montato/scaricato il dataset curato e il checkpoint base:

```bash
git clone --depth 1 https://github.com/kohya-ss/sd-scripts
cd sd-scripts
python3 -m venv venv && source venv/bin/activate
pip install -r requirements.txt
pip install bitsandbytes  # per AdamW8bit
```

File di configurazione dataset `dataset.toml` (accanto a `lora-dataset/`):

```toml
[general]
shuffle_caption = true
caption_extension = ".txt"
keep_tokens = 1

[[datasets]]
resolution = 512
batch_size = 2

  [[datasets.subsets]]
  image_dir = "lora-dataset/10_meltingrun-style"
  num_repeats = 10
  class_tokens = "meltingrun-style"
```

(`batch_size = 2` è prudente su 24 GB VRAM a risoluzione 512; se il pod ha
margine puoi salire a 4 e dimezzare `num_repeats` per lo stesso numero di
step totali.)

Comando di training (rank 8, UNet-only, text encoder congelato,
risoluzione 512, checkpoint a ogni epoca):

```bash
accelerate launch --num_cpu_threads_per_process 8 train_network.py \
  --pretrained_model_name_or_path="/workspace/models/Public-Prompts-Pixel-Model.ckpt" \
  --dataset_config="/workspace/dataset.toml" \
  --output_dir="/workspace/output/run01" \
  --output_name="meltingrun-style-r8-lr1e4-ep10" \
  --save_model_as=safetensors \
  --save_every_n_epochs=1 \
  --network_module=networks.lora \
  --network_dim=8 \
  --network_alpha=4 \
  --network_train_unet_only \
  --learning_rate=1e-4 \
  --optimizer_type="AdamW8bit" \
  --lr_scheduler="cosine" \
  --lr_warmup_steps=100 \
  --max_train_epochs=10 \
  --mixed_precision="fp16" \
  --sdpa \
  --gradient_checkpointing \
  --seed=42 \
  --logging_dir="/workspace/logs" \
  --clip_skip=1
```

Note sui flag chiave (documentazione ufficiale
[train_network.md](https://github.com/kohya-ss/sd-scripts/blob/main/docs/train_network.md),
[train_network_README-ja.md](https://github.com/kohya-ss/sd-scripts/blob/main/docs/train_network_README-ja.md)):
- `--network_train_unet_only`: allena SOLO i moduli LoRA del U-Net, il text
  encoder resta congelato — esattamente il requisito "text encoder
  congelato" del piano.
- `--network_dim` (rank): 4-16 secondo la nota 06. `8` è un punto di
  partenza ragionevole; nello sweep sotto varialo.
- `--network_alpha`: circa metà di `network_dim` è la raccomandazione
  ufficiale (qui 4 per dim 8).
- `--save_every_n_epochs=1`: checkpoint frequenti come richiesto — permette
  di valutare epoca per epoca invece di aspettare la fine, e di accorgersi
  subito di un training che sta andando storto.

**Prima di lanciare il run vero**: fai un test da 100 step
(`--max_train_steps=100` al posto di `--max_train_epochs`) per verificare
che l'ambiente, i path e il dataset siano corretti senza bruciare ore di
GPU su un errore di configurazione (checklist nota 06).

### Sweep: 5-10 run variando rank/learning rate/epoche

Stesso comando, cambia solo `--network_dim`/`--network_alpha`,
`--learning_rate`, `--max_train_epochs` e `--output_name` a ogni run.
Tabella di partenza (aggiustala guardando i checkpoint intermedi):

| Run | dim | alpha | learning_rate | epoche | Nota |
|---|---:|---:|---:|---:|---|
| 01 | 8 | 4 | 1e-4 | 10 | baseline dello sweep |
| 02 | 8 | 4 | 5e-5 | 10 | lr più basso, stesso rank |
| 03 | 8 | 4 | 2e-4 | 8 | lr più alto, meno epoche |
| 04 | 4 | 2 | 1e-4 | 10 | rank minimo |
| 05 | 16 | 8 | 1e-4 | 10 | rank massimo |
| 06 | 8 | 4 | 1e-4 | 15 | più epoche, occhio all'overfitting |
| 07 | 16 | 8 | 5e-5 | 15 | rank alto + lr basso, più stabile |
| 08 | 8 | 4 | 1e-4 | 6 | poche epoche, controllo underfitting |
| 09 | 4 | 2 | 2e-4 | 10 | rank basso + lr alto |
| 10 | 16 | 4 | 1e-4 | 12 | alpha basso rispetto al dim (meno aggressivo) |

Non serve girarli tutti alla cieca: guarda i primi 2-3, capisci se il
dataset/setup produce risultati sensati, poi restringi lo sweep a varianti
del run migliore invece di consumare ore su combinazioni palesemente
sbagliate.

## e. Valutazione: confronto cieco con la baseline congelata

1. **Riporta a casa** ogni LoRA (`.safetensors`, poche decine di MB) subito
   dopo ogni run — non aspettare la fine dello sweep, scarica man mano.
   Mettile in `models/` con un nome che identifichi il run, es.
   `models/meltingrun-style-r8-lr1e4-ep10.safetensors`.
2. **Rigenera gli STESSI prompt/seed della baseline**: usa
   `docs/ai-production/dataset/baseline-prompts.txt` (le 15 coppie tema|stile, invariate)
   e i seed **5** e **17**, esattamente come in
   `logs/sprite-baseline/20260717-053243/`. Non modificare i file di prompt
   in `tools/melting-sprites/prompts/` — la LoRA si attiva da sola grazie al
   token `pixelsprite` (punto c).
3. **Comando**: `bin/melting-sprites` accetta un solo slot LoRA
   (`--lora`, default `models/lcm-lora-sdv1-5.safetensors`, usata per gli 8
   step veloci della baseline). Per testare la Style LoRA, sostituiscilo:

   ```bash
   bin/melting-sprites --out <cartella-run> \
     --model models/Public-Prompts-Pixel-Model.ckpt \
     --lora models/meltingrun-style-r8-lr1e4-ep10.safetensors \
     --seed 5
   ```

   **Attenzione**: la LoRA di default è LCM (Latent Consistency Model, per
   inferenza rapida a pochi step); sostituendola con la Style LoRA perdi
   l'accelerazione LCM. La baseline gira a 8 step (`--steps` di default)
   grazie a LCM; senza LCM, 8 step sono pochi per SD1.5 "normale" e l'atlas
   può uscire sotto-generato. Prova con più step per un confronto onesto,
   es. `--steps 20` o `--steps 30`, e annota chiaramente nel confronto che
   il numero di step NON è lo stesso della baseline (è una differenza nota,
   non un errore) — l'harness attuale non supporta due LoRA
   contemporaneamente (un solo slot, vedi `tools/melting-sprites/main.c`),
   quindi non è possibile avere "LCM veloce + Style LoRA insieme" senza
   modificare il tool, il che è fuori dal perimetro di questo task.
4. **Confronto alla cieca**: metti fianco a fianco l'atlas baseline
   (`logs/sprite-baseline/20260717-053243/atlas-NN-seedS.png`) e l'atlas
   nuovo per la stessa coppia tema/stile e lo stesso seed, senza sapere
   quale sia quale mentre giudichi (falli rinominare a qualcun altro, o
   semplicemente aspetta e guardali il giorno dopo quando non ricordi più
   l'ordine). Criteri dalla nota 05 (sezione "Style Core"): spessore dei
   contorni, palette, shading, direzione della luce, livello di dettaglio,
   proporzioni — la Style LoRA deve spostare questi assi in una direzione
   che TI piace di più, senza rompere la leggibilità della silhouette.
5. **Un solo vincitore**: tieni la LoRA che vince il confronto cieco su più
   coppie tema/stile, scarta le altre (o archivale fuori da `models/` se
   vuoi conservarle per confronto futuro).

## f. Checklist prima di noleggiare (da nota 06)

Prima di aprire un pod, spunta:

- [ ] dataset già curato e caricato (o pronto da caricare) — non improvvisare
      la selezione sul pod mentre l'orologio corre;
- [ ] ambiente riproducibile: script di setup salvato (il blocco `git
      clone` + `pip install` del punto d, magari in un file di note tue, non
      da eseguire a mano ogni volta);
- [ ] checksum del dataset (il registro con gli sha256 di
      `docs/ai-production/dataset/ledger.jsonl` copre già questo per i file grezzi);
- [ ] comando di training salvato da qualche parte (copia-incolla dal
      punto d, non riscriverlo a mente su ogni pod nuovo);
- [ ] test da 100 step fatto PRIMA del run completo;
- [ ] checkpoint su storage persistente (network volume, non il volume disk
      del pod — vedi punto b);
- [ ] timer o alert di costo impostato (punto a.4);
- [ ] piano per scaricare subito LoRA, config e log a fine run, prima di
      spegnere qualunque cosa;
- [ ] spegnimento (e su RunPod: **terminazione**, non solo stop; su Vast:
      **cancellazione**) dell'istanza appena finito;
- [ ] un rigo di registro tuo con GPU usata, driver/versioni, seed e durata
      di ogni run (utile quando fra tre settimane ti chiederai perché il run
      03 era diverso dagli altri).

## g. Cosa NON è stato scaricato automaticamente

- **Ninja Adventure Asset Pack** (itch.io, pixel-boy): il download diretto
  richiede un'interazione JS/pulsante sul sito itch.io (verificato: la
  pagina risponde 200 ma non espone un URL di download statico via
  `curl`/HTTP semplice) — **non è stato forzato**. **Buona notizia**: è
  comunque già presente in `dataset-raw/superpowers-asset-packs/ninja-adventure/`,
  perché quel repository CC0-1.0 lo ridistribuisce integralmente (con
  licenza propria che copre tutto il repo). Se vuoi comunque la versione
  originale da itch.io (per un controllo incrociato o per la pagina/licenza
  "fotografata" alla fonte), scaricala a mano: apri
  <https://pixel-boy.itch.io/ninja-adventure-asset-pack>, clicca
  "Download Now" (offerta a pagamento libera, puoi mettere 0), salva lo zip
  in `dataset-raw/ninja-adventure-itch/` e registralo con
  `dataset_ledger.py add ... --source-url "https://pixel-boy.itch.io/ninja-adventure-asset-pack"`.
- **Urizen 1Bit Tileset** (itch.io, vurmux): stesso problema di Ninja
  Adventure, e qui non c'è un mirror CC0 alternativo già scaricato. Se ti
  serve (utile solo per forme/categorie, è monocromatico — non insegna
  colore né shading, nota 05), scaricalo a mano da
  <https://vurmux.itch.io/urizen-onebit-tileset> e registralo allo stesso
  modo.
- **OpenDuelyst**: il repository GitHub `open-duelyst/duelyst` pesa
  **~950 MB** (verificato via GitHub API, campo `size`) ed è per lo più
  codice sorgente del gioco (TypeScript/CoffeeScript), non un pacchetto di
  soli asset — gli sprite sono nidificati sotto `app/resources` e
  `app/original_resources`. Un clone completo per questo task sarebbe
  sproporzionato rispetto a quanto serve (poche centinaia di immagini
  curate). **Saltato deliberatamente**, come previsto dal task ("salta se
  non è piccolo"). Se in futuro serve davvero questo corpus, usare
  `git clone --filter=blob:none --sparse` e poi
  `git sparse-checkout set app/resources app/original_resources` invece di
  un clone pieno — non fatto qui per restare dentro il perimetro "non
  toccare tools/src, non fare build", ed è comunque un lavoro di pulizia
  (rimozione loghi/nomi come richiede la nota 05) che vale la pena solo
  quando servirà davvero.
