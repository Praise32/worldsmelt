# Multiplayer su Steamworks — direzione proposta

**Questo documento NON è una decisione di design.** `docs/technical/` non ridefinisce il
comportamento di gioco (regola di `AGENTS.md`): ogni riferimento a regole, modalità o
meccaniche qui sotto rimanda ai documenti canonici in `docs/game-design/` senza riformularli.
Questa nota registra una **direzione tecnica proposta**, non ancora scelta in via definitiva
dal proprietario del progetto: valuta se e come appoggiare la visione multiplayer già
approvata (`DEC-016`, `DEC-021`, vedi
[Multiplayer and Competition](../game-design/08-multiplayer-and-competition.md) e
[Run Manifest and Reproducibility](../game-design/systems/run-manifest-and-reproducibility.md))
su Steamworks, senza introdurre nuove regole di gioco.

## Perché Steamworks

Il multiplayer asincrono della visione approvata (`DEC-016`: gare asincrone sulla stessa
run/seed, classifiche a tempo/punteggio; `DEC-021`: due assi Leggera/Classificata × stesso
seed/seed diversi) non richiede infrastruttura di sessione in tempo reale. Steamworks offre
componenti che coprono direttamente questi bisogni senza dover costruire un backend proprio
da zero:

- **API C**: adatta al motore in C del progetto, senza livelli di traduzione aggiuntivi.
- **Leaderboards**: classifiche a tempo/punteggio, separabili per asse — Leggera/Classificata
  × stesso seed/seed diversi (`DEC-021`) — usando leaderboard distinte o filtrate per
  combinazione.
- **Identità giocatore**: la Steam ID autentica il giocatore e può dare validità alla run
  pubblicata (chi ha giocato cosa), senza richiedere un sistema account separato.
- **Achievement**: candidati naturali per esporre le prove specifiche della run (`DEC-027`,
  `DEC-042`, vedi [Rewards and Economy](../game-design/systems/rewards-and-economy.md) e
  [Floor Zero](../game-design/systems/floor-zero.md)) come achievement Steam, oltre alla loro
  presentazione in gioco.
- **Workshop/UGC**: possibile canale di pubblicazione dei RunBundle (il formato di run
  condivisibile con hash sha256 già esistente nel progetto, fuori da questa KB) per gare a
  seed condiviso o per condividere run interessanti al di là della classifica ufficiale.
- **Cloud save**: sincronizzazione del profilo persistente (catalogo, punti sblocco, vedi
  [Save and Meta Progression](../game-design/systems/save-and-meta-progression.md)) tra
  macchine dello stesso giocatore.

## Cosa NON decide questa nota

- Non decide se il gioco sarà distribuito su Steam: registra solo cosa Steamworks
  offrirebbe se lo fosse.
- Non ridefinisce le regole di classifica, i criteri di normalizzazione della difficoltà per
  la Classificata a seed diversi (domanda aperta, vedi
  `../game-design/governance/open-questions.md`), né alcun altro comportamento di gioco.
- Non sceglie un formato tecnico definitivo per il manifest o il RunBundle: si appoggia al
  formato già esistente nel progetto (fuori da questa KB) senza modificarlo qui.

## Vincoli noti

- **Distribuzione su Steam**: pubblicare un'app su Steam richiede la fee Steam Direct di
  100$ per titolo (costo una tantum per app, non per aggiornamento).
- **Multiplayer legato all'account Steam**: Leaderboards, identità giocatore, achievement e
  Workshop tramite Steamworks presuppongono un account Steam attivo e il gioco lanciato
  tramite client Steam (o comunque inizializzato via Steamworks SDK).
- **Fallback per build fuori Steam**: se il gioco è distribuito anche fuori da Steam (altri
  store, build standalone), serve un fallback per multiplayer/classifiche — un backend
  proprio alternativo, oppure l'assenza di classifiche online in quella build. Questa nota
  non sceglie tra le due opzioni: è un vincolo da risolvere quando la distribuzione fuori
  Steam diventa concreta.

## Scelta finale

**Non presa.** Questa nota è una direzione tecnica da valutare, non un impegno del progetto
a usare Steamworks. Qualunque scelta finale sull'infrastruttura multiplayer deve comunque
rispettare la visione di design già approvata (`DEC-016`, `DEC-021`) senza alterarla.
