# Riferimenti — come li uso

Questa cartella contiene i documenti che ho ricavato dallo studio delle repository e
delle fonti che mi hai passato. Non sono codice da copiare: sono **appunti tecnici**,
scritti con parole mie e già mappati sull'architettura di Melting Run, che consulteremo
mentre costruiamo le fasi successive.

## Il metodo (il confine da tenere sempre)

C'è una regola che governa tutto, e vale doppio perché **potresti vendere il gioco**:

- **Le meccaniche, le regole, gli algoritmi e le formule sono fatti.** Non sono coperti
  da copyright, quindi li possiamo reimplementare da zero. La formula della cadenza di
  tiro, l'algoritmo che dispone le stanze di un piano, il pattern «un nemico che ti
  insegue»: tutto questo lo studio, lo capisco, e lo riscrivo in C/Lua a modo nostro.
- **L'espressione è protetta.** I nomi degli oggetti e dei nemici, i testi, gli sprite,
  le tabelle di ID di un gioco commerciale **non si copiano**. Nei nostri documenti i
  comportamenti hanno nomi generici («inseguitore», «torretta»), non i nomi di marca; e
  gli oggetti del gioco li inventa comunque l'IA a ogni run.

Quindi il flusso è: **leggo la fonte → estraggo la meccanica/formula → la riscrivo come
appunto per noi, mappata sul nostro codice → da lì implemento.** Il documento sta in mezzo
apposta: quando costruiamo una fase, non riparto dalla fonte esterna, riparto da questi
appunti che parlano già la lingua del nostro progetto (le nostre callback Lua, la nostra
cache delle statistiche, il nostro `src/world`).

## I documenti, e a cosa servono

| Documento | Cosa contiene | Serve per |
|---|---|---|
| [meccaniche-callback-e-cache.md](meccaniche-callback-e-cache.md) | Il modello a callback di Isaac (evaluate-cache, update, on-fire, on-damage, npc-update, new-room) mappato sulle nostre callback Lua, e il pattern del **ricalcolo statistiche da zero** spiegato a fondo | Sinergie (prossimo passo) e nemici (3b) |
| [formule-statistiche.md](formule-statistiche.md) | Le **formule esatte** (cadenza di tiro, danno, velocità colpo, gittata, fortuna) mappate sui campi `Player` e sui tetti per rarità | Bilanciare gli stat-up e le sinergie |
| [design-sinergie.md](design-sinergie.md) | Come funzionano le sinergie implicite alla Isaac, e il **design concreto** del nostro sistema (rilevamento coppie dai metadati, dove vivono, come si applicano, come restano bilanciate e sicure) con 4 esempi nella nostra API | **Il prossimo passo che costruiremo** |
| [pattern-nemici-e-boss.md](pattern-nemici-e-boss.md) | Il catalogo dei **pattern di comportamento** dei nemici (inseguitore, torretta, caricatore, spezzettatore…) e il **sistema a parti** per i boss (dai tuoi APPUNTI), mappati su una futura `on_enemy_update` | Fase 3b (nemici e boss) |
| [generazione-piani-stanze.md](generazione-piani-stanze.md) | L'**algoritmo esatto** di generazione dei piani (griglia 9×8, formula del numero di stanze, crescita BFS, vicoli ciechi, boss e stanze speciali) e come cambiarlo nel nostro `src/world` | Fase 3c (stanze e piani) |

## L'ordine con cui li useremo

1. **Sinergie** (subito): `design-sinergie.md` + `meccaniche-callback-e-cache.md` + `formule-statistiche.md`.
2. **Nemici e boss** (3b): `pattern-nemici-e-boss.md` + di nuovo il modello a callback.
3. **Stanze e piani** (3c): `generazione-piani-stanze.md`.

## Nota di provenienza

Questi appunti sintetizzano fonti pubbliche (la documentazione dei modder di Isaac,
la wiki della community, l'articolo di BorisTheBrave sulla generazione dei dungeon). Le
fonti sono citate in fondo a ciascun documento. Le repository-clone che mi hai passato
(quella in Python/Pygame e quella in Unity/C#) non le ho usate come fonte primaria: sono
progetti vecchi o non licenziati e le fonti sopra sono più accurate e più sicure — ne ho
spiegato il perché nella valutazione delle repo. Non ho copiato codice, nomi o asset da
nessuna di esse.
