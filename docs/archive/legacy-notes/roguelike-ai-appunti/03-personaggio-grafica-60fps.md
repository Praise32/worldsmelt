# 03 — Personaggio modulare, grafica e 60 FPS

## 60 FPS non significa 60 disegni al secondo

La simulazione può aggiornarsi 60 volte al secondo e lo schermo può essere ridisegnato a 60 FPS, mentre una camminata pixel art usa 8 immagini mostrate a 10–12 fotogrammi artistici al secondo.

Decisione consigliata:

- simulazione: fixed timestep 60 Hz;
- rendering: target 60 FPS;
- interpolazione visiva facoltativa;
- logica, hitbox e origine dei colpi indipendenti dal frame artistico.

## Risoluzione iniziale

Proposta da validare visivamente:

- canvas virtuale: 640×360, formato 16:9;
- output: 1280×720, 1920×1080 o superiore con scala intera quando possibile;
- filtro nearest-neighbor;
- personaggio base: 64×64 px;
- cella atlas: 96×96 px per non tagliare spine, corna e appendici;
- icona oggetto: 32×32 o 48×48 px;
- proiettili: classi 16×16, 24×24 e 32×32 px.

Se i test mostrano che il personaggio occupa troppo spazio, si può scendere a 48×48 senza cambiare architettura. Non conviene scegliere la risoluzione definitiva prima di costruire una stanza campione con giocatore, dieci nemici, proiettili e HUD.

## Frame consigliati

| Animazione | Frame unici | Velocità artistica |
|---|---:|---:|
| Idle | 4 | 6 FPS |
| Camminata | 8 | 10–12 FPS |
| Attacco | 6 | 12–15 FPS |
| Colpito | 2–3 | breve |
| Morte o trasformazione maggiore | 8–10 | circa 12 FPS |

Direzioni iniziali:

- fronte;
- retro;
- lato destro;
- lato sinistro ottenuto per mirroring.

L’aim può essere un layer separato o una direzione quantizzata a otto angoli. Otto camminate complete moltiplicherebbero dataset, QA e composizione senza provare meglio il concept.

## Che cosa significa “rig modulare” in pixel art

Non serve iniziare con uno scheletro deformabile come in un gioco 3D. Il rig v0 è un insieme di metadati condivisi da ogni frame:

- pivot del corpo;
- socket per testa, schiena, mani, lati e aura;
- origine dei proiettili;
- maschere per pelle, occhi, torso e testa;
- parametri di silhouette;
- aree consentite per appendici;
- ordine dei layer;
- hitbox separata.

Ogni accessorio usa gli stessi socket su tutti i frame. Questo rende accettabile e consigliato il personaggio modulare: conserva la qualità artigianale dell’animazione base, mentre le mutazioni vengono applicate con regole.

## Composizione efficiente

Al pickup:

1. il Phenotype Compiler raccoglie gli Appearance Contributions;
2. risolve fusioni e dominanza;
3. ricompone tutti i frame in un nuovo atlas;
4. salva un Phenotype Snapshot;
5. il renderer usa la texture già composta.

Durante il combattimento non vengono disegnati dieci accessori indipendenti per ogni frame e non viene invocata SD. Una trasformazione pesante avviene tra stanze o dentro un’animazione protetta.

## Visual Dominance Budget

Mostrare soltanto i tratti dominanti è la scelta giusta. Ogni oggetto resta tracciato, ma non richiede un accessorio separato.

Budget iniziale del personaggio: 100 punti.

| Canale | Budget |
|---|---:|
| Silhouette principale | 35 |
| Appendici secondarie | 25 |
| Superficie e decal | 15 |
| Palette e materiale | 10 |
| Aura e VFX | 15 |

Limiti leggibili:

- una trasformazione dominante della silhouette;
- due appendici secondarie;
- tre dettagli minori;
- una trasformazione di palette o materiale;
- un’aura principale.

Pesi indicativi:

- comune: 5–10;
- raro: 12–20;
- leggendario o trasformazione: 25–35.

Quando il budget è pieno, il compilatore:

1. fonde tratti compatibili;
2. intensifica un motivo esistente;
3. comprime più contributi in una mutazione;
4. sposta un segnale su proiettile, trail o impatto;
5. riduce la prominenza senza perdere la Contribution Trace.

Tre effetti velenosi, per esempio, diventano una sola mutazione organica più forte.

## Budget separato del proiettile

Il proiettile è spesso il segnale più importante per il gameplay:

| Canale | Budget |
|---|---:|
| Forma | 40 |
| Superficie | 20 |
| Movimento e trail | 20 |
| Impatto e VFX | 20 |

Questo permette di mostrare una palla spinata che rimbalza anche se il corpo del personaggio ha già raggiunto il proprio limite visivo.

## Pipeline SD 1.5 consigliata

SD genera componenti sorgente, non ogni combinazione finale:

1. prompt strutturato dalla RunBible;
2. SD 1.5 a 256 o 512 px;
3. rimozione o verifica dello sfondo;
4. centratura e controllo della silhouette;
5. riduzione deterministica;
6. palette comune;
7. pulizia di pixel isolati;
8. validazione di dimensioni, alpha e socket;
9. inserimento nell’atlante.

Il sito ufficiale di [Retro Diffusion](https://www.retrodiffusion.ai/) descrive un prodotto che combina modelli, reference, animazione, tileset, palette ed editing. La qualità-obiettivo va quindi trattata come una pipeline completa, non come il risultato automatico di una singola LoRA.

## Una base, più LoRA di ruolo

Non conviene mantenere quattro checkpoint completi di SD 1.5. Percorso consigliato:

- un checkpoint base;
- una Style LoRA comune;
- una LoRA item;
- una LoRA projectile/VFX;
- una LoRA player mutation;
- in seguito una LoRA enemy o animation, soltanto se i test lo giustificano.

Le LoRA possono essere caricate o fuse per batch. La palette, i contorni, la direzione della luce e la prospettiva devono restare responsabilità della Style LoRA e del post-processing comune.

## Budget prestazionale

Target da misurare:

- atlas ricomposto soltanto al pickup o al cambio stanza;
- massimo una texture composta principale per personaggio;
- pool e cap per proiettili/VFX;
- batch per sprite compatibili;
- nessuna generazione nel frame;
- degradazione controllata: se manca un asset, usa una parte certificata e conserva la meccanica;
- benchmark con 20 oggetti attivi e massimo numero di proiettili previsto;
- misurare frame time medio, 1% low e picco, non soltanto FPS medi.

