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
