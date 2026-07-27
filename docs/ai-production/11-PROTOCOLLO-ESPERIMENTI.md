---
id: aiprod-protocollo-esperimenti
title: Protocollo degli esperimenti
domain: ai-production
status: approved
authority: supporting
owner: ai-production
summary: >-
  Metodologia per esperimenti LoRA: una domanda per run, massimo due variabili, baseline obbligatoria, metriche automatiche, review umana e regola di promozione; include la variante comparison/bake-off per confronti multi-modello.
last_reviewed: 2026-07-27
last_verified_commit: d30890b
topics: [esperimenti, metodologia, metriche, promozione, id-esperimento, comparison, bake-off]
related: []
supersedes: []
source_files: []
---
# Protocollo degli esperimenti

> **Approvato il 2026-07-27 (DEC-164)** con una precisazione: per i ruoli di
> implementazione e giudizio, e per ogni punto in cui questo protocollo divergesse dalla
> scala di implementazione o dalle regole di chiusura task, prevale sempre `CLAUDE.md`
> (root), nello stesso spirito della nota di precedenza di
> [18-AGENT-ORCHESTRATION.md](18-AGENT-ORCHESTRATION.md). Questo documento resta il
> metodo per condurre esperimenti e comparison, non lo sostituisce.

## Domanda

Ogni run deve rispondere a una domanda singola.

Buona:

> Rank 8 migliora la silhouette rispetto a rank 4 a parità di dataset e step?

Cattiva:

> Trova la configurazione migliore.

## Variabili

Cambiare al massimo due variabili:

- rank;
- learning rate;
- step;
- caption;
- dataset;
- base;
- LoRA weight;
- LCM;
- CFG;
- dimensione.

## Baseline

Ogni esperimento confronta:

- base SD1.5;
- migliore LoRA precedente;
- candidato.

## Metriche automatiche

- tasso di generazione riuscita;
- alpha valido;
- bordo;
- area silhouette;
- palette;
- centro di massa;
- numero componenti;
- duplicazione;
- tempo;
- VRAM;
- dimensione file;
- tasso fallback;
- tasso di asset importabile.

## Review umana

Valutare in-engine:

1. leggibilità a scala reale;
2. coerenza stilistica;
3. riconoscibilità del ruolo;
4. telegraph;
5. assenza di dettagli impossibili da animare;
6. somiglianze sospette;
7. qualità del loop;
8. compatibilità con UI e sfondo.

## Regola di promozione

Un candidato viene promosso soltanto se:

- supera la baseline;
- non peggiora una metrica critica;
- ha licenza/provenienza accettabile;
- è riproducibile;
- produce un report;
- passa il test in-engine;
- ha fallback.

## ID

Formato:

```text
YYYYMMDD-domain-base-rank-steps-index
```

Esempio:

```text
20260720-style-sd15-r8-s1500-01
```

## Registro

```json
{
  "experiment_id": "20260720-style-sd15-r8-s1500-01",
  "git_commit": "...",
  "dataset_hash": "...",
  "config_hash": "...",
  "status": "accepted",
  "parent": null,
  "question": "...",
  "changed_variables": ["rank"],
  "automatic_metrics": {},
  "human_scores": {},
  "decision": "...",
  "artifacts": []
}
```

## Variante comparison/bake-off

Un esperimento normale confronta un candidato contro una baseline. Una **comparison
(bake-off)** confronta **più candidati fra loro e contro la stessa baseline**, in un'unica
sessione, per rispondere a una domanda più ampia del tipo "quale famiglia di modelli è la
più adatta sotto un vincolo dato?" — è il formato adottato dai report del 23/07
(`experiments/image-comparison-2026-07-23.md`,
`experiments/image-comparison-gen2-2026-07-23.md`,
`experiments/model-comparison-testo-2026-07-23.md`,
`experiments/audio-benchmark-2026-07-23.md`).

Regole proprie della variante:

- **Domanda singola anche qui**, ma posta a livello di famiglia di modelli o approccio
  ("SD3.5/SDXL/Flux battono SD1.5 sotto 6 GB VRAM?"), non del singolo iperparametro.
- Il limite di **due variabili** non si applica al numero di candidati messi a confronto
  (la comparazione stessa è lo scopo); resta valido per eventuali variazioni interne a un
  singolo candidato (es. risoluzione, quantizzazione).
- La **baseline resta obbligatoria** ed è inclusa nella stessa tabella dei candidati, non
  solo citata a parte.
- Le **metriche automatiche** si registrano per candidato, in forma tabellare comparabile
  (stesso schema di colonne per tutti i candidati).
- La **review umana** può restare qualitativa e comparativa (quale candidato è più vicino
  alla baseline) quando i candidati non superano comunque la baseline: non è necessario
  applicare la regola di promozione a un candidato che il documento stesso dichiara
  perdente.
- Il **report finale** sostituisce il registro JSON del singolo esperimento con un
  documento narrativo in `experiments/`, purché mantenga: licenze verificate per ogni
  candidato, artefatti tracciati (path dei log/immagini), e una conclusione esplicita su
  quale candidato (se alcuno) supera la baseline.
- Se nessun candidato supera la baseline, il report lo dichiara esplicitamente (come nei
  report gen-2 del 23/07): la comparison è comunque un esperimento valido e chiudibile,
  non un esperimento fallito da rifare.

## Criteri vertical slice

La pipeline è sufficiente quando genera e importa:

- un blob;
- un volante;
- un tentacolare;
- un bipede;
- un oggetto;
- un proiettile;
- un tile/modulo ambiente;

con fallback e senza inferenza in combattimento.
