# Worldsmelt — dossier tecnico e strategico

**Data di consolidamento:** 21 luglio 2026  
**Origine:** analisi della repository `worldsmelt-main`, del blueprint di produzione e delle decisioni emerse nella conversazione.

## Scopo del dossier

Questo pacchetto raccoglie in forma strutturata le informazioni utili per sviluppare e distribuire Worldsmelt come **arcade action roguelite AI-native, offline e local-first**, in cui un modello linguistico locale genera per ogni run nuove meccaniche eseguibili in Lua sandboxato.

Non è una trascrizione integrale della chat. È una sintesi operativa: decisioni, confronti, rischi, architettura, modelli candidati, benchmark, Steam/Steam Deck e piano di sviluppo.

## Tesi centrale

Worldsmelt non usa l’AI soltanto per nomi, descrizioni o variazioni numeriche. La caratteristica distintiva è:

> Un modello locale distribuito con il gioco scrive e valida programmi Lua che compongono nuove meccaniche, comportamenti, attacchi e sinergie per ogni run; il motore C mantiene gli invarianti, limita le risorse e applica fallback sicuri.

## Decisioni principali consolidate

1. **Un solo modello canonico per tutti i giocatori.** Hardware migliore deve ridurre l’attesa, non aumentare la qualità dei contenuti.
2. **C stabile + Lua generato.** Non generare C, DLL o codice macchina.
3. **JSON vincolato per la parte dichiarativa; Lua soltanto per comportamento nuovo.**
4. **LLM e modello immagini caricati in sequenza**, mai residenti insieme sulla RX 5600 XT da 6 GB.
5. **Il PC di sviluppo è il benchmark desktop di riferimento**, ma non è ancora un requisito minimo dimostrato.
6. **Steam Deck è un obiettivo realistico soprattutto con un modello 4B Q4**, ma va verificato su hardware reale.
7. **Non riscrivere subito l’intero runtime di inferenza.** Prima creare una build specializzata e minimale sopra/forkando `llama.cpp`; un runtime totalmente proprietario è una fase successiva condizionata dai benchmark.
8. **Il criterio di scelta del modello è specifico del gioco:** meccaniche valide, nuove e accettabili per minuto di generazione.

## Indice

- `01-visione-prodotto-e-posizionamento.md`
- `02-stato-repository-e-architettura.md`
- `03-nvidia-code-agent-lua.md`
- `04-runtime-specializzato-ds4.md`
- `05-modello-linguistico-unico.md`
- `06-modelli-immagine-audio-stability.md`
- `07-steam-steam-deck-e-mercato.md`
- `08-sicurezza-e-generazione-meccaniche.md`
- `09-benchmark-e-roadmap.md`
- `10-decisioni-e-domande-aperte.md`
- `11-fonti.md`

## File del progetto analizzati

Repository principale:

- `docs/ARCHITECTURE.md`
- `docs/BENCHMARKS.md`
- `docs/SPRITES-SPIKE.md`
- `docs/superpowers/specs/2026-07-13-lua-sandbox-design.md`
- `src/script/script_sandbox.c`
- `src/script/script_api.c`
- `tools/melting-gen/gen_lua.c`
- `tools/melting-gen/prompts/lua_system.txt`
- `tools/melting-sprites/sprite_sd.c`
- `tools/melting-sprites/melting_sprites.h`
- `scripts/download-models.sh`
- `roguelike-ai-appunti/01-visione-e-confini.md`
- `roguelike-ai-appunti/02-architettura-sinergie-dsl.md`

Blueprint:

- `02-STACK-MODELLI.md`
- `16-AUDIO-GENERATION-PIPELINE.md`

## Nota sull’accuratezza

Le informazioni temporali su NVIDIA, DS4, Steam, modelli e licenze sono state verificate sul web il 21 luglio 2026. Prestazioni future su Steam Deck, GPU integrate o modelli non ancora benchmarkati sono valutazioni progettuali, non risultati misurati.
