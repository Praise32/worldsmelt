# Spec: sandbox Lua e contenuti davvero generati (fase 3)

Data: 2026-07-13
Ciclo: 3 di 5
Stato: da rivedere e approvare dall'autore

## 1. Perche' questa fase esiste

E' l'obiettivo dichiarato del progetto: **oggetti unici, sinergie inventate da zero,
comportamenti di nemici e boss, e layout delle stanze** decisi dall'IA a ogni run.

Oggi non e' possibile. La mini-VM che il gioco esegue conosce **quattro operazioni**
(`burst`, `projectile`, `area`, `heal`) con due parametri numerici ciascuna. L'LLM puo'
solo combinare quei mattoncini: puo' inventare il *nome* «Corona Frantumante», ma non puo'
inventare *cosa fa*. Ogni oggetto e' una delle quattro cose che il C sa gia' fare.

Questa fase mette al posto della mini-VM un vero linguaggio — **Lua**, in una sandbox — e
l'LLM ci scrive dentro comportamento vero.

## 2. Il problema centrale: quel Lua e' codice non fidato

Lo scrive un modello da 7 miliardi di parametri che gira sulla tua macchina. Non e'
malevolo, ma e' **inaffidabile**: puo' scrivere un ciclo infinito, allocare memoria senza
fine, sbagliare un nome di campo, o (senza volerlo) chiamare qualcosa che apre un file.

Trattiamolo esattamente come tratteresti codice arrivato da Internet. La ricerca ha
verificato **cinque vie di fuga** che una sandbox Lua ingenua lascia aperte, e che vanno
chiuse tutte:

1. **Bytecode.** `load()` accetta di default chunk binari (`mode = "bt"`), e Lua **non
   verifica il bytecode**: un chunk malformato fa crashare l'interprete. → si carica
   **solo testo**, `mode = "t"`, sempre. Un controllo in fase di build fallisce se nel
   codice compare `luaL_loadbuffer`/`luaL_loadstring`/`luaL_dostring` (che usano `"bt"`).
2. **La metatabella delle stringhe e' condivisa.** `getmetatable("")` restituisce una
   tabella il cui `__index` **e' la libreria `string` vera**. Uno script puo' riscriverla e
   avvelenare l'intera VM. → va protetta esplicitamente.
3. **Le coroutine bypassano il limite di istruzioni.** In Lua gli hook di debug sono
   *per-thread* e **non vengono ereditati** da una coroutine nuova: `coroutine.wrap(function()
   while true do end end)()` gira senza alcun freno. Verificato sperimentalmente durante la
   ricerca: 50 coroutine hanno mandato in fumo 50 volte il budget. → la libreria `coroutine`
   **non si espone**.
4. **`pcall` si mangia l'errore del budget.** `while true do pcall(f) end` sopravvive per
   sempre a un hook che solleva un errore. → `pcall` **non si espone**.
5. **Le funzioni di pattern matching girano in C.** `string.find`, `gsub`, `match`, `gmatch`
   non hanno limiti di backtracking e non fanno mai scattare l'hook, che conta solo
   istruzioni Lua: un pattern patologico blocca il gioco per sempre. → **fuori
   dall'allowlist**.

In piu': niente `io`, `os`, `package`/`require`, `debug`, `collectgarbage`, `setmetatable`,
`rawset`/`rawget`, `string.dump`.

**Regola generale: non si parte da `luaL_openlibs` togliendo roba. Si parte dal vuoto e si
aggiunge solo cio' che serve.**

## 3. Cosa scegliamo, e perche'

**Lua 5.5.0** (MIT), vendorizzato e compilato dentro il gioco. Non LuaJIT (ha la FFI, che e'
una fuga in un colpo solo, e non ha release vere). Non Luau (e' C++: trascinerebbe un
toolchain C++ dentro un gioco C99).

Il motivo decisivo e' il **determinismo**: in Lua 5.5 `lua_newstate` prende un **seed** per
l'hashing delle stringhe. Senza, l'ordine di `pairs()` cambia a ogni avvio (Lua 5.4 mescola
indirizzi ASLR e `time()` nel seed) e **la stessa run con lo stesso seed darebbe risultati
diversi**. Con il seed passato dal gioco, una run e' riproducibile.

**Limite noto del determinismo (trovato dalla revisione di sicurezza finale, non corretto nel
motore Lua stesso - solo aggirato lato prompt): il seed governa l'hashing delle CHIAVI STRINGA
e la RNG del gioco, non tutto cio' che potrebbe apparire deterministico.** Due buchi verificati
nel sorgente vendorizzato e a runtime:
- `tostring({})` e `tostring(function() end)` includono l'indirizzo di memoria grezzo
  dell'oggetto (vedi `deps/lua-5.5.0/src/lauxlib.c`, `luaL_tolstring`: per tabelle/funzioni/
  userdata/thread senza un `__tostring` personalizzato fa `lua_pushfstring(L, "%s: %p", kind,
  lua_topointer(L, idx))`, non un valore derivato dal seed): quell'indirizzo cambia da un
  processo all'altro (ASLR), quindi due run con lo stesso seed di gioco produrrebbero stringhe
  diverse se uno script generato chiamasse `tostring` su una tabella o una funzione.
- `pairs()` su una tabella con **chiavi di tipo riferimento** (tabelle o funzioni come chiave,
  a differenza delle chiavi stringa/numero) itera nell'ordine delle celle della parte hash
  della tabella, che per chiavi di tipo riferimento e' derivato anch'esso dall'indirizzo
  dell'oggetto, non dal seed di hashing delle stringhe: stesso problema, stessa causa.

Il rischio pratico e' basso (uno script tipico generato usa solo chiavi stringa/numero e non
chiama mai `tostring` su una tabella: non c'e' motivo di gioco per farlo), ma il criterio 5
("stesso seed -> stesso comportamento, byte per byte") promette riproducibilita' totale, quindi
va dichiarato onestamente invece di lasciarlo implicito. Non abbiamo patchato l'interprete Lua
vendorizzato per chiudere questi due casi (cambierebbe `luaO_tostringbuff`, funzione centrale
usata da un intero albero di chiamate, per un rischio che nessuno script generato oggi
attraversa): la mitigazione scelta e' lato prompt, la piu' economica che risolve il caso
pratico senza toccare il motore - il cheat-sheet (`tools/melting-gen/prompts/lua_system.txt`)
istruisce esplicitamente il modello a non chiamare mai `tostring` su una tabella o una funzione
e a usare solo chiavi stringa/numero nelle tabelle. Se in futuro emergesse un bisogno di gioco
reale per uno di questi due pattern, la correzione vera (non lato prompt) andrebbe rivalutata.

## 4. Le tre barriere di sicurezza

1. **Ambiente esplicito** (`_ENV` costruito a mano): solo `math` (senza `random`, sostituita
   dalla RNG del gioco), `table` (senza `remove`/`concat` illimitati), poche funzioni base
   (`ipairs`, `pairs`, `type`, `tonumber`, `tostring`, `select`, `error`, `assert`), piu'
   l'**API di gioco** (sotto). Niente altro.
2. **Tetto di memoria**: allocatore custom passato a `lua_newstate`, che restituisce `NULL`
   oltre un budget (indicativo: **1 MB per run**). Lua reagisce con una garbage collection
   d'emergenza e un secondo tentativo; se fallisce ancora, solleva un errore che `lua_pcall`
   cattura. Trappola nota, gia' individuata: nell'allocatore, quando `ptr == NULL` il
   parametro `osize` **non e' una dimensione, e' un codice di tipo** — sottrarlo e' il bug
   classico di questo pattern.
3. **Tetto di istruzioni**: hook `LUA_MASKCOUNT` che, superato il budget, chiama
   `luaL_error` (e' esattamente cio' che fa l'interprete `lua.c` ufficiale). Budget separati:
   generoso all'inizializzazione (`10^6` istruzioni), stretto per le callback di frame
   (`10^4`).

## 5. L'API di gioco esposta a Lua: handle, non puntatori

Uno script non riceve mai un puntatore. Riceve un **identificatore intero** (l'indice
nell'array C) accoppiato a un **contatore di generazione**, e ogni funzione C valida
l'handle prima di toccare qualsiasi cosa. E' l'unica forma sicura: un `lightuserdata` **e'**
il puntatore grezzo (lo script puo' conservarlo dopo che l'entita' e' morta → use-after-free),
e la userdata piena e' sicura ma piu' lenta e comunque soggetta a riferimenti pendenti.

Costo misurato sulla tua CPU: una chiamata con handle e validazione completa costa **60 ns**.
Una callback realistica (un po' di matematica + una chiamata di ritorno in C) costa **130 ns**.
Con 200 proiettili e 64 nemici sono **34 microsecondi per frame: lo 0,2% del budget a 60 FPS**.
Il costo delle callback non e' il problema; il problema sarebbe allocare una tabella nuova a
ogni chiamata (+146 ns e pressione sul garbage collector) → si riusa una tabella di scratch.

## 6. L'architettura dei contenuti: due strati

Non tutto deve essere Lua. La ricerca (e il paper *Correctness-Guaranteed Code Generation via
Constrained Decoding*, 2025, che affronta esattamente questo problema) converge su un ibrido:

**Strato dichiarativo (JSON vincolato da grammatica GBNF).** Per tutto cio' che il C sa gia'
eseguire: modificatori di statistiche, tabelle di spawn, layout delle stanze, regole del
piano. Qui l'LLM non scrive codice: descrive. Il JSON e' **valido per costruzione** (la
grammatica lo impone al campionamento) e il C lo esegue nativamente. Zero rischio.

**Strato Lua.** Solo per il comportamento davvero nuovo, quello che non si puo' descrivere
con dei parametri: la sinergia strana fra due oggetti, il pattern d'attacco di un boss.
Qui l'LLM scrive Lua vero, e valgono tutte le barriere di sopra.

Questa divisione e' anche la ragione per cui il progetto puo' funzionare con un 7B: il
modello scrive poco codice libero, e molto piu' spesso compila una descrizione.

## 7. Il modello concettuale: rubato a Isaac, riscritto da zero

Dalla documentazione dei modder di Isaac prendiamo **le idee**, non il codice (le regole di
un gioco non sono coperte da copyright; sprite, nomi e testi si').

**Il sistema delle cache** e' l'idea piu' importante. In Isaac, quando qualcosa cambia, il
gioco **ricalcola le statistiche da zero** (`MC_EVALUATE_CACHE`): parte dai valori base e
riapplica *tutti* i modificatori. Non muta in posto. Questo lo rende **idempotente**: un
oggetto generato dall'IA che sbaglia i conti non puo' accumulare danno all'infinito, e
rimuovere un oggetto e' banale (basta ricalcolare). Per contenuti generati dinamicamente e'
esattamente la proprieta' che serve.

Le callback che imitiamo (nomi nostri, semantica loro):

| Nostra callback | Equivalente Isaac | A che serve |
|---|---|---|
| `on_evaluate(player)` | `MC_EVALUATE_CACHE` | Applica i modificatori di statistiche. Ricalcolo da zero. |
| `on_fire(player, dir)` | `MC_POST_FIRE_TEAR` | Modifica o aggiunge colpi quando spari. |
| `on_hit(shot, enemy)` | `MC_ENTITY_TAKE_DMG` | Effetti all'impatto; puo' annullare il danno. |
| `on_tick(player, dt)` | `MC_POST_PEFFECT_UPDATE` | Logica passiva per frame (aure, timer). |
| `on_enemy_update(enemy, dt)` | `MC_NPC_UPDATE` | Movimento e attacchi di nemici e boss. |
| `on_room_enter(room)` | `MC_POST_NEW_ROOM` | Regole della stanza. |

Le **formule** (cadenza di tiro, danno, velocita' dei colpi) le reimplementiamo dalle
formule documentate, che sono fatti, non espressione.

## 8. Le stanze

L'LLM descrive le stanze in JSON (griglia di tile + lista di spawn), non in Lua. Il C
**valida** ogni stanza prima di accettarla, e la regola non negoziabile e':

- **raggiungibilita'**: dalle porte si deve poter raggiungere ogni spawn e ogni porta
  (flood fill sulla griglia). Una stanza non attraversabile viene scartata;
- densita' di nemici e ostacoli entro limiti;
- niente spawn dentro un muro o sopra una porta.

Stanza scartata → si usa una stanza di riserva generata proceduralmente, come oggi.

La generazione del *piano* (quali stanze, dove) resta in C con l'algoritmo di Isaac
reimplementato (griglia, conteggio stanze in crescita col piano, boss in un vicolo cieco):
e' una parte che deve *sentirsi* giusta, e un LLM la sbaglierebbe in modi noiosi.

## 9. Il patto di sicurezza

**Nessuno script puo' rompere il gioco.** Se uno script:

- non compila (controllo sintattico in C prima di eseguirlo);
- sfora il budget di memoria o istruzioni;
- solleva un errore a runtime;
- chiama un handle non valido;

allora viene **disabilitato in modo permanente per quella run**, l'entita' torna al suo
comportamento di riserva (l'oggetto usa la mini-VM di oggi, il nemico usa l'IA in C), e
l'evento finisce nel log. Il giocatore vede al massimo un oggetto un po' scialbo, mai un
crash.

La mini-VM attuale **non si butta**: diventa la rete di sicurezza.

## 10. Decomposizione: tre sotto-cicli

Questa fase e' troppo grande per un solo piano. Si fa in tre pezzi, ognuno giocabile:

- **3a — Sandbox e oggetti.** Lua vendorizzato, le tre barriere, l'API a handle, le callback
  degli oggetti (`on_evaluate`, `on_fire`, `on_hit`, `on_tick`), il sistema delle cache, la
  generazione degli script da parte dell'LLM, il fallback alla mini-VM. **Alla fine di 3a gli
  oggetti fanno cose che il C non sapeva fare.**
- **3b — Nemici e boss.** `on_enemy_update`, il sistema a parti per i boss (quello che avevi
  disegnato in `APPUNTI.md`), i pattern d'attacco generati.
- **3c — Stanze.** Layout in JSON, validatore di raggiungibilita', generazione del piano in C
  con l'algoritmo reimplementato.

## 11. Criteri di successo (3a)

1. Uno script Lua generato dall'LLM che fa qualcosa che la mini-VM **non sa esprimere** gira
   in gioco (esempio concreto: «ogni terzo colpo si sdoppia e i frammenti inseguono il nemico
   piu' vicino, ma solo se hai meno di tre cuori»).
2. Un ciclo infinito in uno script non blocca il gioco: viene ucciso e l'oggetto ripiega.
3. Una bomba di memoria non fa crescere la memoria del gioco: viene uccisa e l'oggetto ripiega.
4. Uno script che prova ad aprire un file **non compila nemmeno** (la funzione non esiste).
5. Stessa run, stesso seed → stesso comportamento, byte per byte.
6. Il costo delle callback resta sotto l'1% del frame a 60 FPS, misurato.
