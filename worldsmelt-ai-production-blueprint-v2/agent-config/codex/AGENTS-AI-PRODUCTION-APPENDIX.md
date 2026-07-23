# Appendix — AI Production

Per task immagini, UI, audio, training, curation o Piano 0:

1. Leggere `docs/worldsmelt-ai-production-blueprint/INDEX.md`.
2. Usare `22-TOPIC-ROUTER.md`.
3. Controllare `19-DECISION-QUESTIONNAIRE.md`.
4. Non implementare domande BLOCKING senza risposta.
5. Preparare una Decision Session quando serve.
6. Non avviare run GPU senza `approved_gpu_run: true`.
7. Separare research e commercial.
8. Non introdurre modelli nel binario del gioco.
9. Non eseguire inferenza in combattimento.
10. Preservare fallback.
11. Aggiornare INDEX e manifest per ogni Markdown.
12. Usare un verifier separato prima del commit.

Percorso:

```text
classifica
→ cerca fonti
→ domande
→ piano
→ implementazione
→ verifica
→ documentazione
```
