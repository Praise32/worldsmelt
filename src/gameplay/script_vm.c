#include "gameplay/script_vm.h"

#include "core/game_math.h"
#include "game/game_internal.h"
#include "gameplay/item_traits.h"
#include "script/script_items.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *NextCsvField(char **cursor)
{
    if (!cursor || !*cursor) return NULL;
    char *start = *cursor;
    char *comma = strchr(start, ',');
    if (comma)
    {
        *comma = '\0';
        *cursor = comma + 1;
    }
    else *cursor = start + strlen(start);
    return start;
}

static ScriptTrigger ScriptTriggerFromText(const char *text)
{
    return (text && strstr(text, "on_hit")) ? SCRIPT_ON_HIT : SCRIPT_ON_FIRE;
}

static ScriptOpKind ScriptOpFromText(const char *text)
{
    if (!text) return SCRIPT_OP_NONE;
    if (strstr(text, "burst")) return SCRIPT_OP_BURST;
    if (strstr(text, "projectile")) return SCRIPT_OP_PROJECTILE;
    if (strstr(text, "area")) return SCRIPT_OP_AREA;
    if (strstr(text, "heal")) return SCRIPT_OP_HEAL;
    return SCRIPT_OP_NONE;
}

static void ScriptVmDamageArea(Game *game, Vector2 pos, float radius, float damage, unsigned int traits, Color color)
{
    EntitiesAddParticle(game, pos, color, 28);
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game->enemies[i];
        if (!e->active) continue;
        float r = radius + e->radius;
        if (GameMathLengthSquared(GameMathSubtract(e->pos, pos)) < r*r) CombatDamageEnemy(game, e, damage, traits);
    }
}

static int ExecuteScriptSegment(Game *game, ScriptTrigger trigger, const Item *item, const char *segment, Vector2 pos, Vector2 dir, float damage, unsigned int traits, int scriptDepth)
{
    char temp[96];
    snprintf(temp, sizeof(temp), "%s", segment);
    char *colon = strchr(temp, ':');
    if (!colon) return 0;
    *colon = '\0';
    if (ScriptTriggerFromText(temp) != trigger) return 0;

    char *cursor = colon + 1;
    char *opText = NextCsvField(&cursor);
    char *aText = NextCsvField(&cursor);
    char *bText = NextCsvField(&cursor);
    char *traitText = NextCsvField(&cursor);
    ScriptOpKind op = ScriptOpFromText(opText);
    unsigned int extraTraits = traitText ? ItemTraitsFromText(traitText) : 0;
    if (traitText && strstr(traitText, "none")) extraTraits = 0;

    if (op == SCRIPT_OP_BURST && trigger == SCRIPT_ON_FIRE)
    {
        int count = GameMathClampInt(atoi(aText ? aText : "2"), 1, 6);
        float spread = GameMathClampFloat((float)atof(bText ? bText : "0.25"), 0.05f, 1.20f);
        float baseAngle = atan2f(dir.y, dir.x);
        for (int i = 0; i < count; i++)
        {
            float t = (count == 1) ? 0.0f : ((float)i/(float)(count - 1) - 0.5f);
            float a = baseAngle + t*spread;
            Shot *spawned = EntitiesAddShot(game, true, pos, (Vector2){ cosf(a), sinf(a) }, game->player.shotSpeed*0.88f, damage*0.42f, game->player.shotRadius, traits | extraTraits, item->color);
            if (spawned) spawned->scriptDepth = scriptDepth + 1;
        }
        return 1;
    }
    if (op == SCRIPT_OP_PROJECTILE && trigger == SCRIPT_ON_HIT && scriptDepth < 1)
    {
        int count = GameMathClampInt(atoi(aText ? aText : "1"), 1, 6);
        float speed = GameMathClampFloat((float)atof(bText ? bText : "280"), 120.0f, 720.0f);
        for (int i = 0; i < count; i++)
        {
            float a = (float)i*PI_F*2.0f/(float)count + GameRngFloat(&game->rng, -0.20f, 0.20f);
            Shot *spawned = EntitiesAddShot(game, true, pos, (Vector2){ cosf(a), sinf(a) }, speed, damage*0.38f, 4.0f, (traits | extraTraits) & ~(unsigned int)TRAIT_SPLIT, item->color);
            if (spawned) spawned->scriptDepth = scriptDepth + 1;
        }
        return 1;
    }
    if (op == SCRIPT_OP_AREA && trigger == SCRIPT_ON_HIT)
    {
        float radius = GameMathClampFloat((float)atof(aText ? aText : "48"), 18.0f, 96.0f);
        float scale = GameMathClampFloat((float)atof(bText ? bText : "0.35"), 0.05f, 1.15f);
        ScriptVmDamageArea(game, pos, radius, damage*scale, extraTraits, item->color);
        return 1;
    }
    if (op == SCRIPT_OP_HEAL && trigger == SCRIPT_ON_HIT)
    {
        int chance = GameMathClampInt(atoi(aText ? aText : "12"), 0, 60);
        int amount = GameMathClampInt(atoi(bText ? bText : "1"), 1, 2);
        if (GameRngRange(&game->rng, 0, 99) < chance)
        {
            game->player.hp = GameMathClampInt(game->player.hp + amount, 0, game->player.maxHp);
            EntitiesAddParticle(game, game->player.pos, item->color, 12);
        }
        return 1;
    }
    return 0;
}

void ScriptVmExecutePlayer(Game *game, ScriptTrigger trigger, Vector2 pos, Vector2 dir, float damage, unsigned int traits, int scriptDepth)
{
    int executed = 0;
    for (int i = 0; i < game->player.itemCount && executed < 8; i++)
    {
        /* Un oggetto con Lua attualmente utilizzabile (fase 3a-L2, vedi
           src/script/script_items.h) gestisce se' stesso per intero:
           ScriptItemsOnFire/OnHit lo hanno gia' chiamato altrove (vedi
           combat.c), e questa mini-VM lo salta per non eseguirlo due volte.
           Appena lo script viene disabilitato (patto di sicurezza, spec
           sezione 9) questa condizione torna falsa dal frame successivo e
           l'oggetto ripiega qui sotto da solo, senza alcuno switch esplicito
           da scrivere. */
        if (ScriptItemsHasActiveLua(game, i)) continue;
        const Item *item = &game->player.items[i];
        const char *cursor = item->script;
        while (cursor && *cursor && executed < 8)
        {
            char segment[96];
            int len = 0;
            while (cursor[len] && cursor[len] != '|' && len < (int)sizeof(segment) - 1)
            {
                segment[len] = cursor[len];
                len++;
            }
            segment[len] = '\0';
            if (len > 0) executed += ExecuteScriptSegment(game, trigger, item, segment, pos, dir, damage, traits, scriptDepth);
            cursor = cursor[len] == '|' ? cursor + len + 1 : cursor + len;
        }
    }
}
