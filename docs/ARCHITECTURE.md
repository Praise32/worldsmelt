# Architettura del codice C

Il programma è diviso per responsabilità. `src/main.c` contiene soltanto il punto di ingresso e delega l'esecuzione ad `AppRun`.

```text
src/main.c
    -> app/AppRun
        -> game/GameUpdate
            -> world
            -> gameplay
            -> content
            -> assets
        -> render/RendererDrawApp
```

## Moduli

- `app`: ciclo principale, opzioni da riga di comando e modalità dell'applicazione.
- `assets`: caricamento dell'atlas e rilascio delle risorse.
- `content`: lettura del manifest e fallback della run.
- `core/game_types.h`: costanti, enum e strutture condivise.
- `core/game_math`: matematica, colori e generatore casuale.
- `game`: inizializzazione e orchestrazione del frame.
- `gameplay/combat`: giocatore, nemici, colpi, bombe e pickup.
- `gameplay/entities`: creazione e pulizia delle entità.
- `gameplay/item_traits`: conversione e descrizione dei tratti.
- `gameplay/script_vm`: interprete dichiarativo sandboxato.
- `render`: rendering della scena e dell'interfaccia esistente.
- `tests`: test del portale e della mini-VM.
- `world`: generazione delle stanze, transizioni e ricompense.

## Regola per le nuove funzioni

Una nuova funzione va aggiunta al modulo che possiede quella responsabilità. Se introduce una responsabilità nuova e consistente, va creato un nuovo modulo con cartella e coppia `.h`/`.c`. `game_internal.h` è riservato alle collaborazioni interne e non deve diventare un contenitore generico.

La futura integrazione Raygui dovrà vivere in un modulo `src/ui` separato dal rendering del mondo. La mini-VM rimane il comportamento predefinito; un'eventuale sandbox Lua dovrà avere un modulo e test di sicurezza propri.
