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

#endif
