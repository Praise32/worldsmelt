#ifndef GEN_CORPUS_H
#define GEN_CORPUS_H

#include <stdbool.h>

/* Corpus delle generazioni: ogni passaggio del modello (tentativi JSON,
   tentativi Lua, ripieghi) diventa una riga JSONL in logs/gen-corpus/.
   Un file solo, due scopi: telemetria (quanto spesso il modello sbaglia, e
   come) e materia prima del futuro dataset di fine-tuning — le coppie
   errore->correzione dei retry Lua sono esattamente gli esempi che
   servirebbero a un QLoRA (roguelike-ai-appunti/04, "Dataset futuro per
   Qwen"). MELTING_GEN_NO_CORPUS=1 spegne tutto: le suite di test lanciano
   melting-gen decine di volte e non devono riempire logs/ di corpus finti. */

void GenCorpusConfigure(unsigned int seed, const char *mode);   /* mode: "gen"|"resume"|"fallback" */
void GenCorpusRecordSession(const char *modelPath, int ngl);
void GenCorpusRecordJson(int attempt, bool ok, const char *reason,
                          double seconds, int tokens, const char *raw);
void GenCorpusRecordLua(const char *floorTheme, const char *itemName, bool statUp,
                         int attempt, const char *outcome, const char *error,
                         const char *script);
void GenCorpusRecordFallback(const char *reason, bool explicitRequest);

#endif
