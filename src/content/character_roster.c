#include "content/character_roster.h"

/* M6a (DEC-030/033/049): rosa base FISSA e CURATA, mai generata (a
 * differenza del personaggio alternativo per-run, DEC-014/037, fuori scope
 * qui) -- coerente con AGENTS.md ("i dati curati vivono in un modulo
 * dedicato, la FORMA in core"). Valori DEFAULT PROPOSTI stile DEC-019 (da
 * validare col playtest, vedi docs/design/
 * systems/characters.md, blocco "Default proposti dall'implementazione", e
 * governance/open-questions.md punto 7): non sono ancora numeri approvati
 * dal design, sono un punto di partenza giocabile.
 *
 * Indice 0 (Wayfinder) e' apposta il PIU' VICINO allo storico pre-M6a
 * (baseDamage 8, baseFireDelay 0.23, baseShotSpeed 520, baseShotRadius 5,
 * baseMaxHp 6): e' il preselezionato d'ingresso nel Piano 0 (FloorZeroEnter,
 * src/world/floor_zero.c), quindi un giocatore che non tocca mai il
 * selettore continua a sentire una run il piu' possibile simile a quella di
 * prima di questa fase, anche se non piu' bit-per-bit identica (baseSpeed e
 * baseLuck sono leggermente diversi apposta: il personaggio deve sentirsi
 * un vero esploratore, non un placeholder muto).
 *
 * Nomi in inglese (DEC-052), niente termine riservato della nomenclatura
 * in-game (Smelting/Flux/Tempered/Ingots/Embers.., vedi governance/
 * glossary.md). Niente RNG: la rosa base e' `curato` per costruzione. */
static const CharacterDef kCharacterRoster[CHARACTER_COUNT] = {
    {
        .name = "Wayfinder",
        .role = "Explorer",
        .blurb = "Quick feet and a lucky streak: finds the edge in every room.",
        .baseDamage = 8.0f,
        .baseFireDelay = 0.23f,
        .baseShotSpeed = 520.0f,
        .baseShotRadius = 5.0f,
        .baseSpeed = 240.0f,
        .baseMaxHp = 6,
        .hpCap = 12,
        .baseLuck = 0.5f,
        .palette = (Color){ 64, 200, 168, 255 },   /* teal/verde */
    },
    {
        .name = "Ashblade",
        .role = "Offensive",
        .blurb = "Hits hard and breaks easy: glass with an edge.",
        .baseDamage = 10.0f,
        .baseFireDelay = 0.21f,
        .baseShotSpeed = 520.0f,
        .baseShotRadius = 5.0f,
        .baseSpeed = 230.0f,
        .baseMaxHp = 4,
        .hpCap = 8,
        .baseLuck = 0.0f,
        .palette = (Color){ 224, 96, 48, 255 },    /* rosso/arancio caldo */
    },
    {
        .name = "Bulwark",
        .role = "Defensive",
        .blurb = "Slow and unbothered: a wall of health that outlasts the room.",
        .baseDamage = 7.0f,
        .baseFireDelay = 0.26f,
        .baseShotSpeed = 500.0f,
        .baseShotRadius = 5.0f,
        .baseSpeed = 204.0f,
        .baseMaxHp = 8,
        .hpCap = 16,
        .baseLuck = 0.0f,
        .palette = (Color){ 70, 120, 210, 255 },   /* blu acciaio */
    },
};

const CharacterDef *CharacterRosterGet(int index)
{
    if (index < 0 || index >= CHARACTER_COUNT) index = 0;
    return &kCharacterRoster[index];
}
