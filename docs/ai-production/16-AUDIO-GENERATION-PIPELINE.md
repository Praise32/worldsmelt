---
id: aiprod-audio-generation-pipeline
title: Pipeline audio
domain: ai-production
status: approved
authority: supporting
owner: ai-production
summary: >-
  Pipeline Stable Audio 3 Small (checkpoint music/sfx) per SFX e musica, adottata da DEC-109 (risolve il conflitto con DEC-036, che considerava l'audio generativo futuro). Dal 28/07 rFXGen è uscito dalla pipeline (DEC-178): il fallback garantito è direttamente il pacchetto curato/statico. La demo attuale usa un pacchetto pre-generato offline, non la generazione a runtime (DEC-172).
last_reviewed: 2026-07-31
last_verified_commit: 4d7a410
topics: [audio, stable-audio, dec-036, licenza, fallback, DEC-172, DEC-178, demo, DEC-196]
related: []
supersedes: []
source_files: []
---
# Pipeline audio

> **ADOTTATA da DEC-109 (2026-07-22):** via primaria Stable Audio Small in locale, fallback
> garantito il pacchetto curato/statico; licenza Stability Community accettata (DEC-113).
> Ogni evento critico mantiene un suono curato/fallback (garanzia ereditata da DEC-036);
> nessuna generazione in combattimento; modello audio caricato in sequenza col modello di
> testo attivo e con SD.
>
> **Nota demo (DEC-172, 2026-07-27):** la build demo usa il pacchetto pre-generato offline
> (vedi sezione dedicata sotto), non la generazione a runtime descritta in questo documento.
>
> **Nota pipeline (DEC-178, 2026-07-28):** rFXGen è uscito dalla pipeline audio — non è mai
> stato installato né usato. Il generatore degli SFX (semplici e complessi) è il checkpoint
> `sfx` di Stable Audio Small; la catena di fallback è a due livelli (checkpoint sfx →
> curato), non più tre.
>
> **Nota primo esperimento (DEC-196, 2026-07-31):** il primo esperimento di audio
> generativo copre **SFX e musica insieme, in un'unica milestone** — scelta esplicita del
> proprietario, diversa dalla raccomandazione di default "SFX prima" (superficie di rischio
> minore, iterazione più rapida). Motivazione: valutare in un solo giro la coerenza
> complessiva della pipeline Stable Audio Small (entrambi i checkpoint, music e sfx) prima
> di impegnarsi in iterazioni separate.

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
        +--> effetto (semplice o complesso) --> Stable Audio 3 Small-SFX
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

## rFXGen (rimosso dalla pipeline, DEC-178)

rFXGen era stato previsto (DEC-109) come anello di fallback procedurale per SFX semplici
(click UI, pickup, spari, impatti sintetici, esplosioni brevi, errori, power-up, telegraph
semplici, varianti procedurali) grazie ai suoi vantaggi — nessun modello, costo minimo,
parametri riproducibili, licenza zlib, integrazione con l'ecosistema raylib. Nella pratica
**non è mai stato installato né usato**: il checkpoint `sfx` di Stable Audio 3 Small copre
già tutta la produzione SFX, semplice e complessa. Il proprietario ha confermato il 28/07
che la via reale è sempre stata quella (DEC-178): rFXGen è rimosso dalla pipeline, non è più
un anello di fallback né uno strumento di produzione della demo (DEC-172).

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

## Nota demo (DEC-172): generazione offline, runtime statico

La build demo usa esclusivamente la modalità "resta completamente curato" descritta sopra:
musica/ambience e SFX si generano **offline**, prima della build, con **Stable Audio 3
Small** (checkpoint music e sfx) come **strumento di produzione** — non come dipendenza del
runtime, e senza rFXGen, uscito dalla pipeline (DEC-178). Il risultato entra nel gioco come
**asset statici** (WAV/OGG), letti da un modulo audio raylib
che **non** carica né invoca alcun modello — coerente con la regola di `AGENTS.md` («motore
indipendente dai modelli AI»: il runtime legge solo file locali già validati). La pipeline
generativa a runtime resta quella descritta sopra (DEC-109): la demo non la disabilita,
semplicemente non la usa ancora nella build corrente. Nota gemella in design:
[Audio and Feedback](../design/content/audio-and-feedback.md).

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

Fallback (a due livelli dal 28/07, DEC-178 — rFXGen rimosso):

1. asset curato specifico;
2. famiglia generica;
3. silenzio soltanto per eventi non critici.

Gli eventi critici di gameplay non possono perdere il feedback.
