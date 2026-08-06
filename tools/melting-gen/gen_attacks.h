#ifndef MELTING_GEN_ATTACKS_H
#define MELTING_GEN_ATTACKS_H

/* gen_attacks.{h,c}: WP3 (spec
   docs/engineering/specs/2026-08-05-combat-lab-design.md, sezioni 2 e 5).
   Stesso schema di gen_lua.c (prompt -> modello -> valida nella sandbox VERA
   -> ritenta), ma per un dominio diverso: non lo script di UN oggetto della
   run, bensi' l'on_tick di UN attore della demo Combat Lab (un nemico o
   un'arma del player), validato con l'API VERA della demo
   (tools/procedural-combat-demo/demo_script_api.{h,c}, compilata qui dentro
   melting-gen SOLO per il dry-run: vedi GEN_EXTRA_SRC nel Makefile, stesso
   principio di script_sandbox.c per gli oggetti). A differenza di gen_lua.c
   non serve nessuno stub: l'API della demo e' gia' un'API finta rispetto al
   gioco vero (non tocca mai src/game/, non linka raylib), quindi
   melting-gen la puo' linkare DAVVERO invece di simularla con un secondo
   set di funzioni C da tenere sincronizzato a mano.

   Due responsabilita', stesso taglio di gen_lua.h:
   - GenAttackValidate: sintassi + 120 tick di dry-run con posizioni
     plausibili in movimento, nessun modello. E' quello che esercita
     --attack-check e quello che il ciclo di generazione sotto chiama ad
     ogni tentativo.
   - GenAttackGenerate: il ciclo prompt -> modello -> valida -> ritenta (fino
     a 3 tentativi per script) per un LOTTO di script (default 3,
     --attack-count), scritti in <outDir>/combat-lab/<kind>/. */

#include "melting_gen.h"

#include <stdbool.h>
#include <stddef.h>

/* Capienza dei buffer d'errore di questo modulo -- piu' larga dei 192 byte
   che gen_lua.c usa per il suo dominio, e non per gusto: qui l'errore non
   e' solo per l'utente, e' l'UNICO testo che il ritento rimanda al modello
   (vedi GenAttackGenerate), e va composto da tre pezzi che con 192 byte si
   troncavano a meta' frase (il criterio "questi comandi non contano come
   attacco" spariva sempre; il messaggio Lua vero di un errore a runtime piu'
   il suo suggerimento in inglese non ci stavano affatto). Un prompt che
   chiede di "correggere SOLO il problema segnalato" vale quanto il problema
   che riesce a segnalare. */
#define GEN_ATTACK_ERR_CAP 640

/* n_predict dedicato (spec sezione 5): gli on_tick con macchina a stati
   (telegraph -> pausa -> attacco per i nemici, reazione a fire_held()/
   special_pressed() per le armi) sono piu' lunghi degli script oggetto di
   gen_lua.c, che si fermano a UN solo effetto semplice (vedi
   GEN_LUA_N_PREDICT=384 in gen_lua.h per il confronto). */
#define GEN_ATTACK_N_PREDICT 768

/* Quanti tick di dry-run (spec sezione 5, requisito 4 del task brief):
   abbastanza per attraversare l'intera macchina a fasi di un pattern
   "lento" come tools/procedural-combat-demo/scripts/spider_arc.lua
   (telegraph 1.05s + preavviso 0.46s + rientro 0.38s, ~1.9s a 60 tick/s)
   restando a dt fisso 1/60. */
#define GEN_ATTACK_DRY_RUN_TICKS 120

/* Handle fissi del dry-run (spec sezione 5, requisito 4): NON gli handle
   veri della demo (che dipendono da quanti attori sono vivi in quel momento
   in una partita reale), solo tre numeri distinti e stabili per la
   validazione fuori linea. DemoScriptApiInit rifiuterebbe silenziosamente
   solo un self_handle uguale al player_handle (li sostituirebbe con 1/2):
   200/100/1 non collidono mai, quindi il dry-run valida esattamente gli
   handle che poi finiscono nel prompt come SELF_HANDLE/PLAYER_HANDLE. */
#define GEN_ATTACK_ENEMY_SELF_HANDLE  200u
#define GEN_ATTACK_WEAPON_SELF_HANDLE 100u
#define GEN_ATTACK_PLAYER_HANDLE      1u

/* Arena del dry-run: STESSA di tools/procedural-combat-demo/main.c
   (DEMO_ROOM = {58,88,1164,566}, cioe' (58,88)-(1222,654)). Duplicata qui a
   mano invece di includere quel main.c: e' CONGELATO per WP3 (vedi il task
   brief) e comunque melting-gen non deve dipendere da un .c della demo per
   due numeri, solo dall'API (demo_script_api.h) che valida davvero. */
#define GEN_ATTACK_ROOM_LEFT   58.0f
#define GEN_ATTACK_ROOM_TOP    88.0f
#define GEN_ATTACK_ROOM_RIGHT  1222.0f
#define GEN_ATTACK_ROOM_BOTTOM 654.0f

/* Validazione pura, senza alcun modello (spec sezione 5): carica 'source'
   nella sandbox VERA del gioco (script_sandbox.h) con l'API VERA della demo
   (demo_script_api.h, registrata PRIMA di ScriptSandboxLoad, come richiede
   il contratto in cima a quell'header) per 'kind' ("enemy" o "weapon",
   qualunque altro valore fallisce con un errore chiaro). Richiede on_tick
   (nessun'altra callback esiste in questo dominio: l'alfabeto della demo e'
   un solo hook), poi simula GEN_ATTACK_DRY_RUN_TICKS tick a dt fisso 1/60
   con posizioni plausibili IN MOVIMENTO (vedi il commento sopra
   GenAttackSimulateTicks nel .c per i dettagli per-kind) e ACCUMULA i
   comandi tick per tick (DemoScriptApiBeginFrame azzera il buffer del
   comandi a OGNI tick: leggerlo PRIMA del prossimo BeginFrame e' l'unico
   modo per non perdere i comandi di un tick che il successivo cancella).
   Valido SOLO se la sandbox non si disabilita MAI durante i 120 tick E in
   ALMENO UN tick compare un comando D'ATTACCO vero (emit_arc/emit_ring/
   emit_orbit/emit_beam/melee_sweep/capture_radius/release_echoes): un
   pattern che chiama solo set_velocity/add_status/telegraph_* non e' un
   attacco, e' solo un nemico che si muove o si prepara -- l'errore lo dice
   esplicitamente, cosi' il ciclo di retry (GenAttackGenerate sotto) puo'
   rimandarlo al modello. Ritorna false su qualunque fallimento (sintassi,
   on_tick assente, la sandbox si disabilita, nessun comando d'attacco) e
   riempie 'err' (puo' essere NULL/errSize 0; conviene GEN_ATTACK_ERR_CAP,
   vedi sopra).
   NESSUN gate anti-copia qui dentro, di proposito: --attack-check deve
   continuare ad approvare i cinque script CURATI, quindi "questo script
   esiste gia'" e' una regola della GENERAZIONE, non della validita' -- vive
   in GenAttackGenerate. */
bool GenAttackValidate(const char *source, const char *kind, unsigned int seed,
                        char *err, size_t errSize);

/* Il ciclo prompt -> modello -> valida -> ritenta (stesso schema di
   gen_lua.c) per un LOTTO di 'count' script (clamp 1..8, --attack-count) di
   categoria 'kind' ("enemy"|"weapon"): 'sess' e' GIA' aperta dal chiamante
   (main.c, riusa --model/--ngl gia' esistenti: nessun flag nuovo per il
   modello, spec sezione 5). Nessuna grammatica GBNF (un on_tick completo
   con macchina a stati non si esprime in GBNF, stesso ragionamento del
   percorso Lua degli oggetti in gen_lua.h sezione 6): fino a 3 tentativi
   TOTALI per script, con l'errore di validazione rimandato al modello ad
   ogni ritento.

   Gate anti-copia (bocciatura del giudice, giro reale 05/08: 8 script su 8,
   su 4 semi diversi, erano il few-shot spider_arc.lua ricopiato byte per
   byte -- validi, quindi accettati, ma inutili: il pool si riempiva di
   copie di cio' che gia' conteneva). Vive QUI e non in GenAttackValidate
   (vedi sopra il perche'), fra la validazione e la scrittura: uno script
   che dopo la normalizzazione (commenti via, spazi collassati) e' contenuto
   nel cheat-sheet -- uno degli snippet di codice che stanno in
   prompts/attack_system.txt -- oppure ricalca quasi riga per riga un file
   gia' presente in <outDir>/combat-lab/<kind>/ viene respinto con un errore
   che il ritento rimanda al modello, esattamente come un errore di
   validazione.
   Il gate resta la RETE, non la cura: la cura e' nel prompt, e la bocciatura
   ha costretto a cambiarlo in due punti. (1) Il cheat-sheet non porta piu'
   due script curati completi ma due SCHELETRI piu' l'elenco delle cinque
   idee gia' nel pool: con Gemma-3-4B un esempio completo e' un invito a
   ricopiarlo, e nessun "do not copy this" lo batte (misurato: con gli
   esempi completi 8 copie su 8, con gli scheletri zero). (2) La forma
   d'attacco principale ruota col seed (prompts/attack_user.txt), cosi' due
   semi diversi chiedono compiti diversi invece dello stesso compito due
   volte.

   Ogni script che valida viene scritto SUBITO in
   <outDir>/combat-lab/<kind>/NNN_seed<seedI>.lua (NNN = (indice massimo
   gia' presente nella cartella)+1 a 3 cifre): il lotto non e' atomico, uno
   script riuscito resta anche se un altro del lotto fallisce (spec sezione
   5, "il lotto scrive gli script riusciti").
   'briefPath' (--attack-brief, opzionale): NULL o file assente/vuoto =
   nessun brief nel prompt (comportamento di sempre); non vuoto = il
   contenuto entra nel prompt (vedi prompts/attack_user.txt) e la sua prima
   riga finisce nell'header del file scritto.
   Logga per ogni script (tentativi ed esito) su STDOUT, non solo nel log su
   file: e' l'output che il proprietario legge mentre gioca (spec, "un tasto
   chiede nuovi pattern mentre si gioca").
   Ritorna il numero di script scritti con successo (0 = lotto fallito del
   tutto: il chiamante ne fa l'exit code, vedi main.c). */
int GenAttackGenerate(GenLlmSession *sess, const char *promptsDir, const char *outDir,
                       const char *kind, int count, const char *briefPath, unsigned int seed);

/* Guardia byte-budget del prompt attacchi (spec sezione 5, "budget prompt
   sorvegliato con lo stesso meccanismo di GEN_LUA_PROMPT_BYTE_CEILING"):
   stesso principio di GenLuaPromptBudgetCheck (gen_lua.h) -- compone il
   prompt PIU' GRANDE che questo modulo possa costruire oggi con la stessa
   funzione della generazione vera (worst-case: kind piu' lungo, un brief
   rappresentativo) e lo confronta al ceiling in byte derivato da
   GEN_LLM_SESSION_N_CTX - GEN_ATTACK_N_PREDICT. Nessun modello coinvolto,
   per poter girare dentro `make test-gen` (vedi il ramo
   --prompt-budget-check in main.c, che chiama ANCHE questa funzione oltre a
   GenLuaPromptBudgetCheck: scripts/test-gen.sh la esercita gratis). 'err'
   (puo' essere NULL/errSize 0) riceve il motivo del fallimento. Ritorna
   true se il prompt composto sta dentro il ceiling. */
bool GenAttackPromptBudgetCheck(const char *promptsDir, char *err, size_t errSize);

/* Derivazione IDENTICA a GEN_LUA_PROMPT_BYTE_CEILING (vedi gen_lua.h per il
   ragionamento completo: ~3.2 caratteri/token, misurato una tantum col
   vocabolario del modello, arrotondato per restare generoso): n_ctx della
   sessione condivisa meno il budget riservato all'output di UN tentativo
   d'attacco. */
#define GEN_ATTACK_PROMPT_BYTE_CEILING (((GEN_LLM_SESSION_N_CTX) - (GEN_ATTACK_N_PREDICT)) * 16 / 5)

#endif
