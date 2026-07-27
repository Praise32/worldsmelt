#ifndef MELTING_RUN_FUSION_H
#define MELTING_RUN_FUSION_H

#include "core/game_types.h"

/* LA FUSIONE -- meccanica-firma del gioco
 * (docs/design/systems/item-fusion.md; DEC-022, DEC-023, DEC-101, DEC-102,
 * DEC-125, DEC-143, DEC-162, DEC-171).
 *
 * Due oggetti posseduti + un catalizzatore raro (Flux, Player.flux) si
 * consumano e diventano UN oggetto nuovo che porta tratti riconoscibili di
 * entrambi. Il documento la costruisce in DUE STADI (DEC-023):
 *
 *   stadio 1  composizione IMMEDIATA e DETERMINISTICA -- questo modulo, tutto
 *             qui dentro. Il giocatore non aspetta mai: l'oggetto esiste,
 *             e' valido e usabile nello stesso istante della conferma.
 *   stadio 2  rifinitura IA in sottofondo (nome, comportamento e sprite
 *             dedicati) -- NON ESISTE NELLA DEMO. Vedi FUSION_STAGE_2_HOOK
 *             piu' sotto: il punto di aggancio e' documentato, nessun
 *             processo viene avviato. Il fallback naturale dello stadio 2 e'
 *             proprio lo stadio 1, che e' gia' un oggetto completo -- e'
 *             per questo che la demo puo' farne a meno senza rompere nulla.
 *
 * Prefisso 'Fusion' come vuole AGENTS.md; modulo gemello di item_slots.h /
 * item_pool.h / item_traits.h in questa stessa cartella. Come loro, tutto
 * qui e' PURO rispetto all'RNG: nessun time()/rand(), il solo ingresso
 * casuale e' il seed di run (Game.runSeed) mescolato con la coppia scelta.
 * Nessuna funzione di questo modulo apre un'immagine o crea una texture:
 * l'immagine curata (DEC-171) si sceglie leggendo un manifest di TESTO
 * (src/content/curated_images.h) e la texture la carica il renderer
 * (src/assets/game_assets.h) quando serve davvero disegnarla. */

/* Codifica dei due slot sorgente dentro AppUi (vedi il commento su
   AppUi.fusionSourceA in core/game_types.h): "indice + 1", cosi' lo zero di
   una struttura azzerata significa "nessuna sorgente scelta". */
#define FUSION_UI_NONE 0
#define FUSION_UI_SLOT(field) ((field) - 1)
#define FUSION_UI_FIELD(slot) ((slot) + 1)

/* Quanti trait al massimo porta un oggetto FUSO. Il contenuto generato ne
   dichiara al massimo 2 (NormalizeTraits, tools/melting-gen/gen_validate.c);
   un fuso puo' arrivare a 3 -- uno in piu', perche' "deve valere il costo di
   due oggetti" (DEC-162) -- ma non alla somma dei quattro possibili: il
   budget di LEGGIBILITA' (DEC-146) non si allarga per la fusione. Default
   proposto dall'implementazione (stile DEC-019). */
#define FUSION_MAX_TRAITS 3

/* Esito di un tentativo di fusione. FUSION_OK vale 0 di proposito (stessa
   disciplina zero-default del resto del progetto: un esito azzerato e' "e'
   andata bene", e chi non controlla nulla non vede mai un errore inventato).
   Ogni valore ha un testo leggibile in FusionStatusText: sono i messaggi che
   l'interfaccia mostra, mai un codice numerico a schermo. */
typedef enum FusionStatus {
    FUSION_OK = 0,
    FUSION_ERR_NO_CATALYST,   /* nessun Flux: la conferma resta disabilitata (item-fusion.md, casi limite) */
    FUSION_ERR_NEED_TWO,      /* meno di due oggetti idonei posseduti */
    FUSION_ERR_SAME_ITEM,     /* lo stesso oggetto scelto due volte */
    FUSION_ERR_NOT_ELIGIBLE   /* slot vuoto o indice fuori dall'inventario */
} FusionStatus;

const char *FusionStatusText(FusionStatus status);

/* "Idoneo alla fusione" (item-fusion.md, condizioni di ingresso: "almeno due
   oggetti idonei"). Il documento non restringe per categoria -- anzi DEC-101
   rende la fusione LIBERA fra categorie diverse -- quindi qui e' idoneo
   qualunque oggetto davvero posseduto, compreso un oggetto gia' nato da una
   fusione (DEC-102: ri-fusione ammessa, nessun limite concettuale). L'unica
   coppia vietata e' "un oggetto con se stesso", che non e' una restrizione di
   categoria ma il significato stesso di "due oggetti". Default proposto
   dall'implementazione (stile DEC-019). */
bool FusionItemEligible(const Item *item);
int FusionEligibleCount(const Player *p);

/* Tutto cio' che serve prima di consumare qualcosa: due indici validi,
   diversi, idonei, e almeno un catalizzatore. Non tocca nulla -- e' anche
   quello che l'interfaccia chiama a ogni frame per sapere se la conferma e'
   abilitata e quale messaggio mostrare. */
FusionStatus FusionCheck(const Player *p, int indexA, int indexB);

/* Quale delle due sorgenti e' la DOMINANTE (0 = a, 1 = b): vince la rarita'
   piu' alta; a parita' vince l'oggetto selezionato per PRIMO dal giocatore,
   cioe' 'a' (DEC-143 per la categoria, punto 4 di "Priorita' e conflitti"
   per i tratti: e' la stessa regola, quindi una sola funzione). */
int FusionDominant(const Item *a, const Item *b);

/* Chiave deterministica di QUESTA fusione: seed di run + quante fusioni sono
   gia' avvenute + i nomi dei due genitori. Stesso seed, stessa coppia, stesso
   ordinale => stessa chiave => stesso oggetto (e' cio' che il test di
   determinismo verifica). L'ordinale c'e' perche' il documento vieta le
   ricette fisse: rifondere la stessa coppia piu' tardi nella stessa run non
   deve dare due volte lo stesso identico oggetto. */
unsigned int FusionKey(unsigned int runSeed, int fusionOrdinal, const Item *a, const Item *b);

/* STADIO 1 (DEC-023): compone il risultato. Funzione PURA -- nessun Game,
   nessun file, nessuna allocazione: solo i due genitori e la chiave. Non
   assegna l'immagine (vedi FusionPerform) perche' quella dipende da cosa la
   run ha gia' usato. */
void FusionCompose(unsigned int key, const Item *a, const Item *b, Item *out);

/* Il flusso completo, l'unico punto che TOCCA il Game: verifica, compone,
   pesca l'immagine curata non ancora usata (DEC-171), consuma i due oggetti
   sorgente e un'unita' di catalizzatore, inserisce il risultato
   nell'inventario con la categoria ereditata (DEC-143) e ne carica
   l'eventuale Lua. Su qualunque esito diverso da FUSION_OK non consuma
   NULLA: la verifica sta prima di ogni scrittura, cosi' un errore non puo'
   mai far perdere un oggetto o un catalizzatore.
   'outFused' (se non NULL) riceve una copia del risultato -- serve
   all'interfaccia per mostrarlo subito e ai test per ispezionarlo. */
FusionStatus FusionPerform(Game *game, int indexA, int indexB, Item *outFused);

/* ---------------------------------------------------------------------------
   FUSION_STAGE_2_HOOK -- il punto di aggancio dello STADIO 2 (DEC-023 passo
   2), deliberatamente NON implementato nella demo.

   Cosa farebbe: dopo FusionPerform, avviare in sottofondo la rifinitura IA
   (nome, comportamento e sprite dedicati) e applicarla in silenzio
   all'oggetto quando arriva, senza mai mettere in pausa il giocatore.
   Dove andrebbe: src/app (l'unico modulo che possiede i processi esterni,
   vedi AGENTS.md), riusando src/gen/gen_runner.h esattamente come
   AppStartGeneration/AppStartProposeThemes -- MAI qui dentro: src/gameplay
   non avvia processi e non conosce i modelli.
   Perche' oggi non c'e': DEC-171 fissa che nella demo nessun modello immagine
   gira a runtime, e la priorita' e' la copertura dei sistemi; la
   composizione dello stadio 1 e' gia' un oggetto valido e utilizzabile
   (item-fusion.md, "Fallback"), quindi la sua assenza non lascia mai il
   giocatore senza risultato.
   Cosa cambierebbe quando arrivera': l'origine dichiarata del contenuto
   passerebbe da 'composto' a 'nuovo' (item-fusion.md, "Regole per contenuti
   generati"). Nient'altro di questo modulo.
   --------------------------------------------------------------------------- */

#endif
