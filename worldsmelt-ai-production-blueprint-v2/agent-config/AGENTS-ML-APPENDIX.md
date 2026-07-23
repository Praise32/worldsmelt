# Worldsmelt ML agent rules

Queste regole integrano, non sostituiscono, `AGENTS.md`.

1. Non avviare un training GPU completo senza `approved_gpu_run: true`.
2. Prima di ogni training eseguire dataset validation e smoke test.
3. Non cambiare più di due iperparametri per esperimento.
4. Non eliminare dataset, checkpoint o artifact.
5. Non inserire chiavi, token o cookie nel repository.
6. Registrare ogni esperimento.
7. Usare prompt e seed congelati.
8. Generare una griglia comparativa.
9. Non ritentare automaticamente più di una volta.
10. Non promuovere dataset `research` nel ramo `commercial`.
11. Non distribuire pesi senza licenza e NOTICE.
12. Non eseguire inferenza nel combattimento.
13. Preservare atlas e fallback geometrici durante la migrazione.
14. Eseguire le suite indicate dal root `AGENTS.md`.
15. Concludere con diff, test, rischi e artifact.
