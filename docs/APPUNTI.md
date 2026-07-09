## 1. Stato Attuale del Progetto (Baseline)
Il progetto è un action roguelite top-down scritto in C puro utilizzando Raylib[cite: 6]. 
*   **Architettura Sicura:** L'LLM (tramite OpenAI API) non è interrogato direttamente dal motore C. Un sidecar Node.js gestisce le chiamate API e salva i risultati in file locali[cite: 4].
*   **Pipeline Dati:** L'API Text genera un file JSON che viene parsato in un manifest testuale `.txt`[cite: 4]. Il motore C legge questo manifest all'avvio della partita[cite: 6].
*   **Asset Visivi:** L'API Image genera uno spritesheet tecnico 1024x1024[cite: 4]. Il motore C ritaglia celle fisse da 128x128 e applica un chroma-key sui pixel quasi neri per lo sfondo[cite: 5, 6].
*   **Logica Sandbox Attuale:** Attualmente si usa una mini-VM dichiarativa (es. `on_fire:burst,3,0.36,split`) invece di codice sorgente arbitrario per mantenere la sicurezza e la stabilità in C[cite: 4].

## 2. Obiettivi di Sviluppo (Integrazioni Future)
L'obiettivo è espandere l'engine introducendo una logica più complessa per nemici, sinergie e interfaccia, sfruttando strumenti e script generati proceduralmente:
*   **Transizione a Sandbox Lua:** Integrare un ambiente Lua sicuro all'interno del C per permettere all'IA di generare logiche matematiche complesse (come `onTearUpdate`) basandosi sull'ispirazione della Modding API ufficiale di Isaac (IsaacDocs).
*   **Integrazione LLM Locale:** Passare gradualmente da API esterne a inferenza locale su CPU (Small Language Models e Stable Diffusion quantizzati) usando librerie bare-metal come `gguf-tools` e `llama.cpp` per caricare i modelli solo nella schermata di inizio run.
*   **UI Dinamica:** Utilizzare `raygui` (libreria immediate-mode per Raylib) per gestire i menu, le schermate di pausa e le barre di caricamento dell'IA.

## 3. Architettura per la Generazione di Nemici e Boss
Per permettere all'LLM di inventare boss folli senza compilare codice C dinamicamente o causare crash di memoria, implementeremo un **Sistema a Componenti (Part System)** guidato da JSON/Lua:

### A. Struttura in C (Componenti Rigidi)
Il boss non è una singola texture, ma un array di "parti" indipendenti gestite in C:
```c
typedef enum { PART_CORE, PART_HEAD, PART_ARM, PART_LEG, PART_TENTACLE, PART_WING } PartType;

typedef struct {
    PartType type;
    Texture2D texture; // Ritagliata dall'atlas 128x128
    Vector2 offset_locale; 
    float rotazione;
    float timer_attacco;
    float cooldown_attacco;
    char script_attacco_lua[64]; // Callback generata dall'LLM
} MonsterPart;

typedef struct {
    Vector2 posizione_mondo;
    float hp;
    MonsterPart parti[32]; // Array fisso per sicurezza della memoria
    int numero_parti;
} Boss;

```

### B. Output dell'LLM (Configurazione Anatomica)

All'inizio del piano, il prompt dell'IA (tramite il sidecar Node) genera la "scheda tecnica" del boss, assegnando gli offset, gli sprite e i comportamenti alle parti:

```json
{
  "nome": "Cyber Kraken",
  "parti": [
    { "type": "PART_CORE", "offset": {"x": 0, "y": 0} },
    { "type": "PART_TENTACLE", "offset": {"x": -30, "y": 10}, "script_attacco": "Attack_Laser_Sweep", "cooldown": 2.5 }
  ]
}

```

### C. Animazione e Pattern in C

* **Animazione Procedurale:** In C, non usiamo sprite animati fotogramma per fotogramma per le appendici. Usiamo funzioni matematiche (`sinf`, `cosf` incluse in `raymath.h`) per far oscillare tentacoli o ali basandoci sul tempo di gioco (`GetTime()`).
* **Esecuzione Pattern:** Nel game loop in C, quando il `timer_attacco` di una specifica parte (es. il tentacolo) scade, il C richiama la funzione Lua `Attack_Laser_Sweep` (generata dall'IA) passando la posizione del giocatore e l'offset dell'arto.

## 4. Sistema di Sinergie Visive e Composizione a Layer

Per evitare di far generare all'IA centinaia di frame di animazione per ogni combinazione di oggetti:

* Il giocatore (struct `Player`) è scomposto in layer: corpo, testa, occhi, accessorio.


* L'IA, nel definire un oggetto, assegna un prompt visivo e un livello Z-Index (Priorità).
* Se il giocatore raccoglie un oggetto (es. "Lacrime Infuocate"), il C aggiorna unicamente il layer corrispondente (es. `player.occhi = texture_occhi_fuoco`), sovrapponendo lo sprite sopra l'animazione di base pre-programmata in C.

## 5. Linee Guida di Sicurezza e Licenze

* **Memoria e Prestazioni:** I modelli IA (se locali) devono essere caricati in RAM (`llama_load_model`) esclusivamente durante il caricamento del piano, e scaricati (`llama_free`) prima dell'inizio del gameplay a 60 FPS.
* **Sicurezza Sandbox:** Gli script Lua generati non avranno accesso al file system o all'OS. Il prompt per la generazione del codice sarà fortemente tipizzato, obbligando l'LLM a compilare template prefissati.
* **Licenze (Raylib):** Raylib usa licenza zlib/libpng, permettendo la vendita del file eseguibile chiuso (.exe) senza royalty. È obbligatorio solo includere i crediti dell'autore originale (Ramon Santamaria) in un file `CREDITS.txt`.

```
## 6. Gestione Avanzata delle Sinergie (Visive e Logiche)

La vera forza del motore risiede nella combinazione procedurale degli oggetti, che modificano sia l'estetica del giocatore che le meccaniche di gioco.

### A. Il Modello Base: La "Tela Vuota"
Per far sì che l'equipaggiamento e le mutazioni risultino visivamente puliti, l'IA responsabile della generazione visiva (Image API) non deve generare un personaggio dettagliato. 
*   Il prompt iniziale per il personaggio base deve richiedere un modello minimalista (un manichino o uno "stickman" quasi nudo)[cite: 4, 6].
*   Tutti i dettagli (vestiti, armi, accessori) verranno aggiunti dinamicamente come livelli (layer) 128x128 sovrapposti a questa base tramite il codice C[cite: 4, 6].

### B. Mutazioni e Sovrapposizioni Visive
Il sistema a componenti (Layered Sprite) permette non solo di aggiungere oggetti, ma di **sostituire (override)** parti anatomiche del personaggio.

*   **Aggiunte:** Un oggetto come "Cappello Mago" si sovrappone al layer `oggetto_testa`.
*   **Mutazioni Fisiche (Morphing):** Un oggetto estremo (es. "Testa di Mosca") non si aggiunge, ma *sostituisce* il layer `testa` di base del personaggio. 
*   **Conflitti di Slot (Sinergia Visiva):** Se il giocatore raccoglie due oggetti per lo stesso slot (es. occhiali di fuoco e occhiali di ghiaccio), il sistema gestisce il conflitto *Tramite LLM (Consigliato all'inizio della run):* Lo script Node rileva il conflitto nel JSON e genera un prompt ibrido per l'API Image (es. *"Occhiali con lente sinistra infuocata e destra congelata"*), generando un singolo sprite fuso perfetto.

### C. Sinergie Logiche (Codice LLM/Lua)
L'LLM non si limita a disegnare, ma genera script che controllano l'inventario del giocatore per creare regole sinergiche uniche.

Nel file di configurazione generato dall'IA per gli oggetti, è presente una funzione di callback (es. `onItemPickup` o `evaluateCache`) che istruisce il motore di gioco su cosa fare quando più oggetti interagiscono.

**Esempio di Sinergia Generata (Lua Sandbox):**
```lua
-- Script generato dall'LLM per gestire la logica combinata
function onEvaluateSynergy(player)
    -- Controlla se il giocatore ha sia l'oggetto Fuoco che Ghiaccio
    if player:HasItem("occhi_fuoco") and player:HasItem("occhi_ghiaccio") then
        -- L'IA inventa una nuova meccanica: Lacrime di Ossidiana
        player.tear_type = "obsidian_shards"
        player.tear_damage = player.tear_damage * 1.5
        player.tear_effect_flags = bit32.bor(FLAG_BURN, FLAG_SLOW)
        
        -- Forza il C ad applicare l'effetto visivo ibrido
        player:SetVisualLayer("occhi", "texture_ibrida_fuoco_ghiaccio")
    end
end

### D. Flusso di Aggiornamento nel Motore C

Quando il Lua modifica le statistiche o i layer visivi, il motore C (che rimane il padrone assoluto della sicurezza e delle risorse) si occupa di applicare queste direttive nel game loop:

1. Il giocatore raccoglie un item.
2. Il C invoca lo script dell'LLM (o la mini-VM corrente).
3. L'LLM legge lo stato, calcola la sinergia e restituisce i nuovi parametri.
4. Il C aggiorna i puntatori alle `Texture2D` dei layer e i moltiplicatori di danno/velocità, applicandoli al frame successivo.

```
***


## 7. Evoluzione Visiva e HUD (Da Debug a Stile Isaac)

Attualmente l'HUD e il rendering sono impostati per il debug (visuale piatta 2D e output testuale). Per ottenere l'estetica 2.5D (prospettiva 3/4) e un feeling viscerale, il motore C e le API di generazione devono implementare i seguenti accorgimenti grafici:

### A. Profondità e Prospettiva 2.5D
Il motore di rendering in C deve ingannare l'occhio per creare l'illusione della profondità senza usare il 3D:
*   **Drop Shadow (Ombre):** Ogni entità dinamica (giocatore, nemici, proiettili, pickup) deve avere un'ombra proiettata. In C, prima di chiamare la funzione di disegno dello sprite, usa `DrawEllipse()` per disegnare un ovale nero semitrasparente esattamente alle coordinate dei "piedi" dell'entità.
*   **Vignettatura (Vignette):** Aggiungere un gradiente radiale scuro o una texture overlay semitrasparente lungo i bordi della stanza (viewport centrale) per simulare l'oscurità di uno scantinato.
*   **Pavimento Testurizzato:** Sostituire il rettangolo a colore solido con una texture generata proceduralmente (es. mattonelle, legno, terra) fornita dall'API Image.

### B. Struttura Fisica delle Pareti e delle Porte
La stanza non deve essere definita dai limiti dello schermo, ma da sprite che simulano una "scatola":
*   **Spessore delle Pareti:** La Parete Nord (Top) deve mostrare la "faccia" del muro (es. pietre impilate viste frontalmente). Le Pareti Laterali e la Parete Sud mostrano solo il bordo o uno spessore inclinato.
*   **Porte (Cornice e Vuoto):** Le porte non sono rettangoli colorati, ma veri e propri "buchi" nel muro. La struttura visiva di una porta richiede:
    1. Uno sfondo nero (il vuoto della stanza successiva).
    2. Uno sprite che funge da cornice/telaio.
    3. Uno sprite sovrapposto (es. sbarre di ferro o assi di legno) per lo stato "Chiusa/Bloccata".

## 8. Pipeline Grafica Avanzata: ControlNet Union + LoRA Pixel Art

Per garantire che gli asset generati dall'IA rispettino matematicamente le hitbox, le proporzioni dei layer e i template di animazione senza "allucinazioni" geometriche, l'engine implementa una pipeline SDXL controllata.

### A. I Componenti della Pipeline
La generazione visiva all'inizio della run sfrutta tre moduli compatibili con l'architettura SDXL 1.0, tutti autorizzati per uso commerciale:
1.  **Base Model:** `SDXL Lightning GGUF (4-step)` -> Gestisce la struttura iniziale in pochi passaggi CPU.
2.  **Conditioning Engine:** `ControlNet Union SDXL 1.0 (Apache-2.0)` -> Vincola la forma dello sprite a un'immagine di riferimento passata dal codice.
3.  **Styling Filter:** `Pixel Art XL LoRA (.safetensors)` -> Forza l'output a conformarsi a una palette e a una griglia pixel art a 16-bit.

### B. Flusso di Generazione della Maschera (Hitbox-Safe)
Invece di affidarsi al puro Text-to-Image, il motore in C sfrutta il ControlNet passando un'immagine di input (es. una silhouette binaria o un file di contorni generato proceduralmente in C):

```text
[Sagoma Grezza C / Hitbox] ──> [ControlNet Union] ──┐
                                                    ├──> [Sprite Finale Perfetto]
[Prompt di Testo LLM]     ──> [SDXL Base + LoRA] ──┘