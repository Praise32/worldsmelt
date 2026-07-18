#ifndef MELTING_RUN_CHARACTER_TYPE_H
#define MELTING_RUN_CHARACTER_TYPE_H

/* M6b-1 (DEC-014, prima fetta -- stats+palette+carta del personaggio
 * alternativo per-run; trait Lua DEC-037 e colpo firmato DEC-068 restano
 * gap espliciti per le fette successive, vedi characters.md).
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
 * bug futuro in una delle due reti).
 *
 * A differenza dei tipi di colpo/nemico, qui il modello non inventa un
 * vocabolario nuovo: i campi sono fissi (nome, blurb, sei manopole di
 * statistica, una palette), e le bande sono DEFAULT PROPOSTI stile DEC-019
 * (da playtest, vedi game-design-knowledge-base/docs/game-design/systems/
 * characters.md, blocco "Default proposti dall'implementazione" per il
 * personaggio generato) centrati sulla rosa curata (src/content/
 * character_roster.c): il personaggio generato puo' risultare piu' o meno
 * estremo di un personaggio base, ma sempre dentro limiti garantiti. */

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
 * script/script_items.c): margine dichiarato per il colpo firmato (DEC-068,
 * M6b-3), che nella KB e' descritto come "stats piu' caute" -- quel gap di
 * quanto restano caute e' esplicito in governance/open-questions.md, non
 * ancora un numero. */
#define CHARACTER_HP_CAP_MIN     6
#define CHARACTER_HP_CAP_MAX     18

/* Il personaggio alternativo generato per-run, PRIMA di diventare una
 * CharacterDef vera (src/core/game_types.h): 'palette' resta testo hex
 * ("#rrggbb", stesso formato di GenItem.color in melting-gen) invece di una
 * Color raylib, per lo stesso motivo per cui questo header non include
 * raylib -- il chiamante lato gioco (RunContentLoadCharacterProposal) la
 * converte con la sua stessa ParseHexColor di sempre. */
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
} CharacterGenDef;

/* Riporta ogni manopola numerica dentro la sua banda e deriva
 * hpCap = clamp(2*maxHp, CHARACTER_HP_CAP_MIN, CHARACTER_HP_CAP_MAX). Non
 * tocca name/blurb/palette (testo, validato/troncato a parte dal chiamante:
 * vedi tools/melting-gen/main.c e src/content/character_proposal.c).
 * Idempotente, come ShotTypeClamp/EnemyTypeClamp: applicarla due volte da'
 * lo stesso risultato -- e' cio' che rende sicuro chiamarla in ENTRAMBE le
 * reti (tool prima di scrivere, gioco alla lettura) senza mai stringere
 * ulteriormente un valore gia' in banda. NaN-safe (un JSON malfatto con un
 * numero non finito non deve mai passare indenne): vedi l'implementazione. */
void CharacterGenDefClamp(CharacterGenDef *def);

#endif
