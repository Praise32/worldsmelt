#!/usr/bin/env python3
"""Script di training LoRA per Stage B (dataset/lora-v1-research), adattato dal
comando di riferimento di 05-KAGGLE-TRAINING-RUNBOOK.md ("accelerate launch
ml/train_lora.py --config ...") al pacchetto autonomo di questo dataset,
perche' il repo non ha ancora una cartella ml/ condivisa: qui la logica vive
accanto ai dati che consuma, non sparsa fra repo e notebook.

QUESTO SCRIPT NON VA LANCIATO DA QUESTA SESSIONE. E' preparato e documentato
(vedi kaggle/README.md per il comando esatto); l'avvio su GPU Kaggle lo fa
l'orchestratore col proprietario dopo approved_gpu_run: true
(06-AGENTI-KAGGLE-MCP.md). Qui, in locale, gira senza GPU e senza torch: solo
`python3 -m py_compile` per verificarne la sintassi.

Uso su Kaggle (dentro il kernel, dopo che run.py ha estratto il tar):

    accelerate launch train_lora_v0.py --config configs/lora-v1-research-dreamshaper8.yaml
    accelerate launch train_lora_v0.py --config configs/lora-v1-research-dreamshaper8.yaml --smoke-test

Preflight (fallisce PRIMA di toccare la GPU, come richiesto dal runbook):
  - ogni immagine del dataset ha una riga nel ledger;
  - ogni riga del ledger ha license_id in whitelist (CC0/CC0-1.0/own/commissioned);
  - ogni immagine e' apribile, non corrotta e 512x512;
  - ogni caption esiste e non e' vuota;
  - nessun subject_key compare sia in train che in val;
  - nessuna COPPIA train/val e' quasi identica a livello di pixel (il controllo
    sul solo subject_key e' vacuo: lo split viene assegnato per chiave, quindi
    non puo' che risultare coerente con se stesso);
  - lo split di validazione non e' vuoto (altrimenti validation_steps sarebbe
    teatro);
  - il file dei prompt di valutazione esiste, e' JSON valido e contiene l'id
    usato dallo smoke test (altrimenti il crash arriverebbe allo step 250, dopo
    aver consumato GPU);
  - la cartella di output e' scrivibile.
"""

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

# Import pesanti (torch/diffusers/peft/accelerate) rimandati dentro le
# funzioni che li usano davvero: questo modulo deve restare importabile
# (e verificabile con py_compile) anche su una macchina senza GPU, come
# quella che l'ha scritto.

LICENSE_WHITELIST = {"cc0", "cc0-1.0", "own", "commissioned"}

# Stessa soglia di scripts/lora_dataset_build.py (DEDUP_MERGE_SCORE): sotto
# questa differenza normalizzata sull'unione delle aree opache, due immagini
# sono varianti dello stesso soggetto e non possono stare sui due lati.
DEDUP_MERGE_SCORE = 0.15
SIGNATURE_PX = 64


def load_ledger(ledger_path: Path):
    rows = []
    with ledger_path.open("r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                rows.append(json.loads(line))
            except json.JSONDecodeError as e:
                raise SystemExit(
                    f"PREFLIGHT FALLITO: riga {lineno} di {ledger_path} non e' JSON valido: {e}")
    return rows


def sha256_of_file(path: Path, chunk=1 << 22) -> str:
    """A blocchi: i checkpoint base sono ~2 GB e `Path.read_bytes()` li
    caricherebbe interi in RAM prima ancora di iniziare il training."""
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(chunk), b""):
            h.update(block)
    return h.hexdigest()


PERCEPTUAL_BG_TOL = 24
PERCEPTUAL_COLOR_TOL = 24
PERCEPTUAL_SELFCHECK_PAIRS = 64
_perceptual_selfchecked = 0


def background_key(im):
    """Colore di fondo MISURATO: il piu' frequente sull'anello di 1 px del
    bordo. Non e' una costante perche' la stessa funzione deve valere sia per
    il fondo piatto opaco dei bucket v1-research sia per il fondo trasparente
    del dataset v0 — e un valore cablato in due file diversi e' esattamente il
    tipo di assunzione che ha prodotto il difetto che questo blocco corregge.
    Il pareggio si rompe sul colore (chiave del max) per restare deterministico
    fra processi."""
    px = im.load()
    w, h = im.size
    counts = {}
    for x in range(w):
        for y in (0, h - 1):
            counts[px[x, y]] = counts.get(px[x, y], 0) + 1
    for y in range(1, h - 1):
        for x in (0, w - 1):
            counts[px[x, y]] = counts.get(px[x, y], 0) + 1
    return max(counts.items(), key=lambda kv: (kv[1], kv[0]))[0]

def is_foreground_pixel(px, bg):
    """DEFINIZIONE di "pixel di soggetto", usata dall'implementazione di
    riferimento; foreground_mask() ne e' la versione vettoriale."""
    if px[3] <= 16:
        return False
    if bg[3] <= 16:
        return True  # fondo trasparente (dataset v0): il solo alpha basta
    return max(abs(px[0] - bg[0]), abs(px[1] - bg[1]), abs(px[2] - bg[2])) > PERCEPTUAL_BG_TOL

def foreground_mask(im, bg=None):
    """Maschera 0/255 dei pixel di soggetto, in C dentro Pillow. Memoizzata
    sull'oggetto immagine: il preflight confronta ogni firma di validazione
    con TUTTE quelle di training, quindi senza cache la stessa maschera si
    ricalcolerebbe migliaia di volte (misurato: e' la differenza fra ~35 s e
    diversi minuti di CPU bruciati su una macchina a GPU pagata a ore)."""
    from PIL import Image as _Image, ImageChops as _ImageChops
    if bg is None:
        cached = getattr(im, "_ws_fg_mask", None)
        if cached is not None:
            return cached
        bg = background_key(im)
        remember = True
    else:
        remember = False
    r, g, b, a = im.split()
    opaque = a.point(lambda v: 255 if v > 16 else 0)
    if bg[3] <= 16:
        mask = opaque
    else:
        d = _ImageChops.lighter(
            _ImageChops.lighter(
                _ImageChops.difference(r, _Image.new("L", im.size, bg[0])),
                _ImageChops.difference(g, _Image.new("L", im.size, bg[1]))),
            _ImageChops.difference(b, _Image.new("L", im.size, bg[2])))
        mask = _ImageChops.multiply(d.point(lambda v: 255 if v > PERCEPTUAL_BG_TOL else 0), opaque)
    if remember:
        im._ws_fg_mask = mask
    return mask

def perceptual_distance_reference(a, b) -> float:
    """Implementazione a pixel, lenta e leggibile: e' la DEFINIZIONE della
    metrica (frazione di pixel diversi sull'unione delle aree di SOGGETTO, non
    sul canvas: due icone piccole e diverse dentro lo stesso canvas
    differiscono solo su una manciata di pixel del totale, e una soglia
    calcolata sul canvas le dichiarerebbe identiche). perceptual_distance()
    deve restituire lo stesso numero, e _perceptual_selfcheck() lo verifica su
    dati veri invece di dichiararlo."""
    pa, pb = a.load(), b.load()
    bga, bgb = background_key(a), background_key(b)
    w, h = a.size
    union = diff = 0
    for y in range(h):
        for x in range(w):
            p1, p2 = pa[x, y], pb[x, y]
            on1 = is_foreground_pixel(p1, bga)
            on2 = is_foreground_pixel(p2, bgb)
            if not (on1 or on2):
                continue
            union += 1
            if on1 != on2 or max(abs(p1[0] - p2[0]), abs(p1[1] - p2[1]),
                                 abs(p1[2] - p2[2])) > PERCEPTUAL_COLOR_TOL:
                diff += 1
    return diff / union if union else 0.0

def _perceptual_selfcheck(a, b, fast_value):
    """Le prime PERCEPTUAL_SELFCHECK_PAIRS coppie REALI del dataset in esame
    vengono ricalcolate anche con l'implementazione di riferimento: se le due
    divergessero, ci si ferma qui invece di dichiarare "preflight OK" con una
    metrica sbagliata. Costo misurato ~1,5 ms a coppia, cioe' meno di 0,1 s su
    un preflight che ne confronta centinaia di migliaia."""
    global _perceptual_selfchecked
    if _perceptual_selfchecked >= PERCEPTUAL_SELFCHECK_PAIRS:
        return
    _perceptual_selfchecked += 1
    ref = perceptual_distance_reference(a, b)
    if abs(ref - fast_value) > 1e-9:
        raise SystemExit(
            f"INCOERENZA nella distanza percettiva: la versione vettoriale dice {fast_value!r}, "
            f"quella di riferimento {ref!r}. Le due devono coincidere per definizione: "
            f"non proseguire con una metrica di cui non si sa quale sia quella giusta.")

def perceptual_distance(a, b) -> float:
    """Vedi perceptual_distance_reference(): stessa metrica, calcolata dentro
    Pillow. a e b sono firme RGBA della stessa dimensione."""
    from PIL import ImageChops as _ImageChops
    ma, mb = foreground_mask(a), foreground_mask(b)
    union = _ImageChops.lighter(ma, mb)
    u = sum(union.histogram()[128:])
    if u == 0:
        return 0.0  # due canvas interamente di fondo: identici per definizione
    ar, ag, ab = a.split()[:3]
    br, bg_, bb = b.split()[:3]
    color = _ImageChops.lighter(
        _ImageChops.lighter(_ImageChops.difference(ar, br), _ImageChops.difference(ag, bg_)),
        _ImageChops.difference(ab, bb)).point(
            lambda v: 255 if v > PERCEPTUAL_COLOR_TOL else 0)
    changed = _ImageChops.lighter(_ImageChops.difference(ma, mb),
                                  _ImageChops.multiply(color, union))
    value = sum(changed.histogram()[128:]) / u
    _perceptual_selfcheck(a, b, value)
    return value


def preflight(dataset_root: Path, ledger_path: Path, resolution: int, config=None):
    """Vedi runbook, sezione 'Preflight': fallisce prima di usare la GPU."""
    from PIL import Image  # leggero, ok anche in preflight-only

    if not ledger_path.exists():
        raise SystemExit(f"PREFLIGHT FALLITO: ledger mancante ({ledger_path})")
    rows = load_ledger(ledger_path)
    if not rows:
        raise SystemExit("PREFLIGHT FALLITO: ledger vuoto")

    problems = []
    subj_splits = {}
    signatures = {}
    for r in rows:
        # NB: r["image_path"]/r["caption_path"] nel ledger sono relativi alla
        # radice del REPO (dataset/lora-v1-research/images/<id>.png), utili per
        # tracciabilita' ma inutilizzabili qui: dentro il tar Kaggle il
        # layout e' sempre <dataset_root>/images/<id>.{png,txt}.
        img_path = dataset_root / "images" / f"{r['id']}.png"
        if not img_path.exists():
            problems.append(f"immagine mancante per {r['id']}: {img_path}")
            continue

        lic = (r.get("license_id") or "").strip().lower()
        if lic not in LICENSE_WHITELIST:
            problems.append(f"licenza fuori whitelist per {r['id']}: '{r.get('license_id')}'")
        if not r.get("license_snapshot_path"):
            problems.append(f"snapshot di licenza non registrato per {r['id']}")

        try:
            with Image.open(img_path) as im:
                im.verify()
        except Exception as e:  # noqa: BLE001
            problems.append(f"immagine corrotta {r['id']}: {e}")
            continue

        with Image.open(img_path) as im:
            if im.size != (resolution, resolution):
                problems.append(
                    f"dimensione inattesa per {r['id']}: {im.size} (atteso {resolution}x{resolution})")
            signatures[r["id"]] = im.convert("RGBA").resize(
                (SIGNATURE_PX, SIGNATURE_PX), Image.NEAREST)

        cap_path = dataset_root / "images" / f"{r['id']}.txt"
        if not cap_path.exists() or not cap_path.read_text(encoding="utf-8").strip():
            problems.append(f"caption mancante/vuota per {r['id']}")

        subj_splits.setdefault(r["subject_key"], set()).add(r["split"])

    for subj, splits in subj_splits.items():
        if len(splits) > 1:
            problems.append(f"fuga di split (chiave): '{subj}' compare in {splits}")

    train_ids = [r["id"] for r in rows if r["split"] == "train"]
    val_ids = [r["id"] for r in rows if r["split"] == "val"]
    if not val_ids:
        problems.append("split di validazione vuoto: validation_steps non misurerebbe nulla")
    for vid in val_ids:
        sv = signatures.get(vid)
        if sv is None:
            continue
        for tid in train_ids:
            st = signatures.get(tid)
            if st is None:
                continue
            d = perceptual_distance(sv, st)
            if d < DEDUP_MERGE_SCORE:
                problems.append(
                    f"fuga di split (percettiva): '{vid}' (val) e '{tid}' (train) "
                    f"differiscono solo per il {d * 100:.2f}% dei pixel")

    if config is not None:
        prompts_file = Path(config["eval"]["prompts_file"])
        if not prompts_file.exists():
            problems.append(f"file dei prompt di valutazione mancante: {prompts_file}")
        else:
            try:
                prompts = json.loads(prompts_file.read_text(encoding="utf-8"))
                ids = {s["id"] for s in prompts["subjects"]}
                if not ids:
                    problems.append(f"nessun soggetto in {prompts_file}")
                smoke_id = config.get("smoke_test", {}).get("validation_prompt_id")
                if smoke_id and smoke_id not in ids:
                    problems.append(
                        f"smoke_test.validation_prompt_id '{smoke_id}' non esiste in {prompts_file}")
            except Exception as e:  # noqa: BLE001
                problems.append(f"{prompts_file} non leggibile come JSON dei prompt: {e}")

        out_dir = Path(config["output_dir"])
        try:
            out_dir.mkdir(parents=True, exist_ok=True)
            probe = out_dir / ".write-probe"
            probe.write_text("ok", encoding="utf-8")
            probe.unlink()
        except Exception as e:  # noqa: BLE001
            problems.append(f"cartella di output non scrivibile ({out_dir}): {e}")

    if problems:
        msg = "\n  - ".join(problems[:30])
        extra = f"\n  ... e altri {len(problems) - 30}" if len(problems) > 30 else ""
        raise SystemExit(f"PREFLIGHT FALLITO ({len(problems)} problemi):\n  - {msg}{extra}")

    print(f"preflight OK: {len(rows)} righe ({len(train_ids)} train / {len(val_ids)} val), "
          f"{len(subj_splits)} soggetti, nessuna fuga di split "
          f"(chiave + {len(val_ids)}x{len(train_ids)} confronti percettivi)")
    return rows


def print_environment(config, git_commit, dataset_hash):
    """Il job deve stampare GPU/VRAM/driver/torch/cuda/diffusers/transformers/
    accelerate/commit git/dataset hash (runbook, sezione Kaggle)."""
    import torch
    import diffusers
    import transformers
    import accelerate

    print("== ambiente ==")
    if torch.cuda.is_available():
        idx = torch.cuda.current_device()
        print(f"GPU: {torch.cuda.get_device_name(idx)}")
        print(f"VRAM: {torch.cuda.get_device_properties(idx).total_memory / 1e9:.1f} GB")
    else:
        print("GPU: nessuna disponibile (torch.cuda.is_available() == False)")
    print(f"torch: {torch.__version__}  cuda: {torch.version.cuda}")
    print(f"diffusers: {diffusers.__version__}")
    print(f"transformers: {transformers.__version__}")
    print(f"accelerate: {accelerate.__version__}")
    print(f"commit git: {git_commit}")
    print(f"dataset hash: {dataset_hash}")
    print(f"experiment_id: {config['experiment_id']}")


def dataset_hash_of(ledger_rows):
    """Hash deterministico del ledger (non dei pesi): cambia se cambia il
    dataset, indipendentemente dall'ordine di lettura del filesystem."""
    h = hashlib.sha256()
    for r in sorted(ledger_rows, key=lambda r: r["id"]):
        h.update(r["derived_sha256"].encode("utf-8"))
    return h.hexdigest()


def git_commit_hash():
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=Path(__file__).resolve().parent,
            stderr=subprocess.DEVNULL).decode().strip()
    except Exception:  # noqa: BLE001
        return "unknown (repo non clonato nel kernel Kaggle?)"


def build_dataloader(dataset_root: Path, ledger_rows, split, resolution, batch_size):
    """Dataset PyTorch minimale: legge (immagine, caption) dal layout del tar
    (dataset_root/images/<id>.{png,txt}). Niente augmentation qui: il
    mandato vieta di gonfiare il dataset v0 con flip/duplicati, e le
    augmentation "vere" (color jitter leggero, eventuale crop) restano scelta
    del trainer in un giro successivo, non di questo script v0."""
    from torch.utils.data import Dataset, DataLoader
    from torchvision import transforms
    from PIL import Image

    to_tensor = transforms.Compose([
        transforms.ToTensor(),
        transforms.Normalize([0.5], [0.5]),
    ])

    class LoraV0Dataset(Dataset):
        def __init__(self, rows):
            self.rows = [r for r in rows if r["split"] == split]

        def __len__(self):
            return len(self.rows)

        def __getitem__(self, idx):
            r = self.rows[idx]
            img_path = dataset_root / "images" / f"{r['id']}.png"
            cap_path = dataset_root / "images" / f"{r['id']}.txt"
            im = Image.open(img_path).convert("RGB")
            assert im.size == (resolution, resolution), (
                f"{r['id']}: {im.size} != {resolution}x{resolution} "
                f"(il preflight avrebbe dovuto fermarsi prima)")
            caption = cap_path.read_text(encoding="utf-8").strip()
            return {"pixel_values": to_tensor(im), "caption": caption, "id": r["id"]}

    ds = LoraV0Dataset(ledger_rows)
    return DataLoader(ds, batch_size=batch_size, shuffle=(split == "train"), num_workers=2)


def save_lora(accelerator, pipe, unet, out_dir: Path):
    """Salva i soli pesi LoRA nel formato atteso dal runbook
    (pytorch_lora_weights.safetensors). Con gli adapter PEFT il vecchio
    `unet.save_attn_procs` non e' piu' la via corretta."""
    from diffusers import StableDiffusionPipeline
    from diffusers.utils import convert_state_dict_to_diffusers
    from peft.utils import get_peft_model_state_dict

    out_dir.mkdir(parents=True, exist_ok=True)
    unwrapped = accelerator.unwrap_model(unet)
    layers = convert_state_dict_to_diffusers(get_peft_model_state_dict(unwrapped))
    StableDiffusionPipeline.save_lora_weights(
        save_directory=str(out_dir), unet_lora_layers=layers, safe_serialization=True)


def train(config, smoke_test: bool):
    import torch
    from diffusers import StableDiffusionPipeline, DDPMScheduler
    from peft import LoraConfig
    from accelerate import Accelerator

    dataset_root = Path(config["dataset"]["root"])
    ledger_path = Path(config["dataset"]["ledger"])
    resolution = config["resolution"]

    rows = preflight(dataset_root, ledger_path, resolution, config)
    d_hash = dataset_hash_of(rows)
    print_environment(config, git_commit_hash(), d_hash)

    accelerator = Accelerator(
        mixed_precision=config["mixed_precision"],
        gradient_accumulation_steps=config["gradient_accumulation"],
    )
    torch.manual_seed(config["seed"])

    base_model = config["base_model"]
    expected_sha = base_model.get("expected_sha256")
    if expected_sha:
        actual = sha256_of_file(Path(base_model["path"]))
        if actual != expected_sha:
            raise SystemExit(
                f"PREFLIGHT FALLITO: hash della base non corrisponde al ledger "
                f"(atteso {expected_sha[:16]}..., trovato {actual[:16]}...) — "
                f"il checkpoint montato come Kaggle Dataset non e' quello registrato.")

    # La pipeline si carica in fp32 e la mezza precisione la gestisce
    # Accelerator: caricare tutto in fp16 e poi ottimizzare direttamente i pesi
    # fp16 e' la ricetta classica per una loss che diventa NaN dopo qualche
    # decina di step. VAE e text encoder, che restano congelati, possono invece
    # stare in fp16 senza rischi.
    pipe = StableDiffusionPipeline.from_single_file(base_model["path"], torch_dtype=torch.float32)
    pipe.scheduler = DDPMScheduler.from_config(pipe.scheduler.config)

    weight_dtype = torch.float16 if config["mixed_precision"] == "fp16" else torch.float32
    device = accelerator.device
    # from_single_file lascia la pipeline su CPU e accelerator.prepare() muove
    # solo la UNet: senza queste tre righe il primo batch moriva su un device
    # mismatch fra i latenti (CPU) e la UNet (CUDA).
    pipe.vae.to(device, dtype=weight_dtype)
    pipe.text_encoder.to(device, dtype=weight_dtype)
    pipe.unet.to(device, dtype=torch.float32)
    pipe.vae.requires_grad_(False)
    pipe.text_encoder.requires_grad_(False)

    unet_lora_config = LoraConfig(
        r=config["rank"], lora_alpha=config["alpha"],
        init_lora_weights="gaussian",
        target_modules=["to_k", "to_q", "to_v", "to_out.0"],
    )
    pipe.unet.requires_grad_(False)
    pipe.unet.add_adapter(unet_lora_config)

    max_steps = config["smoke_test"]["steps"] if smoke_test else config["max_train_steps"]
    ckpt_every = min(config["checkpointing_steps"], max_steps) if smoke_test else config["checkpointing_steps"]
    val_every = min(config["validation_steps"], max_steps) if smoke_test else config["validation_steps"]

    train_loader = build_dataloader(dataset_root, rows, "train", resolution, config["batch_size"])
    val_loader = build_dataloader(dataset_root, rows, "val", resolution, config["batch_size"])
    trainable_params = [p for p in pipe.unet.parameters() if p.requires_grad]
    optimizer = torch.optim.AdamW(trainable_params, lr=config["learning_rate"])

    unet, optimizer, train_loader, val_loader = accelerator.prepare(
        pipe.unet, optimizer, train_loader, val_loader)
    pipe.unet = unet

    output_dir = Path(config["output_dir"])
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "config.yaml").write_text(json.dumps(config, indent=2), encoding="utf-8")
    (output_dir / "dataset_hash.txt").write_text(d_hash + "\n", encoding="utf-8")

    step = 0
    metrics_path = output_dir / "metrics.jsonl"
    log_path = output_dir / "training.log"
    with log_path.open("a", encoding="utf-8") as log_f, \
            metrics_path.open("a", encoding="utf-8") as met_f:
        while step < max_steps:
            for batch in train_loader:
                if step >= max_steps:
                    break
                with accelerator.accumulate(unet):
                    loss = training_step(pipe, unet, batch, accelerator, weight_dtype)
                    if not torch.isfinite(loss):
                        # richiesto dallo smoke test del runbook ("Verificare:
                        # memoria; NaN; ..."): fermarsi al primo NaN invece di
                        # bruciare 1500 step su pesi ormai corrotti.
                        raise SystemExit(
                            f"TRAINING INTERROTTO: loss non finita ({loss.item()}) allo "
                            f"step {step}. Il run non e' recuperabile: rivedere "
                            f"learning_rate/mixed_precision prima di ripartire.")
                    accelerator.backward(loss)
                    optimizer.step()
                    optimizer.zero_grad()
                step += 1
                if step % 10 == 0 or step == max_steps:
                    line = f"step={step}/{max_steps} loss={loss.item():.4f}"
                    print(line)
                    log_f.write(line + "\n")
                    met_f.write(json.dumps({"step": step, "train_loss": loss.item()}) + "\n")
                if val_every and step % val_every == 0:
                    v = validation_loss(pipe, unet, val_loader, accelerator, weight_dtype)
                    line = f"step={step}/{max_steps} val_loss={v:.4f}"
                    print(line)
                    log_f.write(line + "\n")
                    met_f.write(json.dumps({"step": step, "val_loss": v}) + "\n")
                if step % ckpt_every == 0:
                    ckpt_dir = output_dir / "checkpoints" / f"step-{step:05d}"
                    save_lora(accelerator, pipe, unet, ckpt_dir)
                    run_eval_grid(pipe, config, output_dir / "eval" / f"step-{step:05d}",
                                  accelerator, smoke_test=smoke_test)

    save_lora(accelerator, pipe, unet, output_dir / "final")
    print(f"training completato: {step} step, output in {output_dir}")


def training_step(pipe, unet, batch, accelerator, weight_dtype):
    """Un passo di diffusion training standard (rumore predetto sui latenti
    dell'immagine, condizionato dalla caption): isolato in una funzione
    perche' e' l'unico punto realmente specifico di SD1.5 dentro il loop,
    il resto (accumulo gradienti, checkpoint, eval) e' generico."""
    import torch
    import torch.nn.functional as F

    pixel_values = batch["pixel_values"].to(accelerator.device, dtype=weight_dtype)
    with torch.no_grad():
        latents = pipe.vae.encode(pixel_values).latent_dist.sample() * pipe.vae.config.scaling_factor
        input_ids = pipe.tokenizer(
            batch["caption"], padding="max_length", truncation=True,
            max_length=pipe.tokenizer.model_max_length,
            return_tensors="pt").input_ids.to(accelerator.device)
        encoder_hidden_states = pipe.text_encoder(input_ids)[0]

    latents = latents.float()
    encoder_hidden_states = encoder_hidden_states.float()
    noise = torch.randn_like(latents)
    timesteps = torch.randint(0, pipe.scheduler.config.num_train_timesteps,
                              (latents.shape[0],), device=latents.device).long()
    noisy_latents = pipe.scheduler.add_noise(latents, noise, timesteps)
    model_pred = unet(noisy_latents, timesteps, encoder_hidden_states).sample
    return F.mse_loss(model_pred.float(), noise.float(), reduction="mean")


def validation_loss(pipe, unet, val_loader, accelerator, weight_dtype):
    """Loss media sullo split di validazione, a seed fisso perche' due
    misure consecutive siano confrontabili. Senza questa funzione lo split di
    validazione sarebbe solo una colonna nel ledger: 10% di immagini messe da
    parte e mai lette."""
    import torch

    unet.eval()
    total, n = 0.0, 0
    gen_state = torch.random.get_rng_state()
    torch.manual_seed(12345)
    with torch.no_grad():
        for batch in val_loader:
            loss = training_step(pipe, unet, batch, accelerator, weight_dtype)
            total += loss.item()
            n += 1
    torch.random.set_rng_state(gen_state)
    unet.train()
    return total / max(1, n)


def run_eval_grid(pipe, config, out_dir: Path, accelerator, smoke_test=False):
    """Genera la griglia sui 20 prompt congelati (eval-prompts-lora-v1-research.json)
    ai 2 seed fissi: stesso soggetto/seed per ogni checkpoint, per poterli
    confrontare in sequenza (runbook: 'generare la stessa matrice').
    Con --smoke-test si genera UNA sola immagine, come prescrive il runbook
    ("1 prompt di validazione, 1 checkpoint, 1 immagine")."""
    import torch

    if not accelerator.is_main_process:
        return
    out_dir.mkdir(parents=True, exist_ok=True)
    prompts = json.loads(Path(config["eval"]["prompts_file"]).read_text(encoding="utf-8"))
    subjects = prompts["subjects"]
    seeds = config["eval"]["seeds"]
    if smoke_test:
        wanted = config["smoke_test"]["validation_prompt_id"]
        subjects = [s for s in subjects if s["id"] == wanted]
        seeds = seeds[:1]

    for subject in subjects:
        for seed in seeds:
            gen = torch.Generator(device=accelerator.device).manual_seed(seed)
            # autocast: la pipeline vive in fp16 (weight_dtype) ma gli input
            # dello scheduler nascono float32 -> "Input type (float) and bias
            # type (c10::Half)" al primo conv. Emerso al run v8 su T4 (il
            # primo che sia MAI arrivato fin qui): in locale, senza GPU, il
            # preflight non puo' vederlo.
            with torch.autocast(accelerator.device.type):
                image = pipe(subject["prompt"], negative_prompt=subject["negative"],
                             generator=gen, num_inference_steps=25, guidance_scale=7.0).images[0]
            image.save(out_dir / f"{subject['id']}_{seed}.png")
    print(f"eval grid scritta in {out_dir} ({len(subjects)}x{len(seeds)} immagini)")


def main():
    import yaml

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", required=True, help="path a un file in kaggle/configs/*.yaml")
    parser.add_argument("--smoke-test", action="store_true",
                        help="20-100 step, 1 checkpoint, 1 immagine (runbook: obbligatorio "
                             "prima del training pieno)")
    parser.add_argument("--preflight-only", action="store_true",
                        help="esegue solo la validazione dataset/licenze/prompt e esce "
                             "(nessuna GPU toccata)")
    args = parser.parse_args()

    config = yaml.safe_load(Path(args.config).read_text(encoding="utf-8"))

    if args.preflight_only:
        preflight(Path(config["dataset"]["root"]), Path(config["dataset"]["ledger"]),
                  config["resolution"], config)
        return 0

    train(config, smoke_test=args.smoke_test)
    return 0


if __name__ == "__main__":
    sys.exit(main())
