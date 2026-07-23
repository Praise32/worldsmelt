---
id: ref-pack-nvidia-code-agent-lua
title: NVIDIA NVIGI Code Agent Lua — cosa fa davvero
domain: references
status: proposed
authority: supporting
owner: design
summary: >-
  Analisi del sample NVIDIA NVIGI Code Agent Lua (LLM locale scrive funzioni Lua per compagno AI); lezioni di sicurezza e design API applicabili a Worldsmelt.
last_reviewed: 2026-07-22
topics: [nvidia, nvigi, code-agent, sicurezza-lua, design-api, ricerca-esterna]
related: []
supersedes: []
source_files: []
---

# NVIDIA NVIGI Code Agent Lua — cosa fa davvero

## Il progetto

NVIDIA ha pubblicato nel proprio **In-Game Inferencing SDK (NVIGI)** un sample chiamato **Code Agent Sample: Lua Dungeon Crawler**.

È un dungeon crawler testuale:

- il giocatore muove il proprio personaggio con WASD;
- invia in linguaggio naturale ordini a un compagno AI;
- un modello locale genera una funzione Lua;
- la funzione viene eseguita a ogni tick finché il compito termina.

Esempio concettuale:

```text
“Prendi la spada, poi torna da me”
       ↓
LLM genera update_func(...)
       ↓
Lua cerca l’oggetto, calcola un percorso, muove l’agente
       ↓
la stessa funzione continua a ogni tick
```

Il sample documentato usa **Qwen3 8B Instruct** e un backend CUDA predefinito.

## Perché NVIDIA parla di “code agent”

NVIDIA distingue due approcci.

### Tool calling

Il modello sceglie funzioni già definite:

```json
{"tool":"move","direction":"north"}
```

È prevedibile e facile da validare, ma la logica complessa richiede molte chiamate e molti tool.

### Code agent

Il modello scrive un programma:

```lua
function update_func(player, ai, monsters, items)
    -- osserva stato, pianifica, conserva logica, agisce
end
```

La funzione può comporre primitive, ramificare, iterare e mantenere uno scopo per più tick. NVIDIA evidenzia che questo permette comportamenti più flessibili e soluzioni non codificate come singoli tool.

## Sicurezza implementata da NVIDIA

Il sample affronta sei classi principali:

1. accesso a filesystem, rete e sistema;
2. esaurimento memoria;
3. stack overflow;
4. loop infinito;
5. manipolazione metatable/prototipi;
6. corruzione dello stato del gioco.

Contromisure:

- caricamento selettivo delle librerie Lua;
- rimozione di `io`, `package`, `debug`, `load`, `require`, funzioni OS pericolose;
- allocator personalizzato con limite;
- debug hook per istruzioni, tempo e profondità chiamate;
- rimozione di `getmetatable`, `setmetatable` e `raw*`;
- userdata C++ per le entità;
- campi built-in in sola lettura;
- metodi controllati per cambiare lo stato;
- shadow tables per stato personalizzato dell’agente.

## Scelte per migliorare il successo dell’LLM

NVIDIA sottolinea che la sandbox non basta. Il design dell’API influenza fortemente la qualità.

Tecniche usate:

- sintassi a metodi, per esempio `ai:move()`;
- firme e tipi chiari;
- esempi espliciti `WRONG` / `RIGHT`;
- ridondanza controllata: metodi preferiti ma funzioni globali mantenute come fallback;
- errore di compilazione/runtime rimandato al modello per un retry;
- prompt più piccolo rispetto a una lunga lista JSON di tool.

Queste scelte sono direttamente applicabili a Worldsmelt.

## Limiti ammessi da NVIDIA

La documentazione è molto utile perché non presenta il sample come perfetto:

- piccoli modelli locali possono fallire comandi composti;
- “uccidi tutti i pipistrelli” può richiedere più ordini semplici;
- stato persistente complesso viene spesso gestito male dagli SLM;
- il code agent è meno prevedibile del tool calling;
- la correttezza non è facilmente verificabile prima dell’esecuzione;
- serve retry e fallback.

NVIDIA propone anche un approccio ibrido con libreria di funzioni curate, ma riconosce che per applicazioni creative aperte la generazione runtime può essere necessaria.

## Somiglianze con Worldsmelt

- LLM locale che scrive Lua;
- programma eseguito ripetutamente nel loop;
- API controllata;
- allocator e instruction budget;
- metatable protette;
- retry con feedback;
- codice generato trattato come non fidato.

## Differenze decisive

| NVIDIA sample | Worldsmelt |
|---|---|
| Comanda un singolo compagno | Genera intere meccaniche di run |
| Dungeon testuale a griglia | Action arcade in tempo reale |
| API di movimento/pathfinding/inventario | Proiettili, collisioni, sinergie, boss, VFX |
| Richiesta del giocatore durante il sample | Generazione prima della run o del piano |
| Qwen3 8B su CUDA predefinito | Obiettivo AMD/NVIDIA/CPU/Steam Deck |
| Dimostrazione tecnica | Prodotto commerciale con contenuti persistenti |
| Correttezza funzionale locale | Anche bilanciamento, leggibilità e 60 FPS |

## Cosa dimostra davvero

NVIDIA conferma che l’architettura **LLM → Lua sandboxato → game loop** è ragionevole e attuale. Non dimostra ancora che:

- un’intera run complessa venga generata rapidamente;
- un 4B sia sufficiente per tutte le meccaniche Worldsmelt;
- il sistema sia pronto per hardware generalista;
- il codice generato sia bilanciato;
- il sample sia una feature di un gioco commerciale già distribuito.

## Implicazione per Worldsmelt

Worldsmelt non deve copiare NVIGI interamente. Deve assorbire le lezioni:

1. API idiomatica e piccola;
2. errori espliciti e retry;
3. metodi oltre a funzioni globali;
4. stato custom protetto;
5. difesa in profondità;
6. test di comandi composti;
7. modello di benchmark basato su comportamento osservabile.

Il backend NVIGI CUDA è inoltre poco adatto alla RX 5600 XT. NVIDIA ha aggiunto plugin GGML Vulkan/D3D12 cross-vendor, ma nelle release notes li classifica ancora sperimentali e non destinati alla produzione. Il percorso `llama.cpp` Vulkan già adottato da Worldsmelt rimane più coerente con l’obiettivo multipiattaforma.
