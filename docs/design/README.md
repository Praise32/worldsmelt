---
id: design-readme
title: Design di Worldsmelt — percorso di lettura canonico
domain: design
status: approved
authority: canonical
owner: design
summary: >-
  Punto d'ingresso del design canonico: percorso di lettura curato di fondamenti, sistemi,
  UI, contenuto e governance, con le regole di precedenza e gli stati dei documenti.
  Erede diretto di INDEX.md e README.md della game-design-knowledge-base.
last_reviewed: 2026-07-22
last_verified_commit: 0ec60d0
topics: [indice, design, governance, navigazione]
related: [design-decision-log]
supersedes: []
source_files: []
---

# Design di Worldsmelt

Questa cartella è il **game design unico e canonico** del progetto: non esiste un'altra
fonte di verità per comportamento, contenuti, flussi e interfaccia del gioco. Ogni decisione
sul gioco va cercata, verificata e registrata qui — in particolare nel
[Decision Log](governance/decision-log.md) (DEC-001..DEC-139).

Il gioco si chiama **Worldsmelt** (titolo definitivo, DEC-071). «Melting Run» resta solo il
nome storico del repository e il titolo di lavoro citato nel decision-log; nei documenti
vivi il gioco è Worldsmelt.

Il progetto descrive un action roguelite a stanze, proprietà originale, nel quale gran parte
dei contenuti di ogni run viene generata da un'IA locale entro regole, pool, rarità e
vincoli di qualità definiti dal game design.

L'indice completo generato (con stato e autorità di ogni documento) è [INDEX.md](INDEX.md);
qui sotto c'è il percorso di lettura curato.

## Fondamenti

- [Project Brief](PROJECT_BRIEF.md) — sintesi dell'idea iniziale e dei vincoli già dichiarati.
- [Vision](00-vision.md) — identità del gioco e promessa al giocatore.
- [Design Pillars](01-design-pillars.md) — principi che guidano ogni decisione.
- [Player Experience](02-player-experience.md) — emozioni, ritmo e leggibilità.
- [Core Loop](03-core-loop.md) — ciclo di azioni e ricompense.
- [Run Structure](04-run-structure.md) — struttura in cinque piani.
- [Game States and Flow](05-game-states-and-flow.md) — stati principali e transizioni.
- [AI Content Model](06-ai-content-generation-model.md) — ruolo concettuale dell'IA locale.
- [Difficulty and Progression](07-difficulty-and-progression.md) — crescita della sfida.
- [Multiplayer and Competition](08-multiplayer-and-competition.md) — gare e classifiche.
- [Originality Guardrails](09-originality-guardrails.md) — limiti di originalità.
- [Sintesi strategica 2026-07](vision/sintesi-strategica-2026-07.md) — posizionamento di
  prodotto (supporting, non canonico).

## Sistemi

- [Floor Zero](systems/floor-zero.md)
- [Player](systems/player.md)
- [Characters](systems/characters.md)
- [Combat and Projectiles](systems/combat-and-projectiles.md)
- [Health and Resources](systems/health-and-resources.md)
- [Items, Pools and Rarity](systems/items-pools-and-rarity.md)
- [Synergies](systems/synergies.md)
- [Item Fusion](systems/item-fusion.md)
- [Active Items](systems/active-items.md)
- [Passive Items](systems/passive-items.md)
- [Innesti](systems/grafts.md)
- [Enemies](systems/enemies.md)
- [Bosses](systems/bosses.md)
- [Rooms and Floors](systems/rooms-and-floor-generation.md)
- [Special Rooms](systems/special-rooms.md)
- [Secrets and Obstacles](systems/secrets-and-obstacles.md)
- [Rewards and Economy](systems/rewards-and-economy.md)
- [Generated Content Validation](systems/generated-content-validation.md)
- [Run Manifest and Reproducibility](systems/run-manifest-and-reproducibility.md)
- [Save and Meta Progression](systems/save-and-meta-progression.md)

## Interfaccia e flussi

- [Navigation Map](ui/navigation-map.md)
- [Main Menu](ui/main-menu.md)
- [Run Setup](ui/run-setup.md)
- [Generation Status](ui/generation-status.md)
- [HUD](ui/hud.md)
- [Pause Menu](ui/pause-menu.md)
- [Inventory and Synergy Screen](ui/inventory-and-synergy-screen.md)
- [Multiplayer Lobby](ui/multiplayer-lobby.md)
- [Results and Leaderboards](ui/results-and-leaderboards.md)
- [Options and Accessibility](ui/options-and-accessibility.md)

## Contenuto e presentazione

- [Content Taxonomy](content/content-taxonomy.md)
- [Visual Language](content/visual-language.md)
- [Audio and Feedback](content/audio-and-feedback.md)
- [Narrative Tone](content/narrative-tone.md)

## Governance

- [Open Questions](governance/open-questions.md)
- [Decision Log](governance/decision-log.md)
- [Glossary](governance/glossary.md)
- [Design Probes](governance/design-probes.md)
- [Definition of Ready](governance/definition-of-ready.md)
- [Definition of Done](governance/definition-of-done.md)

## Template

I template sono in `templates/` e vanno duplicati, non modificati per una singola feature.

## Stati dei documenti

- `draft`: proposta modificabile, non ancora completamente vincolante.
- `approved`: comportamento canonico.
- `experimental`: idea da prototipare.
- `superseded`/`deprecated`: non usare per nuove implementazioni.

## Regole di precedenza

1. Un documento `approved` prevale su un documento `draft`.
2. Il documento più specifico prevale su quello generale.
3. Le `Open questions` non sono requisiti.
4. I file generati automaticamente non sono fonti canoniche.
5. In caso di conflitto irrisolto, non inventare: registra una decisione.

La gerarchia completa fra i domini documentali (design, engineering, ai-production, …) è in
[`docs/_meta/DOCUMENT-STANDARDS.md`](../_meta/DOCUMENT-STANDARDS.md).

## Regola fondamentale

- `docs/design/` descrive **che cosa deve fare il gioco**.
- `docs/engineering/` descrive **come è costruito davvero**.
- `docs/plans/` contiene **come implementare una modifica specifica**.
- Il codice non sostituisce la documentazione di design.

## Originalità

Usare riferimenti di genere per comunicare il tipo di esperienza, ma non copiare nomi,
personaggi, grafica, stanze, oggetti, testi, suoni, interfaccia o contenuti riconoscibili
da opere esistenti.
