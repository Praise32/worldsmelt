---
id: gd-system-run-manifest
title: Run Manifest and Reproducibility
domain: design
status: approved
authority: canonical
owner: design
summary: "Identità della run, riproducibilità, e base del multiplayer asincrono (DEC-016); condivisione del manifest tramite codice breve (seed, versione di gioco, tema e personaggio scelti, DEC-077) o file RunBundle, sempre non classificata (DEC-066); dettagli multiplayer restano experimental."
last_reviewed: 2026-07-19
last_verified_commit: 0ec60d0
topics: [manifest, seed, riproducibilità, multiplayer-asincrono, run-bundle]
related: []
supersedes: []
source_files: []
---

# Run Manifest and Reproducibility

## Intento per il giocatore

Ogni run deve avere una descrizione stabile sufficiente a ricostruire contenuti, regole e
versione del gioco, così che una run possa essere condivisa, confrontata o rivista senza
ambiguità.

## Condizioni di ingresso

Il manifest esiste per ogni run avviata, dal Piano 0 in poi, indipendentemente dalla modalità
(singleplayer o gara asincrona).

## Input/azioni

- avvio di una run (genera un identificatore e fissa il seed);
- ogni contenuto che raggiunge lo stato approvato-per-run o fallback-usato (vedi
  `generated-content-validation.md`) viene registrato nel manifest;
- condivisione del manifest per una gara asincrona;
- richiesta di replay, verifica o confronto tra giocatori.

## Risultato

Un manifest completo che descrive: identificatore della run, versione delle regole, modalità,
pool disponibili, piani e contenuti approvati, modificatori, regole competitive, fallback
avvenuti.

## Feedback

Il manifest è un'informazione concettuale, non necessariamente visibile in dettaglio al
giocatore durante la partita; le sue eventuali rappresentazioni in interfaccia (es. codice run
condivisibile, schermata risultati) restano fuori scope qui e vivono nei documenti di `ui/`.

## Interazioni

- `generated-content-validation.md`: il manifest registra quali contenuti sono
  approvati-per-run e quali fallback-usati.
- `save-and-meta-progression.md`: i punti sblocco guadagnati in singleplayer sono esclusi dalle
  modalità competitive (vedi quel documento per il dettaglio).
- `08-multiplayer-and-competition.md`: fonte unica della regola che le run condivise tramite
  codice breve o RunBundle sono sempre non classificate (DEC-066).
- `ui/run-setup.md`: rappresentazione in interfaccia del codice breve e dell'importazione
  RunBundle (DEC-066).

## Regole per contenuti generati

Un manifest deve rendere ricostruibile ogni contenuto approvato-per-run e ogni fallback-usato
della run, così che due esecuzioni con lo stesso manifest producano gli stessi contenuti.

## Multiplayer asincrono (DEC-016)

La visione fissata per il multiplayer è: **gare asincrone sulla stessa run**. Lo stesso
seed/manifest di run garantisce che due giocatori affrontino contenuti identici — piani,
stanze, nemici e oggetti generati sono gli stessi per entrambi, perché il determinismo della
generazione esiste già nel sistema (lo stesso seed produce sempre lo stesso esito di
generazione, indipendentemente da quando o da chi gioca). È il manifest di run a rendere
possibili le classifiche a tempo o a punteggio su queste gare: senza un manifest condiviso e
riproducibile non ci sarebbe base per confrontare i risultati di due giocatori diversi.

I punti guadagnati per la meta-progressione e i relativi sblocchi sono esclusi dalle modalità
competitive (vedi `save-and-meta-progression.md`).

Ogni altro dettaglio del multiplayer oltre a questa visione fissata (matchmaking, formato
delle classifiche, gestione di lobby, sincronizzazione di eventi non deterministici) resta in
stato **experimental**: non è stato deciso nel dettaglio.

## Condivisione del manifest: codice breve e RunBundle (DEC-066/DEC-077)

Il manifest di run può essere condiviso fuori dalle classifiche in due forme, entrambe
descritte in dettaglio nei documenti di interfaccia:

1. **Codice breve testuale** (seed, versione di gioco, tema scelto e personaggio scelto —
   DEC-077): la forma minima, sufficiente perché chi lo riceve rigeneri localmente lo stesso
   manifest e riparta esattamente dalla stessa run, a patto che la versione di gioco combaci
   (vedi [ui/run-setup.md](../ui/run-setup.md)). Tema e personaggio trasportati dal codice
   arrivano nel Piano 0 come **preselezione**: il giocatore resta libero di cambiarli, ma a
   quel punto sta giocando una run con lo stesso seed, non più la stessa run — il Piano 0
   resta il luogo della scelta. **Gap di implementazione esplicito:** il codice attuale
   trasporta solo seed più versione di gioco; l'estensione a tema e personaggio non è ancora
   implementata.
2. **File RunBundle esportato**: il formato con **verifica d'integrità** già esistente nel
   progetto, che porta con sé il manifest completo e i contenuti registrati, adatto a gare
   private e archivio. Resta la via completa e verificabile, indipendentemente dall'estensione
   del codice breve (DEC-077).

In entrambi i casi la run risultante è sempre **non classificata** (DEC-066, coerenza con
DEC-062): fonte unica della regola in
[08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md#condivisione-run-a-due-vie-dec-066)
(rimando, non riformulato qui). Questo documento non definisce il formato tecnico esatto del
RunBundle o del codice breve: resta fuori scope (vedi Non-obiettivi).

## Regola competitiva

Una run classificata non può cambiare dopo l'avvio in modo non deterministico tra concorrenti,
salvo eventi esplicitamente sincronizzati e verificabili.

## Casi limite

- Se un contenuto della run è stato sostituito da un fallback-usato, il manifest deve
  registrarlo comunque: la riproducibilità vale anche per le run che hanno usato la riserva
  curata.
- Una run con modificatori o pool sbloccati da meta-progressione non è comparabile in
  classifica con una run che non li ha: per questo gli sblocchi restano esclusi dalle
  modalità competitive.
- Un codice breve importato ha una versione di gioco diversa da quella corrente: la
  rigenerazione locale non è garantita identica; il gioco deve segnalarlo (dettaglio in
  `ui/run-setup.md`).
- Un file RunBundle non supera la verifica d'integrità: l'importazione va rifiutata con un
  errore chiaro, senza tentare una ricostruzione parziale.

## Fallback

Il manifest non definisce la regola di fallback: registra soltanto quali fallback sono
avvenuti. La regola stessa è definita in `generated-content-validation.md`.

## Non-obiettivi

- Questo documento non specifica il formato tecnico del manifest (nessuna scelta di
  tecnologia o struttura dati qui).
- Non definisce matchmaking, lobby o interfaccia delle classifiche: restano dettagli
  experimental o fuori scope.

## Domande aperte residue

- Come si sincronizzano, se necessario, eventi non deterministici in una gara asincrona
  (experimental).
- Che formato assume la condivisione di un manifest tra giocatori (experimental).
- Se e come un manifest di run singleplayer può essere "promosso" a run competitiva dopo il
  fatto (non deciso).

## Scenari

**Scenario: due giocatori affrontano la stessa run asincrona**
- Given due giocatori scelgono la stessa run condivisa, identificata dallo stesso
  manifest/seed,
- When ciascuno gioca la propria sessione in modo asincrono, in momenti diversi,
- Then entrambi affrontano esattamente gli stessi piani, stanze, nemici e oggetti generati,
  perché il manifest fissa il seed di generazione.

**Scenario: il manifest registra un fallback avvenuto**
- Given una run è stata generata con alcuni contenuti approvati-per-run e un fallback-usato al
  posto di un contenuto che non ha superato la simulazione,
- When il manifest della run viene salvato,
- Then registra sia i contenuti approvati sia il fallback avvenuto, così la riproduzione resta
  fedele anche quando la generazione ha usato la riserva curata.

**Scenario: classifica a tempo su gara asincrona**
- Given una gara asincrona è aperta su un manifest di run condiviso,
- When un giocatore completa la run e ottiene un tempo o un punteggio,
- Then il risultato è confrontabile con quello di un altro giocatore solo perché entrambi
  hanno giocato lo stesso manifest di run (stesso seed, stessi contenuti).

**Scenario: gli sblocchi di meta-progressione non alterano la gara competitiva**
- Given un giocatore ha sbloccato contenuti extra in singleplayer tramite punti di
  meta-progressione,
- When quel giocatore entra in una gara asincrona su un manifest condiviso,
- Then quegli sblocchi non alterano il manifest della run competitiva: i pool sbloccati sono
  esclusi dalle modalità competitive.

**Scenario: condivisione tramite codice breve**
- Given un giocatore genera un codice breve (seed, versione di gioco, tema scelto e
  personaggio scelto) dalla propria run,
- When un altro giocatore con la stessa versione di gioco lo incolla in `RunSetup`,
- Then tema e personaggio arrivano preselezionati nel Piano 0, il manifest rigenerato è
  identico e la run che ne risulta è sempre non classificata (DEC-066).

**Scenario: il giocatore cambia una scelta preselezionata dal codice breve**
- Given un giocatore ha importato un codice breve che preseleziona tema e personaggio nel
  Piano 0,
- When cambia una di quelle scelte prima di avviare la run,
- Then ottiene una run con lo stesso seed ma non più la stessa run originale, perché il
  Piano 0 resta il luogo della scelta (DEC-077).

**Scenario: condivisione tramite RunBundle**
- Given un giocatore esporta un file RunBundle della propria run,
- When un altro giocatore lo importa e la verifica d'integrità passa,
- Then ottiene una ricostruzione verificabile del manifest completo, sempre come run non
  classificata (DEC-066).
