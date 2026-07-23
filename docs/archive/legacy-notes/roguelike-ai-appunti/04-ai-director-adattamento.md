# 04 — AI Director e adattamento al giocatore

## Principio

L’adattamento non richiede un modello che osservi ogni frame. L’engine raccoglie numeri semplici e riproducibili, costruisce un PlayerStyleProfile e passa a Qwen soltanto un riepilogo compatto.

Questo protegge:

- 60 FPS;
- privacy;
- determinismo;
- costo di contesto;
- leggibilità delle decisioni.

## Segnali da raccogliere

- distanza media e mediana dai nemici;
- tempo fra ingresso nella stanza e primo attacco;
- precisione e dispersione;
- quota di movimento mentre si spara;
- danni subiti e margine medio delle schivate;
- uso di coperture e bordi;
- rischio scelto nelle ricompense;
- risorse consumate o conservate;
- preferenza fra danno diretto, area, status e combo;
- oggetti rifiutati o scelti per pool;
- tempo di clear rispetto alla baseline;
- frequenza con cui il giocatore sfrutta rimbalzo, penetrazione, trappole e ambiente.

Non inviare a Qwen l’intero log eventi. Calcolare localmente feature normalizzate fra 0 e 1, con una finestra mobile e una confidenza.

Esempio:

    {
      "aggression": 0.78,
      "preferred_range": "close",
      "mobility_while_firing": 0.71,
      "aim_precision": 0.54,
      "risk_appetite": 0.82,
      "resource_conservation": 0.31,
      "status_affinity": 0.18,
      "confidence": 0.74
    }

## Che cosa cambia

Il Director può modificare per il piano successivo:

- peso delle famiglie meccaniche;
- offerte e costi;
- geometria di alcune stanze;
- rapporto tra nemici che valorizzano o sfidano la distanza preferita;
- probabilità di presentare una nuova meccanica;
- tema di alcune sinergie;
- scelta dei candidati già validati.

Non può:

- diminuire un oggetto già raccolto;
- cambiare hitbox perché il giocatore è bravo;
- generare un hard counter inevitabile;
- modificare una stanza iniziata;
- nascondere una diversa regola dietro la stessa descrizione.

## Mix iniziale

Per evitare che l’AI trasformi una preferenza in una gabbia:

- 50% contenuti che valorizzano lo stile osservato;
- 30% contenuti che lo sfidano in modo leggibile;
- 20% wildcard estranee allo stile.

Sono parametri di design, non valori finali. Vanno A/B testati.

## Stile e difficoltà sono separati

Un giocatore aggressivo non è necessariamente bravo. Conservare due profili:

- PlayerStyleProfile: come preferisce giocare;
- PerformanceProfile: quanto bene sta giocando rispetto alla baseline.

La difficoltà può usare bande discrete e dichiarate. Lo stile orienta la varietà. Mescolarli produrrebbe rubber-banding invisibile e renderebbe difficile capire perché una run è ingiusta.

## Timeline del primo piano di 10 minuti

### 0–3 minuti

- contenuto stabile;
- raccolta iniziale;
- caricamento del modello solo se non compromette l’avvio;
- uso di fallback se la pipeline non parte.

### Circa 3 minuti

- PlayerStyleProfile v1;
- Qwen riceve RunBible, Capability Registry, budget e profilo;
- genera più candidati del necessario.

### 3–7 minuti

- parsing;
- type checking;
- normalizzazione;
- simulazione headless;
- novelty check;
- bilanciamento e retry mirati.

### 7–9 minuti

- PlayerStyleProfile v2;
- non si rigenera tutto;
- vengono aggiornati selezione, pesi e poche relazioni;
- il contenuto viene congelato.

### 8–10 minuti

- Qwen viene scaricato;
- SD genera i componenti mancanti in ordine di importanza;
- il compilatore costruisce gli atlas;
- il bundle viene pubblicato atomicamente.

### Fine piano

Il piano successivo non cambia più. Se non è pronto:

1. usa asset modulari certificati;
2. conserva le meccaniche generate valide;
3. se necessario usa un bundle completo dalla cache;
4. non bloccare la botola per un tempo indefinito.

## Priorità della generazione

Per non sprecare i dieci minuti:

1. logica degli oggetti;
2. validazione e fallback;
3. sprite dei proiettili dominanti;
4. icone degli oggetti visibili;
5. mutazioni del giocatore;
6. VFX;
7. dettagli cosmetici.

Un piano con logica valida e grafica di fallback è giocabile. Un piano bellissimo con grafi non validi no.

## Trasparenza per il giocatore

Si può comunicare l’adattamento senza mostrare numeri:

- “Il mondo ti percepisce come aggressivo”;
- “Tendi a combattere da vicino”;
- “Questa run sta esplorando rimbalzi e rischio”.

Il giocatore dovrebbe poter disattivare la personalizzazione, utile anche per benchmark, speedrun e modalità ranked.

## Dataset futuro per Qwen

Conservare per ogni proposta:

- input completo normalizzato;
- output grezzo;
- output riparato;
- esito dei validatori;
- motivo del rifiuto;
- punteggi di novelty e bilanciamento;
- valutazione umana;
- telemetria aggregata della run.

Questo corpus specifico del gioco vale più di un generico dataset di codice: insegna al modello la DSL, i limiti e il gusto reale del progetto.

