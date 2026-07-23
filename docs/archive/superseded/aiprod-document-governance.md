---
id: ai-prod-document-governance
status: proposed
owner: documentation
last_reviewed: 2026-07-20
summary: "Precedenza, stati, indicizzazione e aggiornamento dei documenti della pipeline AI."
---

# Governance dei documenti

## Separazione

```text
game-design-knowledge-base/docs/game-design/
→ cosa deve vivere il giocatore

docs/technical/
→ architettura stabile

docs/plans/
→ piano di una modifica

docs/worldsmelt-ai-production-blueprint/
→ proposta e riferimento della pipeline AI

experiments/
→ risultati

generated/
→ output non canonici
```

Dopo l'adozione, le parti approvate della blueprint devono essere promosse nei documenti
canonici appropriati. La blueprint non deve diventare una seconda knowledge base
contraddittoria.

## Stati

- `draft`;
- `proposed`;
- `proposed-conflict`;
- `experimental`;
- `approved`;
- `deprecated`.

## Precedenza

1. decision log;
2. documento specifico approved;
3. documento generale approved;
4. proposta tecnica approvata;
5. piano corrente;
6. esperimento;
7. output generato.

In caso di conflitto: non scegliere in silenzio.

## Metadati

Ogni nuovo documento vivo dovrebbe includere:

```yaml
---
id:
status:
owner:
last_reviewed:
summary:
---
```

## Indici

- `INDEX.md`: percorso umano e per agenti;
- `22-TOPIC-ROUTER.md`: da task a documenti;
- `KNOWLEDGE_BASE_MANIFEST.md`: inventario generato;
- `MANIFEST.md`: hash;
- `19-DECISION-QUESTIONNAIRE.md`: domande tecniche/produttive;
- KB `governance/open-questions.md`: domande di game design.

## Aggiornamento obbligatorio

Quando si aggiunge/rimuove un Markdown:

1. aggiornare `INDEX.md`;
2. aggiornare `KNOWLEDGE_BASE_MANIFEST.md`;
3. rigenerare `MANIFEST.md`;
4. aggiornare il topic router se introduce un dominio;
5. chiudere eventuali domande;
6. registrare decisioni.

## Anti-duplicazione

Una regola deve avere una fonte canonica. Gli altri documenti linkano e spiegano soltanto
l'impatto tecnico.

Esempio:

- la UI di generazione è definita nella KB;
- la blueprint definisce il renderer;
- il piano definisce i file da modificare;
- il codice implementa;
- i test verificano.
