#ifndef GEN_INSPIRE_H
#define GEN_INSPIRE_H

#include <stddef.h>

/* Semi d'ispirazione per il prompt JSON (roadmap 16/07/2026, settimana 1).
   Il difetto misurato del 7B lasciato solo e' la convergenza: run diverse
   ricascano sugli stessi temi e sugli stessi colpi. La contromossa e' la
   stessa di OSS-Instruct (arXiv 2312.02120, la tecnica di dataset con la
   similarita' piu' BASSA fra quelle studiate): seminare ogni richiesta con
   materiale esterno campionato a caso, cosi' l'ancoraggio creativo cambia a
   ogni run. Qui il campione e' deterministico sul seed della run (stesso
   seed = stesse ispirazioni), coerente col determinismo del resto della
   pipeline. Il blocco costa ~150 token su un prompt di ~2800 in n_ctx 8192. */

/* Scrive in buf il blocco "Ispirazioni per QUESTA run" (testo italiano,
   gia' formattato per il prompt utente). Ritorna buf. */
char *GenInspireBuild(unsigned int seed, char *buf, size_t size);

/* Esempi rotanti per il SYSTEM prompt (fase successiva alla misura A/B che ha
   trovato "Colonnato Sacro" duplicato x2 in due run diverse: l'unico
   ancoraggio residuo dopo le ispirazioni sopra era proprio l'esempio fisso di
   system.txt, che il modello copiava nonostante il "NON copiarlo"). Stessa
   contromossa delle ispirazioni -- campionamento deterministico sul seed --
   ma su un POOL di esempi scritti a mano invece che su un blocco generato:
   qui il testo restituito deve essere JSON valido e dentro le bande numeriche
   che il prompt documenta, quindi resta materiale d'autore, non
   combinatoria libera.
   kind seleziona il pool e la forma del blocco:
   - "room": un esempio, riga JSON compatta (nessun newline).
   - "enemies": due esempi indentati di due spazi, separati da '\n', SEMPRE
     uno "da mischia" e uno "da distanza" (pool distinti per costruzione, non
     per estrazione casuale su un pool unico): garantisce il contrasto di
     strategia richiesto dal prompt ("i due nemici devono essere DIVERSI"),
     non solo indici diversi.
   - "shots": tre esempi indentati di due spazi, separati da '\n', indici
     DISTINTI dallo stesso pool.
   Ritorna buf. */
char *GenInspireExamples(unsigned int seed, const char *kind, char *buf, size_t size);

#endif
