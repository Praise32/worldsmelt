---
id: gd-game-states
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Stati principali e transizioni visibili."
---

# Game States and Flow

## Stati principali

- Boot
- Main Menu
- Run Setup
- Generating Run
- Gameplay
- Room Transition
- Floor Transition
- Pause
- Results
- Multiplayer Lobby
- Leaderboards
- Options
- Error Recovery

## Flusso base

```mermaid
flowchart TD
    Boot --> MainMenu
    MainMenu --> RunSetup
    RunSetup --> GeneratingRun
    GeneratingRun --> Gameplay
    Gameplay --> RoomTransition
    RoomTransition --> Gameplay
    Gameplay --> FloorTransition
    FloorTransition --> Gameplay
    Gameplay --> Pause
    Pause --> Gameplay
    Gameplay --> Results
    Results --> MainMenu
    MainMenu --> MultiplayerLobby
    MultiplayerLobby --> GeneratingRun
    MainMenu --> Leaderboards
    MainMenu --> Options
```

## Regola di transizione

Ogni transizione deve definire:

- condizione di ingresso;
- input consentiti;
- cosa viene salvato;
- feedback al giocatore;
- comportamento di annullamento;
- fallback in caso di errore.
