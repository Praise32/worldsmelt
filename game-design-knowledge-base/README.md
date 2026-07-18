# Game Design Knowledge Base

Questa cartella è il **documento di game design unico e canonico** del progetto: non esiste
un'altra fonte di verità per comportamento, contenuti, flussi e interfaccia del gioco. Ogni
decisione sul gioco va cercata, verificata e registrata qui.

Il progetto descrive un action roguelite a stanze, proprietà originale, nel quale gran parte
dei contenuti di ogni run viene generata da un'IA locale entro regole, pool, rarità e vincoli
di qualità definiti dal game design.

Il gioco si chiama **Worldsmelt** (titolo definitivo, DEC-071). "Melting Run" resta solo il
nome storico di questo repository e il titolo di lavoro citato nel
[registro delle decisioni](docs/game-design/governance/decision-log.md); nei documenti vivi
della knowledge base il gioco è Worldsmelt.

## Come iniziare

1. Leggi `docs/game-design/INDEX.md`.
2. Risolvi le domande in `docs/game-design/governance/open-questions.md`.
3. Modifica i documenti `draft` prima di considerarli requisiti approvati.
4. Usa i template in `docs/game-design/templates/` per aggiungere nuovi contenuti.
5. Non inserire decisioni tecniche nei documenti di game design, salvo interfacce comportamentali indispensabili.

## Regola fondamentale

- `docs/game-design/` descrive **che cosa deve fare il gioco**.
- `docs/technical/` descriverà **come viene costruito**.
- `docs/plans/` conterrà **come implementare una modifica specifica**.
- Il codice non sostituisce la documentazione di design.

## Stati dei documenti

- `draft`: proposta modificabile, non ancora completamente vincolante.
- `approved`: comportamento canonico.
- `experimental`: idea da prototipare.
- `deprecated`: non usare per nuove implementazioni.

## Originalità

Usare riferimenti di genere per comunicare il tipo di esperienza, ma non copiare nomi, personaggi, grafica, stanze, oggetti, testi, suoni, interfaccia o contenuti riconoscibili da opere esistenti.
