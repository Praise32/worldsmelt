# Repository locali di riferimento

Questi repository si trovano nella cartella principale di lavoro e sono indicizzati nel grafo `codebase-memory-mcp`. Non sono copiati dentro Melting Run e non diventano automaticamente dipendenze.

## Raylib

Percorso: `raylib/`

È il motore e la libreria già usata dal gioco. Melting Run collega la build Debug locale esistente.

## Raygui

Percorso: `raygui/`

È una libreria immediate-mode a singolo header pensata soprattutto per strumenti. La versione locale dichiara compatibilità con la versione corrente di Raylib. Se verrà integrata, `RAYGUI_IMPLEMENTATION` dovrà essere definito in un solo file `.c` dentro un futuro modulo `src/ui`.

Raygui è un candidato per menu, impostazioni, schermate di caricamento e strumenti di debug. Non sostituisce il rendering del mondo di gioco.

## IsaacDocs

Percorso: `IsaacDocs/`

È documentazione comunitaria della modding API di The Binding of Isaac. Va usata come riferimento concettuale per callback, oggetti, cache delle statistiche e pattern dei boss, non come codice da copiare né come specifica del nostro motore.

## Binding of Isaac style procedural generation

Percorso: `binding-of-isaac-style-procedural-generation/`

È un progetto Unity di riferimento per la generazione procedurale di stanze. Gli algoritmi utili devono essere reinterpretati per il modello dati C di `src/world`, verificando prima licenza e compatibilità.

## gguf-tools

Percorso: `gguf-tools/`

È una libreria e un insieme di strumenti per ispezionare e manipolare file GGUF. Non è, da sola, un motore di inferenza. L'eventuale inferenza locale richiederà un backend dedicato, mentre `gguf-tools` può servire per validazione e diagnostica dei modelli.

## taste-skill

Percorso: `C:\Users\maria\Documents\Codex\Repository Utili Github\taste-skill-main`

La skill è destinata a frontend web e dichiara fuori ambito le interfacce native. Non va applicata letteralmente a Raylib. Restano utili alcuni principi generali: audit prima delle modifiche, gerarchia chiara, una palette coerente, stati di interazione leggibili e assenza di elementi decorativi senza funzione.
