#ifndef MELTING_RUN_SHOT_TYPE_H
#define MELTING_RUN_SHOT_TYPE_H

#include <stdbool.h>

/* Tipi di colpo (step C, docs/engineering/specs/2026-07-14-step-c-shottype-balance.md).
 *
 * DECISIONE DI DESIGN CHE GOVERNA TUTTO QUESTO FILE (feedback del proprietario,
 * 2026-07-14): "i tipi di colpo nuovi devono SEMPRE essere creati dai modelli
 * AI". Non esiste quindi un menu fisso di tipi in C (niente enum SHOT_NAIL/
 * SHOT_LASER/SHOT_ELECTRIC): il C espone solo un VOCABOLARIO parametrico -- una
 * forma di resa (ShotForm, come APPARE) e sette manopole clampate (come si
 * COMPORTA) -- e il modello inventa nome, forma e numeri di ogni tipo nel JSON
 * della run (tools/melting-gen/run.gbnf). "Chiodi", "laser", "scarica" sono solo
 * ESEMPI nel prompt e nel ripiego procedurale (gen_fallback.c), mai una
 * categoria privilegiata dentro il motore.
 *
 * Questo header NON include raylib ne' core/game_types.h di proposito: lo
 * includono SIA il gioco (src/core/game_types.h, che incorpora ShotTypeDef in
 * Item/Player) SIA melting-gen (tools/melting-gen/melting_gen.h). Una sola
 * definizione condivisa, impossibile da far divergere -- lo stesso problema che
 * per rarity/kind e' stato risolto a mano (due elenchi di stringhe da tenere
 * sincronizzati, vedi RarityFromText/GEN_RARITIES): qui il compilatore lo
 * garantisce da solo. Per lo stesso motivo src/core/shot_type.c e' compilato
 * dentro entrambi i binari (vedi GEN_EXTRA_SRC nel Makefile). */

/* La RESA di un colpo. SHOT_FORM_ORB vale 0 di proposito, come ITEM_ACTIVE e
   RARITY_COMMON: un Item azzerato con "{0}", un memset, o un manifest vecchio
   senza righe "shot*" restano la palla di sempre, mai una forma esotica per
   sbaglio. */
typedef enum ShotForm {
    SHOT_FORM_ORB = 0,   /* la palla di sempre: due cerchi */
    SHOT_FORM_SPIKE,     /* proiettile allungato orientato dalla velocita' (chiodo, dardo, scheggia) */
    SHOT_FORM_BEAM,      /* raggio sottile e lungo, con scia */
    SHOT_FORM_ARC,       /* spezzata a zig-zag (scarica, fulmine) */
    SHOT_FORM_BLADE,     /* lama/quadrato che ruota su se' stesso */
    SHOT_FORM_COUNT      /* non e' una forma: conta le forme note */
} ShotForm;

/* Un tipo di colpo completo. 'active' falso = nessun tipo (il colpo base del
   gioco): e' lo zero-default, quindi un Item "{0}" non cambia nulla. I
   moltiplicatori sono relativi alle statistiche del giocatore (player.damage,
   player.shotSpeed, player.shotRadius e la vita di base di un colpo), MAI valori
   assoluti: un tipo di colpo non puo' scavalcare il sistema delle cache delle
   statistiche, lo modula soltanto. */
typedef struct ShotTypeDef {
    bool active;
    char name[32];       /* inventato dal modello; mostrato nella GUI e nel messaggio di pickup */
    ShotForm form;
    float speedMul;
    float damageMul;
    float radiusMul;
    float lifeMul;
    int pierceBonus;     /* nemici attraversati in piu' */
    int chain;           /* salti verso un altro nemico all'impatto */
    int pellets;         /* colpi per sparo (ventaglio) */
} ShotTypeDef;

/* Bande delle manopole: MODIFICA QUI per ribilanciare. Sono i confini entro cui
   qualunque numero inventato dal modello viene riportato (ShotTypeClamp), prima
   ancora del bilanciamento di potenza sotto. */
#define SHOT_TYPE_SPEED_MIN   0.5f
#define SHOT_TYPE_SPEED_MAX   2.0f
#define SHOT_TYPE_DAMAGE_MIN  0.3f
#define SHOT_TYPE_DAMAGE_MAX  2.0f
#define SHOT_TYPE_RADIUS_MIN  0.4f
#define SHOT_TYPE_RADIUS_MAX  2.5f
#define SHOT_TYPE_LIFE_MIN    0.5f
#define SHOT_TYPE_LIFE_MAX    2.0f
#define SHOT_TYPE_PIERCE_MAX  3
#define SHOT_TYPE_CHAIN_MAX   3
#define SHOT_TYPE_PELLETS_MAX 3

/* Banda di potenza accettabile dopo ShotTypeBalance (vedi sotto). Il bersaglio
   e' 1.0 = "esattamente il colpo base": un tipo di colpo deve essere un
   SIDEGRADE alla Isaac (chiodi veloci e deboli, laser che perfora, scarica che
   salta: diversi, non piu' forti), mai un upgrade secco. */
#define SHOT_TYPE_POWER_TARGET 1.0f
#define SHOT_TYPE_POWER_MIN    0.75f
#define SHOT_TYPE_POWER_MAX    1.25f

/* Proxy primario del budget di leggibilita' (DEC-146,
   docs/design/systems/combat-and-projectiles.md#budget-di-leggibilita'):
   percentuale STIMATA di schermo coperta dai proiettili del giocatore che
   questo tipo di colpo dichiara, in un dato istante. Non e' una simulazione
   (il motore non sa qui quanti nemici ci sono ne' quanto dura una stanza):
   e' una formula chiusa, deterministica, sullo stesso spirito di
   ShotTypePower sopra -- pesa cio' che aumenta quanti segnali stanno a
   schermo CONTEMPORANEAMENTE (pellets, quanti pallettoni per sparo) e per
   QUANTO (lifeMul, quanto restano vivi prima di sparire) contro l'AREA che
   ciascuno occupa (radiusMul al quadrato). SHOT_TYPE_READABILITY_BASELINE_PERCENT
   e' la percentuale stimata del colpo BASE (radius=1, pellets=1, life=1):
   valore draft, da playtest (stesso trattamento di SHOT_TYPE_POWER_TARGET),
   scelto piccolo perche' il colpo base e' il riferimento "sempre leggibile".
   SHOT_TYPE_READABILITY_MAX_PERCENT e' la soglia oltre la quale
   ShotTypeReadabilityOk sotto torna falso: DRAFT, da playtest (DEC-146 fissa
   che il proxy esiste ed e' una percentuale di copertura schermo, non il
   valore soglia esatto). DEC-146 e' esplicito sulla conseguenza di uno
   sforamento: "un contenuto che la supera NON PASSA LA VALIDAZIONE e segue la
   normale catena di fallback" -- una riparazione sul posto (tagliare pellets
   poi radiusMul) NON e' quella catena: uscirebbe dalla banda di potenza gia'
   garantita da ShotTypeBalance senza rigirare quel bilanciamento, facendo
   divergere il manifest (che dichiarerebbe il tipo riparato) da quanto il
   gioco vedrebbe. Per questo qui c'e' solo il predicato, mai una funzione che
   muta 'type': chi valida un tipo di colpo generato (tools/melting-gen/
   gen_validate.c) usa ShotTypeReadabilityOk per decidere SE seguire la catena
   di fallback (sostituire l'INTERO tipo con quello procedurale del piano),
   non per limarlo campo per campo. */
#define SHOT_TYPE_READABILITY_BASELINE_PERCENT 4.0f
#define SHOT_TYPE_READABILITY_MAX_PERCENT      18.0f

/* Stima la percentuale di schermo coperta (vedi sopra). 'type' non attivo o
   NULL = il colpo base, SHOT_TYPE_READABILITY_BASELINE_PERCENT alla lettera. */
float ShotTypeReadabilityPercent(const ShotTypeDef *type);

/* Vero se ShotTypeReadabilityPercent(type) <= SHOT_TYPE_READABILITY_MAX_PERCENT. */
bool ShotTypeReadabilityOk(const ShotTypeDef *type);

/* --- Budget del RISULTATO di una sinergia dichiarata (DEC-162) --------------
   docs/design/systems/synergies.md#budget-di-potenza-del-risultato.

   DEC-162 dice due cose distinte sul risultato di una sinergia/fusione: il
   budget di POTENZA e' dedicato e piu' alto di quello del singolo oggetto
   (garanzia a runtime, ScriptItemsClampSynergyResultDelta in
   src/script/script_items.c), mentre il budget di LEGGIBILITA' NON si allarga
   mai (synergies.md, "Limiti di leggibilita'": il limite "e' definito una sola
   volta in Combat and Projectiles", non riformulato ne' rilassato per le
   sinergie). Quello che nessuna delle due garanzie copre e' il RISULTATO come
   contenuto: cio' che finisce a schermo quando la coppia si accende non e' il
   tipo di colpo dichiarato, e' il tipo di colpo PIU' i bonus di canale B della
   regola (src/gameplay/synergies.c). E questi bonus non passano da alcun tetto
   di leggibilita' a runtime: CombatFireShot (src/gameplay/combat.c, ~riga 293)
   somma SynergiesExtraPellets(...) ai pallettoni del tipo di colpo e taglia
   solo a 5 -- ben oltre SHOT_TYPE_PELLETS_MAX. Un tipo di colpo puo' quindi
   stare sotto la soglia DEC-146 da solo e sforarla appena la sinergia che ESSO
   STESSO dichiara si accende: e' il controllo che manca, e vive qui perche' la
   conseguenza (scartare il contenuto e seguire la catena di fallback) e'
   possibile solo a tempo di generazione (tools/melting-gen/gen_validate.c) --
   a runtime la coppia si e' gia' formata.

   "Dichiarata dal contenuto" ha un significato preciso: la tavola delle
   sinergie condiziona su SEGNALI, e i soli segnali che un tipo di colpo
   generato puo' portare sono SIG_SHOT_CHAIN e SIG_SHOT_PIERCE (synergies.c).
   Un tipo di colpo con chain > 0 o pierceBonus > 0 dichiara quindi da solo, nel
   proprio contenuto, meta' di una coppia canonica -- l'altra meta' e' un
   qualunque oggetto del pool, che il pool contiene per costruzione. Oggi solo
   SIG_SHOT_CHAIN e' usato da una regola (Arco Voltaico); SIG_SHOT_PIERCE fa
   parte del vocabolario dei segnali ma nessuna riga della tavola lo usa ancora:
   il predicato sotto lo copre lo stesso, perche' il controllo e' conservativo
   (stessa soglia, non una piu' larga) e perche' un segnale coperto in anticipo
   e' preferibile a una regola futura che passa in silenzio.

   SHOT_TYPE_SYNERGY_RESULT_EXTRA_PELLETS e' quanto il risultato aggiunge, nel
   caso PEGGIORE, ai termini del proxy: i soli bonus di canale B che cambiano
   l'area o quanti proiettili distinti stanno a schermo insieme sono i
   pallettoni (SynergiesExtraPellets, valore piatto che NON scala con la
   rarita'); pierce/bounce/chain non moltiplicano i segnali simultanei ne'
   l'area, entrano nella POTENZA, che ha il suo budget dedicato a runtime.
   Il valore duplica la somma dei 'pelletBonus' della tavola delle sinergie, che
   questo file non puo' includere (gameplay/synergies.h tira dentro
   core/game_types.h e quindi raylib, vietato qui: vedi il commento in cima).
   La duplicazione e' PINNATA da un test, non lasciata alla buona volonta':
   TestSynergyResultReadabilityBudget (test AW di src/tests/script_items_tests.c,
   'make test-script') confronta questa costante con SynergiesExtraPellets(
   maschera piena) e fallisce se la tavola cambia senza che cambi questo
   numero. */
#define SHOT_TYPE_SYNERGY_RESULT_EXTRA_PELLETS 1

/* Vero se 'type' dichiara da solo un segnale di sinergia (chain o pierce): vedi
   sopra. NULL o non attivo = falso (il colpo base non dichiara nulla). */
bool ShotTypeDeclaresSynergySignal(const ShotTypeDef *type);

/* Percentuale stimata di schermo coperta dal RISULTATO: ShotTypeReadabilityPercent
   sul tipo di colpo con i pallettoni che la sinergia dichiarata gli aggiungerebbe.
   Per un tipo che non dichiara alcun segnale e' identica a
   ShotTypeReadabilityPercent (nessuna sinergia dichiarata, nessun risultato da
   stimare). */
float ShotTypeSynergyResultReadabilityPercent(const ShotTypeDef *type);

/* Vero se ShotTypeSynergyResultReadabilityPercent(type) <= SHOT_TYPE_READABILITY_MAX_PERCENT:
   la STESSA soglia del singolo, perche' il budget di leggibilita' non si allarga
   per le sinergie (synergies.md). Come ShotTypeReadabilityOk, e' solo un
   predicato: chi valida decide se seguire la catena di fallback, mai come limare
   il tipo di colpo. */
bool ShotTypeSynergyResultReadabilityOk(const ShotTypeDef *type);

/* Testo canonico <-> enum. Gli stessi testi che il modello scrive nel JSON
   (run.gbnf), che gen_manifest.c scrive nel manifest e che run_content.c
   rilegge: un solo elenco, qui, invece dei due elenchi paralleli che rarity/kind
   devono tenere sincronizzati a mano. Un testo sconosciuto ricade su
   SHOT_FORM_ORB (mai una forma esotica per un dato corrotto). */
ShotForm ShotFormFromText(const char *text);
const char *ShotFormName(ShotForm form);

/* Riporta ogni manopola dentro la sua banda. Non tocca 'active' ne' 'name'. */
void ShotTypeClamp(ShotTypeDef *type);

/* Il "budget di potenza" stimato di un tipo di colpo, in unita' di colpo base
   (1.0 = come sparare senza alcun tipo di colpo). Non e' una simulazione: e' una
   formula chiusa, deterministica e testabile, che pesa ogni manopola per quanto
   conta davvero in combattimento (il danno linearmente, i pallettoni quasi
   linearmente, perforazione e catena come moltiplicatori di bersagli colpiti,
   velocita'/raggio/vita come comodita' sublineari). */
float ShotTypePower(const ShotTypeDef *type);

/* Clampa, poi GARANTISCE che ShotTypePower(type) finisca dentro
   [SHOT_TYPE_POWER_MIN, SHOT_TYPE_POWER_MAX], qualunque cosa il modello abbia
   inventato: se il tipo e' gia' in banda lo lascia stare (la scelta del modello
   e' rispettata quando e' sensata), altrimenti risolve damageMul per centrare il
   bersaglio e, se non basta (un tipo con TUTTE le manopole al massimo resta
   rotto anche col danno minimo), taglia una alla volta la manopola discreta che
   contribuisce di piu' finche' il tipo rientra. E' l'unica ragione per cui il
   motore puo' lasciare che sia un 7B a inventare i tipi di colpo: qualunque cosa
   scriva, non puo' ne' rompere il gioco ne' produrre un dud. Gira due volte,
   indipendentemente: in melting-gen (il manifest e' gia' bilanciato e
   ispezionabile a mano) e in run_content.c al caricamento (difesa in profondita'
   contro un manifest scritto/modificato a mano). Idempotente: applicarla due
   volte da' lo stesso risultato. */
void ShotTypeBalance(ShotTypeDef *type);

/* Lo STESSO bilanciamento, verso una banda diversa da quella del singolo
   oggetto: e' l'unico modo previsto per dare a un contenuto un budget di
   potenza DEDICATO senza scrivere una seconda rete di bilanciamento accanto a
   questa (che divergerebbe al primo ritocco). ShotTypeBalance sopra e' oggi
   esattamente ShotTypeBalanceTo(type, SHOT_TYPE_POWER_TARGET,
   SHOT_TYPE_POWER_MIN, SHOT_TYPE_POWER_MAX).
   Unico chiamante con una banda diversa: la FUSIONE (DEC-162, vedi le
   costanti qui sotto). Attenzione: alzare la banda alza il DANNO, mai
   pallettoni/raggio/vita -- il budget di LEGGIBILITA' (DEC-146) non si
   allarga mai, nemmeno per un risultato di fusione, ed e' il chiamante a
   doverlo comunque verificare con ShotTypeReadabilityOk. */
void ShotTypeBalanceTo(ShotTypeDef *type, float target, float minPower, float maxPower);

/* Banda di potenza del tipo di colpo di un oggetto FUSO (DEC-162,
   docs/design/systems/item-fusion.md: "il risultato di una fusione deve
   valere meccanicamente il costo di due oggetti e un catalizzatore raro").
   Il minimo coincide con SHOT_TYPE_POWER_MAX: un colpo fuso e' quindi SEMPRE
   almeno forte quanto il piu' forte colpo di un singolo oggetto, che e'
   letteralmente "un budget dedicato, piu' alto di quello del singolo
   oggetto sorgente". I tre numeri sono "default proposti
   dall'implementazione" (stile DEC-019): il documento fissa che il budget
   esiste ed e' piu' alto, non il valore -- che resta materia di playtest
   (domanda aperta gia' registrata in item-fusion.md).
   Il bersaglio 1.35 e' raggiungibile anche dal tipo di colpo piu' fiacco
   possibile: rest minimo ~0.71 x damageMul massimo 2.0 = 1.42 > 1.35. */
#define SHOT_TYPE_FUSION_POWER_TARGET 1.35f
#define SHOT_TYPE_FUSION_POWER_MIN    SHOT_TYPE_POWER_MAX
#define SHOT_TYPE_FUSION_POWER_MAX    1.60f

/* Tipi di colpo di ESEMPIO (indice 0..SHOT_TYPE_EXAMPLE_COUNT-1), gia' bilanciati.
 *
 * LEGGERE IL COMMENTO IN CIMA A QUESTO FILE PRIMA DI TOCCARLI: questi NON sono
 * "i tipi di colpo del gioco". Sono il ripiego procedurale per il caso in cui il
 * modello non c'e' (melting-gen --fallback, o il gioco senza alcun manifest sul
 * disco), esattamente come MakeFallbackTheme/MakeFallbackItem lo sono per temi e
 * oggetti: un contenuto di riserva, non una categoria privilegiata. Il motore non
 * sa nulla di "chiodi" o "raggi": sa solo di forme e manopole, e questi tre non
 * hanno alcuna corsia preferenziale (passano per ShotTypeBalance come quelli del
 * modello).
 *
 * Vivono qui, e non nei due ripieghi separati (src/content/run_content.c per il
 * gioco, tools/melting-gen/gen_fallback.c per il generatore), per lo stesso
 * motivo per cui ShotTypeDef vive qui: una sola definizione, impossibile da far
 * divergere. 'index' fuori range ricade sull'esempio 0. */
#define SHOT_TYPE_EXAMPLE_COUNT 3
void ShotTypeExample(ShotTypeDef *out, int index);

#endif
