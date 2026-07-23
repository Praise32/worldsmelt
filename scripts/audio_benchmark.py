#!/usr/bin/env python3
"""Benchmark di Stable Audio Open Small (DEC-109/DEC-113, adottato 22/07/2026)
-- PRIMO prototipo dell'integrazione, nessun runtime audio esiste ancora nel
repo (docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md descrive solo la
pipeline PROPOSTA). Vedi docs/plans/active/model-comparison.md, sezione audio,
per lo stato dei due blocchi che impediscono di ESEGUIRE questo script sulla
macchina di sviluppo al 23/07/2026 (entrambi documentati li', non qui: questo
file resta uno script, non un report).

Uso (una volta risolti i blocchi -- vedi scripts/audio-benchmark.sh):
    ~/venvs/stable-audio/bin/python3 scripts/audio_benchmark.py <out-dir>

Genera 8 prompt fissi (5 SFX + 3 musica, coerenti col gioco) x 2 seed fissi,
salva i .wav in <out-dir>/<prompt-slug>-seed<seed>.wav, misura secondi/clip e
RAM di picco del processo (resource.getrusage, niente dipendenza extra), e
scrive <out-dir>/index.txt con tema/seed/durata/tempo per ogni clip.

NON e' collegato a nessun binario C (AGENTS.md: il gioco non linka mai un
runtime Python) -- e' un prototipo di misura, isolato in ~/venvs/stable-audio,
esattamente come richiesto: "questo benchmark e' il PRIMO prototipo
dell'integrazione", non l'integrazione stessa.
"""

import json
import resource
import sys
import time
from pathlib import Path

# -- prompt fissi (SFX coerenti col game design, musica coerente coi piani
#    generati -- vedi docs/design/content/audio-and-feedback.md) -----------
PROMPTS = [
    # (slug, prompt, secondi, kind)
    ("sfx-melt-complete", "a short satisfying magical melting chime, glass and metal fusing together, fantasy game sound effect", 2.5, "sfx"),
    ("sfx-damage-taken", "a sharp impact hit sound with a metallic ring, player damage feedback, fantasy game sound effect", 1.5, "sfx"),
    ("sfx-room-cleared", "a triumphant short fanfare chime, room cleared notification, fantasy dungeon game sound effect", 2.5, "sfx"),
    ("sfx-item-pickup", "a bright pleasant pickup jingle, collecting a magic item, fantasy game sound effect", 1.5, "sfx"),
    ("sfx-boss-phase-change", "a dark ominous roar with a rising synth stinger, boss entering a new phase, fantasy game sound effect", 3.0, "sfx"),
    ("music-floor-fantasy-dark", "dark fantasy dungeon background music loop, tense low strings and distant metallic drones, no vocals", 20.0, "music"),
    ("music-floor-zero-crucible", "warm molten forge ambient music loop, the crucible hub between runs, gentle mechanical hum and soft bells, no vocals", 20.0, "music"),
    ("music-ambient-cavern", "damp cavern ambient soundscape, dripping water and distant echoes, no vocals, no music", 20.0, "music"),
]
SEEDS = [11, 23]
STEPS = 8   # steps ridotti raccomandati per stable-audio-open-small (sampler rapido, vedi model card)


def main():
    if len(sys.argv) < 2:
        print("uso: audio_benchmark.py <out-dir>", file=sys.stderr)
        return 1
    outDir = Path(sys.argv[1])
    outDir.mkdir(parents=True, exist_ok=True)

    # Import ritardato apposta: se torch/stable-audio-tools non sono
    # installabili (uno dei due blocchi documentati nel piano), l'errore va
    # riportato chiaramente qui, non con uno stack trace all'avvio del file.
    try:
        import torch
        import torchaudio
        from einops import rearrange
        from stable_audio_tools import get_pretrained_model
        from stable_audio_tools.inference.generation import generate_diffusion_cond
    except ImportError as e:
        print(f"audio_benchmark: import fallito ({e}) -- vedi "
              "docs/plans/active/model-comparison.md sezione audio per i passi di sblocco",
              file=sys.stderr)
        return 1

    device = "cpu"  # niente ROCm su RX 5600 XT per torch: CPU per costruzione, non una scelta

    print("== carico stabilityai/stable-audio-open-small (richiede token HF con licenza accettata, DEC-113) ==")
    t0 = time.time()
    try:
        model, model_config = get_pretrained_model("stabilityai/stable-audio-open-small")
    except Exception as e:
        print(f"audio_benchmark: caricamento del modello fallito ({e}) -- probabile gate HF "
              "senza token, vedi docs/plans/active/model-comparison.md sezione audio", file=sys.stderr)
        return 1
    model = model.to(device)
    loadSecs = time.time() - t0
    sampleRate = model_config["sample_rate"]
    print(f"   caricato in {loadSecs:.1f}s (sample_rate={sampleRate})")

    index = []
    peakRssKb = 0
    for slug, prompt, seconds, kind in PROMPTS:
        for seed in SEEDS:
            sampleSize = int(seconds * sampleRate)
            conditioning = [{"prompt": prompt, "seconds_total": seconds}]
            t0 = time.time()
            output = generate_diffusion_cond(
                model, steps=STEPS, conditioning=conditioning,
                sample_size=sampleSize, device=device, seed=seed,
            )
            genSecs = time.time() - t0
            output = rearrange(output, "b d n -> d (b n)")
            output = output.to(torch.float32).div(torch.max(torch.abs(output)).clamp(min=1e-8)) \
                           .clamp(-1, 1).mul(32767).to(torch.int16).cpu()
            wavPath = outDir / f"{slug}-seed{seed}.wav"
            torchaudio.save(str(wavPath), output, sampleRate)
            peakRssKb = max(peakRssKb, resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)
            print(f"   [{kind}] {slug} seed={seed}: {genSecs:.1f}s -> {wavPath.name}")
            index.append({"slug": slug, "prompt": prompt, "kind": kind, "seed": seed,
                          "secondsRequested": seconds, "genSecs": genSecs, "wav": wavPath.name})

    (outDir / "index.json").write_text(json.dumps({
        "model": "stabilityai/stable-audio-open-small",
        "device": device, "steps": STEPS, "loadSecs": loadSecs,
        "peakRssMiB": peakRssKb / 1024.0, "clips": index,
    }, indent=2))
    avgSecs = sum(c["genSecs"] for c in index) / len(index)
    print(f"== fatto: {len(index)} clip, {avgSecs:.1f}s/clip in media, "
          f"picco RAM {peakRssKb/1024.0:.0f} MiB -- indice in {outDir / 'index.json'} ==")
    return 0


if __name__ == "__main__":
    sys.exit(main())
