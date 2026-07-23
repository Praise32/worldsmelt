---
id: gd-multiplayer
title: Multiplayer and Competition
domain: design
status: experimental
authority: canonical
owner: design
summary: "Gare tra giocatori e classifiche: due assi approvati (Leggera/Classificata × stesso seed/seed diversi), dettagli experimental. La modalità Classificata esiste in tre istanze — stesso seed, seed diversi, Classificata giornaliera pubblica ('Daily') — con classifiche divise per metrica, tempo e punteggio separati (DEC-062). La Daily premia con medaglie/cornici cosmetiche, fuori dall'economia dei punti (DEC-064), e ruota alle 00:00 UTC per tutti i giocatori (DEC-081). La Classificata a seed diversi è equa by-design, budget di generazione vincolato uguale per ogni seed della gara (DEC-096). Nelle gare a stesso seed e nella Daily le proposte di rosa, personaggio e tema sono identiche per tutti i partecipanti, la scelta resta libera (DEC-108). Una run è condivisibile fuori dalle classifiche via codice breve — esteso a seed, versione di gioco, tema e personaggio scelti (DEC-077) — o file RunBundle, sempre non classificata (DEC-066)."
last_reviewed: 2026-07-19
topics: [multiplayer, classifiche, daily, seed, equità, condivisione-run]
related: []
supersedes: []
source_files: []
---

# Multiplayer and Competition

## Visione approvata (DEC-016, approved)

Questa sezione, a differenza del resto del documento (che resta `experimental`), registra
una decisione approvata dal proprietario il 2026-07-17:

- Il multiplayer competitivo è **asincrono**: i giocatori non condividono una sessione in
  tempo reale, ma affrontano **la stessa run** — stesso seed/manifest, sfruttando il
  determinismo già presente nel run manifest (vedi
  [Run Manifest and Reproducibility](systems/run-manifest-and-reproducibility.md)).
- Le classifiche si basano su **tempo** e **punteggio**.
- I pool sbloccati tramite meta-progressione sono **esclusi** dalle modalità competitive:
  ogni giocatore affronta la run competitiva con gli stessi pool di base (vedi
  [Core Loop](03-core-loop.md), loop metagioco, DEC-015).

## Struttura del menu multiplayer (DEC-021, approved 2026-07-18)

Il menu multiplayer offre due scelte indipendenti, entrambe approvate dal proprietario:

1. **Modalità**: **Leggera** (non classificata, senza vincoli di classifica) oppure
   **Classificata** (valida per le classifiche, con le regole di correttezza di DEC-016).
2. **Tipo di gara**: **Stesso seed** (tutti i partecipanti affrontano la stessa run
   esatta, stesso seed/manifest, sfruttando il determinismo esistente) oppure
   **Seed diversi** (ogni giocatore ha una run propria).

Le quattro combinazioni sono tutte previste. Per la combinazione Classificata + Seed
diversi la confrontabilità dei risultati (pillar "Competizione verificabile") richiede
un'equivalenza di difficoltà tra run diverse: il criterio è ora fissato — equità by-design
tramite budget di generazione vincolato, non correzione statistica a posteriori (DEC-096,
vedi sezione dedicata più sotto, rimando, non riformulato qui). I valori esatti dei vincoli
restano da playtest (vedi `governance/open-questions.md`).

Ogni altro dettaglio del multiplayer (informazioni visibili durante la
gara, assistenze consentite, dettagli implementativi) resta `experimental` e non è ancora
deciso.

## Tre istanze di Classificata + Daily (DEC-062, approved 2026-07-18)

Questa decisione estende DEC-021: dentro la Modalità **Classificata**, l'asse "Tipo di
gara" si allarga da due a **tre istanze**:

1. **Sfida a stesso seed** — i contendenti affrontano la stessa run generata (già prevista
   da DEC-021).
2. **Sfida a seed diversi** — run generate casualmente, diverse per ciascun giocatore, con
   una componente di casualità dichiarata (già prevista da DEC-021).
3. **Classificata giornaliera pubblica ("Daily")** — una run generata **scelta dallo
   sviluppatore**, che **cambia ogni giorno**: tutti i giocatori affrontano lo **stesso
   seed**, quello del giorno corrente, con una **classifica globale giornaliera**.

La Daily è distinta dalla "sfida a stesso seed" ordinaria: quest'ultima nasce da una run
pubblicata da un giocatore per sfidare altri giocatori (vedi
[Multiplayer Lobby](ui/multiplayer-lobby.md)), mentre la Daily è una singola run scelta
centralmente, condivisa da tutti, che ruota ogni giorno. La Modalità **Leggera** mantiene
solo le due istanze già previste da DEC-021 (stesso seed / seed diversi): la Daily esiste
solo dentro la Classificata.

## Classifiche divise per metrica (DEC-062)

Le classifiche della Classificata (in tutte e tre le sue istanze) sono **divise per
metrica**: una graduatoria per il **tempo** e una separata per il **punteggio**, mai un
punteggio combinato. Il punteggio usato come metrica è il punteggio composito
multi-percorso definito in
[Rewards and Economy](systems/rewards-and-economy.md#punteggio-composito-multi-percorso-dec-060)
(DEC-060, rimando, non riformulato qui).

## Ricompense della Daily: cosmetici (DEC-064)

La Classificata giornaliera pubblica ("Daily", DEC-062) premia i partecipanti con **medaglie
e cornici cosmetiche**, visibili nel profilo e nel museo del Piano 0, legate ai piazzamenti e
alle streak di partecipazione. Queste ricompense **non assegnano punti sblocco** e non
toccano in alcun modo l'economia dei punti di meta-progressione: DEC-015 (niente
potenziamenti permanenti, sblocchi solo singleplayer) e DEC-027 (punti a doppio canale, solo
singleplayer) restano intatte. Dettaglio di persistenza nel profilo:
[Save and Meta Progression](systems/save-and-meta-progression.md#ricompense-cosmetiche-della-classificata-giornaliera-dec-064)
(rimando, non riformulato qui); dettaglio in schermata risultati:
[Results and Leaderboards](ui/results-and-leaderboards.md#ricompense-cosmetiche-della-daily-dec-064)
(rimando).

Questa decisione risolve la parte "ricompense dedicate?" della domanda aperta sulla Daily;
l'orario di rotazione è ora fissato da DEC-081 (sezione seguente): non restano punti aperti
sulla Daily.

## Orario di rotazione della Daily (DEC-081)

La Classificata giornaliera pubblica (Daily, DEC-062) ruota alle **00:00 UTC**: un istante
globale unico per tutti i giocatori (non mezzanotte locale, non un orario fisso regionale).
Questo documento resta la **fonte unica** dell'orario della Daily. La lobby mostra il conto
alla rovescia al prossimo cambio; dettaglio del feedback in interfaccia:
[Multiplayer Lobby](ui/multiplayer-lobby.md) (rimando, non riformulato qui).

## Equità della Classificata a seed diversi: budget vincolato (DEC-096)

Per la combinazione Classificata + Seed diversi (DEC-021), l'equità tra run diverse è
**by-design**: i seed della stessa gara generano dentro gli **stessi vincoli di budget** —
difficoltà di nemici e boss, rarità disponibili, numero di stanze in banda stretta. Nessuna
correzione statistica del punteggio a posteriori: il giocatore capisce in anticipo cosa sta
gareggiando. I valori esatti dei vincoli (bande di difficoltà, rarità, numero di stanze)
restano da playtest, stile DEC-019 (vedi punto 9 di `governance/open-questions.md`, che
resta aperto solo per questi valori e per gli altri dettagli del multiplayer asincrono —
disconnessioni, metriche extra, regole di parità e validità).

## Proposte identiche nelle gare a stesso seed e nella Daily (DEC-108)

Nelle gare a **stesso seed** (sfida a stesso seed, DEC-062) e nella **Daily** (DEC-062,
DEC-081) lo stesso seed genera le **stesse proposte generate per tutti i partecipanti**: lo
stesso personaggio alternativo generato e lo stesso tema proposto. La rosa base disponibile
resta quella **sbloccata dal singolo giocatore** (DEC-100): la parità riguarda il contenuto
generato dal seed, non la progressione personale. La
**scelta resta libera e strategica** — nessun personaggio imposto dalla gara, nessuna
esclusione del personaggio generato — l'equità è garantita dal determinismo della
generazione dal seed, non da un vincolo sulla scelta. Dettaglio della generazione del
personaggio alternativo e della sua relazione col seed:
[systems/characters.md](systems/characters.md) (rimando, non riformulato qui). Gap di
implementazione esplicito: il determinismo completo delle proposte dal seed non è ancora
garantito dalla pipeline (backlog noto: RNG di gioco su `time(NULL)`, inferenza non
deterministica) — vedi anche
[Run Manifest and Reproducibility](systems/run-manifest-and-reproducibility.md) (rimando,
non riformulato qui).

## Condivisione run a due vie (DEC-066, DEC-077)

Una run è condivisibile **fuori dalle classifiche** in due modi, entrambi indipendenti dalla
visione multiplayer asincrona fissata sopra (DEC-016):

1. **Codice breve testuale** — codifica **seed, versione di gioco, tema scelto e personaggio
   scelto** (DEC-077, estende DEC-066), da incollare in `RunSetup`: chi lo riceve rigioca
   esattamente la stessa run. Le scelte trasportate arrivano nel Piano 0 come preselezione;
   il giocatore resta libero di cambiarle (a quel punto sta giocando una run con lo stesso
   seed, non più la stessa run — il Piano 0 resta il luogo della scelta). Dettaglio esatto
   del contenuto del codice: [Run Manifest and Reproducibility](systems/run-manifest-and-reproducibility.md)
   (fonte unica, rimando, non riformulato qui). Gap di implementazione esplicito: il codice
   attuale non trasporta ancora tema e personaggio (DEC-077).
2. **File RunBundle esportato**, il formato con verifica d'integrità già esistente nel
   progetto: una via completa e verificabile, adatta a gare private e archivio, che trasporta
   già il manifest completo.

Le run condivise in questo modo sono **sempre non classificate**, per coerenza con DEC-062:
la Classificata passa solo dalle gare pubblicate nella lobby o dalla Daily, non da una
condivisione diretta fuori da quei canali. Dettaglio del campo "codice run" e
dell'importazione RunBundle in `RunSetup`: [ui/run-setup.md](ui/run-setup.md) (rimando);
dettaglio del manifest condiviso e della verifica d'integrità:
[Run Manifest and Reproducibility](systems/run-manifest-and-reproducibility.md) (rimando, non
riformulato qui).

## Scenari (DEC-064, DEC-066, DEC-077, DEC-081, DEC-096, DEC-108)

**Scenario: medaglia cosmetica dalla Daily**
- Given un giocatore ottiene un piazzamento in una Classificata giornaliera (Daily)
- When la classifica del giorno si chiude
- Then il giocatore riceve una medaglia o cornice cosmetica nel profilo, senza alcun punto
  sblocco aggiuntivo (DEC-064)

**Scenario: rotazione della Daily alle 00:00 UTC**
- Given una Classificata giornaliera pubblica (Daily) attiva per il giorno corrente
- When l'orologio globale raggiunge le 00:00 UTC
- Then la Daily ruota verso una nuova run scelta centralmente, uguale per tutti i giocatori
  nel mondo nello stesso istante (DEC-081)

**Scenario: condivisione via codice breve**
- Given un giocatore vuole condividere la propria run con un altro giocatore fuori dalle
  classifiche, comprese le scelte di tema e personaggio fatte nel Piano 0
- When genera un codice breve testuale (seed, versione di gioco, tema e personaggio, DEC-077)
- Then chi lo incolla in `RunSetup` ottiene quelle stesse scelte come preselezione nel Piano
  0 e può rigiocare esattamente la stessa run, sempre come run non classificata (DEC-066)

**Scenario: condivisione via RunBundle**
- Given un giocatore vuole archiviare o condividere una run per una gara privata
- When esporta il file RunBundle con verifica d'integrità
- Then chi lo importa ottiene una copia verificabile della run, sempre non classificata
  (DEC-066)

**Scenario: budget vincolato nella Classificata a seed diversi**
- Given una gara Classificata a seed diversi con più partecipanti
- When ciascun seed genera la propria run
- Then tutte le run generate rispettano gli stessi vincoli di budget (difficoltà di nemici
  e boss, rarità disponibili, numero di stanze in banda stretta), senza alcuna correzione
  statistica a posteriori del punteggio (DEC-096)

**Scenario: proposte identiche in una gara a stesso seed**
- Given due giocatori affrontano la stessa gara a stesso seed (o la stessa Daily)
- When ciascuno arriva al Piano 0 della propria run
- Then entrambi vedono lo stesso personaggio alternativo generato e lo stesso tema proposto
  — ciascuno con la propria rosa base sbloccata (DEC-100) — e restano liberi di scegliere
  autonomamente tra le proposte (DEC-108)

## Obiettivo

Permettere a due o più giocatori di affrontare una sfida confrontabile e competere su tempo, punteggio o completamento.

## Gara a stesso seed (asse "Tipo di gara", DEC-021)

- I giocatori ricevono lo stesso manifest di run (stesso seed), coerentemente con la
  visione approvata sopra.
- Contenuti, pool, ordine dei piani e principali opportunità sono identici.
- Il risultato considera tempo, completamento e possibili penalità.
- La run deve essere riproducibile per verifica e replay.
- Poiché la gara è asincrona, ogni giocatore affronta la run separatamente: non è previsto
  un incontro in tempo reale nella stessa sessione.

## Gara a seed diversi (asse "Tipo di gara", DEC-021)

Ogni giocatore affronta una run propria. In modalità Leggera non serve alcuna garanzia di
equivalenza. In modalità Classificata le run diverse devono avere budget di difficoltà
equivalente: il criterio è fissato da DEC-096 — equità **by-design**, stessi vincoli di
budget di generazione per tutti i seed della gara (vedi sezione dedicata sopra, rimando,
non riformulato qui). I valori esatti dei vincoli restano da playtest.

## Informazioni visibili (experimental)

Poiché la modalità classificata approvata è asincrona, il confronto tra giocatori avviene
sul risultato registrato, non su una sessione condivisa in tempo reale. Da definire, come
dettaglio `experimental`, quali informazioni della run registrata di un avversario possono
essere mostrate, ad esempio:

- piano raggiunto e tempo totale dell'avversario, a run conclusa;
- eventi principali della sua run, evitando di rivelare informazioni strategiche eccessive
  prima che il giocatore corrente completi la propria.

## Classifiche

Le classifiche della Classificata sono divise per metrica — tempo e punteggio separati,
non combinati (DEC-062, sopra) — su ciascuna delle tre istanze (stesso seed, seed diversi,
Daily). Altre categorie restano `experimental`:

- serie di vittorie;
- stagione o set di run ufficiali;
- modalità caos separata.

## Correttezza

Le classifiche richiedono regole stabili, identificazione della versione di gioco e manifest verificabile.

## Decisioni aperte

Risolte da DEC-016, DEC-021, DEC-062, DEC-064, DEC-066, DEC-077, DEC-081, DEC-096 e DEC-108
(non più aperte): multiplayer simultaneo o asincrono → **asincrono**; struttura del menu →
**Leggera/Classificata × stesso seed/seed diversi**; numero e natura delle istanze di
Classificata → **tre istanze (stesso seed, seed diversi, Daily)**; come si dividono le
classifiche → **per metrica, tempo e punteggio separati**; ricompense della Daily →
**cosmetiche, medaglie/cornici** (DEC-064); orario di rotazione della Daily → **00:00 UTC,
istante globale unico** (DEC-081); condivisione di una run fuori classifica → **codice breve
(seed, versione di gioco, tema e personaggio, DEC-077) o file RunBundle, sempre non
classificata** (DEC-066); equità della Classificata a seed diversi → **budget di
generazione vincolato, stessi vincoli per ogni seed della gara, nessuna correzione a
posteriori** (DEC-096); scelta del personaggio nelle gare a stesso seed e nella Daily →
**proposte identiche per tutti i partecipanti, scelta libera e strategica** (DEC-108).

Ancora aperte, tutte `experimental` o registrate in `governance/open-questions.md`:

- Quali assistenze e mod sono consentite.
- Quali informazioni della run di un avversario mostrare e quando.
- I valori esatti dei vincoli di budget della Classificata a seed diversi (DEC-096 fissa il
  criterio, non i numeri, vedi punto 9 di `governance/open-questions.md`).
