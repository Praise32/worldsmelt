#ifndef MELTING_RUN_CHARACTER_TYPE_H
#define MELTING_RUN_CHARACTER_TYPE_H

#include "core/shot_type.h"

/* M6b-1 (DEC-014, prima fetta -- stats+palette+carta del personaggio
 * alternativo per-run) e M6b-2 (DEC-037, trait Lua). M6b-3 (DEC-068, questa
 * fetta) aggiunge il COLPO FIRMATO opzionale: qui sotto solo il budget
 * (CHARACTER_SHOT_CAUTION_FRACTION e i campi hasShot/signatureShot); la
 * generazione/validazione del colpo vero riusa ShotTypeDef/ShotTypeClamp/
 * ShotTypeBalance COSI' COME SONO (shot_type.h), nessuna modifica: e' per
 * questo che questo header ora lo include.
 *
 * Come shot_type.h/enemy_type.h (vedi i loro commenti in cima), questo
 * header NON include raylib ne' core/game_types.h di proposito: lo include
 * SIA il gioco (src/content/character_proposal.c, che converte in
 * CharacterDef con una Color vera) SIA melting-gen (tools/melting-gen/
 * main.c, che scrive generated/character_proposal.json). Una sola
 * definizione di bande e di clamp, impossibile da far divergere fra le DUE
 * reti di sicurezza che questa fetta richiede (spec, punto (d)): il tool le
 * applica PRIMA di scrivere il json, il gioco le riapplica ALLA LETTURA
 * (difesa in profondita' contro un file forgiato a mano fuori banda, o un
 * bug futuro in una delle due reti). shot_type.h non include raylib nemmeno
 * lui (stesso principio), quindi includerlo qui non trascina il gioco
 * dentro questo header "core" indipendente.
 *
 * A differenza dei tipi di colpo/nemico, qui il modello non inventa un
 * vocabolario nuovo per le SEI manopole di statistica: i campi sono fissi
 * (nome, blurb, sei manopole, una palette), e le bande sono DEFAULT
 * PROPOSTI stile DEC-019 (da playtest, vedi game-design-knowledge-base/
 * docs/game-design/systems/characters.md, blocco "Default proposti
 * dall'implementazione" per il personaggio generato) centrati sulla rosa
 * curata (src/content/character_roster.c): il personaggio generato puo'
 * risultare piu' o meno estremo di un personaggio base, ma sempre dentro
 * limiti garantiti. Il colpo firmato, quando c'e', INVENTA forma+manopole
 * come i tipi di colpo di run.gbnf -- stesso vocabolario, stessa doppia
 * rete (vedi sopra). */

#define CHARACTER_GEN_NAME_LEN  32
#define CHARACTER_GEN_BLURB_LEN 160

#define CHARACTER_DAMAGE_MIN     6.0f
#define CHARACTER_DAMAGE_MAX     11.0f
#define CHARACTER_FIRE_DELAY_MIN 0.19f
#define CHARACTER_FIRE_DELAY_MAX 0.28f
#define CHARACTER_SHOT_SPEED_MIN 480.0f
#define CHARACTER_SHOT_SPEED_MAX 560.0f
#define CHARACTER_SPEED_MIN      190.0f
#define CHARACTER_SPEED_MAX      260.0f
#define CHARACTER_MAX_HP_MIN     3
#define CHARACTER_MAX_HP_MAX     9
#define CHARACTER_LUCK_MIN       0.0f
#define CHARACTER_LUCK_MAX       1.5f
/* hpCap non e' generato: SEMPRE derivato da maxHp (2*maxHp), poi clampato
 * qui dentro -- mai una settima manopola libera. 18 resta ben sotto la
 * guardia ASSOLUTA di motore (SCRIPT_ITEMS_MAX_HP_ABSOLUTE_MAX=24, src/
 * script/script_items.c): il margine fra 18 e 24 non serve al colpo firmato
 * (DEC-068, M6b-3) -- quella fetta comprime maxHp verso il basso (vedi
 * CHARACTER_SHOT_CAUTION_FRACTION sotto), non lo fa mai crescere oltre
 * CHARACTER_MAX_HP_MAX. Il margine resta un cuscinetto generico verso la
 * guardia assoluta, indipendente da questa fetta. */
#define CHARACTER_HP_CAP_MIN     6
#define CHARACTER_HP_CAP_MAX     18

/* M6b-3 (DEC-068), default PROPOSTO (open question 18 di characters.md,
 * RESTA aperta -- questo e' solo il numero scelto per renderla giocabile
 * subito, stile DEC-019, non un valore approvato dal design): un personaggio
 * col colpo firmato ha stats "piu' caute" comprimendo il tetto EFFETTIVO
 * (non la banda intera, che resta quella di sempre) verso la meta' cauta
 * della banda. Per damage/maxHp/luck (piu' alto = piu' forte) il massimo
 * effettivo scende a bandMin + FRACTION*(bandMax-bandMin); per fireDelay
 * (piu' BASSO = spara piu' veloce = piu' forte) e' il minimo effettivo che
 * sale a bandMax - FRACTION*(bandMax-bandMin) -- stesso principio, direzione
 * opposta perche' la banda e' invertita. shotSpeed e speed (movimento) NON
 * sono compressi: il colpo firmato paga il proprio vantaggio offensivo/
 * difensivo, non la mobilita' del personaggio. 0.6 lascia comunque il 60%
 * INFERIORE della banda intatto (un personaggio col colpo firmato puo'
 * ancora essere quasi-massimo su meta' della banda), mai un dimezzamento
 * secco: e' un budget, non una penalita' punitiva. */
#define CHARACTER_SHOT_CAUTION_FRACTION 0.6f

/* Il personaggio alternativo generato per-run, PRIMA di diventare una
 * CharacterDef vera (src/core/game_types.h): 'palette' resta testo hex
 * ("#rrggbb", stesso formato di GenItem.color in melting-gen) invece di una
 * Color raylib, per lo stesso motivo per cui questo header non include
 * raylib -- il chiamante lato gioco (RunContentLoadCharacterProposal) la
 * converte con la sua stessa ParseHexColor di sempre.
 *
 * M6b-3 (DEC-068): 'hasShot'/'signatureShot' sono il colpo firmato
 * OPZIONALE -- "a volte", mai una garanzia (KB). hasShot falso (lo zero-
 * default di un memset, come 'active' di ShotTypeDef) e' lo stato piu'
 * comune: nessun colpo firmato, nessuna compressione delle stats, un
 * personaggio alternativo come nella fetta precedente. 'signatureShot.active'
 * viene forzato a seguire 'hasShot' dentro CharacterGenDefClamp (mai letto
 * per conto suo prima del clamp): due bandiere che devono restare in
 * sincrono, non due fonti di verita' indipendenti. */
typedef struct CharacterGenDef {
    char name[CHARACTER_GEN_NAME_LEN];
    char blurb[CHARACTER_GEN_BLURB_LEN];
    float damage;
    float fireDelay;
    float shotSpeed;
    float speed;
    int maxHp;
    int hpCap;      /* derivato da maxHp, non generato: vedi CharacterGenDefClamp */
    float luck;
    char palette[8]; /* "#rrggbb" */
    bool hasShot;
    ShotTypeDef signatureShot;   /* significativo SOLO se hasShot; {0} altrimenti (vedi CharacterGenDefClamp) */
} CharacterGenDef;

/* Riporta ogni manopola numerica dentro la sua banda e deriva
 * hpCap = clamp(2*maxHp, CHARACTER_HP_CAP_MIN, CHARACTER_HP_CAP_MAX). Non
 * tocca name/blurb/palette (testo, validato/troncato a parte dal chiamante:
 * vedi tools/melting-gen/main.c e src/content/character_proposal.c).
 * Idempotente, come ShotTypeClamp/EnemyTypeClamp: applicarla due volte da'
 * lo stesso risultato -- e' cio' che rende sicuro chiamarla in ENTRAMBE le
 * reti (tool prima di scrivere, gioco alla lettura) senza mai stringere
 * ulteriormente un valore gia' in banda. NaN-safe (un JSON malfatto con un
 * numero non finito non deve mai passare indenne): vedi l'implementazione.
 *
 * M6b-3 (DEC-068): se def->hasShot e' vero, applica ANCHE la compressione
 * cauta (CHARACTER_SHOT_CAUTION_FRACTION, vedi sopra) a damage/maxHp/luck/
 * fireDelay PRIMA del clamp normale, e ribilancia def->signatureShot con
 * ShotTypeClamp/ShotTypeBalance COSI' COME SONO (src/core/shot_type.h,
 * riuso puro, nessuna modifica) -- se falso, azzera def->signatureShot per
 * intero (mai un residuo di un colpo scartato che sopravvive a meta': un
 * personaggio senza colpo firmato deve avere il campo ESATTAMENTE vuoto,
 * come i personaggi della rosa base). Un'unica funzione per entrambe le
 * reti: e' cio' che rende il budget IDENTICO e byte-deterministico su tool
 * e gioco (spec, punto (b)), non due implementazioni da tenere sincronizzate
 * a mano. */
void CharacterGenDefClamp(CharacterGenDef *def);

#endif
