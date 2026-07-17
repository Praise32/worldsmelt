---
id: gd-system-combat
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Regole di combattimento e proiettili. Fonte unica del budget di leggibilità. Incorpora i vincoli di leggibilità imposti dai controlli DEC-007; le bande di potenza dei colpi generati (DEC-019) sono documentate come default draft."
---

# Combat and Projectiles

## Obiettivo

Combattimento responsivo, leggibile e adatto a variazioni generate.

## Intento per il giocatore

Ogni minaccia deve essere leggibile in tempo utile per reagire. La spettacolarità di un attacco o di una sinergia non deve mai costare la capacità del giocatore di distinguere origine, direzione e causa del danno.

## Condizioni di ingresso

Le regole di questo documento si applicano ovunque siano presenti nemici, ostacoli reattivi o attacchi del giocatore: piani 1–5 e arene di sfida opzionali del Piano 0 (DEC-004).

## Controlli e vincoli imposti da DEC-007

Il giocatore si muove liberamente ma spara solo lungo le 4 direzioni cardinali (DEC-007, approved). Conseguenze per la leggibilità:

- i proiettili del giocatore percorrono sempre uno dei 4 assi cardinali: la loro traiettoria è geometricamente semplice, quindi consuma poco budget di leggibilità rispetto a una mira libera a 360°;
- i proiettili nemici non sono vincolati alle 4 direzioni cardinali e possono avere traiettorie curve od oblique: questa asimmetria tra "leggere il proprio colpo" e "leggere il colpo nemico" deve restare esplicita nel telegraph (forma, colore, tempistica), così da non essere scambiata per un errore di lettura;
- poiché il giocatore spara solo su 4 assi, il posizionamento nello spazio libero diventa la leva principale per allinearsi ai bersagli: stanze e nemici generati devono garantire spazio di manovra sufficiente a farlo (vedi [rooms-and-floor-generation.md](rooms-and-floor-generation.md)).

## Input/azioni

- emissione di un attacco nella direzione di mira cardinale corrente (vedi [player.md](player.md));
- attivazione di oggetti che modificano proprietà dell'attacco (vedi [active-items.md](active-items.md), [passive-items.md](passive-items.md));
- reazione del giocatore a telegraph e proiettili nemici (schivata, riposizionamento entro il movimento libero).

## Proprietà fondamentali di un attacco

- origine;
- direzione;
- velocità;
- cadenza;
- danno;
- dimensione;
- durata;
- traiettoria;
- interazioni con muri, nemici e ostacoli;
- tag visivi e meccanici.

## Budget di leggibilità — definizione canonica

**Questa è la fonte unica del concetto. Ogni altro documento della KB rimanda qui senza riformulare.**

Il budget di leggibilità è la quantità massima di segnali visivi/telegraph simultaneamente presenti sullo schermo che un giocatore medio può leggere e interpretare correttamente, in tempo utile per reagire, senza perdere la capacità di distinguere le minacce attive da elementi decorativi o innocui. È un'istanza specifica del concetto generale di **budget** (quantità massima spendibile di un attributo prima che l'esperienza degradi), applicato qui all'attenzione visiva del giocatore.

Ogni fonte di segnale sulla scena — proiettili nemici, telegraph di attacco, effetti di sinergia, particellari, indicatori di stato sul personaggio — consuma una quota di questo budget. Superarlo produce confusione anche quando ogni singolo elemento, preso da solo, sarebbe perfettamente leggibile: il budget è una proprietà della composizione della scena, non del singolo effetto.

Una sinergia o un attacco possono aumentare spettacolarità e quantità di effetti, ma devono sempre preservare, indipendentemente da quanto budget consumano:

- posizione del personaggio;
- proiettili nemici;
- hitbox percepita;
- direzione dell'attacco;
- causa del danno.

Questi cinque elementi non sono negoziabili: un contenuto (curato o generato) che li rende illeggibili non è accettabile, anche se resta entro ogni altro limite numerico.

## Risultato

Un attacco valido produce danno, spostamento dei partecipanti, feedback visivo/sonoro e, quando previsto, un'interazione con l'ambiente (distruzione, attivazione, blocco), sempre entro il budget di leggibilità sopra definito.

## Feedback

Ogni attacco comunica in modo distinguibile: chi lo ha originato, in che direzione viaggia e quale area minaccia (telegraph), prima o al momento dell'impatto. Il feedback di danno subito dal giocatore non deve competere per attenzione con il telegraph di minacce ancora attive.

## Interazioni

### Collisioni tra effetti

Ogni combinazione deve dichiarare se gli effetti:

- si sommano;
- si moltiplicano;
- si trasformano;
- si escludono;
- producono una terza regola.

### Con altri sistemi

- personaggio e sue statistiche: [player.md](player.md);
- nemici e loro telegraph: [enemies.md](enemies.md);
- boss e fasi: [bosses.md](bosses.md);
- fusione visiva di due tratti in sinergia: [synergies.md](synergies.md) (usa gli strati definiti in [../content/visual-language.md](../content/visual-language.md), rimando senza riformulare);
- stanze e spazio di manovra: [rooms-and-floor-generation.md](rooms-and-floor-generation.md).

## Regole per contenuti generati

I tipi di colpo (per il giocatore e per i nemici) non sono scelti da un menu curato: sono **inventati** dall'IA come combinazioni parametriche dentro bande di garanzia, non selezionati da un catalogo fisso (DEC-020, approved sul modello concettuale). Ogni tipo di colpo generato deve:

- rispettare tutte le proprietà fondamentali di un attacco elencate sopra;
- restare entro il budget di leggibilità canonico definito sopra;
- dichiarare la propria origine con uno dei 4 valori: `curato | composto | variato | nuovo`.

**Bande di potenza dei colpi (DEC-019 — draft, default di implementazione, da validare col playtest):** i colpi generati per il giocatore usano come default attuale una banda di potenza **[0.75–1.25]** (moltiplicatore rispetto a un colpo di riferimento). Questo valore è già presente nell'implementazione corrente ed è riportato qui come punto di partenza, **non come decisione di design chiusa**: resta soggetto a validazione tramite playtest. Bande equivalenti per nemici e boss vivono in [enemies.md](enemies.md) e [bosses.md](bosses.md) rispettivamente (rimando, valori non duplicati qui).

Per il fallback quando un tipo di colpo generato non supera la validazione, vedi [generated-content-validation.md](generated-content-validation.md) (rimando, non riformulare).

## Casi limite

- un attacco singolo tecnicamente leggibile ma che, sommato ad altri effetti attivi in scena, eccede il budget di leggibilità;
- proiettile nemico con traiettoria che imita visivamente un proiettile del giocatore (deve essere evitato per la regola di asimmetria sopra);
- sinergia che moltiplica il numero di proiettili oltre la soglia leggibile: deve degradare graziosamente (es. raggruppamento visivo) invece di rompere il budget.

## Fallback

Vedi [generated-content-validation.md](generated-content-validation.md) — fonte unica della regola di fallback (rimando, non riformulare).

## Non-obiettivi

- Non definisce valori numerici finali di danno, velocità o cadenza.
- Non progetta l'interfaccia di mira o gli indicatori a schermo — vedi `ui/`, fuori scope.
- Non stabilisce un numero massimo fisso di segnali simultanei: il budget di leggibilità è un principio qualitativo di composizione, non ancora una soglia numerica validata.

## Domande aperte residue

- Esiste una soglia numerica esplicita (es. massimo N proiettili nemici leggibili in scena) o il budget resta un principio qualitativo affidato al playtest?
- Le bande di potenza [0.75–1.25] restano invariate per tutte le stanze o variano con la difficoltà del piano?
- Come si comporta il budget di leggibilità in multiplayer asincrono, dove più run indipendenti potrebbero condividere l'interfaccia dei risultati (`experimental`)?

## Scenari verificabili

### Scenario 1 — telegraph nemico distinguibile dal proiettile del giocatore

Given un nemico e il giocatore sparano contemporaneamente,  
When entrambi i proiettili sono visibili a schermo,  
Then il proiettile nemico è visivamente distinguibile da quello del giocatore per forma, colore o tempistica, anche se le traiettorie si incrociano.

### Scenario 2 — sinergia che rispetta il budget di leggibilità

Given una sinergia tra due oggetti aumenta il numero di proiettili generati dal giocatore,  
When l'effetto combinato viene mostrato a schermo,  
Then posizione del personaggio, proiettili nemici, hitbox percepita, direzione dell'attacco e causa del danno restano tutti leggibili, anche se l'effetto visivo complessivo è più denso.

### Scenario 3 — colpo generato entro la banda di potenza di default

Given l'IA genera un nuovo tipo di colpo per il giocatore,  
When il colpo supera la validazione strutturale,  
Then il suo moltiplicatore di potenza rientra nella banda draft [0.75–1.25], salvo diverso esito del playtest che aggiorni questo default.

### Scenario 4 — contenuto generato che eccede il budget di leggibilità

Given un tipo di colpo generato produce un numero di particelle tale da nascondere un proiettile nemico,  
When la validazione lo verifica,  
Then il contenuto è respinto o sostituito da un fallback curato, secondo la regola in [generated-content-validation.md](generated-content-validation.md).
