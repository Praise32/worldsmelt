/* Test del runtime Lua del trait UNICO del personaggio generato (M6b-2,
   DEC-037, src/script/script_character.c). Stesso stile di
   src/tests/script_items_tests.c: un Game minimo sullo stack, nessuna
   finestra/atlas -- un'esecuzione reale delle API pubbliche vere, non una
   simulazione della logica.

   A differenza degli oggetti (il cui script Lua vive in Item.luaSource, una
   stringa in memoria), il trait del personaggio si carica SEMPRE da un file
   fisso su disco (generated/scripts/character_trait.lua, vedi il commento
   su SCRIPT_CHARACTER_TRAIT_PATH in script_character.c): questi test
   scrivono/rimuovono quel file per davvero, esattamente come fa il propose
   reale (tools/melting-gen/main.c) e come lo simula tests/fake-gen.sh per i
   test di integrazione del Piano 0 (src/tests/game_tests.c,
   GameFloorZeroTest scenari 8/8d/11). */

#include "tests/game_tests.h"

#include "content/character_roster.h"
#include "core/game_math.h"
#include "game/game_internal.h"
#include "script/script_character.h"
#include "script/script_items.h"
#include "script/script_sandbox.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifndef _WIN32

/* Stesso percorso LETTERALE di script_character.c (SCRIPT_CHARACTER_TRAIT_PATH,
   static li' dentro, non esposto: questi test lo duplicano invece di
   esportarlo, cosi' script_character.h resta senza un dettaglio interno che
   nessun chiamante vero deve mai conoscere -- il gioco lo legge SEMPRE per
   convenzione fissa, mai per parametro). Se cambia li', va cambiato anche
   qui: sono le due meta' della stessa convenzione, come gia' documentato
   per WriteItemLua/GenLuaLoadExisting in tools/melting-gen. */
#define TEST_TRAIT_PATH "generated/scripts/character_trait.lua"

static void WriteTraitFile(const char *source)
{
    mkdir("generated", 0755);           /* ignora EEXIST: la directory normalmente esiste gia' (Makefile, o un run precedente) */
    mkdir("generated/scripts", 0755);
    FILE *f = fopen(TEST_TRAIT_PATH, "w");
    if (!f) return;
    fputs(source, f);
    fclose(f);
}

static void RemoveTraitFile(void)
{
    remove(TEST_TRAIT_PATH);
}

/* Un Game minimo ma "vero": stessi valori di base di MakeBaseGame
   (script_items_tests.c) -- ScriptItemsInit deriva gia' player.damage/... dai
   base* con zero oggetti, esattamente come farebbe GameResetRun. */
static Game MakeCharacterTestGame(unsigned int seed, const CharacterDef *character)
{
    Game game;
    memset(&game, 0, sizeof(game));
    game.phase = PHASE_PLAY;
    game.rng = seed;
    game.player.baseDamage = 8.0f;
    game.player.baseFireDelay = 0.23f;
    game.player.baseShotSpeed = 520.0f;
    game.player.baseShotRadius = 5.0f;
    game.player.baseSpeed = 224.0f;
    game.player.baseMaxHp = 6;
    ScriptItemsInit(&game, character);
    return game;
}

/* Stesso helper di script_items_tests.c (TestAddItem): riproduce SOLO la
   parte di CombatApplyItem che serve qui (assegna lo slot, passa dalla
   stessa coppia ScriptItemsOnAcquire/ScriptItemsProcessDirty del gioco
   vero). Un oggetto senza luaSource usa solo il suo bonus "built-in"
   (slot/trait), niente Lua: basta a dare un secondo contributo NOTO da
   sommare a quello del trait. */
static void TestAddItem(Game *game, Item item)
{
    int idx = game->player.itemCount;
    if (idx >= MAX_ITEMS) idx = MAX_ITEMS - 1; else game->player.itemCount++;
    game->player.items[idx] = item;
    game->player.traits |= item.traits;
    ScriptItemsOnAcquire(game, idx);
    ScriptItemsProcessDirty(game);
}

static CharacterDef MakeGeneratedCharacterDef(const char *traitHook)
{
    CharacterDef def;
    memset(&def, 0, sizeof(def));
    snprintf(def.name, sizeof(def.name), "Test Forgeling");
    snprintf(def.role, sizeof(def.role), "FORGED THIS RUN");
    snprintf(def.blurb, sizeof(def.blurb), "A character built for this test.");
    def.baseDamage = 8.0f;
    def.baseFireDelay = 0.23f;
    def.baseShotSpeed = 520.0f;
    def.baseShotRadius = 5.0f;
    def.baseSpeed = 224.0f;
    def.baseMaxHp = 6;
    def.hpCap = 12;
    def.baseLuck = 0.0f;
    if (traitHook) snprintf(def.traitHook, sizeof(def.traitHook), "%s", traitHook);
    return def;
}

/* ============================================================
   Test A (ciclo di vita): nessun trait -> inattivo; un trait valido si
   carica alla selezione; Shutdown lo scarica per davvero (sandbox=NULL, non
   solo "disabilitata").
   ============================================================ */
static bool TestLifecycle(void)
{
    RemoveTraitFile();
    bool ok = true;

    /* Nessun personaggio (NULL): mai un tentativo di caricare alcunche'. */
    Game game = MakeCharacterTestGame(1u, NULL);
    bool inactiveNoCharacter = !ScriptCharacterHasActiveLua(&game);
    ok = ok && inactiveNoCharacter;
    printf("  [A] nessun personaggio -> trait inattivo: %s\n", inactiveNoCharacter ? "si" : "NO");

    /* Personaggio della rosa curata (traitHook vuoto per costruzione): idem,
       anche se generated/scripts/character_trait.lua esistesse ancora da un
       run precedente (qui non esiste: RemoveTraitFile sopra), la rosa
       curata non tenta MAI di caricarlo. */
    WriteTraitFile("function on_evaluate(stats) stats.damage = stats.damage + 2 end\n");
    CharacterDef curated = *CharacterRosterGet(0);
    ScriptItemsInit(&game, &curated);
    bool inactiveCurated = !ScriptCharacterHasActiveLua(&game);
    ok = ok && inactiveCurated;
    printf("  [A] personaggio curato (traitHook vuoto) col file presente -> comunque inattivo: %s\n", inactiveCurated ? "si" : "NO");

    /* Personaggio generato con traitHook impostato: si carica per davvero. */
    CharacterDef generated = MakeGeneratedCharacterDef("on_evaluate");
    ScriptItemsInit(&game, &generated);
    bool activeGenerated = ScriptCharacterHasActiveLua(&game);
    ok = ok && activeGenerated;
    printf("  [A] personaggio generato con traitHook -> trait attivo: %s\n", activeGenerated ? "si" : "NO");

    /* Shutdown: sandbox distrutta per davvero, non solo disabilitata. */
    ScriptItemsShutdown(&game);
    bool inactiveAfterShutdown = !ScriptCharacterHasActiveLua(&game) && game.characterTrait.sandbox == NULL;
    ok = ok && inactiveAfterShutdown;
    printf("  [A] ScriptItemsShutdown -> sandbox NULL: %s\n", inactiveAfterShutdown ? "si" : "NO");

    RemoveTraitFile();
    return ok;
}

/* ============================================================
   Test B (ordine del ricalcolo, valori NOTI): base -> trait -> oggetti ->
   clamp. Trait: +2 danno fisso (on_evaluate). Oggetto (SLOT_HAND, nessun
   Lua): +1 danno "built-in" (stesso bonus storico di ScriptItemsApplyBuiltin,
   vedi script_items.c). Atteso: 8 (base) + 2 (trait) + 1 (oggetto) = 11,
   ben dentro banda [0.5,200] -- nessuna curva dei rendimenti decrescenti
   scatta sotto 2*base=16.
   ============================================================ */
static bool TestTraitBeforeItemsKnownValues(void)
{
    RemoveTraitFile();
    WriteTraitFile("function on_evaluate(stats) stats.damage = stats.damage + 2 end\n");

    CharacterDef generated = MakeGeneratedCharacterDef("on_evaluate");
    Game game = MakeCharacterTestGame(7u, &generated);

    bool traitOnlyOk = fabsf(game.player.damage - 10.0f) < 0.01f;   /* 8 + 2, zero oggetti */
    printf("  [B] trait da solo (0 oggetti): damage=%.2f (atteso 10.00 = 8+2): %s\n",
           game.player.damage, traitOnlyOk ? "si" : "NO");

    Item item = { 0 };
    item.active = true;
    snprintf(item.name, sizeof(item.name), "Test Hand Item");
    item.slot = SLOT_HAND;   /* ScriptItemsApplyBuiltin: SLOT_HAND -> +1.0f damage, nessun Lua */
    TestAddItem(&game, item);

    bool combinedOk = fabsf(game.player.damage - 11.0f) < 0.01f;   /* 8 + 2 (trait) + 1 (oggetto) */
    printf("  [B] trait + oggetto: damage=%.2f (atteso 11.00 = 8+2+1): %s\n",
           game.player.damage, combinedOk ? "si" : "NO");

    /* Ricalcolo esplicito (idempotenza, come il sistema delle cache
       richiede ovunque): lo stesso risultato, mai una deriva. */
    ScriptItemsRecomputeStats(&game);
    bool idempotent = fabsf(game.player.damage - 11.0f) < 0.01f;
    printf("  [B] ricalcolo ripetuto: damage=%.2f (idempotente): %s\n", game.player.damage, idempotent ? "si" : "NO");

    ScriptItemsShutdown(&game);
    RemoveTraitFile();
    return traitOnlyOk && combinedOk && idempotent;
}

/* ============================================================
   Test C (sopravvivenza a un reset/una riselezione): riapplicare LO STESSO
   personaggio generato (stesso schema di GameResetRun + il case
   APP_FLOOR_ZERO/GameUpdate in src/app/app.c e src/game/game.c: cattura la
   CharacterDef PRIMA, la riapplica DOPO) lascia il trait attivo con lo
   stesso valore -- nessuna deriva, nessun leak di sandbox (Shutdown SEMPRE
   chiamato prima di un nuovo Load, mai due sandbox vive per lo stesso
   slot). Vedi anche GameFloorZeroTest scenario 8d (src/tests/game_tests.c)
   per la prova equivalente al livello dell'app intera, col reset rapido
   VERO.
   ============================================================ */
static bool TestReloadSurvivesReapplication(void)
{
    RemoveTraitFile();
    WriteTraitFile("function on_evaluate(stats) stats.max_hp = stats.max_hp + 1 end\n");

    CharacterDef generated = MakeGeneratedCharacterDef("on_evaluate");
    Game game = MakeCharacterTestGame(9u, &generated);
    bool firstLoadOk = ScriptCharacterHasActiveLua(&game) && game.player.maxHp == 7;
    printf("  [C] primo caricamento: attivo=%s maxHp=%d (atteso 7=6+1)\n",
           ScriptCharacterHasActiveLua(&game) ? "si" : "no", game.player.maxHp);

    /* Simula un GameResetRun/reset rapido: la def catturata PRIMA (qui e'
       la stessa 'generated', copiata per valore) viene riapplicata DOPO. */
    ScriptItemsInit(&game, &generated);
    bool reloadOk = ScriptCharacterHasActiveLua(&game) && game.player.maxHp == 7;
    printf("  [C] riapplicazione (reset survival): attivo=%s maxHp=%d (atteso 7)\n",
           ScriptCharacterHasActiveLua(&game) ? "si" : "no", game.player.maxHp);

    /* Switch verso un personaggio SENZA trait e ritorno: nessun leak (un
       solo slot, sempre Shutdown-prima-di-Load, vedi
       ScriptCharacterSetActive), stato coerente a ogni passo. */
    CharacterDef curated = *CharacterRosterGet(0);
    ScriptItemsInit(&game, &curated);
    bool unloadedOk = !ScriptCharacterHasActiveLua(&game);
    ScriptItemsInit(&game, &generated);
    bool reloadedAgainOk = ScriptCharacterHasActiveLua(&game) && game.player.maxHp == 7;
    printf("  [C] switch generato->base->generato: scaricato=%s ricaricato=%s maxHp=%d\n",
           unloadedOk ? "si" : "no", reloadedAgainOk ? "si" : "no", game.player.maxHp);

    ScriptItemsShutdown(&game);
    RemoveTraitFile();
    return firstLoadOk && reloadOk && unloadedOk && reloadedAgainOk;
}

/* ============================================================
   Test D (fallimento di compilazione -> inattivo, mai un crash): un file
   presente ma con una sintassi Lua non valida (lo stesso scenario anomalo
   "rotto" di tests/fake-gen.sh FAKE_GEN_CHARACTER_LUA_MODE=broken).
   ============================================================ */
static bool TestBrokenTraitStaysInactive(void)
{
    RemoveTraitFile();
    WriteTraitFile("function on_evaluate(stats\n  stats.max_hp = stats.max_hp + 1\nend\n");   /* parentesi non chiusa */

    CharacterDef generated = MakeGeneratedCharacterDef("on_evaluate");
    Game game = MakeCharacterTestGame(13u, &generated);

    bool inactive = !ScriptCharacterHasActiveLua(&game);
    bool baseHpUnchanged = game.player.maxHp == 6;   /* nessun +1: il trait non e' mai partito */
    printf("  [D] script rotto: attivo=%s maxHp=%d (atteso inattivo, maxHp=6)\n",
           ScriptCharacterHasActiveLua(&game) ? "si" : "no", game.player.maxHp);

    /* Un ricalcolo esplicito su un trait rotto non deve crashare ne'
       cambiare nulla. */
    ScriptItemsRecomputeStats(&game);
    bool stillOk = game.player.maxHp == 6 && !ScriptCharacterHasActiveLua(&game);

    ScriptItemsShutdown(&game);
    RemoveTraitFile();
    return inactive && baseHpUnchanged && stillOk;
}

/* ============================================================
   Test E (file assente ma traitHook impostato, caso anomalo esplicito
   della spec M6b-2 -- "lua":true col file sparito): mai un crash, trait
   inattivo.
   ============================================================ */
static bool TestMissingTraitFileStaysInactive(void)
{
    RemoveTraitFile();   /* nessun file, apposta */

    CharacterDef generated = MakeGeneratedCharacterDef("on_evaluate");
    Game game = MakeCharacterTestGame(21u, &generated);

    bool inactive = !ScriptCharacterHasActiveLua(&game);
    bool baseHpUnchanged = game.player.maxHp == 6;
    printf("  [E] file assente: attivo=%s maxHp=%d (atteso inattivo, maxHp=6)\n",
           ScriptCharacterHasActiveLua(&game) ? "si" : "no", game.player.maxHp);

    ScriptItemsShutdown(&game);
    return inactive && baseHpUnchanged;
}

bool ScriptCharacterSelfTest(void)
{
    struct { const char *label; bool (*fn)(void); } tests[] = {
        { "A (ciclo di vita: nessun trait/curato/generato/shutdown)", TestLifecycle },
        { "B (ordine del ricalcolo: base -> trait -> oggetti, valori noti)", TestTraitBeforeItemsKnownValues },
        { "C (sopravvivenza a un reset/una riselezione, switch senza leak)", TestReloadSurvivesReapplication },
        { "D (script rotto -> inattivo, mai un crash)", TestBrokenTraitStaysInactive },
        { "E (file assente, 'lua':true anomalo -> inattivo, mai un crash)", TestMissingTraitFileStaysInactive },
    };
    bool allOk = true;
    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        printf("-- test %s --\n", tests[i].label);
        bool ok = tests[i].fn();
        if (!ok) { fprintf(stderr, "  FALLITO: %s\n", tests[i].label); allOk = false; }
    }
    return allOk;
}

#else

bool ScriptCharacterSelfTest(void)
{
    return true;
}

#endif
