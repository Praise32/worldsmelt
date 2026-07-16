# Appunti di sviluppo — roguelike AI locale

Aggiornamento: 16 luglio 2026

Questa cartella trasforma il piano strategico iniziale e le decisioni emerse nella discussione in una specifica di lavoro. Non è ancora la documentazione del codice: è il riferimento architetturale da usare prima di modificare il progetto.

## Verdetto sintetico

Il progetto è fattibile come vertical slice, ma la promessa corretta non è “l’AI inventa qualunque codice durante il combattimento”. La promessa sostenibile è:

> Un roguelike d’azione con regole solide, nel quale l’AI compone prima del piano oggetti, sinergie, aspetto e variazioni del mondo all’interno di un linguaggio molto espressivo e verificabile.

Le decisioni principali sono:

- simulazione fissa a 60 Hz e rendering a 60 FPS;
- animazioni pixel art a pochi frame artistici, mostrate dentro il rendering a 60 FPS;
- risoluzione virtuale iniziale 640×360, scalata in modo pixel-perfect;
- personaggio modulare a frame, socket, maschere e deformazioni controllate;
- massimo pochi tratti visivi dominanti; gli altri vengono fusi in palette, materiali, aura, trail e impatti;
- 20 oggetti nella prima vertical slice;
- primo piano stabile di circa 10 minuti, usato anche per misurare lo stile di gioco e preparare il secondo;
- DSL tipizzata a tre livelli come percorso principale; Lua generato rimandato a un laboratorio di sviluppo;
- Qwen non va addestrato subito;
- SD 1.5 può essere specializzato prima con una LoRA di stile e poi con LoRA di ruolo;
- training serio su GPU NVIDIA a noleggio; RX 5600 XT usata soprattutto per inferenza, integrazione e prove ridotte;
- multiplayer ranked basato sullo stesso RunBundle, non sullo stesso seed o sullo stesso modello.

## Indice

1. [Visione e confini del prodotto](01-visione-e-confini.md)
2. [Architettura, sinergie e DSL](02-architettura-sinergie-dsl.md)
3. [Personaggio modulare, grafica e 60 FPS](03-personaggio-grafica-60fps.md)
4. [AI Director e adattamento al giocatore](04-ai-director-adattamento.md)
5. [Dataset e licenze](05-dataset-e-licenze.md)
6. [Training dei modelli, hardware e costi GPU](06-training-hardware-costi.md)
7. [Multiplayer e classifiche](07-multiplayer-classifiche.md)
8. [Roadmap della vertical slice](08-roadmap-vertical-slice.md)
9. [Decisioni e domande aperte](09-decisioni-domande-aperte.md)
10. [Fonti verificate](10-fonti-verificate.md)

## Come leggere le note

Ogni affermazione rientra in una delle seguenti categorie:

- **Fatto verificato**: supportato da documentazione ufficiale collegata.
- **Decisione consigliata**: scelta architetturale proposta per questo progetto.
- **Stima**: ordine di grandezza da confermare con un benchmark sul PC Ubuntu.

Le cifre dei noleggi GPU sono una fotografia del 16 luglio 2026 e possono cambiare. I costi di training nei documenti non sono preventivi: sono scenari costruiti moltiplicando ore GPU e tariffa pubblicata.

## Risultato che deve dimostrare la vertical slice

La prova non deve dimostrare “contenuto infinito”. Deve dimostrare contemporaneamente che:

1. raccogliere due oggetti produce una sinergia meccanica leggibile;
2. l’aspetto di giocatore e proiettili comunica quella sinergia;
3. l’output di Qwen è validabile e non rompe la simulazione;
4. il piano successivo è pronto senza fermare il giocatore;
5. il combattimento resta stabile a 60 FPS;
6. una run resta divertente anche quando il contenuto generato è soltanto discreto.

