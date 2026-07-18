---
id: gd-ui-options-accessibility
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Impostazioni e accessibilità, incluso lo schema di controllo approvato. Parità rigorosa tastiera/controller (DEC-057) e tre garanzie canoniche di accessibilità: rimappatura totale, nessuna informazione affidata al solo colore, riduzione effetti (DEC-058)."
---

# Options and Accessibility

## Intento

Dare al giocatore controllo su comodità, accessibilità e chiarezza visiva senza alterare
in modo nascosto l'equilibrio competitivo.

## Condizioni di ingresso

Da `MainMenu` o da `PauseMenu`; al ritorno, il focus torna alla voce che ha aperto `Options`
(vedi `ui/navigation-map.md`, `ui/pause-menu.md`, `ui/main-menu.md`).

## Categorie minime

- audio;
- video;
- controlli (rimappatura di movimento libero e sparo a 4 direzioni, DEC-007; parità
  tastiera/controller, DEC-057);
- accessibilità;
- gameplay;
- privacy e online, se applicabile.

## Parità rigorosa di input (DEC-057)

Ogni scelta di design deve funzionare in modo **identico su tastiera e su controller**:
questo è un vincolo esplicito su tutti i documenti UI della KB, non solo su questo. Nessuna
meccanica può richiedere un dispositivo specifico per essere completata. Il **mouse è
ammesso solo nei menu** (navigazione, selezione), mai come requisito per un'azione di
`Gameplay`. Questo documento è la **fonte unica** della regola di parità di input; gli
altri documenti UI vi rimandano senza riformulare (vedi ad es.
[Navigation Map](navigation-map.md)).

## Garanzie di accessibilità canoniche (DEC-058)

Tre garanzie sono **canone approvato**, non semplici voci di progettazione:

1. **Rimappatura totale** di ogni input, su tastiera e su pad.
2. **Nessuna informazione di gioco affidata al solo colore**: forme e pattern distinti
   comunicano ogni informazione di gioco anche senza percezione del colore; si aggancia al
   budget di leggibilità, fonte unica
   [Combat and Projectiles](../systems/combat-and-projectiles.md) (rimando, non
   riformulare).
3. **Opzione di riduzione effetti** (particelle, scuotimenti, lampi — anche per
   fotosensibilità) che **non altera le informazioni di gioco**: riduce solo la resa
   visiva, mai il contenuto informativo di un telegraph o di un segnale.

La **"modalità assistita" (riduzione della difficoltà) non è nel canone**: queste tre
garanzie riguardano accessibilità e chiarezza, non un abbassamento del livello di sfida
(coerente con DEC-038, difficoltà unica senza livelli selezionabili). Le eventuali
assistenze o velocità che renderebbero una run non classificata restano, distintamente, una
domanda aperta (vedi sotto), non una modalità assistita canonica.

## Accessibilità da progettare

- rimappatura controlli (canonica, DEC-058, sopra);
- supporto controller e tastiera con parità rigorosa (canonica, DEC-057, sopra);
- riduzione flash e particelle (canonica, DEC-058, sopra);
- contrasto di proiettili e minacce (coerente col budget di leggibilità, fonte unica `systems/combat-and-projectiles.md`);
- dimensione testi;
- alternative ai soli colori (canonica, DEC-058, sopra);
- velocità o assistenze in modalità non classificata (da non confondere con una modalità
  assistita di riduzione difficoltà, esclusa dal canone da DEC-058);
- descrizioni leggibili degli oggetti (Innesti compresi).

## Regola

Le opzioni che alterano la difficoltà competitiva devono essere dichiarate e gestite dalle
regole della classifica (vedi `ui/results-and-leaderboards.md`, DEC-016).

## Non-obiettivi

- Non ridefinisce lo schema di controllo di base (movimento libero, sparo a 4 direzioni): solo la sua rimappatura.
- Non introduce una modalità di riduzione della difficoltà: esclusa esplicitamente dal canone (DEC-058).

## Domande aperte residue

- Le assistenze e le velocità esatte che rendono una run non classificata restano da
  dettagliare in `governance/open-questions.md` (sezione Multiplayer); restano distinte
  dalle tre garanzie di accessibilità canoniche (DEC-058).

## Scenari verificabili

1. **Given** il giocatore apre `Options` da `PauseMenu`, **when** torna indietro, **then** il focus ritorna sull'elemento "Opzioni" di `PauseMenu`.
2. **Given** il giocatore attiva un'assistenza dichiarata come non classificante, **when** avvia una run competitiva, **then** la run viene etichettata come non classificata.
3. **Given** il giocatore aumenta il contrasto di proiettili e minacce, **when** rientra in `Gameplay`, **then** il budget di leggibilità applicato resta coerente con `systems/combat-and-projectiles.md`.
4. **Given** una qualunque meccanica di `Gameplay`, **when** il giocatore la esegue solo con tastiera oppure solo con controller, **then** il risultato è identico in entrambi i casi, e il mouse non è mai richiesto (DEC-057).
5. **Given** il giocatore attiva l'opzione di riduzione effetti, **when** rientra in `Gameplay`, **then** particelle, scuotimenti e lampi sono ridotti, ma nessuna informazione di gioco (telegraph, minacce, stato) va persa (DEC-058).
6. **Given** un giocatore daltonico che gioca senza alcuna assistenza di colore, **when** osserva una minaccia o un indicatore di stato, **then** riesce comunque a distinguerlo perché l'informazione è comunicata anche da forma e pattern, non solo dal colore (DEC-058).
