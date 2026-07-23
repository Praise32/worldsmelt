# Protocollo degli esperimenti

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
