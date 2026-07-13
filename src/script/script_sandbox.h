#ifndef MELTING_RUN_SCRIPT_SANDBOX_H
#define MELTING_RUN_SCRIPT_SANDBOX_H

/* ScriptSandbox: la "gabbia" C attorno a un interprete Lua 5.5 vendorizzato,
   pensata per eseguire codice scritto da un modello da 7B che gira sulla
   macchina del giocatore. Vedi la spec:
   docs/superpowers/specs/2026-07-13-lua-sandbox-design.md (sezioni 2,3,4,9).

   Decisione di design: UNO STATO LUA PER SCRIPT, non uno stato condiviso fra
   tutti gli oggetti/nemici della run. Ogni ScriptSandbox e' un lua_State
   indipendente. Motivi:
   - un oggetto generato male (kill switch) non deve spegnere gli script di
     TUTTI gli altri oggetti della run: la spec (sezione 9) descrive il
     fallback come per-entita', non globale;
   - il tetto di memoria e' cosi' un numero fisso e semplice per script
     (es. 1 MB), non una quota dinamica da dividere fra N script attivi;
   - in caso di bug nella sandbox stessa (es. una fuga non ancora scoperta),
     l'isolamento per-stato e' una barriera in piu': anche se uno script
     riuscisse a corrompere qualcosa nel SUO lua_State, non toccherebbe gli
     altri, che vivono in heap C separate.

   Nota sul nome: il prefisso "ScriptVm" e' gia' usato da
   src/gameplay/script_vm.c per la mini-VM CSV a quattro operazioni (la rete
   di sicurezza a cui uno script rotto ripiega, vedi spec sezione 9). Questo
   modulo usa il prefisso ScriptSandbox per restare distinto: sono due cose
   diverse (un vero interprete Lua contro un parser di stringhe a operazioni
   fisse) e condividere il prefisso avrebbe violato la regola di AGENTS.md
   sui simboli generici invece di rispettarla. */

#include "lua.h"

#include <stdbool.h>
#include <stddef.h>

/* Budget dell'hook di conteggio istruzioni (LUA_MASKCOUNT). Due livelli
   (spec, sezione 4): generoso per compilare ed eseguire il corpo di primo
   livello dello script (dove tipicamente definisce le sue funzioni/tabelle),
   stretto per le chiamate di frame che il gioco fara' 60 volte al secondo. */
#define SCRIPT_SANDBOX_LOAD_BUDGET  1000000
#define SCRIPT_SANDBOX_FRAME_BUDGET 10000

/* Tetto di memoria indicativo suggerito dalla spec (sezione 3, punto 2).
   Il chiamante puo' passare un valore diverso a ScriptSandboxCreate. */
#define SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP (1024u*1024u)

typedef struct ScriptSandbox ScriptSandbox;

/* Crea uno stato Lua vuoto, blindato: _ENV costruito da zero (niente
   luaL_openlibs), allocatore custom con tetto 'memoryCapBytes', hash seed
   di Lua 5.5 impostato a 'seed' (determinismo di pairs(), vedi spec sezione
   3). Ritorna NULL se anche la sola inizializzazione dello stato Lua non
   entra nel budget di memoria (memoryCapBytes troppo piccolo: servono
   qualche decina di KB solo per le tabelle math/table filtrate). */
ScriptSandbox *ScriptSandboxCreate(unsigned int seed, size_t memoryCapBytes);

/* Chiude lo stato Lua (se presente) e libera la struttura. Sicura su NULL. */
void ScriptSandboxDestroy(ScriptSandbox *sb);

/* Compila (SOLO testo, mai bytecode: vedi il commento su ScriptSandboxLoad
   nel .c) ed esegue il corpo di primo livello di una sorgente non fidata,
   sotto il budget di caricamento. Ritorna false su qualunque errore
   (sintassi, memoria, budget di istruzioni, errore a runtime) e riempie
   'err' con un messaggio leggibile (puo' essere NULL/errSize 0 se il
   chiamante non lo vuole). Mai crash, mai un ciclo che non ritorna: uno
   script disonesto puo' al massimo far fallire questa chiamata.

   Qualunque fallimento marca la sandbox come disabilitata per sempre (vedi
   ScriptSandboxIsDisabled): chiamate successive a Load/CallVoid tornano
   false immediatamente senza toccare lo stato Lua. */
bool ScriptSandboxLoad(ScriptSandbox *sb, const char *name, const char *source,
                        char *err, size_t errSize);

/* Chiama una funzione globale definita dallo script (dopo una Load riuscita)
   con 'nargs' argomenti numerici, sotto il budget di frame. I valori di
   ritorno dello script vengono scartati (da cui "Void"): per leggere un
   risultato lo script scrive in una variabile globale, che il chiamante
   legge con ScriptSandboxGetGlobalNumber/String.

   ATTENZIONE alla chiamata variadica: ogni argomento DEVE essere di tipo
   'double' (letterali come 0.5 vanno bene; un intero letterale come 0 NON
   si promuove automaticamente a double attraverso "...", ed e' undefined
   behavior leggerlo con va_arg(ap, double) - scrivere 0.0).

   IMPORTANTE: se 'fn' non e' una funzione globale definita dallo script
   (es. un oggetto che non implementa quel particolare hook opzionale), la
   chiamata torna false SENZA disabilitare la sandbox: non e' una fuga ne'
   un errore dello script, e' normale che un item non implementi tutti gli
   hook del gioco. Solo un errore a runtime (o lo sforamento di un budget)
   durante l'ESECUZIONE della funzione disabilita la sandbox. */
bool ScriptSandboxCallVoid(ScriptSandbox *sb, const char *fn, int nargs, ...);

/* Restituisce lo stato Lua grezzo di 'sb', per chi (SOLO dentro src/script/:
   script_api.c e script_items.c, mai src/gameplay/, vedi AGENTS.md) deve
   costruire argomenti non numerici (es. la tabella delle statistiche di
   on_evaluate, o registrare nuove funzioni C nell'_ENV) prima di una
   chiamata protetta con ScriptSandboxProtectedCall sotto. Torna NULL se
   'sb' e' NULL o gia' disabilitata: il chiamante non deve toccare lo stack
   in quel caso (nessuna chiamata Lua e' mai sicura su una sandbox morta). */
lua_State *ScriptSandboxRawState(ScriptSandbox *sb);

/* Versione generica di ScriptSandboxCallVoid, per chiamate i cui argomenti
   non sono tutti 'double' (es. una tabella) o che vogliono leggere valori di
   ritorno. Il chiamante deve aver GIA' pushato sullo stack di
   ScriptSandboxRawState(sb), in quest'ordine: la funzione da chiamare, poi
   i suoi 'nargs' argomenti. Questa funzione esegue lua_pcall(L, nargs,
   nresults, 0) sotto lo STESSO budget di istruzioni di frame e con la
   STESSA classificazione di errore/uccisione permanente di
   ScriptSandboxCallVoid (patto di sicurezza, spec sezione 9): e' l'unico
   punto in cui quella logica vive, cosi' script_api.c/script_items.c non
   devono duplicarla. In caso di successo lascia 'nresults' valori sullo
   stack (il chiamante li legge e li ripulisce); in caso di fallimento lo
   stack e' gia' ripulito e la sandbox e' disabilitata. */
bool ScriptSandboxProtectedCall(ScriptSandbox *sb, int nargs, int nresults);

/* Vero se 'fn' e' attualmente una funzione globale chiamabile (per decidere
   se vale la pena chiamare ScriptSandboxCallVoid, es. per un hook opzionale
   come on_hit che non tutti gli oggetti definiscono). Non esegue script. */
bool ScriptSandboxHasFunction(const ScriptSandbox *sb, const char *fn);

/* Legge una variabile globale numerica/stringa impostata dallo script
   (tipicamente il risultato di una ScriptSandboxCallVoid). Non esegue
   codice Lua (nessun metodo/metatabella e' mai installato sulle tabelle di
   questa sandbox, quindi un accesso a globale e' un semplice lookup senza
   possibilita' di sollevare un errore). Tornano false se la sandbox e'
   disabilitata, se la variabile non esiste, o se ha il tipo sbagliato. */
bool ScriptSandboxGetGlobalNumber(const ScriptSandbox *sb, const char *name, double *out);
bool ScriptSandboxGetGlobalString(const ScriptSandbox *sb, const char *name, char *out, size_t outSize);

/* Diagnostica per i test e per il gioco. */
size_t ScriptSandboxMemoryUsed(const ScriptSandbox *sb);
bool ScriptSandboxIsDisabled(const ScriptSandbox *sb);
/* Motivo dell'ultima disattivazione (stringa vuota se non disabilitata).
   Il puntatore resta valido finche' la sandbox non viene distrutta. */
const char *ScriptSandboxDisabledReason(const ScriptSandbox *sb);

/* Scrive una riga con timestamp in logs/script-sandbox.log (la directory e'
   creata dal target del Makefile che produce il binario, non a runtime: vedi
   il .c). Usata internamente per loggare ogni kill con il suo motivo (spec,
   sezione 9: "l'evento finisce nel log"); esposta anche al chiamante per
   coerenza con le altre righe di log del gioco. */
void ScriptSandboxLogLine(const char *fmt, ...);

#endif
