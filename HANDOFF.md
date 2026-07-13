# Cosa fare quando torni

Scritto da Claude mentre eri via. Ordine consigliato: **prima la sezione 1** (5 minuti, ti fa vedere il lavoro fatto), poi il resto quando hai tempo.

---

## 1. Provalo (la parte bella)

```bash
cd ~/progetti/melting-run-gpu
make run-gen
```

Premi **INVIO** nel menu. Vedi una barra di caricamento: e' il modello Qwen 7B che gira sulla tua 5600 XT e inventa la run. Dopo ~50 secondi parti a giocare con temi, boss, oggetti e sinergie appena generati. Tutto in locale: niente rete, niente chiave API.

Durante la generazione **ESC** annulla e torna al menu. In partita **R** rigenera una run nuova.

Cose da guardare mentre ci giochi:
- I nomi degli oggetti e dei boss: sono inventati dal modello a ogni run.
- Il pannello sinistro dice `Fonte: LLM cache` quando la run viene dal modello.
- Se stacchi la rete, funziona lo stesso. Se cancelli i modelli da `models/`, il gioco ripiega sul generatore deterministico interno e parte comunque.

### Se qualcosa non parte

```bash
make                 # ricompila
make test            # 5 test del gioco
make test-gen        # test del generatore (senza modello, veloci)
make test-llm        # generazione vera col modello (~1 minuto)
```

---

## 2. Decisione che spetta a te: il merge

Il lavoro e' su due branch, **non pushati e non mergiati**. Ho lasciato la scelta a te perche' vorrai prima vederlo girare.

| Branch | Cosa contiene | Stato |
|---|---|---|
| `linux-local-llm` | Build Linux + generazione testuale locale (17 commit) | Completo, testato, review passata |
| `local-sprites` | Parte da quello sopra + sprite con Stable Diffusion | In corso — vedi sezione 4 |

Per prendere il primo:

```bash
git checkout main
git merge linux-local-llm
git push                       # se lo vuoi anche su GitHub
```

Se invece vuoi buttare tutto: `git branch -D linux-local-llm local-sprites` e non e' successo niente (il tuo `main` non e' stato toccato).

---

## 3. Cose che ho cambiato sul tuo sistema

- **Installato `xvfb` e `xdotool`.** Servivano per testare senza di te: i test aprono una finestra, e con lo schermo bloccato il gioco si piantava (su Wayland una finestra non visibile non riceve piu' frame). Ora i test girano su uno schermo virtuale. Se non li vuoi: `sudo apt remove xvfb xdotool` — ma il Makefile li usa, quindi i test tornerebbero a richiedere lo schermo sbloccato.
- **Sospensione automatica**: l'avevo disattivata per non farti spegnere il PC a meta' lavoro, poi **ripristinata** com'era (suspend dopo 2h). Controlla pure: `gsettings get org.gnome.settings-daemon.plugins.power sleep-inactive-ac-type` deve dire `'suspend'`.
- **La password che mi hai passato in chat e' finita nella trascrizione della sessione.** L'ho usata solo per installare quei due pacchetti e non l'ho salvata da nessuna parte. Valuta di cambiarla.

Non ho toccato: GRUB, partizioni, dischi NTFS, la tua configurazione GNOME (a parte la sospensione, ripristinata).

---

## 4. Dove sono arrivato con gli sprite (fase 2)

Branch `local-sprites`. Obiettivo: generare gli sprite del gioco in locale con Stable Diffusion, come gia' facciamo col testo.

**Stato: sto misurando se e' possibile.** La ricerca ha trovato una cosa importante: **non esiste nessun benchmark pubblico di stable-diffusion.cpp su una GPU RDNA1 come la tua**, e la tua scheda non ha le unita' matriciali (coopmat) che rendono veloci le RDNA3. Quindi sto costruendo tutto e generando un'immagine vera per misurare, invece di promettere numeri inventati.

Se trovi questo file con la sezione qui sotto ancora vuota, vuol dire che la misura non e' finita: guarda `docs/SPRITES-SPIKE.md`, che scrivo appena ho il numero.

Due scoperte della ricerca che cambiano il progetto e che ti riguardano:

1. **Gli sprite NON vanno generati su sfondo nero.** Il gioco oggi fa chroma-key sul quasi-nero, ma la pixel art ha i *contorni neri*: il ritaglio mangerebbe i bordi degli sprite. Vanno generati su sfondo **magenta**, ritagliati con un flood fill dai bordi (che non puo' raggiungere i pixel neri *dentro* lo sprite) e salvati con vera trasparenza. E' un miglioramento anche per l'atlas che gia' hai.
2. **L'API di stable-diffusion.cpp e' cambiata**: le guide in giro (e la mia memoria) parlano di una funzione `txt2img()` che non esiste piu'. Ho gia' i nomi giusti dalla versione fissata.

---

## 5. Cose su cui ho bisogno di te (decisioni, non lavoro)

Nessuna e' urgente. Rispondimi quando vuoi e proseguo.

1. **Qualita' dei contenuti generati.** Il 7B in italiano e' altalenante a seconda del seed: a volte tira fuori *"Catacombe Crepuscolari — Ossamento Scuro"*, a volte *"deserto — Sandstorm"*. Ho gia' tolto un difetto grosso (copiava l'esempio del prompt) e aggiunto la penalita' sulle ripetizioni. Per migliorare ancora ci sono tre strade, e vorrei sapere quale preferisci:
   - accettare la varianza (una run brutta la ributti con R);
   - aggiungere un *controllo qualita*': se i nomi sono pochi o sciatti, rigenera in automatico (ma raddoppia l'attesa);
   - provare un modello piu' grande (14B a Q4 ~9 GB: **non ci sta nei tuoi 6 GB di VRAM**, girerebbe in parte su CPU e ci metterebbe minuti).

2. **Licenze, se un giorno vuoi vendere il gioco.** I modelli di testo (Qwen 7B/1.5B) sono Apache 2.0: nessun problema. Per gli sprite il modello pixel-art migliore (All-In-One-Pixel-Model) e' OpenRAIL-M: puoi usarlo e vendere le immagini, ma se **ridistribuisci i pesi** insieme al gioco devi propagare le sue restrizioni d'uso. C'e' un'alternativa Apache 2.0 (SD_PixelArt_SpriteSheet_Generator) leggermente peggiore. Dimmi se il gioco resta un hobby o se punti a venderlo, cosi' scelgo di conseguenza.

3. **Il tuo obiettivo vero.** Oggetti davvero unici, sinergie inventate da zero, comportamenti dei nemici: quella e' la **sandbox Lua** (fase 3). La mini-VM attuale ha 4 operazioni, non basta. E' il pezzo piu' grosso e piu' bello, e vorrei progettarlo con te presente, non da solo — le decisioni li' dentro (quali callback esporre, quanto potere dare all'IA) sono di design, non tecniche.

---

## 6. Se vuoi capire cosa e' stato fatto

- `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md` — il progetto approvato, con la roadmap in 5 fasi.
- `docs/superpowers/plans/2026-07-13-linux-local-llm.md` — il piano di implementazione, task per task.
- `docs/BENCHMARKS.md` — i numeri veri misurati sulla tua macchina.
- `git log --oneline main..linux-local-llm` — i 17 commit, uno per pezzo.

I tuoi appunti (`docs/APPUNTI.md`, `docs/DESIGN_NOTES.md`) non li ho toccati: sono tuoi.
