#ifndef MELTING_RUN_ENEMY_TYPE_H
#define MELTING_RUN_ENEMY_TYPE_H

#include <stdbool.h>

/* Tipi di nemico (fase 3b, docs/engineering/specs/2026-07-14-step-3b-enemies.md).
 *
 * STESSO PRINCIPIO DEI TIPI DI COLPO (core/shot_type.h), che ormai e' la regola
 * della casa: il motore NON ha un catalogo di nemici. Espone un vocabolario
 * parametrico -- una forma (come appare), un movimento (come si muove), un modo di
 * sparare (come attacca) e cinque manopole clampate -- e il MODELLO inventa i
 * nemici di ogni run nel JSON. I quattro nemici storici (inseguitore, tiratore,
 * corazzato, boss) sopravvivono solo come ripiego procedurale (EnemyTypeExample) e
 * come esempi nel prompt.
 *
 * Come shot_type.h, questo header NON include raylib ne' game_types.h: lo includono
 * SIA il gioco SIA melting-gen, quindi la definizione e' una sola e non puo'
 * divergere (src/core/enemy_type.c e' compilato dentro entrambi i binari, vedi
 * GEN_EXTRA_SRC nel Makefile). */

/* Come APPARE. BLOB = 0 (zero-default): un Enemy azzerato con memset resta la
   palla di sempre, mai una forma esotica per sbaglio. */
typedef enum EnemyForm {
    ENEMY_FORM_BLOB = 0,   /* massa tonda, occhi: l'inseguitore di sempre */
    ENEMY_FORM_SPIKY,      /* stella spinosa */
    ENEMY_FORM_ARMORED,    /* blocco squadrato con piastre */
    ENEMY_FORM_FLOATER,    /* medusa che fluttua, con tentacoli */
    ENEMY_FORM_COUNT
} EnemyForm;

/* Come SI MUOVE. CHASE = 0 (zero-default): ti viene addosso, il comportamento piu'
   semplice e piu' innocuo da subire. */
typedef enum EnemyMove {
    ENEMY_MOVE_CHASE = 0,  /* dritto verso il giocatore */
    ENEMY_MOVE_KITE,       /* si tiene a distanza: avanza se sei lontano, indietreggia se sei vicino */
    ENEMY_MOVE_ORBIT,      /* gira attorno al giocatore a distanza fissa */
    ENEMY_MOVE_ZIGZAG,     /* verso il giocatore ma serpeggiando: difficile da colpire */
    ENEMY_MOVE_CHARGE,     /* si ferma, prende la mira, poi scatta */
    ENEMY_MOVE_COUNT
} EnemyMove;

/* Come SPARA. NONE = 0 (zero-default): non spara affatto, fa danno solo al
   contatto -- di nuovo, il caso piu' innocuo. */
typedef enum EnemyFire {
    ENEMY_FIRE_NONE = 0,   /* solo contatto */
    ENEMY_FIRE_SINGLE,     /* un colpo mirato */
    ENEMY_FIRE_SPREAD,     /* un ventaglio di colpi verso il giocatore */
    ENEMY_FIRE_RING,       /* una corona di colpi in tutte le direzioni */
    ENEMY_FIRE_COUNT
} EnemyFire;

typedef struct EnemyTypeDef {
    bool active;
    char name[32];         /* inventato dal modello: mostrato per i boss, usato nei log */
    /* W8: l'IMAGE-ID di questo nemico (DEC-175(b)), cioe' la chiave con cui il
       renderer cerca il suo spritesheet in assets/art/{enemies,bosses}. NON e'
       un percorso di file e non deve diventarlo: il contenuto referenzia un
       image-id, la risoluzione a file resta di src/assets. Vuoto = nessun
       originale artistico, e il gioco ricade sulle sagome geometriche di
       DrawEnemy come prima di W8 -- e' il caso di OGNI nemico inventato dal
       modello (melting-gen non scrive mai questo campo: gli image-id li
       assegna il layer di indirezione del contenuto CURATO, che e' l'unico a
       conoscerli) e quindi lo zero-default corretto.
       Viaggia dentro EnemyTypeDef per lo stesso motivo di tutto il resto qui:
       il tipo viene copiato per valore dentro l'Enemy allo spawn, e il
       contenuto del piano puo' cambiare sotto i piedi (generazione pigra). */
    char imageId[40];
    EnemyForm form;
    EnemyMove move;
    EnemyFire fire;
    float hpMul;           /* moltiplicatori delle BASI del motore, mai valori assoluti: */
    float speedMul;        /* un tipo di nemico modula il nemico base, non lo sostituisce, */
    float sizeMul;         /* cosi' la scalatura per piano resta l'unica fonte di verita'. */
    float fireRate;        /* colpi al secondo (0 = non spara, qualunque cosa dica 'fire') */
    int pellets;           /* colpi per raffica, per SPREAD/RING */
    bool boss;             /* un boss ha basi diverse (vedi ENEMY_TYPE_BOSS_*) e vita in una stanza da solo */
} EnemyTypeDef;

/* Bande delle manopole: MODIFICA QUI per ribilanciare. */
#define ENEMY_TYPE_HP_MIN     0.5f
#define ENEMY_TYPE_HP_MAX     3.0f
#define ENEMY_TYPE_SPEED_MIN  0.4f
#define ENEMY_TYPE_SPEED_MAX  2.0f
#define ENEMY_TYPE_SIZE_MIN   0.6f
#define ENEMY_TYPE_SIZE_MAX   2.2f
#define ENEMY_TYPE_RATE_MIN   0.0f
#define ENEMY_TYPE_RATE_MAX   2.5f
#define ENEMY_TYPE_PELLETS_MAX 8

/* Banda di potenza dopo EnemyTypeBalance. 1.0 = "il nemico base": un tipo inventato
   dal modello deve essere DIVERSO dal nemico base, non piu' forte.
   Il BOSS ha una banda sua, piu' alta: bilanciarlo come un nemico normale sarebbe
   un errore grosso e silenzioso -- il boss d'esempio (corona di 8 proiettili) ha
   potenza ~2.2, quindi la rete gli avrebbe DIMEZZATO la vita per riportarlo a 1.0,
   rendendo il boss piu' debole di un corazzato. Un boss deve essere forte: la banda
   dice solo che non puo' diventare un muro impossibile. Attenzione: i
   moltiplicatori di un boss si applicano alle BASI DA BOSS (vita ~150+, non 24),
   quindi "potenza 2.2" qui significa "due volte scomodo rispetto a un nemico
   normale", non "il boss e' due nemici". */
#define ENEMY_TYPE_POWER_TARGET 1.0f
#define ENEMY_TYPE_POWER_MIN    0.7f
#define ENEMY_TYPE_POWER_MAX    1.35f
#define ENEMY_TYPE_BOSS_POWER_TARGET 2.2f
#define ENEMY_TYPE_BOSS_POWER_MIN    1.4f
#define ENEMY_TYPE_BOSS_POWER_MAX    3.2f

/* Testo canonico <-> enum (gli stessi testi che il modello scrive nel JSON, che
   gen_manifest.c scrive nel manifest e che run_content.c rilegge). Testo
   sconosciuto -> il valore 0, cioe' il piu' innocuo. */
EnemyForm EnemyFormFromText(const char *text);
EnemyMove EnemyMoveFromText(const char *text);
EnemyFire EnemyFireFromText(const char *text);
const char *EnemyFormName(EnemyForm form);
const char *EnemyMoveName(EnemyMove move);
const char *EnemyFireName(EnemyFire fire);

void EnemyTypeClamp(EnemyTypeDef *type);

/* La potenza stimata di un nemico, in unita' di nemico base (1.0 = come
   l'inseguitore di sempre). Formula chiusa, deterministica, testabile: pesa la vita
   (quanto ci metti a farlo fuori), l'offesa (contatto + fuoco), e la difficolta' di
   colpirlo (piccolo e veloce = difficile, grande e lento = facile). */
float EnemyTypePower(const EnemyTypeDef *type);

/* Clampa e GARANTISCE che EnemyTypePower finisca dentro
   [ENEMY_TYPE_POWER_MIN, ENEMY_TYPE_POWER_MAX], qualunque cosa il modello abbia
   inventato: se e' gia' in banda lo lascia stare (la sua scelta e' rispettata),
   altrimenti risolve hpMul per centrare il bersaglio e, se non basta, taglia le
   manopole offensive (pellets, poi cadenza di fuoco). E' la prima delle due reti
   che permettono di lasciare a un 7B l'invenzione dei nemici; la seconda e' il
   budget di difficolta' della stanza (src/world/world.c), che decide QUANTI
   nemici spawnare in base a quanto costano. Idempotente. */
void EnemyTypeBalance(EnemyTypeDef *type);

/* Tipi di ESEMPIO (ripiego procedurale, NON un catalogo del motore -- vedi il
   commento in cima). 0..2 sono i tre nemici normali (l'inseguitore, il tiratore e
   il corazzato di sempre), 3 e' il boss. Gia' bilanciati. */
#define ENEMY_TYPE_EXAMPLE_COUNT 3
void EnemyTypeExample(EnemyTypeDef *out, int index);
void EnemyTypeExampleBoss(EnemyTypeDef *out);

#endif
