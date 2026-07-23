#!/usr/bin/env python3
"""Benchmark di Stable Audio 3 Small (DEC-109/DEC-113) -- PRIMO benchmark audio
eseguito con successo, 23/07/2026, contro i due checkpoint scaricati in
models/stable-audio-3-small-sfx/ e models/stable-audio-3-small-music/ (pesi
model.safetensors + model_config.json + text-encoder t5gemma-b-b-ul2/, non
versionati). Vedi docs/plans/active/model-comparison.md, sezione audio, per lo
storico dei blocchi precedenti (stable-audio-tools su Python 3.14, gate HF su
stable-audio-open-small) -- questo script usa una libreria diversa
(`stable-audio-3`, installata via pip da git perche' non e' su PyPI, vedi
scripts/audio-benchmark.sh) e un modello diverso: non ha ereditato quei blocchi.

Uso:
    ~/venvs/stable-audio/bin/python3 scripts/audio_benchmark.py <out-dir>

Genera 6 prompt SFX x 2 seed (durata 4s, models/stable-audio-3-small-sfx) e
4 prompt musica x 2 seed (durata 20s, models/stable-audio-3-small-music),
steps=8/cfg_scale=1.0/sampler_type="pingpong" (la ricetta della model card;
"pingpong" e' anche il default di libreria per i modelli rf_denoiser come
questo, vedi stable_audio_3.inference.sampling.sample_diffusion). Salva i
.wav in <out-dir>/{sfx,music}/<slug>-seed<seed>.wav, scarta la prima clip di
ogni variante come warmup (paga il primo giro a freddo di thread pool/cache,
non e' rappresentativa del regime), scrive <out-dir>/report.md con la
tabella dei tempi e <out-dir>/index.json con i dati grezzi.

CARICAMENTO DAI FILE LOCALI: la libreria `stable_audio_3.StableAudioModel.
from_pretrained(model_name)` accetta solo nomi noti ("small-sfx", "small-
music", ...) e li risolve via hf_hub_download (rete, e i repo sono gated).
Qui NON la usiamo: load_local_model() replica la stessa costruzione con
stable_audio_3.loading_utils.load_diffusion_cond usando direttamente i path
locali di model_config.json/model.safetensors, e riscrive in memoria il
"repo_id" del conditioner T5Gemma con il path assoluto della cartella locale
-- transformers (AutoTokenizer/AutoConfig/T5GemmaEncoderModel.from_pretrained)
risolve un repo_id che e' anche una directory esistente come percorso locale
+ subfolder, senza toccare la rete. Con HF_HUB_OFFLINE=1/TRANSFORMERS_OFFLINE=1
qualunque tentativo residuo di rete fallisce subito con un errore leggibile
invece di un ri-download silenzioso.

NON e' collegato a nessun binario C (AGENTS.md: il gioco non linka mai un
runtime Python) -- resta un prototipo di misura, isolato in
~/venvs/stable-audio.
"""

import json
import os
import resource
import sys
import time
from pathlib import Path

# Vanno impostate PRIMA di importare transformers/huggingface_hub (lette
# a import-time in alcuni percorsi interni): qualunque tentativo di rete deve
# fallire in modo leggibile, mai ripiegare su un ri-download silenzioso.
os.environ.setdefault("HF_HUB_OFFLINE", "1")
os.environ.setdefault("TRANSFORMERS_OFFLINE", "1")
os.environ.setdefault("HF_HUB_DISABLE_PROGRESS_BARS", "1")

REPO_ROOT = Path(__file__).resolve().parent.parent

STEPS = 8
CFG_SCALE = 1.0
SAMPLER_TYPE = "pingpong"   # default di libreria per i modelli rf_denoiser (questi lo sono), esplicitato per documentare la ricetta
SEEDS = [4242, 4343]

# (slug, prompt) -- prompt normalizzati dal testo del task sostituendo " - " con ", "
SFX_MODEL_DIR = REPO_ROOT / "models" / "stable-audio-3-small-sfx"
SFX_DURATION = 4.0
SFX_PROMPTS = [
    ("sfx-fusion-completed", "fusion completed, metallic pour and sizzle"),
    ("sfx-damage-taken", "player takes damage, dull metallic hit"),
    ("sfx-room-cleared", "room cleared, short triumphant chime"),
    ("sfx-item-pickup", "item pickup, bright metallic tick"),
    ("sfx-boss-phase", "boss enters new phase, deep ominous impact"),
    ("sfx-bomb-explosion", "bomb explosion, stylized blast"),
]

MUSIC_MODEL_DIR = REPO_ROOT / "models" / "stable-audio-3-small-music"
MUSIC_DURATION = 20.0
MUSIC_PROMPTS = [
    ("music-dark-fantasy-dungeon", "dark fantasy dungeon theme, ominous and rhythmic"),
    ("music-crucible-hub", "the Crucible hub, warm forge ambience with distant hammering"),
    ("music-cavern-ambience", "cavern ambience with dripping water and low drones"),
    ("music-final-floor-theme", "final floor theme, intense dark orchestral loop"),
]


def load_local_model(model_dir: Path, device: str = "cpu"):
    """Costruisce uno StableAudioModel dai file locali di model_dir, senza
    passare da hf_hub_download (repo gated + rete). Ritorna (modello, secondi
    di caricamento)."""
    from stable_audio_3.loading_utils import load_diffusion_cond
    from stable_audio_3.model import StableAudioModel

    config_path = model_dir / "model_config.json"
    ckpt_path = model_dir / "model.safetensors"
    if not config_path.exists() or not ckpt_path.exists():
        raise FileNotFoundError(
            f"pesi mancanti in {model_dir} (attesi model_config.json + model.safetensors)"
        )

    with open(config_path) as f:
        model_config = json.load(f)

    abs_dir = str(model_dir.resolve())
    patched = 0
    for cond_cfg in model_config.get("model", {}).get("conditioning", {}).get("configs", []):
        if cond_cfg.get("type") == "t5gemma":
            cond_cfg["config"]["repo_id"] = abs_dir  # dir locale: transformers risolve repo_id+subfolder come path, zero rete
            patched += 1
    if patched == 0:
        print(
            f"ATTENZIONE: nessun conditioner t5gemma in {config_path.name} -- "
            "lo schema del model_config e' cambiato rispetto a quanto verificato il 23/07/2026",
            file=sys.stderr,
        )

    t0 = time.time()
    model = load_diffusion_cond(model_config, str(ckpt_path), device=device, model_half=False)
    model.use_lora = False
    model.lora_names = []
    sa_model = StableAudioModel(model, model_config, device, False)
    return sa_model, time.time() - t0


def run_variant(sa_model, prompts, duration, out_dir, kind):
    """Genera tutte le clip di una variante (sfx o music). La primissima clip
    e' il warmup (thread pool / cache a freddo, non rappresentativa) ed e'
    marcata ma non esclusa dal file -- il report.md la separa nella media."""
    import torch
    import torchaudio

    out_dir.mkdir(parents=True, exist_ok=True)
    sample_rate = sa_model.model.sample_rate
    clips = []
    first = True
    for slug, prompt in prompts:
        for seed in SEEDS:
            t0 = time.time()
            audio = sa_model.generate(
                prompt=prompt,
                duration=duration,
                steps=STEPS,
                cfg_scale=CFG_SCALE,
                sampler_type=SAMPLER_TYPE,
                seed=seed,
                disable_tqdm=True,
            )
            gen_secs = time.time() - t0

            wav = audio[0].to(torch.float32).clamp(-1, 1).mul(32767).to(torch.int16).cpu()
            wav_path = out_dir / f"{slug}-seed{seed}.wav"
            torchaudio.save(str(wav_path), wav, sample_rate)

            peak_rss_mib = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024.0
            tag = " [warmup]" if first else ""
            print(f"   [{kind}] {slug} seed={seed}: {gen_secs:.1f}s (durata {duration:.1f}s, "
                  f"rapporto {gen_secs / duration:.2f}x){tag} -> {wav_path.name}")
            clips.append({
                "slug": slug, "prompt": prompt, "kind": kind, "seed": seed,
                "durationSec": duration, "genSecs": gen_secs,
                "ratioGenOverDuration": gen_secs / duration,
                "peakRssMiB": peak_rss_mib, "warmup": first, "wav": wav_path.name,
            })
            first = False
    return clips


def write_report(out_dir: Path, sections):
    """sections: lista di (titolo, model_dir, load_secs, clips)."""
    lines = [
        "# Benchmark audio -- Stable Audio 3 Small (DEC-109/DEC-113)",
        "",
        f"Generato: {time.strftime('%Y-%m-%d %H:%M:%S')}",
        f"Ricetta: steps={STEPS}, cfg_scale={CFG_SCALE}, sampler_type=\"{SAMPLER_TYPE}\", device=cpu (float32)",
        "Caricamento: pesi letti direttamente da models/stable-audio-3-small-{sfx,music}/ "
        "(model_config.json + model.safetensors + text-encoder t5gemma-b-b-ul2/), "
        "nessuna chiamata di rete (HF_HUB_OFFLINE=1/TRANSFORMERS_OFFLINE=1).",
        "",
    ]
    for title, model_dir, load_secs, clips in sections:
        regime = [c for c in clips if not c["warmup"]]
        warmup = [c for c in clips if c["warmup"]]
        lines.append(f"## {title}")
        lines.append("")
        lines.append(f"- Modello: `{model_dir}`")
        lines.append(f"- Caricamento: {load_secs:.1f}s")
        if warmup:
            w = warmup[0]
            lines.append(f"- Warmup (prima clip, scartata dalla media): {w['slug']} seed={w['seed']} "
                          f"-> {w['genSecs']:.1f}s ({w['ratioGenOverDuration']:.2f}x la durata)")
        if regime:
            avg_secs = sum(c["genSecs"] for c in regime) / len(regime)
            avg_ratio = sum(c["ratioGenOverDuration"] for c in regime) / len(regime)
            peak = max(c["peakRssMiB"] for c in clips)
            lines.append(f"- A regime ({len(regime)} clip, warmup escluso): {avg_secs:.1f}s/clip medio, "
                          f"rapporto medio {avg_ratio:.2f}x la durata richiesta")
            lines.append(f"- RAM di picco (resource.getrusage, processo intero): {peak:.0f} MiB")
        lines.append("")
        lines.append("| prompt | seed | durata (s) | tempo gen. (s) | rapporto | warmup | file |")
        lines.append("|---|---|---|---|---|---|---|")
        for c in clips:
            lines.append(
                f"| {c['prompt']} | {c['seed']} | {c['durationSec']:.1f} | {c['genSecs']:.1f} | "
                f"{c['ratioGenOverDuration']:.2f}x | {'si' if c['warmup'] else ''} | `{c['wav']}` |"
            )
        lines.append("")
    (out_dir / "report.md").write_text("\n".join(lines))


def main():
    if len(sys.argv) < 2:
        print("uso: audio_benchmark.py <out-dir>", file=sys.stderr)
        return 1
    out_dir = Path(sys.argv[1])
    out_dir.mkdir(parents=True, exist_ok=True)

    # Import ritardato apposta: se torch/stable-audio-3 non sono installati,
    # l'errore va riportato qui in modo chiaro, non con uno stack trace
    # all'avvio del file -- vedi scripts/audio-benchmark.sh per i controlli
    # di precondizione prima di arrivare qui.
    try:
        import torch  # noqa: F401
        import torchaudio  # noqa: F401
        import stable_audio_3  # noqa: F401
    except ImportError as e:
        print(f"audio_benchmark: import fallito ({e}) -- vedi scripts/audio-benchmark.sh "
              "per come preparare il venv", file=sys.stderr)
        return 1

    sections = []
    index = {"steps": STEPS, "cfgScale": CFG_SCALE, "samplerType": SAMPLER_TYPE, "device": "cpu", "sections": []}

    print(f"== carico {SFX_MODEL_DIR} (locale, nessuna rete) ==")
    sfx_model, sfx_load_secs = load_local_model(SFX_MODEL_DIR, device="cpu")
    print(f"   caricato in {sfx_load_secs:.1f}s (sample_rate={sfx_model.model.sample_rate})")
    sfx_clips = run_variant(sfx_model, SFX_PROMPTS, SFX_DURATION, out_dir / "sfx", "sfx")
    sections.append(("SFX (models/stable-audio-3-small-sfx)", SFX_MODEL_DIR, sfx_load_secs, sfx_clips))
    index["sections"].append({"kind": "sfx", "modelDir": str(SFX_MODEL_DIR), "loadSecs": sfx_load_secs, "clips": sfx_clips})
    del sfx_model  # libera prima di caricare music: mai due modelli in RAM insieme se evitabile

    print(f"== carico {MUSIC_MODEL_DIR} (locale, nessuna rete) ==")
    music_model, music_load_secs = load_local_model(MUSIC_MODEL_DIR, device="cpu")
    print(f"   caricato in {music_load_secs:.1f}s (sample_rate={music_model.model.sample_rate})")
    music_clips = run_variant(music_model, MUSIC_PROMPTS, MUSIC_DURATION, out_dir / "music", "music")
    sections.append(("Music (models/stable-audio-3-small-music)", MUSIC_MODEL_DIR, music_load_secs, music_clips))
    index["sections"].append({"kind": "music", "modelDir": str(MUSIC_MODEL_DIR), "loadSecs": music_load_secs, "clips": music_clips})
    del music_model

    write_report(out_dir, sections)
    (out_dir / "index.json").write_text(json.dumps(index, indent=2))

    all_clips = sfx_clips + music_clips
    regime = [c for c in all_clips if not c["warmup"]]
    avg = sum(c["genSecs"] for c in regime) / len(regime) if regime else 0.0
    print(f"== fatto: {len(all_clips)} clip ({len(regime)} a regime, {avg:.1f}s/clip medio) -- "
          f"report in {out_dir / 'report.md'} ==")
    return 0


if __name__ == "__main__":
    sys.exit(main())
