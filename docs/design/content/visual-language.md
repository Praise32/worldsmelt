---
id: gd-content-visual-language
title: Visual Language
domain: design
status: approved
authority: canonical
owner: design
summary: "Fonte unica dei 7 strati di trasformazione visiva usati per fusioni e sinergie in tutta la KB. L'aspetto è uno dei quattro assi dell'escalation leggibile del tema per piano (DEC-024). Fonte unica della regola: tutto il gioco, UI compresa, è pixel art (DEC-046). Fonte unica anche dei 6 slot visivi degli oggetti sul personaggio, comuni a sprite curati e generati (DEC-049). Fonte unica anche della silhouette iconica stabile delle risorse fisse tra i World, con gap di implementazione noto (DEC-073b). Fonte unica anche della palette ufficiale «Fucina di Worldsmelt», 31 colori, esplicitamente non-neon (DEC-173). Fonte unica anche dello stile pixel-art ufficiale «S1 – outline nero» e della scala base sprite 24px, chiude la domanda aperta 12 (DEC-176)."
last_reviewed: 2026-07-28
last_verified_commit: 0ec60d0
topics: [linguaggio visivo, pixel art, 7 strati, 6 slot visivi, DEC-046, DEC-049, DEC-073b, palette, fucina, DEC-173, stile S1, outline nero, scala 24px, DEC-176]
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
rimandano a questa regola senza riformularla: `ui/hud.md`. **Eccezione (DEC-176):** la
**scala base degli sprite pixel-art** non è più un default di implementazione ma un
valore di design approvato — vedi «Stile pixel-art ufficiale e scala base sprite» sotto.

## Palette ufficiale «Fucina di Worldsmelt» (DEC-173)

Il gioco ha una **palette ufficiale unica**, chiamata **«Fucina di Worldsmelt»**: 31 colori
custom, **esplicitamente non-neon**. La palette copre HUD, sprite originali e il remap
batch dei 189 sprite curati CC0 (vedi [Asset Curation and Floor Zero](../../ai-production/17-ASSET-CURATION-AND-FLOOR-ZERO.md)):
qualunque pixel art del gioco pesca da questi 31 colori, non da una palette generica o da
una libreria di terze parti.

La palette è stata scelta dal proprietario il 28/07 dopo un confronto visivo su 4
proposte con sprite rimappati. Alternative scartate: **Endesga 32**, **Resurrect 64** e
**Apollo** (palette canoniche di terze parti); il mantenimento del look **neon** in uso
fino a questo punto, esplicitamente non gradito.

La fonte operativa per Aseprite è il file `.gpl` canonico nel repository,
`assets/art-src/palette/worldsmelt-fucina.gpl` (commit `e358d5f`; prima di quel commit
viveva solo fuori dal repository, in `~/tools/aseprite-workspace/`) — questa sezione ne
copia i valori RGB/hex come riferimento di design, senza sostituirlo come strumento di
lavoro.

### I 31 colori

| Nome | Hex | RGB | Famiglia |
|---|---|---|---|
| slag-nero | `#14100E` | 20, 16, 14 | Bronzo/metallo |
| slag-scuro | `#241A16` | 36, 26, 22 | Bronzo/metallo |
| slag-caldo | `#3A2620` | 58, 38, 32 | Bronzo/metallo |
| terra-bruciata | `#55352A` | 85, 53, 42 | Bronzo/metallo |
| bronzo-scuro | `#7A4A2B` | 122, 74, 43 | Bronzo/metallo |
| bronzo | `#9C6526` | 156, 101, 38 | Bronzo/metallo |
| bronzo-chiaro | `#C98A2E` | 201, 138, 46 | Bronzo/metallo |
| oro-fuso | `#E8B74A` | 232, 183, 74 | Bronzo/metallo |
| oro-pallido | `#F5DF8F` | 245, 223, 143 | Bronzo/metallo |
| brace-scura | `#7E2216` | 126, 34, 22 | Brace/fiamma |
| brace | `#B13A1E` | 177, 58, 30 | Brace/fiamma |
| fiamma | `#E05B23` | 224, 91, 35 | Brace/fiamma |
| fiamma-chiara | `#F7913E` | 247, 145, 62 | Brace/fiamma |
| bagliore | `#FFC46B` | 255, 196, 107 | Brace/fiamma |
| cenere-nera | `#2B2B31` | 43, 43, 49 | Cenere/neutri |
| cenere-scura | `#4A4A55` | 74, 74, 85 | Cenere/neutri |
| cenere | `#737382` | 115, 115, 130 | Cenere/neutri |
| cenere-chiara | `#A7A7B5` | 167, 167, 181 | Cenere/neutri |
| fumo | `#D8D8E0` | 216, 216, 224 | Cenere/neutri |
| bianco-caldo | `#F4F2EC` | 244, 242, 236 | Cenere/neutri |
| verderame-scuro | `#1E4D44` | 30, 77, 68 | Verderame (accento) |
| verderame | `#3A7D63` | 58, 125, 99 | Verderame (accento) |
| verderame-chiaro | `#6DB388` | 109, 179, 136 | Verderame (accento) |
| patina | `#A8DCA8` | 168, 220, 168 | Verderame (accento) |
| ardesia-scura | `#26303F` | 38, 48, 63 | Ardesia (accento) |
| ardesia | `#40546B` | 64, 84, 107 | Ardesia (accento) |
| ardesia-chiara | `#6A86A0` | 106, 134, 160 | Ardesia (accento) |
| ardesia-pallida | `#9DB6C9` | 157, 182, 201 | Ardesia (accento) |
| prugna-scura | `#5B2A4D` | 91, 42, 77 | Prugna (accento) |
| prugna | `#94406E` | 148, 64, 110 | Prugna (accento) |
| prugna-chiara | `#CF6F96` | 207, 111, 150 | Prugna (accento) |

### Ruoli delle famiglie

- **Bronzo/metallo** (9 colori, dal nero fumo al dorato caldo) — la rampa strutturale
  principale: metallo, terra bruciata, superfici di fonderia. È la famiglia più estesa,
  coerente con l'identità "fucina" del gioco.
- **Brace/fiamma** (5 colori) — calore attivo, fuoco, energia: colore funzionale per
  danno da fuoco/calore (vedi lo strato "Colore funzionale" sopra) e per accenti di
  pericolo/attivazione nell'HUD.
- **Cenere/neutri** (6 colori, dal nero al bianco caldo) — la rampa acromatica per
  ombre, contorni, testo e superfici neutre: non satura, coerente con la regola non-neon.
- **Verderame** (4 colori) — accento freddo/organico (ossidazione, veleno, natura),
  distinto sia dal bronzo caldo sia dall'ardesia.
- **Ardesia** (4 colori) — accento freddo/minerale (pietra, acqua, elementi UI secondari),
  distinto dal verderame per tonalità più blu che verde.
- **Prugna** (4 colori) — accento raro/magico (rarità, elementi speciali), la famiglia più
  satura della palette, usata con parsimonia.

Questa palette **vincola** la variazione per-World di palette e dettagli già prevista da
DEC-073b per la silhouette stabile delle risorse fisse: la variazione resta dentro questi
31 colori, non introduce tinte fuori palette.

## Stile pixel-art ufficiale e scala base sprite (DEC-176)

Oltre alla palette, il gioco ha uno **stile pixel-art ufficiale** e una **scala base**
per gli sprite, entrambi scelti dal proprietario al checkpoint CP1 della produzione
pixel-art dopo un confronto visivo su cinque prove di stile e su tre scale. Vale per
**sprite originali nuovi** e per il **remap batch dei 189 sprite curati CC0**, la stessa
copertura della palette (DEC-173).

### Stile: S1 «outline nero»

- **Outline nero 1px**: ogni sprite ha un contorno di 1 pixel nel colore `slag-nero`
  della palette attorno al perimetro esterno della silhouette.
- **Shading piatto a 2 toni per materiale**: ogni superficie usa un tono base più un solo
  tono d'ombra (nessuna banda intermedia, nessun gradiente).
- **Niente dithering**: nessun pattern di retinatura per simulare toni intermedi.
- **Eccezione dettagli piccoli**: dettagli piccoli e ad alto contrasto (occhi, punte
  luminose, bagliori) possono restare **senza outline interno** quando l'outline li
  renderebbe illeggibili sotto i 24px; l'outline nero resta obbligatorio solo sul
  perimetro esterno della silhouette.
- Scelto fra cinque prove sugli stessi soggetti (goblin, pozione, cuore HUD, cornice
  slot): S1 outline nero (scelto), S2 outline colorato selettivo + cluster shading, S3
  no-outline silhouette + rim-light, S4 2-bit alto contrasto, S5 dithering leggero
  retro. Sorgenti in `assets/art-src/style-tests/` (commit `3752eef`); riferimento
  canonico `assets/art-src/style-tests/S1-outline-nero.aseprite`, preview a
  ingrandimento ×8 in `assets/art-src/style-tests/preview/S1-outline-nero-x8.png`.

### Scala base: 24px

- **24px** è la scala base per **personaggi, nemici e oggetti**.
- I **boss** possono superare questa scala.
- Le **icone HUD seguono la propria griglia**, indipendente dalla scala base: non c'è
  obbligo che mondo e interfaccia condividano la stessa taglia in pixel.
- Scelta dopo un confronto diretto 16px/24px/32px sullo stesso soggetto (goblin): 16px
  scartato per dettaglio insufficiente a comunicare i 7 strati di trasformazione visiva
  definiti sopra (in particolare silhouette e materiale); 32px scartato per il costo di
  produzione più alto sul volume richiesto dal catalogo curato e dal dataset LoRA.

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

**Scenario: la pixel art del gioco resta dentro la palette ufficiale**
- Given un artista che disegna un nuovo sprite originale o rimappa uno sprite curato CC0,
- When sceglie i colori in Aseprite,
- Then usa solo colori della palette «Fucina di Worldsmelt» (31 colori, non-neon), la
  stessa per HUD, sprite originali e remap batch (DEC-173).

**Scenario: uno sprite nuovo rispetta lo stile e la scala ufficiali**
- Given un artista che disegna un nuovo nemico o un nuovo oggetto,
- When lo esporta come sprite di gioco,
- Then l'esterno della silhouette ha un outline `slag-nero` di 1px, le superfici usano
  shading piatto a 2 toni senza dithering, e lo sprite è disegnato alla scala base di
  24px (più grande solo se è un boss), coerente con lo stile S1 (DEC-176).

**Scenario: le icone delle risorse fisse restano riconoscibili tra World diversi**
- Given un giocatore che ha giocato una run nel World A e un'altra nel World B,
- When confronta le icone di Ingots, Cast Keys, Blast Charges, Crust, Flux o Heat nell'HUD
  delle due run,
- Then la silhouette di ciascuna icona è la stessa in entrambi i World; solo palette e
  dettagli possono variare, dentro il budget di leggibilità (DEC-073b).
