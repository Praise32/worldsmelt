#!/usr/bin/env python3
"""Produzione del pacchetto audio pre-generato della demo (DEC-172).

Genera l'intero pacchetto musica/ambience/SFX di assets/audio/ da una lista
DICHIARATIVA di spec (SPECS, in fondo alle costanti: id, prompt, durata, seed
fisso, categoria, loop) con i due checkpoint locali di Stable Audio 3 Small
(models/stable-audio-3-small-{music,sfx}). Riusa il percorso di codice del
benchmark verificato il 23/07/2026 (scripts/audio_benchmark.py:
load_local_model, caricamento 100% offline dai file locali) e la stessa
ricetta della model card: steps=8, cfg_scale=1.0, sampler_type="pingpong",
device=cpu. Nota: rFXGen NON e' presente ne' nel repo ne' nell'ambiente,
quindi anche gli SFX passano dal checkpoint sfx (fallback previsto da
DEC-172).

Uso:
    ~/venvs/stable-audio/bin/python3 scripts/audio-pack.py [--out DIR]
        [--log-dir DIR] [--only id1,id2,...]

    --out      cartella del pacchetto (default: assets/audio)
    --log-dir  cartella per i .wav intermedi e i log (default:
               logs/audio-pack/<timestamp>)
    --only     rigenera solo gli id elencati; il manifest.json esistente
               viene aggiornato per-voce, le altre voci restano intatte
               (utile per la curation: rigenerare una singola traccia
               cambiando seed/prompt nella SPECS)

Post-processing per traccia (pipeline di 16-AUDIO-GENERATION-PIPELINE.md,
sezione post-processing, ridotta al necessario per il pacchetto demo):
  - SFX: trim del silenzio in testa/coda, fade 5ms/25ms, normalizzazione di
    picco a -1 dBFS (durata finale sempre < 2s: la generazione e' <= 1.8s);
  - musica/ambience: crossfade equal-power di 1.5s coda->testa per un loop
    senza click, normalizzazione di picco a -1 dBFS.

Validazione minima (silenzio via RMS, durata attesa): una traccia che fallisce
viene rigenerata UNA volta con seed+1 (policy della run di produzione); se
fallisce ancora resta nel pacchetto marcata "validation": "failed" nel
manifest -- la curation estetica la fa il proprietario, non questo script.

Output in --out: {music,sfx}/<id>.ogg (44.1 kHz, libvorbis via ffmpeg) e
manifest.json (id -> file, categoria, loop, durata reale, seed e prompt).

NON e' collegato a nessun binario C (AGENTS.md: il gioco non linka mai un
runtime Python): e' uno strumento di PRODUZIONE offline, il motore legge
solo gli asset statici risultanti (DEC-172).
"""

import argparse
import json
import math
import shutil
import subprocess
import sys
import time
from pathlib import Path

# audio_benchmark imposta HF_HUB_OFFLINE/TRANSFORMERS_OFFLINE a import-time
# (prima di transformers): importarlo per primo conserva quella garanzia.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from audio_benchmark import load_local_model  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent

# Ricetta della model card, identica al benchmark del 23/07/2026 (0.68x
# realtime su CPU per la musica) -- costanti locali, non importate dal
# benchmark: se quello cambiasse, il pacchetto non deve cambiare in silenzio.
STEPS = 8
CFG_SCALE = 1.0
SAMPLER_TYPE = "pingpong"
DEVICE = "cpu"

SAMPLE_RATE = 44100          # nativo di entrambi i checkpoint (model_config.json)
PEAK_TARGET_DB = -1.0        # normalizzazione di picco, headroom anti-clip
LOOP_XFADE_SECS = 1.5        # crossfade coda->testa per i loop musicali
SFX_FADE_IN_SECS = 0.005
SFX_FADE_OUT_SECS = 0.025
TRIM_THRESHOLD_DB = -50.0    # sotto questa RMS di finestra e' "silenzio"
TRIM_HOP_SECS = 0.02
TRIM_TAIL_KEEP_SECS = 0.06   # coda naturale conservata dopo l'ultima finestra attiva
SILENCE_RMS_DB = -45.0       # RMS complessiva sotto cui la traccia e' da rifare
SFX_MIN_SECS = 0.15          # un SFX piu' corto di cosi' dopo il trim e' un fallimento
MUSIC_DURATION_TOL_SECS = 1.0

MUSIC_MODEL_DIR = REPO_ROOT / "models" / "stable-audio-3-small-music"
SFX_MODEL_DIR = REPO_ROOT / "models" / "stable-audio-3-small-sfx"
MUSIC_OGG_QUALITY = 4        # libvorbis -q:a ~128 kbps: 6 tracce ~ 8 MB, budget < 25 MB
SFX_OGG_QUALITY = 5          # gli SFX sono minuscoli, qualita' in piu' per i transienti

# ---------------------------------------------------------------------------
# SPECS: la lista dichiarativa del pacchetto. Tono del progetto:
# ironico-leggero, pixel-fantasy (cornice del crogiolo, DEC-078). I prompt
# musicali di gameplay_1/gameplay_2 condividono la stessa palette e si
# differenziano solo per intensita': e' l'asse audio dell'escalation leggibile
# del tema (DEC-024) -- intensificazione dentro la stessa grammatica, mai
# rumore. floor_zero riprende la famiglia sonora del crogiolo gia' apprezzata
# nel benchmark ("warm forge ambience with distant hammering"). Gli SFX
# formano famiglie coerenti (UI "soft digital retro", impatti percussivi,
# fusion_complete estende il prompt di benchmark che funzionava) come chiede
# la grammatica di audio-and-feedback.md.
# Campi: id, category (music|ambience|sfx), loop, duration (secondi di
# GENERAZIONE; per gli SFX la durata finale scende col trim, per la musica
# scende di LOOP_XFADE_SECS col crossfade), seed (fisso: riproducibilita'),
# prompt.
# ---------------------------------------------------------------------------
SPECS = [
    # --- musica / ambience (60-90s, pensati per il loop) ---
    dict(id="main_menu", category="music", loop=True, duration=72.0, seed=7101,
         prompt="playful medieval fantasy title theme, warm inviting melody, gentle flutes "
                "and plucked strings, lighthearted retro videogame feel, slightly whimsical"),
    dict(id="floor_zero", category="ambience", loop=True, duration=84.0, seed=7102,
         prompt="calm magical forge ambience, soft crackling embers, distant gentle hammering, "
                "low warm drone, molten metal simmering, peaceful and mysterious"),
    dict(id="gameplay_1", category="music", loop=True, duration=78.0, seed=7103,
         prompt="adventurous dungeon exploration theme, medieval fantasy with playful retro "
                "flair, steady relaxed tempo, plucked strings and light hand percussion, "
                "curious and lighthearted"),
    dict(id="gameplay_2", category="music", loop=True, duration=78.0, seed=7104,
         prompt="adventurous dungeon exploration theme, medieval fantasy with playful retro "
                "flair, faster driving tempo, plucked strings joined by bold drums and brass "
                "accents, urgent but melodic and clearly listenable"),
    dict(id="boss", category="music", loop=True, duration=72.0, seed=7105,
         prompt="epic boss battle theme with a mischievous edge, medieval fantasy, pounding "
                "drums, heroic brass, fast strings, dramatic yet playful retro videogame energy"),
    # 62s e non 60: il crossfade di loop toglie LOOP_XFADE_SECS, la durata
    # finale deve restare dentro la banda 60-90s del pacchetto.
    dict(id="results", category="music", loop=True, duration=62.0, seed=7106,
         prompt="gentle victorious results screen theme, warm reflective melody, soft chimes "
                "and strings, satisfied and calm, playful medieval fantasy touch"),

    # --- SFX (< 2s; generati <= 1.8s, il trim accorcia ancora) ---
    dict(id="shot_base", category="sfx", loop=False, duration=0.8, seed=7201,
         prompt="short magic bolt shot, bright synthetic zap, retro videogame feel, clean and punchy"),
    dict(id="hit_enemy", category="sfx", loop=False, duration=0.8, seed=7202,
         prompt="short impact hit on enemy, thumpy percussive smack, retro videogame character"),
    dict(id="hit_player", category="sfx", loop=False, duration=1.0, seed=7203,
         prompt="player takes damage, dull heavy thud with a low warning tone, retro videogame"),
    dict(id="pickup_item", category="sfx", loop=False, duration=0.9, seed=7204,
         prompt="item pickup, bright cheerful metallic tick, short ascending sparkle, retro videogame"),
    dict(id="door_open", category="sfx", loop=False, duration=1.5, seed=7205,
         prompt="heavy wooden dungeon door opening, short creak and stone scrape"),
    dict(id="fusion_complete", category="sfx", loop=False, duration=1.8, seed=7206,
         prompt="fusion completed, metallic pour and sizzle resolving into a bright triumphant chime"),
    dict(id="discovery_card", category="sfx", loop=False, duration=1.4, seed=7207,
         prompt="discovery card revealed, magical paper flourish with soft shimmering chime, playful"),
    dict(id="ui_move", category="sfx", loop=False, duration=0.8, seed=7208,
         prompt="UI cursor move, tiny soft digital blip, retro menu tick"),
    dict(id="ui_confirm", category="sfx", loop=False, duration=0.9, seed=7209,
         prompt="UI confirm, soft digital chime, two rising notes, pleasant retro menu sound"),
    dict(id="ui_cancel", category="sfx", loop=False, duration=0.9, seed=7210,
         prompt="UI cancel, soft digital blip, two descending notes, muted retro menu sound"),
]


# --------------------------- post-processing -------------------------------

def peak_db(x):
    import numpy as np
    peak = float(np.max(np.abs(x))) if x.size else 0.0
    return 20.0 * math.log10(peak) if peak > 0 else -120.0


def rms_db(x):
    import numpy as np
    if not x.size:
        return -120.0
    rms = float(np.sqrt(np.mean(np.square(x, dtype="float64"))))
    return 20.0 * math.log10(rms) if rms > 0 else -120.0


def peak_normalize(x, target_db=PEAK_TARGET_DB):
    import numpy as np
    peak = float(np.max(np.abs(x))) if x.size else 0.0
    if peak <= 0:
        return x
    return x * (10.0 ** (target_db / 20.0) / peak)


def trim_silence(x, sr):
    """Trim del silenzio in testa e in coda di un SFX: RMS per finestre di
    TRIM_HOP_SECS sul mix mono; si conserva un piccolo pre-attacco e
    TRIM_TAIL_KEEP_SECS di coda naturale dopo l'ultima finestra attiva."""
    import numpy as np
    hop = max(1, int(sr * TRIM_HOP_SECS))
    mono = np.mean(x, axis=0)
    n_win = max(1, mono.shape[0] // hop)
    active = []
    for i in range(n_win):
        win = mono[i * hop:(i + 1) * hop]
        if rms_db(win) > TRIM_THRESHOLD_DB:
            active.append(i)
    if not active:
        return x  # tutto silenzio: lasciamo decidere alla validazione
    start = max(0, active[0] * hop - int(sr * 0.01))
    end = min(mono.shape[0], (active[-1] + 1) * hop + int(sr * TRIM_TAIL_KEEP_SECS))
    return x[:, start:end]


def apply_fades(x, sr, fade_in_secs, fade_out_secs):
    import numpy as np
    n = x.shape[1]
    fi = min(int(sr * fade_in_secs), n // 2)
    fo = min(int(sr * fade_out_secs), n // 2)
    if fi > 0:
        x[:, :fi] *= np.linspace(0.0, 1.0, fi, dtype="float32")
    if fo > 0:
        x[:, -fo:] *= np.linspace(1.0, 0.0, fo, dtype="float32")
    return x


def loop_crossfade(x, sr, xfade_secs=LOOP_XFADE_SECS):
    """Crossfade equal-power coda->testa: la traccia si accorcia di
    xfade_secs e il punto di loop (fine -> inizio) resta senza click."""
    import numpy as np
    n = x.shape[1]
    xf = int(sr * xfade_secs)
    if xf <= 0 or n <= 2 * xf:
        return x
    t = np.linspace(0.0, 1.0, xf, dtype="float32")
    fade_in = np.sqrt(t)
    fade_out = np.sqrt(1.0 - t)
    body = x[:, :n - xf].copy()
    tail = x[:, n - xf:]
    body[:, :xf] = body[:, :xf] * fade_in + tail * fade_out
    return body


def postprocess(x, sr, spec):
    """Ritorna l'audio post-processato secondo la categoria della spec."""
    import numpy as np
    x = np.asarray(x, dtype="float32")
    want = int(spec["duration"] * sr)
    if x.shape[1] > want:  # difensivo: mai piu' lungo del richiesto
        x = x[:, :want]
    if spec["category"] == "sfx":
        x = trim_silence(x, sr)
        x = apply_fades(x.copy(), sr, SFX_FADE_IN_SECS, SFX_FADE_OUT_SECS)
    elif spec["loop"]:
        x = loop_crossfade(x, sr)
    return peak_normalize(x)


def validate(x, sr, spec):
    """Ritorna la lista dei problemi (vuota = traccia valida)."""
    problems = []
    dur = x.shape[1] / sr
    level = rms_db(x)
    if level < SILENCE_RMS_DB:
        problems.append(f"quasi silenzio (RMS {level:.1f} dBFS < {SILENCE_RMS_DB})")
    if spec["category"] == "sfx":
        if dur < SFX_MIN_SECS:
            problems.append(f"troppo corto dopo il trim ({dur:.2f}s < {SFX_MIN_SECS}s)")
        if dur >= 2.0:
            problems.append(f"SFX non sotto i 2s ({dur:.2f}s)")
    else:
        expected = spec["duration"] - (LOOP_XFADE_SECS if spec["loop"] else 0.0)
        if abs(dur - expected) > MUSIC_DURATION_TOL_SECS:
            problems.append(f"durata inattesa ({dur:.1f}s, attesi ~{expected:.1f}s)")
    return problems


# ------------------------------ generazione --------------------------------

def generate_raw(sa_model, spec, seed):
    """Una singola generazione col modello gia' caricato; ritorna
    (audio numpy float32 (canali, campioni), secondi di generazione)."""
    import torch
    t0 = time.time()
    audio = sa_model.generate(
        prompt=spec["prompt"],
        duration=spec["duration"],
        steps=STEPS,
        cfg_scale=CFG_SCALE,
        sampler_type=SAMPLER_TYPE,
        seed=seed,
        disable_tqdm=True,
    )
    gen_secs = time.time() - t0
    x = audio[0].to(torch.float32).clamp(-1, 1).cpu().numpy()
    return x, gen_secs


def encode_ogg(wav_path, ogg_path, quality):
    subprocess.run(
        ["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
         "-i", str(wav_path), "-ar", str(SAMPLE_RATE), "-ac", "2",
         "-c:a", "libvorbis", "-q:a", str(quality), str(ogg_path)],
        check=True,
    )


def produce_track(sa_model, spec, out_dir, work_dir):
    """Genera, post-processa, valida (con UN retry a seed+1) e codifica una
    traccia. Ritorna la voce di manifest."""
    import soundfile as sf

    sub = "sfx" if spec["category"] == "sfx" else "music"
    (out_dir / sub).mkdir(parents=True, exist_ok=True)
    (work_dir / sub).mkdir(parents=True, exist_ok=True)
    ogg_path = out_dir / sub / f"{spec['id']}.ogg"
    quality = SFX_OGG_QUALITY if sub == "sfx" else MUSIC_OGG_QUALITY

    total_gen_secs = 0.0
    problems = []
    for attempt, seed in enumerate([spec["seed"], spec["seed"] + 1]):
        raw, gen_secs = generate_raw(sa_model, spec, seed)
        total_gen_secs += gen_secs
        x = postprocess(raw, SAMPLE_RATE, spec)
        problems = validate(x, SAMPLE_RATE, spec)
        dur = x.shape[1] / SAMPLE_RATE
        note = "" if not problems else f" PROBLEMI: {'; '.join(problems)}"
        retry = " [retry seed+1]" if attempt == 1 else ""
        print(f"   [{spec['category']}] {spec['id']} seed={seed}: gen {gen_secs:.1f}s, "
              f"finale {dur:.2f}s, RMS {rms_db(x):.1f} dBFS{retry}{note}", flush=True)
        if not problems:
            break
        if attempt == 0:
            print(f"   [{spec['category']}] {spec['id']}: rigenero una volta con seed+1", flush=True)

    wav_path = work_dir / sub / f"{spec['id']}.wav"
    sf.write(str(wav_path), x.T, SAMPLE_RATE, subtype="PCM_16")
    encode_ogg(wav_path, ogg_path, quality)

    return {
        "id": spec["id"],
        "file": f"{sub}/{spec['id']}.ogg",
        "category": spec["category"],
        "loop": spec["loop"],
        "durationSec": round(x.shape[1] / SAMPLE_RATE, 3),
        "genDurationSec": spec["duration"],
        "seed": seed,
        "seedRetried": attempt > 0,
        "prompt": spec["prompt"],
        "rmsDb": round(rms_db(x), 1),
        "peakDb": round(peak_db(x), 1),
        "genSecs": round(total_gen_secs, 1),
        "sizeBytes": ogg_path.stat().st_size,
        "validation": "ok" if not problems else "failed",
        **({"validationProblems": problems} if problems else {}),
    }


def write_manifest(out_dir, entries_by_id, load_secs_by_model):
    manifest = {
        "version": 1,
        "generated": time.strftime("%Y-%m-%d %H:%M:%S"),
        "decision": "DEC-172 (pacchetto pre-generato offline; licenza output: DEC-113)",
        "recipe": {
            "steps": STEPS, "cfgScale": CFG_SCALE, "samplerType": SAMPLER_TYPE,
            "device": DEVICE, "sampleRate": SAMPLE_RATE,
            "oggQuality": {"music": MUSIC_OGG_QUALITY, "sfx": SFX_OGG_QUALITY},
            "loopCrossfadeSecs": LOOP_XFADE_SECS,
        },
        "models": {
            "music": "models/stable-audio-3-small-music",
            "sfx": "models/stable-audio-3-small-sfx",
        },
        "modelLoadSecs": load_secs_by_model,
        "tracks": [entries_by_id[s["id"]] for s in SPECS if s["id"] in entries_by_id],
    }
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")


def main():
    parser = argparse.ArgumentParser(description="Genera il pacchetto audio della demo (DEC-172).")
    parser.add_argument("--out", type=Path, default=REPO_ROOT / "assets" / "audio")
    parser.add_argument("--log-dir", type=Path,
                        default=REPO_ROOT / "logs" / "audio-pack" / time.strftime("%Y%m%d-%H%M%S"))
    parser.add_argument("--only", type=str, default="",
                        help="lista di id separati da virgola: rigenera solo quelli")
    args = parser.parse_args()

    if not shutil.which("ffmpeg"):
        print("audio-pack: ffmpeg non trovato nel PATH (serve per la codifica OGG)", file=sys.stderr)
        return 1
    try:
        import numpy  # noqa: F401
        import soundfile  # noqa: F401
        import torch  # noqa: F401
    except ImportError as e:
        print(f"audio-pack: import fallito ({e}) -- vedi scripts/audio-benchmark.sh "
              "per la ricetta del venv", file=sys.stderr)
        return 1

    specs = SPECS
    if args.only:
        wanted = {s.strip() for s in args.only.split(",") if s.strip()}
        unknown = wanted - {s["id"] for s in SPECS}
        if unknown:
            print(f"audio-pack: id sconosciuti in --only: {sorted(unknown)}", file=sys.stderr)
            return 1
        specs = [s for s in SPECS if s["id"] in wanted]

    args.out.mkdir(parents=True, exist_ok=True)
    args.log_dir.mkdir(parents=True, exist_ok=True)

    # Manifest incrementale: con --only si aggiornano solo le voci rigenerate.
    entries_by_id = {}
    load_secs_by_model = {}
    manifest_path = args.out / "manifest.json"
    if manifest_path.exists():
        old = json.loads(manifest_path.read_text())
        entries_by_id = {t["id"]: t for t in old.get("tracks", [])}
        load_secs_by_model = old.get("modelLoadSecs", {})

    t_start = time.time()
    groups = [
        ("sfx", SFX_MODEL_DIR, [s for s in specs if s["category"] == "sfx"]),
        ("music", MUSIC_MODEL_DIR, [s for s in specs if s["category"] != "sfx"]),
    ]
    for model_key, model_dir, group in groups:
        if not group:
            continue
        print(f"== carico {model_dir} (locale, nessuna rete) ==", flush=True)
        sa_model, load_secs = load_local_model(model_dir, device=DEVICE)
        got_sr = sa_model.model.sample_rate
        if got_sr != SAMPLE_RATE:
            print(f"audio-pack: sample rate inatteso dal modello ({got_sr} != {SAMPLE_RATE})",
                  file=sys.stderr)
            return 1
        load_secs_by_model[model_key] = round(load_secs, 1)
        print(f"   caricato in {load_secs:.1f}s ({len(group)} tracce da generare)", flush=True)
        for spec in group:
            entries_by_id[spec["id"]] = produce_track(sa_model, spec, args.out, args.log_dir)
            write_manifest(args.out, entries_by_id, load_secs_by_model)  # crash-safe
        del sa_model  # mai due modelli in RAM insieme (regola di scheduling)

    total = time.time() - t_start
    done = [entries_by_id[s["id"]] for s in specs if s["id"] in entries_by_id]
    failed = [e["id"] for e in done if e.get("validation") == "failed"]
    size_mib = sum(e["sizeBytes"] for e in entries_by_id.values()) / (1024 * 1024)
    print(f"== fatto: {len(done)} tracce in {total / 60.0:.1f} min, pacchetto {size_mib:.1f} MiB "
          f"-> {args.out / 'manifest.json'} ==", flush=True)
    if failed:
        print(f"== ATTENZIONE: validazione fallita anche dopo il retry per: {', '.join(failed)} "
              "(voci marcate nel manifest, servono occhi umani) ==", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
