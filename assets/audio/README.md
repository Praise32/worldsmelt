# Pacchetto audio della demo (DEC-172)

Pacchetto **pre-generato offline** per la build demo: il motore legge solo questi file
statici, nessun modello audio gira a runtime (DEC-172). OGG Vorbis, 44.1 kHz stereo.

- `music/` — musica e ambience (60–90 s, pensate per il loop: crossfade coda→testa
  già applicato, il punto di loop è senza click);
- `sfx/` — effetti brevi (< 2 s);
- `manifest.json` — per ogni traccia: file, categoria, loop, durata reale, seed,
  prompt, parametri di generazione e stato di validazione.

## Provenienza e licenza

Tutto generato in locale con **Stable Audio 3 Small** (checkpoint `small-music` e
`small-sfx`, pesi in `models/stable-audio-3-small-{music,sfx}/`, non versionati).
Gli output sono soggetti alla **Stability AI Community License** (accettata con
DEC-113; il modello include componenti soggetti ai termini Gemma — vedi
`models/*/LICENSE.md` e `LICENSE_GEMMA.md`): uso gratuito sotto 1 M$ di ricavi annui,
rischio commerciale registrato in `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md`.

Nota: DEC-172 prevede rFXGen per gli effetti procedurali, ma rFXGen non è presente
né nel repo né nell'ambiente di produzione: anche gli SFX di questo pacchetto vengono
dal checkpoint sfx (fallback previsto). Il pacchetto è passato solo dalla validazione
automatica (silenzio/durata, un retry a seed+1): la curation estetica è pendente.

## Rigenerare

```sh
scripts/audio-pack.sh                      # pacchetto completo (~10 min su CPU)
scripts/audio-pack.sh --only boss,ui_move  # solo alcune tracce (manifest aggiornato per-voce)
```

Le spec (id, prompt, durata, seed) sono la lista dichiarativa `SPECS` in
`scripts/audio-pack.py`: seed fissi, stessa ricetta del benchmark del 23/07/2026
(steps=8, cfg_scale=1.0, sampler "pingpong", CPU) — a parità di spec l'output è
riproducibile. Prerequisiti: venv `~/venvs/stable-audio` (ricetta in
`scripts/audio-benchmark.sh`), pesi scaricati, ffmpeg con libvorbis.
