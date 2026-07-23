# 01 — Visione e confini del prodotto

## Identità

The Binding of Isaac è il riferimento del loop, non l’identità da copiare. Il progetto deve prendere:

- stanze brevi e leggibili;
- scelta tra pool con carattere diverso;
- oggetti che cambiano davvero il modo di combattere;
- sinergie cumulative;
- rischio, ricompensa e run ripetibili.

Deve invece costruire nomi, simboli, tono, narrativa, silhouette, nemici e rituali propri. Una direzione possibile è fare del corpo del personaggio una “cronaca vivente” della run: ogni potere lascia una mutazione, ma il compilatore visivo protegge la leggibilità.

## I cinque pilastri

### 1. Base stabile

Movimento, collisioni, danni, invulnerabilità, porte, pickup, economia e regole fondamentali sono codice del gioco. L’AI non può cambiarli arbitrariamente.

### 2. Novità composta

Qwen non crea ogni volta una nuova funzione C. Compone primitive generali in un Effect Graph tipizzato. La novità nasce dalla combinazione di eventi, geometrie, stati, payload e costi.

### 3. Sinergia meccanica e visiva

La stessa struttura semantica alimenta:

- il comportamento del proiettile;
- la descrizione dell’oggetto;
- la mutazione del personaggio;
- il suono e il VFX;
- i test automatici.

La grafica non deve inventare il significato dell’oggetto: deve visualizzare il significato già validato.

### 4. Adattamento trasparente

Il gioco osserva preferenze come distanza, aggressività, rischio e uso delle risorse. Il piano successivo combina contenuti che valorizzano, sfidano ed espandono quello stile. Non modifica segretamente statistiche già acquisite e non “bara” dentro una stanza.

### 5. Local-first con fallback

Qwen e SD lavorano in locale, ma ogni fase generativa ha:

- cache;
- timeout;
- validazione;
- asset e comportamenti di fallback;
- bundle pubblicato solo quando completo.

Una run non deve mai finire perché un modello non ha terminato.

## Cosa significa davvero “AI-centrico”

L’AI è centrale se decide e compone una parte importante dell’esperienza, non se viene invocata ogni frame. Nel progetto dovrebbe avere quattro ruoli distinti:

| Ruolo | Modello/sistema | Quando |
|---|---|---|
| Autore di regole | Qwen | prima del piano |
| Direttore | telemetria locale + Qwen | durante la preparazione del piano successivo |
| Artista di componenti | SD 1.5 + LoRA | fuori dal combattimento |
| Critico | simulatori e validatori deterministici | prima di accettare il bundle |

Il motore resta l’autorità. Il modello propone; compilatore e test decidono.

## Ciclo consigliato della run

1. Nel menu il gioco prova a preparare o recuperare dalla cache un RunBundle.
2. La nuova run parte in pochi secondi con il primo piano stabile.
3. Nei primi minuti vengono raccolte metriche sullo stile del giocatore.
4. Qwen prepara RunBible, oggetti e grafi del piano 2.
5. I validatori compilano, simulano e correggono i candidati.
6. Qwen viene scaricato dalla VRAM.
7. SD genera solo i componenti visivi mancanti, per priorità.
8. Il bundle del piano 2 viene pubblicato atomicamente.
9. Alla morte o nei menu il sistema continua il banking di bundle futuri.

Sulla RX 5600 XT Qwen e SD devono essere considerati carichi sequenziali. Il gioco non deve contare sulla loro co-residenza in 6 GB.

## Cosa non includere nella prima versione

- generazione di codice nativo;
- inferenza AI durante il combattimento;
- nemici, boss, stanze, animazioni e UI tutti generati insieme;
- networking in tempo reale;
- classifiche pubbliche e anti-cheat;
- otto direzioni animate a mano;
- più personaggi base;
- più stili grafici;
- full fine-tuning di SD o Qwen;
- riproduzione dell’intera piattaforma Retro Diffusion.

Questi elementi allargherebbero la superficie del problema prima di avere provato la tesi centrale: sinergia meccanica più trasformazione visiva.

## Metriche di successo

Target iniziali, da rivedere dopo il benchmark:

- 60 FPS target e simulazione fissa a 60 Hz;
- nessuna inferenza nel percorso critico di un frame;
- piano 2 pronto prima della fine del piano 1 nel 95% delle prove sul PC di sviluppo;
- zero crash causati da contenuto generato;
- meno del 2% dei candidati finali costretto al fallback dopo retry e validazione;
- tutte le 190 coppie dei 20 oggetti eseguite in test automatico;
- almeno 1.000 triple campionate;
- stessa serializzazione di un RunBundle uguale allo stesso hash;
- descrizione dell’oggetto derivata dal comportamento, non scritta indipendentemente;
- playtester capaci di spiegare almeno il tratto dominante di un oggetto guardando giocatore o proiettile.

