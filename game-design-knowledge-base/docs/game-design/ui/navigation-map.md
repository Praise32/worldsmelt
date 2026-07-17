---
id: gd-ui-navigation
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Mappa delle schermate e ritorno del focus."
---

# Navigation Map

## Mappa

```mermaid
flowchart TD
    Boot --> MainMenu
    MainMenu --> RunSetup
    MainMenu --> MultiplayerLobby
    MainMenu --> Leaderboards
    MainMenu --> Options
    MainMenu --> ExitConfirm
    RunSetup --> GenerationStatus
    GenerationStatus --> Gameplay
    Gameplay --> PauseMenu
    PauseMenu --> Gameplay
    PauseMenu --> Options
    PauseMenu --> MainMenuConfirm
    Gameplay --> Results
    Results --> MainMenu
    MultiplayerLobby --> GenerationStatus
```

## Regole comuni

- Ogni schermata definisce focus iniziale.
- Il comando Indietro deve avere risultato prevedibile.
- Le azioni distruttive richiedono conferma.
- Dopo la chiusura di una schermata secondaria, il focus torna all'elemento che l'ha aperta.
- Durante caricamenti critici, prevenire attivazioni duplicate.
