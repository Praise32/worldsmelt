---
id: aiprod-audio-generation-pipeline
title: Pipeline audio
domain: ai-production
status: approved
authority: supporting
owner: ai-production
summary: >-
  Pipeline ibrida rFXGen + Stable Audio 3 Small per SFX/musica, adottata da DEC-109 (risolve il conflitto con DEC-036, che considerava l'audio generativo futuro).
last_reviewed: 2026-07-27
last_verified_commit: 892911a
topics: [audio, rfxgen, stable-audio, dec-036, licenza, fallback]
related: []
supersedes: []
source_files: []
---
# Pipeline audio

> **ADOTTATA da DEC-109 (2026-07-22):** via primaria Stable Audio Small in locale, catena
> di fallback rFXGen → curato; licenza Stability Community accettata (DEC-113). Ogni evento
> critico mantiene un suono curato/fallback (garanzia ereditata da DEC-036); nessuna
> generazione in combattimento; modello audio caricato in sequenza col modello di testo
> attivo e con SD.

## Conflitto risolto (DEC-109)

La knowledge base, in `content/audio-and-feedback.md`, affermava che musica e suoni erano
per ora curati e statici e che la generazione audio era futura, in base a DEC-036. Questa
tensione è stata **risolta da DEC-109** (22/07): l'audio diventa generativo, con questa
pipeline come via primaria, mantenendo intatta la garanzia di DEC-036 (ogni evento critico
ha sempre un suono curato o di fallback — vedi il callout in testa a questo documento).
`content/audio-and-feedback.md` è stato aggiornato di conseguenza. La domanda `Q-AUD-001`
(già nel questionario, ora archiviato in
`docs/archive/superseded/19-DECISION-QUESTIONNAIRE.md` — DEC-147) è chiusa.

## Architettura proposta

```text
AudioSpec generata dal modello di testo attivo
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
Modello di testo attivo
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
