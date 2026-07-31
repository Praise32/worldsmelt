# Visual language Worldsmelt v2

> Stato: **DECISA nelle scelte chiave** il 31/07/2026 sera (sessione remota del
> proprietario): outline = OPZIONE B (nessun outline, stile masse RoR pura),
> voti card accettati, onda 1 = personaggi + nemici. Batch DEC in registrazione
> nel decision-log (supera DEC-176(a); conferma scala DEC-177). Dopo la
> registrazione questo documento va riversato in
> docs/design/content/visual-language.md (fonte canonica).

## Regole fissate dal proprietario (31/07, non negoziabili)

1. **Personaggi senza volto**: vuoto nero sotto cappucci ed elmi. Mai occhi,
   musi o tratti facciali sui personaggi giocabili.
2. **Personaggi senza armi in mano**: le armi sono oggetti equipaggiati
   (overlay su slot DEC-049), mai parte dello sprite base.
3. **Gusto di riferimento**: stile a masse alla Risk of Rain Returns
   (misurato: corpi 22–34px), formato piccolo.

## Formato (evidenza: misure RoR + debts 16-24px + G&E 35 + intellikat 30 + DEC-200)

- Cella personaggio **32×32**, corpo utile **~26px** (fino a 30 per i tozzi).
- Mondo a densità uniforme **32px/tile**; boss = entità multi-tile (64+ su 2 tile).
- Risoluzione interna di gioco 640×360 (DEC-200) → vista da 20×11,25 tile: la
  scala 32 è l'unica che tiene insieme stanza leggibile e corpo espressivo.
- Colpi 16px (24 i grandi). Icone HUD su griglia propria.

## Colore

- Palette unica di gioco: **Fucina 31** (DEC-173), confermata dalla pratica dei
  pack migliori (debts: ~46 colori per TUTTO il bestiario+fx+tile; industrial
  punk: 39; nostra Fucina: 31 è nello stesso ordine di disciplina).
- Budget per entità: **3–8 colori** (ghost: 3; lizard monk: 5; G&E: 5-6).
- **Monocromia di fazione**: una famiglia di hue per fazione/tier; i tier dei
  nemici sono recolor + micro-variazione dello stesso body plan (debts).
- Un solo **accento saturo/emissivo** per entità come punto focale (brace,
  bagliore visiera, gemma) su base desaturata.

## Luce e volumi

- **2–3 toni per materiale**, un **grande piano di luce** compatto,
  luce dall'alto/alto-sinistra coerente in tutta la scena.
- Le ombre si **raggruppano** in masse scure (non distribuire mezzitoni).
- **Mai dithering. Mai noise fill.** La texture è sempre: (a) un motivo
  geometrico seamless usato come maschera con 2 toni adiacenti della Fucina,
  oppure (b) accenti sparsi ≤10–15% dei pixel su base piatta (scoperta chiave
  dei pack di GoncaloMCOliveira).
- Pieghe dei tessuti: tratti corti scuri (2–4px) con crinale chiaro adiacente;
  bordi mai in linea retta lunga (gradini irregolari).

## Outline — DECISO: Opzione B (31/07, proprietario)

**Nessun outline nero, da nessuna parte.** Silhouette per contrasto di valore
+ bordo implicito scuro dove serve (il tono più scuro del materiale, mai nero
puro perimetrale). È il look del trio mini approvato.

Obbligo conseguente: **ogni asset passa il test su pavimento chiaro E scuro**
prima dell'approvazione — senza outline il contrasto va garantito caso per
caso (masse scure → accento emissivo che stacca sul buio; masse chiare →
ancoraggio d'ombra alla base).

## Animazione (evidenza: supernova + intellikat + debts, convergenti)

- **Clock comune 100ms**; idle 2 frame a 400-500ms.
- **Idle** = respiro 1px + un dettaglio vivo su un solo frame (flicker brace,
  blink emissivo, lembo del mantello).
- **Walk** = 4 frame passaggio–estremo–passaggio–estremo, con il frame di
  passaggio riusato (cel linkato); bob 2–3px; torso ancorato al centro cella;
  **bottom fisso** sui frame di contatto; drift del pivot ≤1px.
- Entità senza gambe: frangia/lembi alternati (formula del ghost) — idle
  standard per melme, spettri e masse fuse.
- Il peso si comunica con l'oscillazione del busto, non con le gambe (minotaur).

## Struttura dei file (rig)

- Un livello per parte: guides (nascosto) / shadow / body / details / emissive
  / eventuale outline; per i personaggi anche parti-arto quando animati
  (lezione intellikat + demon di G&E: l'ARMA è già su livello separato →
  perfetto per armi-come-oggetti).
- Guida d'ingombro su livello nascosto (mai nell'export).
- Tag di animazione espliciti sempre (walk_down, idle, …), durations reali.
- Sorgenti gemelle idle/walk con struttura identica (pipeline supernova).

## VFX

- Esplosioni: **5 step geometrici, un colore per frame** (flash bianco → disco
  oro → anello spesso → anello sottile → anello tratteggiato). Ricetta
  implementabile a runtime per i colpi generati, senza sprite dedicati.
- Scintille/energia: pixel isolati deliberati, senza glow.
- Raffreddamento cromatico caldo→freddo tra frame invece dei gradienti.

## Tile e ambienti (per il rifacimento dei 5 tileset)

- Base piatta + **giunti a T sparsi** (mattone suggerito, mai griglia completa).
- Ogni materiale = **1 hue + 1 micro-pattern esclusivo** (coste, borchie,
  puntini): riconoscibile anche in miniatura.
- Dettaglio concentrato sui **bordi illuminati** (in top-down: bordo sud dei
  muri illuminato, corpo piatto).
- Danni/ossidi/verderame come **accento localizzato raro**, non pattern.
- Riempimenti scuri punteggiati di pixel luminosi per le zone profonde.

## Oggetti e icone (ItemVisualBundle — evidenza: medieval items + kaenine)

- Arma-oggetto (mai in mano): **diagonale a 45°**, manico sempre **2px**
  (mai aste 1px), testa a massa unica; 2–4 cluster per icona.
- Volume dato da **bande di tono lungo l'asse**, non da linee interne.
- La **giunzione tra materiali** è il confine di cluster (tono più scuro
  dello sprite): dice "qui si impugna" senza disegnare mani.
- Un solo **accento saturo** per differenziare varianti/rarità.
- Armature come **pezzi indossabili flottanti** senza corpo.
- Mai elementi 1px in diagonale (corde, fili): ingrossare a 2px o raddrizzare.
- Sempre 1px di margine dal bordo del canvas.
- Outline unico condiviso da tutto il set di icone per coesione (nel nostro
  caso: secondo l'opzione outline che sceglierà il proprietario).

## Regole negative (da negative_rules.md, sintesi)

- Niente volti. Niente armi in mano. Niente dithering. Niente noise fill.
- Niente outline nero sul mondo (in entrambe le opzioni outline).
- Niente colori neon fuori Fucina. Niente pixel isolati accidentali
  (solo scintille deliberate). Niente frame duplicati come cel separati
  (usare cel linkati). Niente guide visibili negli export.
- Niente campiture piatte grandi senza accento o piano di luce.
