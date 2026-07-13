# Cosa fare quando torni

Scritto da Claude mentre eri via. **Prima la sezione 1** (dieci minuti, ti fa vedere tutto), il resto con calma.

---

## 1. Provalo

```bash
cd ~/progetti/melting-run-gpu
git checkout local-sprites
make run-gen
```

Premi **INVIO** nel menu. Parte una barra di caricamento in due tempi:

1. **Testo** (~50 s) — il Qwen 7B inventa i 5 piani: temi, boss, oggetti, sinergie.
2. **Sprite** (~85 s) — Stable Diffusion disegna i 12 sprite del gioco, coerenti col tema appena inventato.

Dopo circa **2 minuti e mezzo** giochi con contenuti e grafica generati sul momento, in locale,
senza rete e senza chiavi API. **ESC** annulla, **R** rigenera.

Se vuoi solo il testo (85 secondi in meno):

```bash
make run-gen-fast     # equivale a --no-sprites: tiene gli sprite geometrici di prima
```

Guarda anche l'atlas da solo:

```bash
xdg-open generated/current_atlas.png
```

### Se qualcosa non parte

```bash
make                 # ricompila tutto
make test            # test del gioco
make test-gen        # test del generatore di testo (senza modello, veloci)
make test-sprites    # test degli sprite (senza modello: usa --dry-run)
make test-llm        # generazione di testo vera (~1 min)
```

---

## 2. Lo stato del lavoro: due branch, nessuno pushato

Ho lasciato a te la decisione di mergiare: vorrai prima vederlo girare.

| Branch | Cosa | Stato |
|---|---|---|
| `linux-local-llm` | Build Linux + generazione **testuale** locale | 18 commit, completo, review passata |
| `local-sprites` | Il precedente + **sprite** con Stable Diffusion | 9 commit, completo, review passata |

La review della fase 2 ha trovato un bug serio che ho poi corretto, e che vale la pena
raccontarti perche' spiega come e' fatto il sistema: quando uno sprite veniva scartato dal
controllo qualita', la sua cella restava vuota — e il gioco disegnava il vuoto. Nemici,
boss e portale sarebbero diventati **invisibili**. Ora ogni cella vuota fa ricadere quella
singola entita' sulla vecchia forma geometrica, ed esiste un test che lo dimostra (fallisce
apposta se qualcuno rompe di nuovo quel percorso).

`local-sprites` contiene gia' tutto `linux-local-llm`, quindi per prendere tutto:

```bash
git checkout main
git merge local-sprites
git push               # solo se vuoi mandarlo anche su GitHub
```

Per buttare via tutto: `git branch -D linux-local-llm local-sprites`. Il tuo `main` non e' stato toccato.

---

## 3. I numeri veri, misurati sulla tua macchina

| | |
|---|---|
| Testo: Qwen2.5-Coder **7B** | ~50 s, **28 token/s**, 4,5 GB di VRAM |
| Sprite: SD1.5 pixel-art + LCM | ~5,7 s per sprite, **~85 s** per 12, 2,0 GB di VRAM |
| Totale a inizio run | **~2 min 30 s** |

Il 7B ci sta **tutto** nei tuoi 6 GB (era la mia previsione piu' pessimista, sbagliata: bene cosi').
I due modelli non possono stare in VRAM insieme, ma non serve: sono due processi che si
alternano e ognuno libera tutto quando esce. Dettagli in `docs/BENCHMARKS.md` e `docs/SPRITES-SPIKE.md`.

---

## 4. Le due cose che devi guardare tu, perche' sono gusto tuo

### a) La qualita' degli sprite

Personaggi, nemici, boss, cuore, porta: **vengono bene**. Le icone piccole (moneta, chiave,
bomba, colpo) sono un terno al lotto: a volte la chiave e' una chiave, a volte e' un
lucchetto o peggio. Ho gia' fatto due giri di miglioramenti (soggetto in testa al prompt,
negativi mirati, cfg alzato a 1.8 — sotto ignorava il soggetto, sopra sporcava lo sfondo).

Da qui in poi e' **direzione artistica, ed e' tua**. I prompt sono file di testo, uno per
sprite, modificabili senza ricompilare:

```
tools/melting-sprites/prompts/{player,boss,coin,key,bomb,shot,...}.txt
tools/melting-sprites/prompts/negative.txt      <- cosa NON deve disegnare
```

Cambi il testo, rilanci `./bin/melting-sprites --seed 5`, guardi `generated/current_atlas.png`.
Un giro costa 85 secondi. Se una cella esce indecente il gioco **non si rompe**: la lascia
vuota e ridisegna la forma geometrica di prima per quell'entita'.

### b) La qualita' dei testi

Il 7B in italiano e' altalenante a seconda del seed: a volte *"Catacombe Crepuscolari —
Ossamento Scuro"*, a volte *"deserto — Sandstorm"*. Ho tolto il difetto piu' grosso (copiava
pari pari l'esempio dal prompt) e aggiunto la penalita' sulle ripetizioni. I prompt sono in
`tools/melting-gen/prompts/`.

---

## 5. Decisioni che aspettano te (nessuna urgente)

1. **Il gioco resta un hobby o punti a venderlo?** Cambia la scelta del modello per gli sprite.
   Quello che uso ora (All-In-One-Pixel-Model) e' OpenRAIL-M: le immagini generate sono tue e
   vendibili, ma se **ridistribuisci i pesi** insieme al gioco devi propagare le restrizioni
   d'uso della licenza. Esiste un'alternativa Apache 2.0 (`SD_PixelArt_SpriteSheet_Generator`),
   un po' peggiore in resa. I modelli di testo (Qwen) sono Apache 2.0: nessun problema.

2. **Attesa a inizio run.** Adesso sono 2 minuti e mezzo. Alternative: generare gli sprite
   una volta sola e riusarli per piu' run; oppure generarli in sottofondo mentre giochi il
   primo piano. Dimmi cosa preferisci.

3. **Il tuo obiettivo vero: oggetti unici, sinergie inventate, comportamenti dei nemici.**
   E' la **sandbox Lua** (fase 3), il pezzo piu' grosso. La mini-VM di adesso ha 4 operazioni,
   non basta a esprimerli. Vorrei progettarla **con te presente**: le decisioni li' dentro
   (quali callback esporre all'IA, quanto potere darle, cosa resta blindato in C) sono scelte
   di design del *tuo* gioco, non dettagli tecnici che posso decidere io.

---

## 6. Cose che ho cambiato sul tuo sistema

- **Installato `xvfb` e `xdotool`.** Servivano per testare senza di te: i test aprono una
  finestra e, con lo schermo bloccato, su Wayland il gioco si piantava (una finestra non
  visibile non riceve piu' frame). Ora i test girano su uno schermo virtuale. Il Makefile li
  usa: se li togli (`sudo apt remove xvfb xdotool`) i test torneranno a richiedere lo schermo
  sbloccato.
- **Sospensione automatica**: disattivata mentre lavoravo, **ripristinata** com'era (suspend
  dopo 2 ore). Verifica: `gsettings get org.gnome.settings-daemon.plugins.power sleep-inactive-ac-type`
  deve dire `'suspend'`.
- **La password che mi hai passato in chat e' finita nella trascrizione della sessione.**
  L'ho usata solo per installare quei due pacchetti e non l'ho salvata da nessuna parte.
  Valuta di cambiarla.

Non ho toccato GRUB, partizioni, dischi NTFS, ne' il resto della tua configurazione.

---

## 7. Se vuoi capire cosa e' stato fatto

- `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md` — il progetto della fase 1 e la roadmap.
- `docs/superpowers/specs/2026-07-13-local-sprites-design.md` — il progetto della fase 2.
- `docs/SPRITES-SPIKE.md` — le misure con cui ho deciso che gli sprite erano fattibili, e le
  due trappole che hanno cambiato il progetto (perche' **non** si generano su sfondo nero).
- `docs/BENCHMARKS.md` — i numeri del modello di testo.
- `git log --oneline main..local-sprites` — i commit, uno per pezzo.

I tuoi appunti (`docs/APPUNTI.md`, `docs/DESIGN_NOTES.md`) non li ho toccati: sono tuoi.
