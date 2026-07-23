---
id: gd-system-item-fusion
title: Item Fusion
domain: design
status: approved
authority: canonical
owner: design
summary: "Meccanica-firma: consumare due oggetti e un catalizzatore raro (DEC-022) per ottenere un oggetto composto subito con regole deterministiche e rifinito dall'IA in sottofondo (DEC-023, doppio stadio)."
last_reviewed: 2026-07-22
last_verified_commit: 0ec60d0
topics: [fusione, meccanica-firma, DEC-023, DEC-022, catalizzatore, doppio stadio]
related: []
supersedes: []
source_files: []
---

# Item Fusion

## Intento per il giocatore

La fusione è la meccanica-firma del gioco: trasformare deliberatamente due oggetti già
posseduti in un oggetto nuovo, mai visto prima in quella forma, che porta con sé tratti
riconoscibili di entrambi. È una decisione di build a rischio controllato, non un bonus
automatico.

## Condizioni di ingresso

- Il giocatore deve trovarsi nella stanza di fusione (vedi [Special Rooms](special-rooms.md)).
- Il giocatore deve possedere almeno due oggetti idonei alla fusione.
- Il giocatore deve possedere almeno un catalizzatore di fusione, la risorsa dedicata che
  abilita l'operazione (vedi [Health and Resources](health-and-resources.md)).

## Input/azioni

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Slot oggetto sorgente 1 | Nella stanza di fusione | Il giocatore possiede almeno un oggetto idoneo | Selezionare il primo oggetto da fondere | L'oggetto viene messo in attesa di conferma | L'oggetto selezionato si evidenzia nello slot |
| Slot oggetto sorgente 2 | Nella stanza di fusione | Il giocatore possiede almeno un secondo oggetto idoneo, diverso dal primo | Selezionare il secondo oggetto da fondere | La coppia è pronta per la fusione | Anteprima dei tratti ereditabili dai due oggetti |
| Conferma fusione | Due oggetti sorgente selezionati | Il giocatore possiede almeno un catalizzatore di fusione | Confermare l'operazione | I due oggetti sorgente e un catalizzatore vengono consumati; il gioco compone SUBITO un oggetto risultante con regole deterministiche (eredita trait e strati visivi da entrambi, DEC-023); nome, comportamento e sprite dedicati generati dall'IA si applicano appena pronti, senza attesa per il giocatore | Sequenza di fusione visibile, presentazione immediata dell'oggetto composto; aggiornamento silenzioso quando la rifinitura IA arriva |
| Annulla selezione | Almeno un oggetto sorgente selezionato | Sempre, prima della conferma | Svuotare gli slot | Nessuna risorsa viene consumata | Gli slot tornano vuoti |

## Risultato

I due oggetti sorgente e un catalizzatore di fusione vengono consumati. Il risultato si
costruisce in **doppio stadio** (DEC-023):

1. **Composizione immediata (deterministica):** appena confermata la fusione, il gioco
   compone SUBITO l'oggetto risultante con regole deterministiche, ereditando trait e
   strati visivi (vedi [Visual Language](../content/visual-language.md)) da entrambi gli
   oggetti sorgente. Il giocatore non aspetta mai in questo momento.
2. **Rifinitura in sottofondo (IA):** in parallelo, l'IA genera nome, comportamento e
   sprite dedicati per l'oggetto composto e li applica appena pronti, senza interrompere o
   mettere in pausa il giocatore.

Se la rifinitura IA non arriva (generazione mancante, in errore o respinta), la
composizione deterministica del passo 1 resta valida così com'è: è il fallback naturale di
questa meccanica (vedi [Fallback](#fallback) sotto). L'oggetto risultante entra
nell'inventario nello slot corrispondente alla sua categoria.

## Feedback

- La sequenza di fusione mostra chiaramente quali due oggetti sono stati consumati.
- Il nuovo oggetto viene presentato con un momento dedicato (non un semplice popup di
  drop), coerente con il suo ruolo di meccanica-firma.
- La scheda del nuovo oggetto indica da quali due oggetti sorgente deriva, senza esporre
  dettagli tecnici della generazione.
- Quando la rifinitura IA (nome, comportamento, sprite dedicati) arriva dopo la
  composizione immediata, l'aggiornamento è silenzioso e non interrompe il giocatore
  (DEC-023): l'oggetto composto era già pienamente utilizzabile.

## Interazioni

- [Special Rooms](special-rooms.md): la stanza di fusione è l'unico luogo in cui questa
  meccanica è disponibile.
- [Health and Resources](health-and-resources.md): il catalizzatore di fusione è definito
  lì come risorsa (fonti, cap, visibilità in HUD).
- [Items, Pools and Rarity](items-pools-and-rarity.md): l'oggetto generato rispetta gli
  stessi campi obbligatori di ogni altro oggetto del gioco.
- [Synergies](synergies.md): la fusione esplicita è il secondo binario delle sinergie,
  complementare a quelle implicite/automatiche descritte in quel documento.

## Rarità del catalizzatore (DEC-022)

Il catalizzatore di fusione è una risorsa **rara**: si ottiene da drop di boss o di arene
di sfida, oppure con un acquisto costoso nel negozio (vedi
[Health and Resources](health-and-resources.md) per le regole generali della risorsa e
[Rewards and Economy](rewards-and-economy.md) per l'economia del negozio). Il ritmo atteso
è di **1-2 fusioni per run**: la fusione resta un momento memorabile e una scommessa
strategica, non un'azione disponibile a piacere.

## Regole per contenuti generati

- Ogni fusione deve cambiare sia il comportamento sia la presentazione visiva
  dell'oggetto risultante: non è ammessa una fusione che cambi solo le statistiche o solo
  l'aspetto.
- Per la presentazione visiva del nuovo oggetto si usano gli strati definiti in
  [Visual Language](../content/visual-language.md) (silhouette, materiale, colore
  funzionale, particelle, animazione, segnale d'impatto, indicatore sul personaggio).
- Il numero di strati e di effetti aggiunti dalla fusione rientra nel budget di
  leggibilità definito in [Combat and Projectiles](combat-and-projectiles.md).
- L'oggetto generato dichiara la propria origine come `nuovo` o `composto`, secondo la
  tassonomia unica di origine del contenuto: la composizione deterministica immediata
  (DEC-023, passo 1) dichiara origine `composto`; quando la rifinitura IA (passo 2)
  applica nome, comportamento e sprite dedicati generati, l'origine sale a `nuovo`.
- Il catalizzatore di fusione è una risorsa rara (DEC-022): la cadenza attesa è 1-2
  fusioni per run, quindi il pool di oggetti fusi che il giocatore incontra in una run
  resta piccolo e ogni singolo risultato pesa molto sulla build.

## Priorità e conflitti

Quando i due oggetti sorgente hanno tratti che competono sulla stessa proprietà (per
esempio entrambi modificano la traiettoria del proiettile), si applica in ordine:

1. una regola di fusione esplicita già definita per quella coppia di tratti, se esiste nel
   catalogo dei tratti;
2. il tratto dichiarato dominante da uno dei due oggetti sorgente, se dichiarato;
3. una terza variante generata dall'IA che fonde i due tratti in un comportamento coerente,
   quando la generazione produce un risultato strutturalmente valido;
4. in assenza delle precedenti, vince il tratto dell'oggetto sorgente di rarità più alta;
   a parità di rarità, vince il tratto dell'oggetto selezionato per primo dal giocatore.

## Casi limite

- Il giocatore ha due oggetti idonei ma nessun catalizzatore di fusione: la conferma resta
  disabilitata e l'interfaccia lo segnala.
- I due oggetti selezionati appartengono a categorie diverse (per esempio un attivo e un
  Innesto): la fusione è **libera tra categorie diverse** (DEC-101), è anzi il tipo di
  sorpresa che rende la meccanica memorabile. L'oggetto risultante dichiara la propria
  categoria e resta dentro il budget e le regole di validazione dei contenuti generati (vedi
  [Generated Content Validation](generated-content-validation.md)).
- L'oggetto risultante di una fusione precedente viene selezionato come oggetto sorgente
  per una nuova fusione: **ammesso, nessun limite concettuale** (DEC-102). La cadenza
  attesa di 1-2 fusioni per run (DEC-022, vedi la sezione "Rarità del catalizzatore" sopra)
  limita già naturalmente le catene. Resta aperta la domanda su un eventuale limite rigido
  al numero totale di fusioni per run (vedi domande aperte).
- La rifinitura IA (nome, comportamento, sprite dedicati) fallisce, non arriva o non supera
  la validazione dopo che il giocatore ha confermato: la composizione deterministica
  immediata (DEC-023, passo 1) resta l'oggetto valido e utilizzabile, senza che il
  giocatore perda mai la fusione o resti in attesa (fallback naturale, vedi
  [Fallback](#fallback) sotto).

## Fallback

Se la rifinitura IA (passo 2 di DEC-023) non è disponibile o non è valida, si applica la
regola di fallback unica definita in
[Generated Content Validation](generated-content-validation.md): qui il fallback ha una
forma concreta e naturale, perché la composizione deterministica del passo 1 è già un
oggetto valido e utilizzabile, non un'attesa o un contenuto rotto.

## Non-obiettivi

- La fusione non è un sistema di crafting con ricette fisse e prevedibili: il risultato
  esatto non è mai conoscibile in anticipo dal giocatore.
- La fusione non permette di annullare un'operazione già confermata.
- La fusione non è obbligatoria per completare una run.

## Domande aperte residue

- ~~Esiste un limite rigido al numero di fusioni per run~~: risolto da DEC-125 — nessun
  tetto artificiale, il limite è l'economia del catalizzatore (cadenza attesa 1-2 resta il
  riferimento di bilanciamento).
- ~~Quale cap massimo ha il catalizzatore di fusione~~: risolto da DEC-129 — **nessun
  cap**, accumulo libero, il limite è la rarità delle fonti (vedi
  [Health and Resources](health-and-resources.md)).

## Scenari

**Scenario: fusione riuscita**
- Given il giocatore è nella stanza di fusione con due oggetti idonei e un catalizzatore
  di fusione
- When il giocatore seleziona i due oggetti e conferma la fusione
- Then i due oggetti sorgente e il catalizzatore vengono consumati e il giocatore riceve
  subito un oggetto composto con regole deterministiche che eredita trait e strati visivi
  da entrambi (DEC-023), rifinito in seguito dall'IA con nome, comportamento e sprite
  dedicati

**Scenario: catalizzatore mancante**
- Given il giocatore è nella stanza di fusione con due oggetti idonei ma senza
  catalizzatore di fusione
- When il giocatore prova a confermare la fusione
- Then la conferma resta disabilitata e l'interfaccia segnala l'assenza del catalizzatore

**Scenario: tratti in conflitto sulla stessa proprietà**
- Given i due oggetti sorgente modificano entrambi la stessa proprietà del proiettile
- When la fusione viene confermata
- Then si applica l'ordine di priorità definito, e il risultato dichiara quale regola ha
  determinato l'esito

**Scenario: generazione non valida dopo conferma**
- Given il giocatore ha confermato una fusione
- When la rifinitura IA (nome, comportamento, sprite dedicati) non supera la validazione
- Then la composizione deterministica immediata resta l'oggetto valido del giocatore
  (fallback naturale, DEC-023), senza perdere la fusione a vuoto

**Scenario: catalizzatore raro e cadenza attesa**
- Given una run in corso in cui il giocatore ha ottenuto catalizzatori solo da drop di
  boss/arene o da un acquisto costoso
- When si conta il numero di fusioni completate a fine run
- Then il numero rientra tipicamente in 1-2 fusioni per run (DEC-022), coerente con la
  fusione come momento raro e memorabile

**Scenario: composizione immediata seguita da rifinitura IA**
- Given il giocatore conferma una fusione con due oggetti idonei e un catalizzatore
- When il gioco compone subito il risultato con regole deterministiche
- Then il giocatore riceve immediatamente un oggetto utilizzabile che eredita trait e
  strati visivi da entrambi i genitori, e in seguito nome, comportamento e sprite dedicati
  generati dall'IA si applicano senza interrompere la partita

**Scenario: fusione tra categorie diverse**
- Given il giocatore ha nella stanza di fusione un oggetto attivo e un oggetto passivo,
  entrambi idonei, e un catalizzatore di fusione
- When il giocatore seleziona i due oggetti di categorie diverse e conferma la fusione
- Then l'operazione è ammessa (DEC-101) e l'oggetto risultante dichiara la propria
  categoria, restando dentro le regole di validazione dei contenuti generati

**Scenario: oggetto fuso riusato come sorgente**
- Given il giocatore possiede un oggetto nato da una fusione precedente, un secondo oggetto
  idoneo e un catalizzatore di fusione
- When il giocatore seleziona l'oggetto fuso come uno dei due sorgente e conferma una nuova
  fusione
- Then l'operazione procede senza limiti concettuali (DEC-102), e la cadenza attesa di 1-2
  fusioni per run (DEC-022) continua a limitare naturalmente le catene
