# Game Design Knowledge Base

Questa cartella definisce l'esperienza e il comportamento previsto del gioco.

## Fondamenti

- [Project Brief](PROJECT_BRIEF.md) — sintesi dell'idea iniziale e dei vincoli già dichiarati.
- [Vision](00-vision.md) — identità del gioco e promessa al giocatore.
- [Design Pillars](01-design-pillars.md) — principi che guidano ogni decisione.
- [Player Experience](02-player-experience.md) — emozioni, ritmo e leggibilità.
- [Core Loop](03-core-loop.md) — ciclo di azioni e ricompense.
- [Run Structure](04-run-structure.md) — struttura proposta in cinque piani.
- [Game States and Flow](05-game-states-and-flow.md) — stati principali e transizioni.
- [AI Content Model](06-ai-content-generation-model.md) — ruolo concettuale dell'IA locale.
- [Difficulty and Progression](07-difficulty-and-progression.md) — crescita della sfida e competenza del giocatore.
- [Multiplayer and Competition](08-multiplayer-and-competition.md) — gare, classifiche e correttezza competitiva.
- [Originality Guardrails](09-originality-guardrails.md) — limiti per mantenere il progetto originale.

## Sistemi

- [Player](systems/player.md)
- [Combat and Projectiles](systems/combat-and-projectiles.md)
- [Health and Resources](systems/health-and-resources.md)
- [Items, Pools and Rarity](systems/items-pools-and-rarity.md)
- [Synergies](systems/synergies.md)
- [Active Items](systems/active-items.md)
- [Passive Items](systems/passive-items.md)
- [Trinkets](systems/trinkets.md)
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

## Regole di precedenza

1. Un documento `approved` prevale su un documento `draft`.
2. Il documento più specifico prevale su quello generale.
3. Le `Open questions` non sono requisiti.
4. I file generati automaticamente non sono fonti canoniche.
5. In caso di conflitto irrisolto, non inventare: registra una decisione.
