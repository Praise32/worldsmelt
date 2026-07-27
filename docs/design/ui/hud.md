---
id: gd-ui-hud
title: HUD
domain: design
status: approved
authority: canonical
owner: design
summary: "Salute stratificata, risorse per funzione, slot attivo e Innesto. Stile pixel art come tutta la UI (DEC-046, fonte unica in content/visual-language.md). Timer di run sempre visibile in ogni momento del gameplay, non solo in competitivo (DEC-051). Alla prima occorrenza di un contenuto generato mai visto, una card di scoperta breve appare in coda, non bloccante (DEC-065). L'HUD in pixel art della demo è disegnato per il canvas logico attuale 960×640, senza attendere la risoluzione logica definitiva (DEC-174, domanda aperta 11)."
last_reviewed: 2026-07-28
last_verified_commit: 0ec60d0
topics: [hud, gameplay, salute, risorse, timer-run, card-scoperta, floor-zero, DEC-065, DEC-051, DEC-152, DEC-169, DEC-174, canvas-960x640]
related: []
supersedes: []
source_files: []
---

# HUD

> Aggiunta del 22/07 (DEC-137): la GUI vive **in overlay sulla game view a tutto schermo**
> — una sola schermata, niente pannelli laterali che sottraggono spazio al mondo; i
> pannelli diventano overlay adattivi/a comparsa. Refactor in corso.

## Intento

Mostrare durante `Gameplay` solo le informazioni necessarie a decisioni immediate di
sopravvivenza e di gestione delle risorse.

## Condizioni di ingresso

Sempre visibile durante `Gameplay`; nascosto o attenuato durante `PauseMenu` e `BuildScreen`.

In `FloorZero` l'HUD segue una regola diversa (DEC-169): **nascosto** durante
l'esplorazione dell'hub, **consultabile su richiesta** aprendo il menu di pausa per chi
vuole controllare salute, risorse e build senza uscire dal Piano 0, e **di nuovo visibile**
quando il giocatore entra in una **prova del Piano 0** — le arene di sfida e il tutorial
integrato (DEC-047), da non confondere con le «prove» specifiche della run di DEC-042. Il
Piano 0 resta uno spazio di preparazione con lo schermo pulito, senza togliere informazione a
chi la cerca. Dettagli delle prove e della consultazione in pausa: fonte unica
`systems/floor-zero.md` e `ui/pause-menu.md` (rimando, non riformulato qui).

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Salute base | Sempre | — (sola lettura) | Nessuna | — | Rappresentazione distinta dalla salute temporanea |
| Salute temporanea/protettiva | Il giocatore ne possiede | — (sola lettura) | Nessuna | — | Livello visivamente separato dalla salute base; si consuma per prima (DEC-008) |
| Valuta principale | Sempre | — (sola lettura) | Nessuna | — | Contatore numerico |
| Strumento di breccia | Sempre | Disponibile se quantità > 0 | Uso dedicato fuori HUD | — | Contatore numerico |
| Strumento di apertura | Sempre | Disponibile se quantità > 0 | Uso dedicato fuori HUD | — | Contatore numerico |
| Catalizzatore di fusione | Sempre | Disponibile se quantità > 0 | Nessuna diretta (si spende in stanza di fusione) | — | Contatore numerico, evidenziato quando sufficiente per una fusione |
| Slot attivo | Sempre | Un oggetto attivo è equipaggiato e carico | Attiva l'oggetto attivo | Effetto dell'oggetto attivo | Indicatore di carica/cooldown |
| Slot Innesto | Sempre | Un Innesto è equipaggiato | Passiva, nessuna azione diretta dall'HUD | — | Icona dell'Innesto attivo |
| Piano e stanza | Sempre | — (sola lettura) | Nessuna | — | Indicatore di progressione |
| Timer di run | Sempre, durante `Gameplay` | — (sola lettura) | Nessuna | — | Contatore del tempo trascorso, sempre visibile in ogni modalità (DEC-051) |
| Stato competitivo essenziale | Modalità competitiva attiva | — (sola lettura) | Nessuna | — | Indicatore minimo aggiuntivo, distinto dal timer di run sempre visibile (DEC-051) |
| Card di scoperta breve (DEC-065) | Alla prima occorrenza di un contenuto generato mai visto (oggetto, nemico, boss, sinergia/fusione) | — (non bloccante, non mette in pausa) | Nessuna azione richiesta; si accoda automaticamente se altre card sono in corso (coda limitata ~5, le più vecchie si perdono senza essere mostrate: DEC-131) | Mostra sprite, nome e una riga di descrizione del contenuto scoperto | Appare e scompare da sola senza bloccare l'input; una sola card visibile alla volta, le altre attendono in coda |

## Principio

L'HUD mostra informazioni necessarie a decisioni immediate. Dettagli complessi delle
sinergie e della fusione appartengono a `BuildScreen` (vedi
`ui/inventory-and-synergy-screen.md`).

## Salute stratificata

La salute base e la salute temporanea/protettiva devono essere distinguibili a colpo
d'occhio (colore, forma o strato separato); l'ordine di consumo è sempre: prima la
temporanea, poi la base (DEC-008). Fonte di sistema: `systems/health-and-resources.md`.

## Risorse per funzione

Valuta principale, strumento di breccia, strumento di apertura e catalizzatore di fusione
sono definiti per funzione (DEC-013): nessun riferimento al set cuori/monete/bombe/chiavi di
altri giochi. Il catalizzatore di fusione è una risorsa nuova, distinta dalle altre tre.

I nomi mostrati in gioco sono quelli inglesi della nomenclatura ufficiale (Ingots, Blast
Charges, Cast Keys, Flux — DEC-072, fonte unica [Glossary](../governance/glossary.md), non
riformulata qui). Le icone di queste risorse mantengono una silhouette stabile tra i World,
con variazione ammessa solo in palette e dettagli (DEC-073b, fonte unica
[Visual Language](../content/visual-language.md), non riformulata qui).

## Slot attivo e Innesto

Si parte con 1 slot attivo e 1 slot Innesto; oggetti o eventi rari possono aggiungere slot
durante la run (DEC-011). L'HUD mostra sempre lo stato corrente degli slot posseduti, non
il numero massimo teorico.

Gli oggetti equipaggiati si sovrappongono visivamente al personaggio secondo gli stessi
strati/slot visivi definiti in [Visual Language](../content/visual-language.md),
indipendentemente dal fatto che lo sprite del personaggio sia curato o generato (DEC-049);
questo documento non ripete quel dettaglio.

## Timer di run sempre visibile (DEC-051)

Il tempo trascorso nella run è **sempre visibile** nell'HUD durante `Gameplay`, in ogni
modalità, non solo nelle modalità competitive: il gioco si dichiara esplicitamente una
corsa. Questo è distinto dall'indicatore minimo di stato competitivo (vedi tabella sopra),
che resta specifico delle modalità competitive e non ripete il timer generale.

Il timer di run è anche il segnale con cui il giocatore valuta se raggiungere in tempo le
stanze a tempo dei piani avanzati (vedi [Rewards and Economy](../systems/rewards-and-economy.md)
e [Special Rooms](../systems/special-rooms.md), DEC-051); questo documento non ripete il
dettaglio di quell'archetipo.

## Card di scoperta breve (DEC-065)

Alla prima occorrenza in assoluto di un contenuto generato mai visto dal giocatore — oggetto,
nemico, boss, sinergia/fusione — il gioco mostra una **card di scoperta breve** nell'HUD:
sprite, nome, una riga di descrizione. La card **non mette in pausa** la simulazione e **non
blocca l'input**: il giocatore continua a muoversi e a combattere mentre la card è visibile.

I dettagli completi del contenuto scoperto vivono nella scheda dedicata del Catalogo (vedi
[Save and Meta Progression](../systems/save-and-meta-progression.md), DEC-045); questa card
è solo un annuncio rapido, non la sostituisce.

Regola di coda: **una sola card alla volta**. Se più scoperte arrivano insieme, si accodano
ed escono in sequenza, senza invadere lo schermo con più card contemporaneamente. I casi
limite di questa coda sono risolti: il cap e l'overflow da DEC-131 (vedi sotto), e il
destino delle card ancora in attesa quando il giocatore muore o cambia stanza da DEC-152
(vedi sotto).

Se il giocatore **muore** o **cambia stanza** mentre altre card attendono in coda, quelle
non ancora mostrate vengono **scartate silenziosamente** (DEC-152): nessuna coda che
insegue il giocatore nella stanza successiva, nessun recupero differito. La scoperta resta
comunque registrata nel Catalogo permanente con la sua scheda, esattamente come
nell'overflow di DEC-131: la card è la notifica, non il contenuto.

## Stile visivo (DEC-046, rimando)

L'HUD, come tutta l'interfaccia del gioco, è pixel art: fonte unica della regola è
[Visual Language](../content/visual-language.md), non riformulata qui.

## Canvas di riferimento della demo (DEC-174)

L'HUD in pixel art della demo si disegna per il **canvas logico attuale, 960×640** — lo
stesso rettangolo su cui sono costruite le stanze multi-taglia e la telecamera a zoom
fisso di [Rooms and Floor Generation](../systems/rooms-and-floor-generation.md) (DEC-170).
Questo **non** fissa la risoluzione logica canonica dell'interfaccia: la domanda aperta 11
(proposta ricorrente 640×360 con scaling intero) **resta aperta**, si decide dopo la demo.
960×640 è il canvas su cui si lavora **oggi**, non un valore di design definitivo — stesso
trattamento dei default proposti stile DEC-019 già usati altrove in questo documento e in
`content/visual-language.md`.

Elementi indipendenti dalla risoluzione restano una buona pratica per i componenti a
9-patch (bordi/riempimento che si adattano a più dimensioni), ma non sono la via
principale scelta per l'HUD della demo: costruire subito componenti solo relativi
avrebbe rimandato la disegnazione concreta dell'HUD senza necessità, dato che il canvas
di lavoro (960×640) è già stabile per l'implementazione M2 in corso.

## Priorità visiva

1. sopravvivenza (salute base e temporanea);
2. minacce e cooldown;
3. risorse spendibili (valuta, breccia, apertura, catalizzatore di fusione);
4. progressione della run (piano e stanza);
5. informazioni competitive.

## Non-obiettivi

- Non mostra formule interne, dettagli tecnici o prompt dell'IA (fonte unica: `06-ai-content-generation-model.md`).
- Non sostituisce `BuildScreen` per la spiegazione delle sinergie.
- La card di scoperta non sostituisce la scheda completa del Catalogo (DEC-045, vedi
  `systems/save-and-meta-progression.md`).

## Domande aperte residue

- ~~Casi limite della coda delle card di scoperta~~: risolti in due decisioni distinte.
  DEC-131 copre **cap e overflow** — coda limitata (~5, valore esatto da playtest); quando
  trabocca le più vecchie escono senza essere mostrate. DEC-152 copre il caso separato di
  **morte o cambio stanza** con card ancora in attesa — si scartano silenziosamente. In
  entrambi i casi la scoperta resta comunque registrata nel Catalogo.

## Scenari verificabili

1. **Given** il giocatore ha sia salute base sia salute temporanea, **when** subisce danno, **then** la salute temporanea si riduce per prima e resta visivamente distinta da quella base.
2. **Given** il giocatore possiede catalizzatore di fusione sufficiente per una fusione, **when** osserva l'HUD, **then** l'indicatore del catalizzatore appare evidenziato rispetto allo stato "insufficiente".
3. **Given** il giocatore raccoglie un oggetto raro che aggiunge uno slot Innesto, **when** l'HUD si aggiorna, **then** compare un secondo slot Innesto vuoto.
4. **Given** una modalità competitiva è attiva, **when** il giocatore gioca in `Gameplay`, **then** l'HUD mostra lo stato competitivo essenziale senza rivelare informazioni tecniche della generazione.
5. **Given** un giocatore in `Gameplay` in qualunque modalità, **when** osserva l'HUD, **then** il timer di run è sempre visibile, indipendentemente dalla modalità (DEC-051).
6. **Given** un giocatore incontra per la prima volta un nemico generato mai visto, **when** lo affronta, **then** l'HUD mostra una card di scoperta breve con sprite, nome e una riga, senza mettere in pausa né bloccare l'input (DEC-065).
7. **Given** più contenuti mai visti compaiono nella stessa stanza, **when** il giocatore li incontra quasi contemporaneamente, **then** le card di scoperta si accodano e vengono mostrate una alla volta, senza sovrapporsi sullo schermo (DEC-065).
8. **Given** il giocatore ha card di scoperta ancora in coda non mostrate, **when** muore oppure cambia stanza, **then** quelle card vengono scartate silenziosamente senza inseguirlo nella stanza successiva, e la scoperta resta comunque registrata nel Catalogo (DEC-152).
9. **Given** il giocatore è in `FloorZero`, **when** esplora l'hub senza aprire la pausa e senza entrare in una prova, **then** l'HUD di combattimento resta nascosto; **when** apre il menu di pausa, **then** può consultare salute, risorse e build; **when** entra in una prova, **then** l'HUD ricompare (DEC-169).
10. **Given** l'HUD della demo è disegnato in pixel art, **when** viene posizionato sullo schermo, **then** usa come riferimento il canvas logico 960×640 in uso oggi, senza attendere la risposta alla domanda aperta 11 sulla risoluzione logica definitiva (DEC-174).
