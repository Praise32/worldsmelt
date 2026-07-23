---
id: aiprod-audio-generation-pipeline
title: Pipeline audio
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Pipeline ibrida rFXGen + Stable Audio 3 Small per SFX/musica, in esplicito conflitto con DEC-036 che considera l'audio generativo futuro.
last_reviewed: 2026-07-22
topics: [audio, rfxgen, stable-audio, dec-036, licenza, fallback]
related: []
supersedes: []
source_files: []
---
# Pipeline audio

> **PROPOSTA NON APPROVATA — bloccata da DEC-036.** L'audio del gioco resta curato/statico
> finché la open question 12 (già Q-AUD-001) in
> `docs/design/governance/open-questions.md` non riceve risposta dal proprietario. Se
> approvata, la decisione va registrata nel decision-log (nuova DEC-NNN), mai qui.

## Conflitto da risolvere

La knowledge base, in `content/audio-and-feedback.md`, afferma che musica e suoni sono per
ora curati e statici e che la generazione audio è futura, in base a DEC-036.

La nuova direzione propone invece di introdurre una pipeline audio generativa già nella
pre-alpha. Un agente non deve scegliere da solo fra le due versioni.

Domanda bloccante: `Q-AUD-001` in `19-DECISION-QUESTIONNAIRE.md`.

## Architettura proposta

```text
AudioSpec generata da Qwen
        |
        +--> effetto semplice --> rFXGen
        |
        +--> effetto complesso --> Stable Audio 3 Small-SFX
        |
        +--> musica/ambiente --> Stable Audio 3 Small-Music
        v
post-processing
        v
validazione e review
        v
candidate → curated/cache/fallback
        v
raylib audio
```

## rFXGen

Uso principale:

- click UI;
- pickup;
- spari;
- impatti sintetici;
- esplosioni brevi;
- errori;
- power-up;
- telegraph semplici;
- varianti procedurali.

Vantaggi:

- nessun modello;
- costo minimo;
- parametri riproducibili;
- licenza zlib;
- integrazione naturale con l'ecosistema raylib.

Non serve generare ogni variante con un modello neurale.

## Stable Audio 3 Small

Proposta da valutare:

- `small-sfx`: effetti complessi;
- `small-music`: musica e ambienti;
- 433M parametri dichiarati per ciascun modello;
- inferenza anche CPU dichiarata dal repository;
- durata variabile fino a 120 secondi;
- supporto LoRA e adattatori impilabili dichiarato dal progetto ufficiale.

Uso iniziale: tool Python esterno, non dipendenza del binario C.

## Scheduling

Caricamento sequenziale:

```text
Qwen
→ unload
SD1.5
→ unload
Stable Audio
→ unload
```

Non tenere tre modelli contemporaneamente sulla GPU da 6 GB.

L'audio non viene generato quando il colpo viene sparato. Si genera:

- durante lo sviluppo;
- nel Piano 0, se approvato dal design;
- fra i piani;
- in background;
- oppure resta completamente curato nella modalità compatibile.

## AudioSpec

```json
{
  "version": 1,
  "theme": "flesh cathedral",
  "music": {
    "mood": "oppressive ritual",
    "bpm": 74,
    "duration_seconds": 90,
    "loop": true,
    "palette": ["detuned organ", "low percussion", "metallic drone"]
  },
  "families": {
    "ui": "soft retro digital",
    "projectile": "wet synthetic",
    "impact": "bone and metal",
    "door": "heavy organic membrane"
  },
  "fallback_pack": "curated_audio_core_v1"
}
```

## Post-processing

Ogni output passa attraverso:

```text
trim
→ DC offset removal
→ normalizzazione
→ limiter
→ clipping check
→ fade
→ loop/crossfade
→ resample
→ conversione
→ loudness/tag
```

Formati:

- WAV per SFX molto brevi;
- OGG per musica e ambienti;
- parametri JSON per rFXGen;
- metadati e hash sempre presenti.

## Validazione

SFX:

- durata;
- picco;
- clipping;
- silenzio;
- rumore;
- spettro;
- categoria;
- intelligibilità nel mix;
- duplicazione.

Musica:

- loop;
- transizione;
- durata;
- BPM, se richiesto;
- loudness;
- assenza di voce/testo non desiderato;
- somiglianze sospette;
- ascolto umano obbligatorio.

## Licenza

Stable Audio 3 usa la Stability AI Community License e include componenti soggetti ai
termini Gemma. La pagina licenza Stability AI indica attualmente uso gratuito per soggetti
sotto 1 milione di dollari di ricavi annui; sopra tale soglia serve un accordo Enterprise.

Questa condizione va registrata come rischio commerciale e ricontrollata prima della
release.

## LoRA audio

Non addestrare subito.

Ordine:

```text
modello stock
→ candidate
→ selezione di una grammatica sonora
→ dataset documentato
→ eventuale worldsmelt-audio-style
```

Possibili LoRA future:

- `worldsmelt-audio-style`;
- `worldsmelt-organic-sfx`;
- `worldsmelt-machine-sfx`;
- `worldsmelt-ambient-music`;
- `worldsmelt-boss-music`.

## Fallback

La modalità base deve includere una libreria curata sufficiente. Un fallimento audio non
blocca mai una run.

Fallback:

1. asset curato specifico;
2. variante rFXGen;
3. famiglia generica;
4. silenzio soltanto per eventi non critici.

Gli eventi critici di gameplay non possono perdere il feedback.
