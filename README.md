# Melting Run GPU

Piccolo action roguelite top-down GPU, costruito come base per testare run
generate da OpenAI senza mettere la chiave API dentro raylib o dentro
l'eseguibile C.

Il gameplay segue una struttura da dungeon shooter stanza per stanza: 5 piani,
mappa di stanze, nemici, boss, negozio, bombe, chiavi, monete, stanze tesoro e
oggetti visibili sul personaggio.

## Avvio rapido

Da questa cartella:

```bat
build_gpu.bat
run_gpu.bat
```

Generazione solo testo con OpenAI o fallback locale:

```bat
run_gpu_llm.bat
```

Generazione testo + spritesheet via Image API:

```bat
run_gpu_dynamic.bat
```

Se vuoi solo generare i contenuti senza avviare il gioco:

```bat
generate_llm_content.bat
```

Fallback offline, utile per testare senza chiave API o senza consumare crediti:

```bat
generate_llm_content.bat --fallback --seed=12345
```

Spritesheet dinamico, con qualita' media consigliata:

```bat
generate_dynamic_assets.bat --seed=12345 --quality=medium
```

Per forzare l'atlas locale invece del PNG API:

```bat
generate_dynamic_assets.bat --seed=12345 --quality=medium --local-atlas
```

## OpenAI API

La chiave API non viene mai salvata nel gioco e non entra nell'eseguibile C.
Resta in una variabile d'ambiente o in `.env.local`, che e' ignorato da git.

Esempio `.env.local`:

```text
OPENAI_API_KEY=la_tua_chiave
OPENAI_MODEL=gpt-5.5
OPENAI_REASONING_EFFORT=medium
OPENAI_IMAGE_MODEL=gpt-image-2
OPENAI_IMAGE_QUALITY=medium
```

La generazione testo usa la Responses API con JSON strutturato. La generazione
grafica usa la Image API per salvare uno spritesheet PNG 1024x1024. Il gioco
ritaglia le celle 128x128 da quel PNG e applica una pulizia chroma-key sui pixel
quasi neri, cosi' gli sprite possono essere disegnati senza quadrati di sfondo.

## Pipeline dinamica

```text
OpenAI Responses API
  -> generated/current_run.json
  -> generated/current_run.txt
  -> raylib C

OpenAI Image API
  -> generated/current_atlas.png
  -> celle 128x128 ritagliate da raylib

Fallback/forzatura locale
  -> generated/current_atlas.bmp
```

Il gioco legge sempre un manifest semplice, non JSON diretto. Questo tiene il C
leggero e rende facile capire cosa e' stato generato.

## Script sandboxati

L'LLM non genera codice C. Genera piccole istruzioni dichiarative che il gioco
interpreta con una mini VM interna.

Formato nel manifest:

```text
floor1.item1.script=on_fire:burst,3,0.36,split|on_hit:area,54,0.22,slow
```

Operazioni ammesse:

- `on_fire:burst,count,spread,trait`: aggiunge colpi extra quando spari.
- `on_hit:projectile,count,speed,trait`: genera colpi extra quando colpisci.
- `on_hit:area,radius,damageScale,trait`: danno ad area sul colpo.
- `on_hit:heal,chancePercent,amount,trait`: cura con probabilita' controllata.

Ogni script ha un budget massimo di operazioni e non puo' accedere a file,
rete, memoria libera o funzioni arbitrarie. In pratica l'LLM puo' inventare
sinergie nuove mescolando mattoncini sicuri, ma non puo' eseguire codice libero.

## Test

Smoke test automatico:

```bat
bin\melting_run_gpu.exe --smoke-test
```

Regression test del portale boss:

```bat
bin\melting_run_gpu.exe --portal-test
```

Test della mini VM degli script:

```bat
bin\melting_run_gpu.exe --script-test
```

Screenshot test, utile per controllare HUD e rendering:

```bat
bin\melting_run_gpu.exe --screenshot-test
```

Lo screenshot viene salvato in:

```text
logs/melting-run-screen.png
```

## Controlli

- `WASD`: movimento.
- `Mouse + click sinistro` oppure `Frecce`: spara.
- `SPACE`: piazza una bomba, se ne hai almeno una.
- `R`: nuova run.
- `ESC`: esce.

I pickup si raccolgono camminandoci sopra. Le stanze con nemici o boss bloccano
le porte finche' non le ripulisci.

## Meccaniche presenti

- 5 piani completi.
- Ogni piano ha una mappa 5x5 con stanza iniziale, stanze combattimento, stanza
  tesoro, negozio e boss.
- Stanza tesoro con oggetto, sbloccata da una chiave.
- Negozio con oggetto, cuore, chiave e bomba acquistabili con monete.
- Boss a ogni piano.
- Boss del piano 5 piu' resistente e aggressivo.
- Bombe con esplosione ad area.
- Drop di cuori, monete, bombe, chiavi e oggetti.
- Oggetti visibili sullo stickman: cappelli impilati, occhiali, oggetti in mano,
  mantelli, corpo colorato e aura.
- Effetti sugli spari: rimbalzo, homing, esplosione, split, perforazione,
  rapidita', proiettili grandi, rallentamento e vampirismo.
- Script sandboxati per sinergie generate.
- Spritesheet API con estrazione celle e chroma-key, piu' fallback locale.
- Finestra fullscreen con viewport di gioco interna e pannelli UI laterali.
- Menu principale e menu pausa.

## Architettura

- `src/main.c`: punto di ingresso minimo che delega ad `AppRun`.
- `src/app/`: ciclo applicativo, finestra e modalità menu/gioco/pausa.
- `src/assets/`: caricamento e rilascio delle risorse Raylib.
- `src/content/`: manifest e contenuti della run.
- `src/core/`: strutture dati, costanti, matematica, colori e RNG.
- `src/game/`: inizializzazione e orchestrazione dello stato.
- `src/gameplay/`: entità, combattimento, oggetti e mini VM.
- `src/render/`: rendering del gioco e dell'interfaccia.
- `src/tests/`: test interni richiamabili da riga di comando.
- `src/world/`: stanze, mappe, transizioni e ricompense.
- `llm/run_content.mjs`: chiamate OpenAI, schema, fallback e scrittura file.
- `llm/generate_run.mjs`: comando da terminale per generare una run.
- `llm/server.mjs`: piccolo sidecar HTTP locale opzionale.
- `generated/current_run.json`: risposta strutturata dell'LLM.
- `generated/current_run.txt`: manifest letto dal gioco C.
- `generated/current_atlas.png`: spritesheet generato da OpenAI e letto dal gioco.
- `generated/current_atlas.bmp`: fallback locale o atlas forzato con `--local-atlas`.
- `generated/current_atlas.json`: mappa celle dell'atlas.
- `docs/`: appunti, decisioni architetturali, setup e problemi noti.

I file in `generated/` sono ignorati da git per non committare contenuti
temporanei, asset generati o dati legati alla tua macchina.

La struttura completa e le regole per aggiungere nuovi moduli sono descritte in
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Limiti intenzionali

- L'LLM lavora prima della run, non durante il gameplay.
- Lo spritesheet IA puo' non rispettare perfettamente la griglia; per questo il
  prompt e' molto vincolato e resta disponibile `--local-atlas` come fallback.
- Niente dipendenze npm.
- Niente chiavi API dentro il C.
- Niente audio per ora.
- Il sistema e' piccolo di proposito: serve a validare OpenAI + roguelite, poi
  si puo' espandere con fusione oggetti, boss pattern e asset migliori.
