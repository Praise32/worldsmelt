---
id: gd-system-health-resources
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Salute stratificata con un tetto di salute base proprio di ciascun personaggio, parte delle sue statistiche (DEC-033), e risorse di run ri-tematizzate per funzione, con nomi placeholder (DEC-013)."
---

# Health and Resources

## Intento per il giocatore

Le risorse devono produrre decisioni, non solo accumulo. Ogni utilizzo importante deve competere con almeno un'altra opportunità d'uso.

## Condizioni di ingresso

Le regole di questo documento si applicano ovunque il giocatore possa accumulare o spendere risorse: piani 1–5, Piano 0 (per quanto reso disponibile lì, DEC-004) e arene di sfida opzionali.

## Risorse (per funzione, nomi placeholder — DEC-013)

Tutte le risorse del gioco sono ri-tematizzate per funzione, con nomi **placeholder** in attesa del nome definitivo del gioco (DEC-003). Il set di risorse precedente non è più canone: questo documento definisce le risorse solo in termini delle funzioni astratte qui sotto.

### Salute (stratificata)

- **Composizione:** salute base + salute temporanea/protettiva (DEC-008, approved).
- **Ordine di consumo:** si consuma **prima** la salute temporanea/protettiva, **poi** la salute base. Questo ordine è esplicito e non negoziabile.
- **Esaurimento:** salute base a zero = run persa, permadeath (DEC-006, cita solo di sfuggita). Per le condizioni di vittoria legate al boss del piano 5 vedi [bosses.md](bosses.md) (rimando, non riformulare qui i dettagli boss).
- **Come si ottiene:** fonti di cura (oggetti, stanze, eventi) — quantità e frequenza esatte non definite (draft).
- **Cap/limite massimo (DEC-033):** la salute base ha un **tetto proprio di ciascun personaggio**, parte delle sue statistiche: non è un valore unico condiviso da tutti. Personaggi-vetro (tetto basso) e personaggi-roccia (tetto alto) esistono per design, sia nella rosa base (DEC-030) sia nel personaggio generato per run (DEC-014). I contenitori di salute crescono tramite stat-up e oggetti fino al tetto di quel personaggio specifico, non oltre. Le bande min/max entro cui possono variare i tetti — soprattutto per i personaggi generati — sono valori di default da playtest, come i valori di DEC-019 (stato `draft`). La salute temporanea/protettiva segue regole proprie (DEC-008) e non è soggetta a questo stesso tetto.
- **HUD:** mostra separatamente la quota di salute temporanea/protettiva e quella base (rimando concettuale; la progettazione dell'interfaccia vive fuori scope in `ui/`, non trattata qui).
- **Fine piano:** presumibilmente persiste (nessuna DEC afferma il contrario) — draft, da confermare.
- **Fine run:** la run termina quando la salute base arriva a zero (vedi sopra); non applicabile oltre quel punto.

### Valuta principale

- **Funzione:** economia di run (acquisti in stanze di negozio, prezzi in stanze ad alto rischio — vedi [special-rooms.md](special-rooms.md)).
- **Come si ottiene:** fonti canoniche definite da DEC-048 — nemici sconfitti e stanze ripulite (completate); il negozio inoltre ricompra oggetti e Innesti indesiderati a prezzo ridotto, unica via per convertirli in valuta durante la run. Fonte unica del dettaglio: [rewards-and-economy.md](rewards-and-economy.md) (rimando, non riformulare).
- **Cap/limite massimo:** non definito (domanda aperta).
- **HUD:** mostra il totale corrente (rimando concettuale, fuori scope UI).
- **Fine piano:** presumibilmente persiste tra i piani della stessa run (draft, non confermato da nessuna DEC).
- **Fine run:** si azzera al termine della run, salvo un ruolo nella meta-progressione — domanda aperta, vedi [save-and-meta-progression.md](save-and-meta-progression.md) (rimando).

### Strumento di breccia

- **Funzione:** distruggere ostacoli o muri, infliggere danno d'area.
- **Come si ottiene:** stanze di ricompensa, tesoro, negozio, oggetti (draft, dettagli non definiti).
- **Cap/limite massimo:** non definito (domanda aperta).
- **HUD:** mostra la scorta corrente (rimando concettuale, fuori scope UI).
- **Fine piano/fine run:** non definito (domanda aperta); relazione con ostacoli e scorciatoie in [secrets-and-obstacles.md](secrets-and-obstacles.md) (rimando).

### Strumento di apertura

- **Funzione:** aprire stanze o forzieri bloccati, incluso l'accesso ad alcuni archetipi speciali di stanza.
- **Come si ottiene:** stanze, nemici, ricompense (draft, dettagli non definiti).
- **Cap/limite massimo:** non definito (domanda aperta).
- **HUD:** mostra la scorta corrente (rimando concettuale, fuori scope UI).
- **Fine piano/fine run:** non definito (domanda aperta); relazione con stanze speciali in [special-rooms.md](special-rooms.md) e con segreti in [secrets-and-obstacles.md](secrets-and-obstacles.md) (rimando).

### Catalizzatore di fusione

- **Funzione:** risorsa nuova che abilita/paga la fusione esplicita di due oggetti nella stanza di fusione (DEC-012b, DEC-013). La meccanica di fusione in sé vive in [item-fusion.md](item-fusion.md) (rimando, non riformulare qui).
- **Come si ottiene:** non definito nel dettaglio (draft); presumibilmente stanze di fusione o eventi dedicati.
- **Cap/limite massimo:** non definito (domanda aperta).
- **HUD:** mostra la scorta corrente (rimando concettuale, fuori scope UI).
- **Fine piano/fine run:** non definito (domanda aperta).

## Input/azioni

- raccolta di risorse rilasciate da stanze, nemici o oggetti;
- spesa di valuta principale in stanze di negozio o scambio;
- uso dello strumento di breccia per distruggere ostacoli o infliggere danno d'area;
- uso dello strumento di apertura per sbloccare stanze o forzieri;
- consumo del catalizzatore di fusione nella stanza di fusione (vedi [item-fusion.md](item-fusion.md));
- assorbimento del danno, prima dalla salute temporanea/protettiva poi dalla salute base.

## Risultato

Ogni raccolta o spesa di risorsa altera lo stato della run (salute residua, capacità economica, accesso a stanze o alla fusione) e genera una decisione successiva, non solo un incremento numerico.

## Feedback

Ogni variazione di una risorsa è comunicata al giocatore in modo distinguibile dalle altre (rimando concettuale all'HUD, fuori scope qui). Il consumo di salute deve rendere leggibile quale strato (temporaneo/protettivo o base) è stato intaccato.

## Interazioni

- con le stanze speciali che erogano o richiedono risorse: [special-rooms.md](special-rooms.md);
- con ostacoli e segreti sbloccabili: [secrets-and-obstacles.md](secrets-and-obstacles.md);
- con ricompense ed economia generale: [rewards-and-economy.md](rewards-and-economy.md);
- con la fusione esplicita: [item-fusion.md](item-fusion.md);
- con la meta-progressione tra run: [save-and-meta-progression.md](save-and-meta-progression.md);
- con le condizioni di vittoria/sconfitta legate al boss: [bosses.md](bosses.md).

## Regole per contenuti generati

Le quantità e i punti di distribuzione delle risorse in una stanza generata devono rispettare i cap dichiarati (dove definiti) e non rendere una stanza impossibile da completare per mancanza di risorse essenziali. Ogni fonte di risorsa generata dichiara la propria origine con uno dei 4 valori: `curato | composto | variato | nuovo`.

## Casi limite

- danno che eccede la salute temporanea/protettiva residua: l'eccedenza si applica alla salute base nello stesso evento;
- risorsa (strumento di breccia/apertura/catalizzatore di fusione) esaurita nel momento in cui serve: non deve bloccare il completamento della run, un percorso alternativo deve restare disponibile;
- stanza generata che richiede una risorsa non ancora ottenibile a quel punto della run.

## Fallback

Vedi [generated-content-validation.md](generated-content-validation.md) — fonte unica della regola di fallback (rimando, non riformulare).

## Non-obiettivi

- Non definisce valori numerici finali (quantità, cap, prezzi).
- Non progetta l'interfaccia che mostra le risorse — vedi `ui/`, fuori scope.
- Non definisce il nome finale delle risorse: quelli qui sono placeholder in attesa del nome del gioco (DEC-003).

## Domande aperte residue

- Bande min/max esatte dei tetti di salute dei personaggi (il principio del tetto per-personaggio è approvato da DEC-033; i valori restano da validare col playtest, vedi `../governance/open-questions.md`).
- Cap/limite massimo per valuta, strumento di breccia, strumento di apertura e catalizzatore di fusione: non definiti da nessuna DEC.
- Persistenza tra piani e a fine run per ciascuna risorsa oltre alla salute: non definita da nessuna DEC (draft, ipotesi di persistenza tra piani nella stessa run indicate sopra come non confermate).
- Presenza di contenitori permanenti (capacità massima ampliabile) nella run.
- Relazione esatta tra strumento di apertura e i singoli archetipi di stanza speciale (quali richiedono quale risorsa).

## Scenari verificabili

### Scenario 1 — ordine di consumo della salute stratificata

Given il giocatore possiede sia salute temporanea/protettiva sia salute base,  
When subisce danno,  
Then il danno consuma prima la salute temporanea/protettiva e solo l'eccedenza, se presente, intacca la salute base.

### Scenario 2 — permadeath a salute zero

Given la salute base del giocatore raggiunge zero,  
When l'evento di danno viene risolto,  
Then la run termina come persa (permadeath, DEC-006), indipendentemente dal piano raggiunto.

### Scenario 3 — uso dello strumento di apertura

Given il giocatore possiede almeno un'unità di strumento di apertura,  
When lo usa su una stanza o un forziere bloccato,  
Then l'accesso si sblocca e l'unità consumata viene sottratta dalla scorta corrente.

### Scenario 4 — catalizzatore di fusione nella stanza di fusione

Given il giocatore è nella stanza di fusione con due oggetti idonei e almeno un'unità di catalizzatore di fusione,  
When avvia la fusione,  
Then il catalizzatore viene consumato e la meccanica procede secondo le regole di [item-fusion.md](item-fusion.md).

### Scenario 5 — la salute base non supera il tetto del personaggio

Given il giocatore ha già raggiunto il tetto di salute base del proprio personaggio (DEC-033) tramite stat-up e oggetti raccolti,  
When ottiene un ulteriore stat-up o oggetto che aumenterebbe la capacità massima di salute base,  
Then la salute base non supera il tetto proprio di quel personaggio: l'eventuale eccedenza non ha effetto sul contenitore di salute base (la salute temporanea/protettiva, se applicabile, non è soggetta a questo stesso limite).

### Scenario 6 — personaggi diversi con tetti di salute diversi

Given due personaggi della run — uno con un tetto di salute base basso ("personaggio-vetro") e uno con un tetto alto ("personaggio-roccia") — entrambi parte delle statistiche definite in DEC-033,  
When ciascuno raccoglie la stessa quantità di stat-up e oggetti che aumentano la salute base,  
Then il contenitore di salute base di ciascun personaggio cresce fino al proprio tetto individuale, e i due tetti restano diversi tra loro per design.
