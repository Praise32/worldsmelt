# Registro decisioni e domande aperte

## Decisioni consolidate

### D1 — AI locale come caratteristica primaria

Non rendere il modello opzionale nella visione del prodotto. Il gioco deve essere progettato attorno alla generazione locale.

### D2 — Un solo modello canonico

Niente vantaggio qualitativo per PC più potenti. La differenza accettabile è la latenza.

### D3 — Lua, non codice nativo

Generare Lua sandboxato. Il C contiene motore, invarianti e primitive.

### D4 — Architettura ibrida

- JSON/GBNF per dati dichiarativi;
- Lua per comportamento nuovo;
- mini-VM/IA C come fallback.

### D5 — Modelli sequenziali

LLM, Stable Diffusion e audio non devono essere residenti insieme sulla macchina da 6 GB.

### D6 — Baseline LLM

Qwen2.5-Coder-7B Q4_K_M resta la baseline finché un 4B non lo supera sul benchmark del gioco.

### D7 — Candidato di distribuzione

Qwen3-4B-Instruct-2507 Q4 è il candidato iniziale più promettente, da verificare.

### D8 — Runtime

Prima un fork/build minimale di llama.cpp; runtime completamente proprietario soltanto dopo profiling e modello congelato.

### D9 — Immagini

SD 1.5 LCM resta baseline compatibile. SD 3.5 Medium è il candidato moderno. Large Turbo è studio/teacher.

### D10 — Steam Deck

Obiettivo reale, non dichiarazione immediata. Il 4B è più plausibile del 7B; serve test fisico.

## Domande aperte ad alta priorità

1. Qual è il tempo completo per generare una run con tutti gli script, non soltanto il manifest attuale?
2. Quanti token servono realmente per una run?
3. Quanti retry medi richiede il 7B? Quanti il 4B?
4. Quali primitive minime superano il test del laser senza trasformare l’API in un engine completo duplicato?
5. Come vengono composte le sinergie tra script senza creare loop di eventi?
6. Quale quota del prompt è documentazione ridondante eliminabile?
7. Il prefisso KV può essere serializzato e distribuito in modo portabile per il modello congelato?
8. Q4_K_M è sufficiente oppure una quantizzazione Worldsmelt-specifica migliora la qualità?
9. Il modello canonico funziona su Steam Deck entro un tempo percepito accettabile?
10. La generazione degli sprite avviene per ogni run, per piano o soltanto per componenti mancanti?
11. Stable Audio runtime è compatibile con il budget totale o deve essere una fase successiva?
12. Come si gestiscono classifiche e condivisione: seed, RunBundle firmato o entrambi?

## Rischi maggiori

- API troppo povera: il modello produce soltanto variazioni degli archetipi.
- API troppo potente: sandbox e bilanciamento diventano ingestibili.
- modello piccolo: compila ma non inventa.
- modello grande: qualità ottima ma Deck troppo lento.
- prompt enorme: prefill domina la latenza.
- retry numerosi: il modello piccolo perde il vantaggio.
- runtime proprietario prematuro: mesi spesi senza migliorare il gioco.
- immagini/audio simultanei: download e tempi eccessivi.
- documentazione contraddittoria: agenti futuri implementano la visione vecchia.

## Prossimo esperimento consigliato

Creare una micro-build isolata chiamata, per esempio, `mechanics-lab`:

- stessa sandbox del gioco;
- arena 2D minima;
- command queue;
- query segmento;
- beam rendering;
- tre modelli intercambiabili soltanto per benchmark;
- 20 prompt “impossibili” per l’API attuale;
- report CSV/JSON.

Il primo criterio di successo è generare un laser, una catena e un’orbita senza primitive ad alto livello dedicate a “laser”, “catena” o “orbita”.
