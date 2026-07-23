# Stato della repository e architettura

## Hardware di riferimento già misurato

Dalla repository:

- Ryzen 5 3600;
- Radeon RX 5600 XT, 6 GB;
- Mesa RADV/Vulkan;
- circa 16 GB di RAM;
- Ubuntu 26.04 nella macchina di benchmark.

### Benchmark LLM esistenti

| Modello | Offload | Tempo totale | Velocità | VRAM Vulkan |
|---|---:|---:|---:|---:|
| Qwen2.5-Coder 1.5B Q4_K_M | 29/29 layer | 28,8 s | 46,0 tok/s | 1,29 GiB |
| Qwen2.5-Coder 7B Q4_K_M | 29/29 layer | 49,6 s | 28,1 tok/s | 4,53 GiB |
| Qwen2.5-Coder 7B Q4_K_M | 20/29 layer | 96,7 s | 15,1 tok/s | 3,30 GiB |

Il 7B entra completamente nella RX 5600 XT. Togliere layer dalla GPU rallenta progressivamente per calcolo CPU e trasferimenti PCIe.

### Benchmark immagini esistenti

La pipeline SD 1.5 pixel-art + LCM produce, sulla macchina di riferimento:

- 512×512;
- 8 step;
- circa 5,3 secondi per sprite;
- circa 75 secondi per un atlas da 12 sprite;
- consumo nell’ordine di circa 2 GB di VRAM documentato nello spike.

## Architettura positiva già presente

Il progetto separa bene le responsabilità:

```text
worldsmelt
  -> melting-gen        (LLM, processo esterno)
  -> melting-sprites    (diffusione, processo esterno)
  -> gioco C            (autorità, rendering, simulazione)
```

Vantaggi:

- i modelli possono essere caricati in sequenza;
- un crash del generatore non deve far cadere il gioco;
- il processo AI può essere terminato e la memoria liberata;
- il manifest/RunBundle può essere pubblicato solo quando completo;
- il fallback resta disponibile.

## Sandbox Lua già progettata e in parte implementata

La specifica della fase 3 identifica correttamente i rischi:

- bytecode non verificato;
- librerie pericolose;
- coroutine che possono sfuggire agli hook;
- `pcall` che può intercettare l’errore del budget;
- pattern stringa eseguiti in C;
- metatable e raw access;
- memoria, ricorsione e loop infiniti;
- determinismo.

La scelta è Lua 5.5 vendorizzato, senza LuaJIT, con:

- caricamento solo testo;
- allowlist costruita dal vuoto;
- allocator con limite;
- instruction budget;
- seed deterministico;
- API a handle;
- disattivazione permanente dello script guasto per la run;
- fallback alla mini-VM o IA C.

## API Lua attuale

L’API esposta oggi comprende principalmente:

- lettura giocatore, nemici, colpi e confini della stanza;
- `spawn_shot`;
- `damage_enemy`;
- `heal_player`;
- `set_enemy_velocity`;
- `add_particle`;
- `nearest_enemy`;
- tratti predefiniti come bounce, homing, explode, split, pierce.

Il prompt attuale impone inoltre che un oggetto attivo definisca esattamente una callback e un solo effetto. Questa è una scelta utile per stabilizzare il primo ciclo, ma è più restrittiva dell’obiettivo finale.

## Gap principale: “test del laser”

Con l’API attuale il modello può comporre colpi e impulsi, ma non può creare un laser continuo vero perché mancano primitive come:

- query su segmento;
- raycast;
- hit list geometrica;
- rendering temporaneo di linee/archi/poligoni;
- entità generiche;
- timer/event scheduler controllato;
- stato persistente strutturato;
- effetti/status generici;
- callback di collisione e distruzione;
- command queue con budget.

Quindi il prossimo obiettivo tecnico non è semplicemente cambiare modello. È ampliare il **Generative Gameplay Kernel** fino a permettere al modello di esprimere meccaniche non previste come archetipi.

## Conflitto documentale da risolvere

Alcuni documenti più vecchi descrivono una DSL/Effect Graph come sistema principale e rinviano Lua. La specifica Lua e l’obiettivo attuale hanno superato quella decisione.

Nuova gerarchia consigliata:

```text
C              = invarianti e primitive universali
JSON/DSL       = dati dichiarativi, layout, statistiche, fallback
Lua sandboxato = comportamento realmente nuovo
```

I documenti storici vanno marcati come superati o aggiornati, altrimenti futuri agenti di sviluppo potrebbero semplificare via la tesi centrale.
