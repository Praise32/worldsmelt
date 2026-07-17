---
id: gd-system-run-manifest
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Identità della run, riproducibilità, e base del multiplayer asincrono (DEC-016); dettagli multiplayer restano experimental."
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
