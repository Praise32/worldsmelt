# Note di design

## Obiettivo attuale

Questa versione prova a realizzare il nucleo dell'idea: un roguelite semplice
dove ogni run puo' cambiare tema, oggetti, sinergie e sprite, ma senza dare
all'LLM il controllo diretto del codice di gioco.

Il punto chiave e' separare creativita' e sicurezza:

```text
OpenAI inventa contenuti e piccole regole dichiarative.
Raylib esegue solo comportamenti gia' autorizzati dal codice C.
```

## Cosa e' stato implementato

- 5 piani.
- Mappa a stanze 5x5 per ogni piano.
- Stanze combattimento, tesoro, negozio e boss.
- Chiavi, bombe, monete e cuori.
- Boss a ogni piano, con boss finale piu' duro al piano 5.
- Oggetti passivi raccolti dal giocatore.
- Oggetti visibili graficamente sullo stickman.
- Effetti modulari sugli spari.
- Mini VM sandboxata per script generati.
- Atlas grafico caricato da PNG/BMP.
- Fallback locale quando OpenAI non e' disponibile.
- HUD con FPS.
- Manifest LLM esterno letto dal gioco.

## Perche' OpenAI sta fuori dal gioco C

La scelta piu' pulita e sicura e' usare un piccolo sidecar Node:

- la chiave API non viene compilata dentro l'eseguibile;
- non servono librerie HTTP o JSON nel codice C;
- il gioco parte anche offline;
- si puo' rigenerare una run senza ricompilare;
- il manifest TXT e' facile da debuggare.

Il flusso e':

```text
Responses API -> JSON strutturato -> manifest TXT -> raylib C
Image API -> PNG tecnico 1024x1024 -> celle 128x128 in raylib
Atlas locale -> BMP di fallback -> celle 128x128 in raylib
```

## Perche' una mini VM invece di Lua

Lua sarebbe piu' potente, ma aggiunge una dipendenza e una superficie piu'
grande da mettere in sicurezza. Per questa fase basta un linguaggio minuscolo:

```text
trigger:operazione,parametroA,parametroB,tratto
```

Esempi:

```text
on_fire:burst,3,0.36,split
on_hit:projectile,2,260,homing
on_hit:area,58,0.48,explode
on_hit:heal,18,1,vamp
```

Vantaggi:

- e' facile da generare con JSON strutturato;
- e' facile da validare;
- non puo' leggere file o chiamare funzioni esterne;
- e' abbastanza creativo per creare sinergie nuove;
- si puo' sostituire con Lua piu' avanti se diventa necessario.

## Cosa genera l'LLM

Per ogni piano:

- tema;
- stile visivo;
- nome boss;
- palette;
- 3 oggetti con slot visivo;
- tratti meccanici ammessi dal codice;
- script sandboxati sugli oggetti.

Per la parte visuale:

- uno spritesheet 1024x1024;
- celle fisse da 128x128;
- ordine celle noto al codice C.

## Limiti pratici

- Lo spritesheet IA non e' garantito perfetto: i modelli immagine possono
  sbagliare posizionamento o fondere celle. Per questo il prompt e' molto
  vincolato, il runtime applica chroma-key sullo sfondo e resta disponibile
  `--local-atlas` come fallback tecnico.
- Le sinergie sono nuove come combinazioni, ma usano operazioni base ammesse.
- OpenAI lavora prima della run, quindi il tempo di caricamento puo' aumentare.
- Il PNG della Image API e' il percorso principale; il BMP locale serve quando
  vuoi una griglia sicuramente giocabile senza chiamare o usare l'immagine API.

## Potenziamenti futuri sensati

- Aggiungere un validatore visuale dell'atlas.
- Salvare seed e run history.
- Far scegliere al giocatore se tenere, vendere o fondere due oggetti.
- Aggiungere piu' layout stanza e piu' pattern boss.
- Aggiungere audio leggero.
- Aggiungere una modalita' benchmark.
- Valutare Lua solo quando servono regole davvero piu' espressive.
