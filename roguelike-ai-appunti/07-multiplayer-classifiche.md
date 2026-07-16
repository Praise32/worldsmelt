# 07 — Multiplayer e classifiche

## Principio fondamentale

Lo stesso modello e lo stesso seed non garantiscono lo stesso output su hardware, backend o versioni diverse. Per una gara equa bisogna distribuire lo stesso RunBundle già generato e congelato.

Il modello crea il contenuto. La gara consuma il contenuto.

## Modalità

| Modalità | Contenuto | Adattamento | Classifica |
|---|---|---|---|
| Mirror Race | stesso RunBundle | gameplay disattivato; cosmetica facoltativa | ranked principale |
| Crossed AI Duel | due mondi A e B giocati da entrambi | congelato prima di ogni manche | ranked sperimentale |
| Chaos Race | mondo diverso e personalizzato | attivo | casual |
| Daily/Weekly | bundle fisso per il periodo | disattivato | asincrona |
| Model Battle | bundle prodotti da modelli/versioni diverse | regole dell’evento | classifica separata |

## Mirror Race

È la prima modalità competitiva da progettare.

- stesso bundle;
- stesso bilanciamento;
- stessi oggetti;
- stessa disposizione;
- stessi seed interni;
- cronometro basato sui tick;
- pausa disabilitata o regolata ugualmente;
- adattamento personale del gameplay disattivato.

La grafica può essere precomposta per ogni macchina, ma non deve cambiare hitbox, collisioni o statistiche.

## Crossed AI Duel

Permette di usare mondi diversi senza sacrificare completamente l’equità.

- Mondo A costruito dal profilo del giocatore 1.
- Mondo B costruito dal profilo del giocatore 2.
- Giocatore 1 esegue A e poi B.
- Giocatore 2 esegue B e poi A.
- Vince il tempo o punteggio aggregato.
- Un mancato completamento riceve una penalità fissa dichiarata.

Ordini opposti riducono l’effetto apprendimento. Entrambi affrontano comunque entrambe le difficoltà.

## Chaos Race

È la modalità che esprime meglio la visione AI:

- ogni giocatore ha un mondo proprio;
- il Director si adatta;
- gli oggetti possono essere diversi;
- vince chi completa prima o ottiene più punteggio.

Non è una misura pura di abilità. Va presentata come gara caotica/casual o deve usare handicap e punteggi normalizzati molto ben testati. Non mescolare i risultati con Mirror Race.

## Regole ranked

- nessuna generazione durante il combattimento;
- nessun adattamento individuale dopo il via;
- fixed timestep 60 Hz;
- collisioni determinate dalla DSL;
- RunBundle canonico;
- classifica separata per versione di gioco;
- classifica separata quando cambia schema DSL o bilanciamento;
- replay obbligatorio per un risultato verificabile;
- hash periodico dello stato.

## Contenuto del RunBundle competitivo

- versione formato;
- versione eseguibile e bilanciamento;
- RunBible;
- mappa e stanze;
- nemici e boss;
- Item Definition;
- Effect Graph e sinergie;
- seed principale e seed per sottosistemi;
- asset manifest con checksum;
- provenienza di modello, LoRA e prompt;
- hash SHA-256 della serializzazione canonica.

Non basta inserire il seed. Il contenuto generato deve essere già presente.

## Replay

Registrare:

- hash del bundle;
- versione esatta del gioco;
- input associati al tick;
- hash di stato a intervalli;
- numero totale di tick;
- tempo monotono;
- risultato.

Il replay viene rieseguito dallo stesso simulatore. Il primo hash divergente indica dove iniziare la diagnosi. Gli hash controllano integrità e riproducibilità, ma non sostituiscono un anti-cheat completo.

## Vertical slice multiplayer

Includere soltanto:

- esportazione/importazione di un bundle;
- due esecuzioni locali o asincrone dello stesso bundle;
- cronometro a tick;
- input recording;
- hash periodici;
- replay;
- schermata confronto risultati;
- identificativo bundle visibile.

Rimandare:

- networking in tempo reale;
- matchmaking;
- account;
- classifica pubblica;
- rollback;
- spettatori;
- anti-cheat remoto;
- determinismo multipiattaforma certificato;
- Crossed Duel completo;
- Chaos ranked.

La milestone non è “due giocatori online”. È:

> Due esecuzioni dello stesso bundle producono la stessa simulazione e un replay verificabile.

