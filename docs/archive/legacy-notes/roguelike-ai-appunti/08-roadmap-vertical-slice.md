# 08 — Roadmap della vertical slice

## Scope congelato proposto

- un personaggio base;
- un solo stile grafico;
- un primo piano stabile di circa 10 minuti;
- un secondo piano generato;
- un boss stabile che accetta modificatori validati;
- 20 oggetti;
- quattro pool con identità provvisoria;
- sistema di sinergie Effect Graph;
- trasformazione visiva del personaggio;
- adattamento fra piani;
- replay locale dello stesso bundle;
- niente multiplayer realtime.

## Composizione dei 20 oggetti

- 8 anchor item autoriali;
- 8 oggetti con Effect Graph di livello 2;
- 4 oggetti signature con macchina a stati limitata.

| Pool provvisoria | Totale | Anchor | Livello 2 | Livello 3 |
|---|---:|---:|---:|---:|
| Offensiva/esplorazione | 8 | 3 | 4 | 1 |
| Negozio/utilità | 4 | 2 | 2 | 0 |
| Rischio-ricompensa | 4 | 1 | 2 | 1 |
| Difensiva | 2 | 1 | 0 | 1 |
| Boss/evoluzione | 2 | 1 | 0 | 1 |

Nota: la tabella usa cinque famiglie funzionali; nell’interfaccia possono essere accorpate in quattro pool.

I 20 oggetti devono coprire:

- forma del proiettile;
- traiettoria;
- collisione;
- payload/status;
- eventi e proc;
- contatori/stati;
- mutazione corporea;
- costo o rischio.

## Fase 0 — Benchmark e stanza campione

Costruire:

- una stanza 640×360;
- personaggio 64×64 e atlas 96×96;
- dieci nemici;
- cap di proiettili e VFX;
- HUD provvisorio;
- animazioni base;
- misure di frame time.

Eseguire sul PC Ubuntu:

- Qwen: modello, quantizzazione, contesto, tok/s, RAM e VRAM;
- SD: risoluzione, step, tempo/immagine, RAM e VRAM;
- carico/scarico sequenziale;
- gioco sotto carico mentre il generatore è fermo;
- generazione fra stanze.

Gate:

- 60 FPS stabili senza generazione;
- cap di proiettili definito;
- frame time e 1% low registrati;
- tempo reale disponibile per preparare il piano 2.

## Fase 1 — Effect Graph senza AI

Implementare:

- schema;
- type checker;
- normalizzatore;
- runtime;
- budget per tick;
- tooltip derivato;
- test headless;
- 8 anchor item.

Gate:

- tutte le coppie anchor terminano;
- nessun effetto cancellato in silenzio;
- stessa seed produce stesso stato nel medesimo build;
- stress test a 60 FPS.

## Fase 2 — Qwen propone, il motore decide

Implementare:

- prompt strutturato;
- GBNF/JSON Schema;
- Capability Registry;
- retry con errore;
- fallback;
- log accettati/rifiutati;
- Novelty Ledger;
- altri 12 oggetti.

Gate:

- 20 oggetti validi;
- 190 coppie;
- 1.000 triple;
- tasso di compilazione e retry misurato;
- nessun bisogno di Lua per completare lo scope.

## Fase 3 — Fenotipo visivo

Implementare:

- frame base e socket;
- Appearance Contribution;
- Phenotype Compiler;
- Visual Dominance Budget;
- ricomposizione atlas al pickup;
- componenti certificati di fallback;
- sprite proiettili e icone.

Gate:

- palla rimbalzante più spine chiaramente visibile;
- un tratto dominante leggibile per ogni oggetto;
- personaggio ancora leggibile con 10 oggetti;
- nessuna inferenza nel combattimento;
- 60 FPS nello stress test.

## Fase 4 — Style LoRA e pipeline SD

Implementare:

- dataset ledger;
- baseline di 30 prompt;
- Style LoRA;
- Item LoRA;
- post-processing;
- validatori alpha/palette/silhouette;
- cache per ruolo.

Gate:

- valutazione cieca migliore della baseline;
- output utilizzabile in-engine, non soltanto bello isolato;
- tasso di fallback visivo accettabile;
- costo per esperimento registrato.

Non addestrare ancora Qwen.

## Fase 5 — Primo piano e AI Director

Implementare:

- piano stabile da 10 minuti;
- telemetria;
- PlayerStyleProfile;
- PerformanceProfile separato;
- timeline di generazione;
- pubblicazione atomica;
- bundle fallback.

Gate:

- piano 2 pronto nel 95% delle prove sul PC di sviluppo;
- nessun blocco alla fine del piano;
- adattamento percepibile ma non punitivo;
- profilo disattivabile.

## Fase 6 — RunBundle e replay

Implementare:

- serializzazione canonica;
- hash;
- versioni;
- input log;
- state hash;
- replay;
- confronto di due esecuzioni.

Gate:

- replay completo senza divergenza sul build target;
- bundle esportabile;
- errore riproducibile allegando il bundle.

## Fase 7 — Playtest della tesi

Domande ai tester:

- il gioco è divertente senza sapere che usa AI?
- i tooltip sono affidabili?
- le sinergie sembrano scoperte o casuali?
- le mutazioni spiegano il gameplay?
- il piano adattivo sembra personale o manipolato?
- dopo dieci oggetti il personaggio resta leggibile?
- una run mediocre resta comunque giocabile?

Go/no-go:

- se il gameplay non regge con gli anchor item, fermare il training e migliorare il gioco;
- se la DSL produce varietà ma non sinergie, migliorare algebra e test;
- se la grafica è incoerente, ridurre ruoli e stile, non aggiungere modelli;
- se il piano 2 non è pronto, aumentare banking e fallback;
- se Qwen richiede troppi retry, costruire dataset e solo allora valutare QLoRA.

## Revisioni rispetto al piano strategico iniziale

Confermati:

- RunBundle;
- primo piano stabile;
- generazione in background;
- local-first;
- benchmark reale;
- cache e fallback;
- separazione fra generazione e simulazione.

Modificati:

- da codice Lua generato come percorso principale a DSL tipizzata;
- da molti asset completi generati a componenti modulari;
- da trasformazioni visive illimitate a budget di dominanza;
- da “stesso seed” a “stesso bundle” per il competitivo;
- da più checkpoint SD completi a base più LoRA;
- da fine-tuning Qwen iniziale a raccolta dati prima del training.

Queste modifiche non riducono la visione. Concentrano la novità nella parte che il giocatore percepisce e che il motore può garantire.

