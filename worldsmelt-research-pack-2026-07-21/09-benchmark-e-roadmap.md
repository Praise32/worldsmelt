# Benchmark e roadmap proposta

## Obiettivo

Determinare una singola build che massimizzi:

- qualità creativa;
- affidabilità del codice;
- velocità;
- dimensione;
- compatibilità desktop e Steam Deck.

## Fase A — congelare la tesi tecnica

1. aggiornare i documenti che trattano la DSL come sistema principale;
2. definire `Worldsmelt Lua API v1`;
3. definire il test del laser e altre 20 meccaniche non esprimibili oggi;
4. definire RunBundle e versione API;
5. separare nettamente JSON dichiarativo e Lua.

## Fase B — ampliare il kernel

Implementare in piccoli passi:

1. command queue;
2. query circolare e segmentale;
3. beam/line telegraph;
4. timer e stato persistente limitato;
5. status/eventi;
6. callback nemici;
7. callback boss;
8. validatore comportamentale.

Ogni passo deve avere test e fallback.

## Fase C — benchmark modelli

Usare gli stessi prompt, seed, API e retry su:

- Qwen2.5-Coder-7B Q4_K_M;
- Qwen3-4B-Instruct-2507 Q4;
- Phi-4-mini-instruct Q4.

Nessun fine-tuning iniziale.

Output:

```csv
model,quant,prompt_id,seed,prefill_ms,decode_ms,retries,compile_ok,dryrun_ok,sim_ok,novelty,score
```

Il 4B diventa modello canonico soltanto se eguaglia il 7B sulla qualità Worldsmelt con vantaggio sostanziale di memoria/tempo.

## Fase D — runtime minimale

1. congelare commit `llama.cpp`;
2. CPU + Vulkan;
3. rimuovere server e funzionalità inutili;
4. contesto 4–8K;
5. KV cache del prompt di sistema;
6. un formato modello;
7. benchmark di avvio, prefill, decode e peak memory;
8. test Windows/Linux.

## Fase E — quantizzazione specifica

1. corpus di calibrazione Worldsmelt;
2. importance matrix;
3. Q4_K_M baseline;
4. ricette miste per tensor sensibili;
5. confronto cieco e test di compilazione;
6. scegliere un solo GGUF e congelarne l’hash.

## Fase F — pipeline immagini

Confronto finale a 128×128:

- SD 1.5 pixel + LCM;
- SDXL Turbo quantizzato, se il backend è stabile;
- SD 3.5 Medium Q5/Q4;
- Large Turbo soltanto come riferimento esterno.

Valutare:

- tempo atlas;
- VRAM/RAM;
- silhouette;
- ritaglio;
- coerenza;
- percentuale di asset accettati;
- leggibilità in gioco.

## Fase G — Steam Deck e hardware esterno

### Prima di dichiarare Deck

- build Linux e Windows/Proton;
- modello canonico incluso;
- generazione completa di una run;
- 1280×800;
- controller;
- misurazione temperatura/potenza non necessaria per certificazione ma utile;
- nessuna inferenza durante il combattimento;
- 30 FPS minimi;
- test sospensione/ripresa;
- spazio disco e cache.

### Playtest hardware

Cercare tester con:

- Steam Deck LCD e OLED;
- Intel integrata;
- AMD 4 GB;
- NVIDIA 4/6/8 GB;
- Windows 10/11;
- Linux AMD/NVIDIA.

## Fase H — decisione sul runtime proprietario

Aprire un progetto di runtime indipendente solo se il profiler dimostra un limite non risolvibile ragionevolmente nel fork.

Gate suggeriti:

- modello congelato;
- test qualitativi stabili;
- almeno tre classi hardware reali;
- baseline riproducibile;
- target di miglioramento misurabile;
- capacità di mantenere CPU e Vulkan;
- suite di equivalenza numerica.

## Criterio di release

Una build candidata è accettabile quando:

- il 100% delle run avvia gameplay, con fallback se necessario;
- nessuno script può bloccare il frame;
- il modello viene liberato prima del combattimento;
- il tempo di generazione rientra nel loop progettato;
- la stessa build usa lo stesso modello su tutti gli hardware;
- il RunBundle è serializzabile e condivisibile;
- Windows, Linux e Deck hanno risultati misurati;
- la dichiarazione AI Steam è completa.
