---
id: gd-system-save-meta
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Cosa persiste tra le run (DEC-015): catalogo contenuti, museo del Piano 0, punti singleplayer per sblocchi; niente potenziamenti permanenti del personaggio."
---

# Save and Meta Progression

## Intento per il giocatore

Perdere una run non deve mai corrompere il profilo del giocatore. La progressione persistente
amplia le possibilità di gioco (più contenuti nei pool), ma non deve mai rendere irrilevante
la competenza del giocatore dentro la singola run.

## Condizioni di ingresso

Si applica a ogni run completata o interrotta in modalità singleplayer; le modalità
competitive interagiscono con questo sistema solo in lettura limitata (vedi sotto).

## Input/azioni

- una run singleplayer termina (vittoria, permadeath, o abbandono);
- un contenuto generato durante la run raggiunge lo stato approvato-per-run (vedi
  `generated-content-validation.md`);
- il giocatore guadagna punti in una run singleplayer;
- il giocatore spende punti per sbloccare contenuti generati nei pool delle run future.

## Cosa persiste tra le run (DEC-015)

- **Il catalogo di tutti i contenuti generati**: ogni contenuto approvato-per-run in qualunque
  run singleplayer entra nel catalogo permanente del profilo.
- **Il museo del Piano 0**, che raccoglie i migliori contenuti tra quelli catalogati (vedi
  `floor-zero.md` per cosa sia il museo).
- **I punti guadagnati solo in singleplayer**, spendibili per sbloccare contenuti generati nei
  pool delle run future.

## Cosa NON persiste

Nessun potenziamento permanente del personaggio: non esiste una progressione di potenza
persistente. Ogni run parte dallo stesso punto di partenza in termini di forza del
personaggio; ciò che cambia tra run è l'ampiezza dei pool di contenuti generati disponibili,
non la potenza di base del personaggio.

## Risultato

Un profilo che accumula un catalogo di contenuti e punti spendibili, senza mai alterare
l'equilibrio di potenza di partenza di una run.

## Feedback

Le rappresentazioni concrete del catalogo, del museo e degli sblocchi (schermate, elenchi,
notifiche) restano fuori scope qui e vivono nei documenti di `ui/`; questo documento descrive
solo cosa persiste e con quali regole.

## Interazioni

- `floor-zero.md`: il museo del Piano 0 espone i migliori contenuti del catalogo persistente.
- `run-manifest-and-reproducibility.md`: contesto sulle gare asincrone per cui gli sblocchi
  sono disattivati.
- `generated-content-validation.md`: solo i contenuti approvati-per-run entrano nel catalogo
  (i fallback-usati restano un caso da chiarire, vedi Domande aperte).

## Regole per contenuti generati

- Un contenuto entra nel catalogo permanente solo se ha raggiunto lo stato approvato-per-run
  in una run singleplayer.
- Gli sblocchi acquistati con i punti ampliano i pool disponibili nelle run future, ma non
  garantiscono la comparsa di un contenuto specifico in una run specifica (restano soggetti
  alle stesse regole di generazione e di peso nel pool).

## Sblocchi disattivati nelle modalità competitive

Gli sblocchi acquisiti con i punti di meta-progressione sono **disattivati** nelle modalità
competitive: una gara asincrona su un manifest condiviso usa i pool di base, non i pool
ampliati dagli sblocchi personali di un giocatore (vedi `run-manifest-and-reproducibility.md`
per il contesto delle gare asincrone).

## Casi limite

- Un giocatore con un catalogo molto ampio non ottiene alcun vantaggio di potenza: solo più
  varietà potenziale nei pool delle run future singleplayer.
- Una run interrotta a metà (abbandono) non deve corrompere il profilo: i contenuti già
  approvati-per-run fino a quel momento restano acquisiti nel catalogo secondo le stesse
  regole di una run completata.

## Fallback

Questo sistema non definisce la regola di fallback per i contenuti generati: vedi
`generated-content-validation.md`.

## Non-obiettivi

- Nessuna forma di potenziamento permanente del personaggio (statistiche, vite extra,
  vantaggi di potere che persistono tra run): esplicitamente escluso.
- Questo documento non definisce l'interfaccia di catalogo/museo/sblocchi (vedi `ui/`).
- Non definisce il tasso di conversione punti-sblocco (valore non deciso qui).

## Domande aperte residue

- Se i contenuti fallback-usati durante una run singleplayer entrano comunque nel catalogo, o
  solo i contenuti approvati-per-run (vedi anche `generated-content-validation.md`).
- Il tasso esatto di guadagno e spesa dei punti di meta-progressione (non deciso).
- Se il museo del Piano 0 mostra l'intero catalogo o solo un sottoinsieme curato dei migliori
  contenuti (vedi `floor-zero.md`).

## Idee future (experimental)

Sezione dedicata a idee parcheggiate, non requisiti attuali.

- **DEC-018 — Lobby online custom con contenuti sbloccati**: l'idea di lobby online
  personalizzate che usano i contenuti sbloccati dal catalogo di un giocatore è un'idea
  futura, non un requisito attuale. Non è stata progettata nel dettaglio e non deve essere
  trattata come funzionalità pianificata.

## Scenari

**Scenario: un contenuto approvato entra nel catalogo permanente**
- Given un giocatore sta giocando una run singleplayer,
- When un oggetto generato durante quella run raggiunge lo stato approvato-per-run,
- Then quel contenuto entra nel catalogo permanente del profilo, disponibile per essere
  esposto nel museo del Piano 0 e nei pool di run future.

**Scenario: permadeath non corrompe il profilo**
- Given un giocatore perde una run singleplayer (salute a zero, permadeath),
- When la run termina,
- Then il catalogo dei contenuti già approvati durante quella run e i punti già guadagnati
  restano acquisiti nel profilo, senza alcuna perdita del profilo stesso.

**Scenario: nessun potenziamento permanente altera la run successiva**
- Given un giocatore ha accumulato un catalogo ampio e molti punti spesi in sblocchi,
- When avvia una nuova run,
- Then il personaggio parte con la stessa potenza di base di qualunque altra run: cambia solo
  l'ampiezza dei pool di contenuti generati disponibili, non la forza di partenza.

**Scenario: sblocchi disattivati in una gara asincrona**
- Given un giocatore ha sbloccato contenuti extra spendendo punti guadagnati in singleplayer,
- When entra in una gara asincrona su un manifest di run condiviso,
- Then quegli sblocchi non si applicano: la run competitiva usa i pool di base, non ampliati.
