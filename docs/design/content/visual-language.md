---
id: gd-content-visual-language
title: Visual Language
domain: design
status: approved
authority: canonical
owner: design
summary: "Fonte unica dei 7 strati di trasformazione visiva usati per fusioni e sinergie in tutta la KB. L'aspetto è uno dei quattro assi dell'escalation leggibile del tema per piano (DEC-024). Fonte unica della regola: tutto il gioco, UI compresa, è pixel art (DEC-046). Fonte unica anche dei 6 slot visivi degli oggetti sul personaggio, comuni a sprite curati e generati (DEC-049). Fonte unica anche della silhouette iconica stabile delle risorse fisse tra i World, con gap di implementazione noto (DEC-073b)."
last_reviewed: 2026-07-18
last_verified_commit: 0ec60d0
topics: [linguaggio visivo, pixel art, 7 strati, 6 slot visivi, DEC-046, DEC-049, DEC-073b]
related: []
supersedes: []
source_files: []
---

# Visual Language

Questo documento è la fonte UNICA dei 7 strati di trasformazione visiva usati per fusioni e
sinergie in tutta la knowledge base. `synergies.md` e `item-fusion.md` (in `systems/`)
descrivono le trasformazioni usando esattamente questo vocabolario: questo documento deve
reggersi da solo, senza richiedere la lettura di quei documenti.

## Obiettivi

- distinguere giocatore, nemici, proiettili e ricompense;
- comunicare ruolo e pericolo;
- mostrare sinergie senza accumulo caotico;
- mantenere identità originale.

## I 7 strati di una trasformazione visiva

Ogni trasformazione visiva (sinergia implicita o fusione esplicita) si costruisce combinando
un sottoinsieme di questi 7 strati. Uno strato può applicarsi al proiettile, al personaggio o
all'ambiente, secondo la natura della trasformazione.

1. **Silhouette** — la forma esterna riconoscibile dell'elemento (proiettile, nemico,
   personaggio). Cambia il profilo geometrico immediatamente leggibile a colpo d'occhio, prima
   di ogni dettaglio interno; è lo strato più determinante per il riconoscimento a distanza.
2. **Materiale** — la superficie e la texture percepita (metallico, organico, cristallino,
   liquido, ecc.). Si applica soprattutto a proiettili e ambiente, comunicando la natura fisica
   dell'effetto senza richiedere lettura ravvicinata.
3. **Colore funzionale** — la palette che comunica ruolo o effetto (es. una famiglia di colori
   costante per un certo tipo di danno o comportamento). Si applica a proiettili, indicatori
   ambientali e accenti sul personaggio; è distinto dal colore puramente estetico perché porta
   informazione meccanica.
4. **Particelle** — gli effetti dinamici emessi dall'elemento (scintille, fumo, gocciolamento,
   scariche). Si applicano soprattutto a proiettili e punti di impatto, e comunicano
   continuità di un effetto nel tempo (es. un effetto che persiste dopo il colpo).
5. **Animazione** — il modo in cui l'elemento si muove nello spazio e nel tempo (traiettoria,
   ritmo, deformazione). Si applica a proiettili, nemici e personaggio; comunica
   comportamento, non solo aspetto (es. una traiettoria ondulata comunica un movimento
   diverso da una rettilinea anche a parità di silhouette).
6. **Segnale d'impatto** — l'effetto visivo mostrato nel momento del contatto (esplosione,
   scintilla, increspatura, dissolvenza). Si applica al punto di collisione tra proiettile e
   bersaglio o ambiente; è lo strato che comunica l'esito immediato di un'azione.
7. **Indicatore sul personaggio** — un segnale persistente sul personaggio giocante che
   comunica quali effetti/sinergie sono attivi (un accento visivo stabile, non un'animazione
   isolata). Si applica esclusivamente al personaggio ed è lo strato che permette al
   giocatore di ricordare, senza aprire un menu, quali combinazioni sono in corso.

## Regola di fusione

Quando più oggetti modificano lo stesso elemento, usare una gerarchia coerente di strati e un
limite di complessità. Gli effetti secondari possono essere rappresentati con accenti (es.
solo colore funzionale o solo particelle) anziché trasformazioni complete su tutti e 7 gli
strati.

La fusione esplicita degli oggetti (vedi `../systems/item-fusion.md`, non riformulare) deve
produrre una trasformazione visiva coerente usando questi 7 strati, non un semplice collage
degli aspetti dei due oggetti di partenza.

## Escalation del tema tra piani (DEC-024)

L'aspetto è uno dei quattro assi su cui il tema di una run si intensifica piano dopo piano
(vedi [Difficulty and Progression](../07-difficulty-and-progression.md) per il principio
generale, non riformulato qui). Anche nei piani più avanzati, l'intensificazione dei 7
strati sopra definiti deve restare uno schema visivo comprensibile: il budget di
leggibilità di [Combat and Projectiles](../systems/combat-and-projectiles.md) vale su tutti
i piani, senza eccezioni per i piani finali.

## Pixel art come linguaggio canonico totale (DEC-046)

Tutto il gioco si basa sulla **pixel art**: non solo gli elementi di gioco (personaggio,
nemici, proiettili, oggetti, ambiente), ma anche l'**intera interfaccia** — menu, HUD, font,
indicatori. La UI è **custom e in pixel art anch'essa**, non un linguaggio pulito non-pixel
separato dal resto del gioco. I 7 strati di trasformazione visiva definiti sopra si
esprimono quindi sempre in pixel art, e lo stesso vale per gli elementi di interfaccia che
comunicano stato di gioco (es. l'indicatore sul personaggio, strato 7).

Le risoluzioni di riferimento attuali (atlas generati, resa a campionamento a punto) sono
**default dell'implementazione**, nello stesso stato `draft` dei valori numerici di DEC-019:
documentate come default proposto, non come valore di design definitivo. Fonti che
rimandano a questa regola senza riformularla: `ui/hud.md`.

## Slot visivi degli oggetti sul personaggio (DEC-049)

Gli oggetti equipaggiati si sovrappongono al personaggio attraverso **6 slot visivi** fissi.
Questi slot si applicano identicamente a **tutti** i personaggi, indipendentemente
dall'origine del loro sprite: sia ai personaggi della rosa base, con sprite pixel art curati
a mano, sia al personaggio alternativo generato per la run, con sprite generato dalla stessa
pipeline usata per i nemici (vedi [Characters](../systems/characters.md), DEC-049, fonte del
dettaglio sulla scelta curato/generato; questo documento non lo ripete).

I 6 slot visivi sono un concetto distinto dai 7 strati di trasformazione visiva definiti
sopra: gli strati descrivono COME una trasformazione visiva si compone (silhouette,
materiale, ecc.), i 6 slot visivi descrivono DOVE sul personaggio un oggetto equipaggiato si
sovrappone visivamente. Il numero esatto di slot funzionali (attivo, Innesto, espandibili —
DEC-011) e la relazione precisa con questi 6 slot visivi restano da chiarire (vedi Domande
aperte residue).

## Silhouette stabile delle risorse fisse (DEC-073b)

Le risorse fisse dell'interfaccia — Ingots, Cast Keys, Blast Charges, Crust, Flux, Heat (nomi
in-game, fonte unica [Glossary](../governance/glossary.md), DEC-072) — hanno una
**silhouette iconica stabile** tra le run: la forma riconoscibile dell'icona di ciascuna
risorsa non cambia da un World all'altro. La variazione per-World è ammessa solo in palette
e dettagli, sempre dentro il budget di leggibilità (fonte unica
[Combat and Projectiles](../systems/combat-and-projectiles.md), non riformulato qui); questa
regola è coerente con la garanzia che nessuna informazione di gioco dipenda dal solo colore
(DEC-058).

**Gap noto rispetto al codice:** il codice attuale genera le icone di valuta, chiave, bomba e
cuore per-run seguendo il tema, senza silhouette stabile. È un requisito di design non ancora
implementato — il codice dovrà adeguarsi a questa regola, non viceversa (stesso trattamento
dei gap già registrati per DEC-009 e DEC-052).

## Accessibilità

Nessuna informazione critica deve dipendere solo dal colore.

## Casi limite

- Se due sinergie attive competono sullo stesso strato (es. entrambe vogliono definire la
  silhouette del proiettile), vale la priorità di composizione definita in `systems/synergies.md`
  (non riformulata qui): questo documento definisce solo il vocabolario degli strati, non
  l'ordine di priorità tra effetti concorrenti.
- Un effetto che modifica solo uno o due strati (es. solo colore funzionale e particelle) è
  legittimo quanto una trasformazione su tutti e 7: non ogni sinergia deve toccare ogni strato.

## Non-obiettivi

- Questo documento non definisce asset, tecniche di rendering o pipeline di produzione
  grafica: definisce solo il vocabolario concettuale degli strati.
- Non definisce l'ordine di priorità tra effetti concorrenti sullo stesso strato (vedi
  `../systems/synergies.md`).

## Domande aperte residue

- Numero massimo di strati leggibili simultaneamente prima che una trasformazione risulti
  confusa (valore non ancora definito).
- Relazione esatta tra i 6 slot visivi (DEC-049) e il numero di slot funzionali di
  inventario (attivo, Innesto, espandibili — DEC-011): coincidono, o sono concetti
  indipendenti?

## Scenari

**Scenario: una sinergia implicita tocca pochi strati**
- Given due oggetti passivi compatibili convivono nell'inventario del giocatore,
- When il sistema attiva una sinergia implicita tra loro,
- Then la trasformazione visiva usa solo gli strati necessari a comunicare l'effetto (es.
  colore funzionale e particelle), senza obbligo di alterare la silhouette.

**Scenario: una fusione esplicita produce una trasformazione coerente su più strati**
- Given il giocatore consuma due oggetti nella stanza di fusione,
- When ottiene l'oggetto nuovo generato dall'IA che eredita comportamento e presentazione da
  entrambi,
- Then la trasformazione visiva risultante coordina silhouette, materiale e colore funzionale
  in modo coerente, e non si limita a sovrapporre gli aspetti dei due oggetti di partenza.

**Scenario: l'indicatore sul personaggio resta leggibile con più sinergie attive**
- Given il giocatore ha più sinergie attive contemporaneamente,
- When osserva il proprio personaggio senza aprire un menu,
- Then l'indicatore sul personaggio comunica in modo persistente quali effetti sono attivi,
  senza dipendere solo dal colore.

**Scenario: l'aspetto si intensifica ma resta leggibile anche nel piano più avanzato**
- Given una run con tema scelto nel Piano 0, arrivata al piano 5,
- When il gioco compone l'aspetto delle minacce e degli elementi di quel piano usando i 7
  strati con l'intensità massima prevista per il tema,
- Then lo schema visivo resta comprensibile e rispetta il budget di leggibilità di
  [Combat and Projectiles](../systems/combat-and-projectiles.md), coerente con DEC-024.

**Scenario: la UI è pixel art quanto gli elementi di gioco**
- Given un giocatore che osserva sia un nemico generato sia l'HUD durante `Gameplay`,
- When confronta lo stile visivo dei due elementi,
- Then entrambi sono resi in pixel art con lo stesso linguaggio, perché la UI non usa uno
  stile pulito non-pixel separato (DEC-046).

**Scenario: gli stessi slot visivi su personaggio curato e generato**
- Given un giocatore che equipaggia lo stesso oggetto su un personaggio della rosa base
  (sprite curato) e, in un'altra run, sul personaggio alternativo (sprite generato),
- When osserva come l'oggetto si sovrappone al personaggio,
- Then l'oggetto occupa lo stesso slot visivo con la stessa logica di sovrapposizione in
  entrambi i casi, perché i 6 slot visivi (DEC-049) sono indipendenti dall'origine dello
  sprite del personaggio.

**Scenario: le icone delle risorse fisse restano riconoscibili tra World diversi**
- Given un giocatore che ha giocato una run nel World A e un'altra nel World B,
- When confronta le icone di Ingots, Cast Keys, Blast Charges, Crust, Flux o Heat nell'HUD
  delle due run,
- Then la silhouette di ciascuna icona è la stessa in entrambi i World; solo palette e
  dettagli possono variare, dentro il budget di leggibilità (DEC-073b).
