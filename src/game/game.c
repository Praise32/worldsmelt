#include "game/game.h"

#include "content/character_roster.h"
#include "content/run_content.h"
#include "game/game_internal.h"
#include "script/script_items.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

void GameSetMessage(Game *game, const char *message)
{
    snprintf(game->message, sizeof(game->message), "%s", message);
    game->messageTimer = 3.2f;
}

/* M6a (DEC-030/033): come GamePlayerResetBaseStats sotto, ma parametrizzata
   sul personaggio APPLICATO -- 'character' NULL riproduce esattamente il
   comportamento storico pre-M6a (compreso hpCap 12, il tetto assoluto di
   sempre), cosi' MakeBaseGame/i test esistenti e ogni chiamante che non sa
   ancora nulla di personaggi non cambiano risultato di un bit. coins/bombs/
   keys/radius restano fissi indipendentemente dal personaggio (fanno parte
   della progressione della run, non dell'identita' del personaggio, vedi
   Player.md): solo le statistiche di combattimento/salute variano. */
void GamePlayerResetBaseStatsFor(Player *player, const CharacterDef *character)
{
    player->radius = 14.0f;
    player->coins = 3;
    player->bombs = 2;
    player->keys = 1;
    /* Catalizzatore di fusione (Flux): si parte SEMPRE senza (DEC-022, e' una
       risorsa rara che arriva da boss/arene o da un acquisto costoso), quindi
       la prima fusione di una run e' sempre qualcosa che il giocatore si e'
       guadagnato. Esplicito qui come coins/bombs/keys: "da dove parte una
       risorsa" si deve leggere in questa funzione e in nessun altro posto. */
    player->flux = 0;
    /* Slot funzionali di partenza (items-pools-and-rarity.md, "Slot"): 1
       attivo + 1 Innesto, come coins/bombs/keys sopra parte della
       progressione della run e non dell'identita' del personaggio -- nessun
       personaggio della rosa ne concede di piu' o di meno. Gli slot in piu'
       arrivano solo da oggetti/eventi rari, e valgono solo per la run
       (DEC-123). Esplicito qui anche se ItemActiveSlotCount/
       ItemGraftSlotCount trattano gia' lo zero come 1: "da dove parte" si
       deve leggere in questa funzione e in nessun altro posto, come per
       baseLuck. */
    player->activeSlotCount = 1;
    player->graftSlotCount = 1;
    player->activeSelected = 0;
    /* Valori di PARTENZA del sistema delle cache (spec, sezione 7): non
       vengono piu' assegnati direttamente ai campi "vivi" (damage,
       fireDelay, shotSpeed, shotRadius, speed, maxHp). ScriptItemsInit del
       chiamante li deriva chiamando ScriptItemsRecomputeStats con zero
       oggetti posseduti, che per costruzione produce esattamente questi
       stessi numeri (nessun cambiamento di comportamento per una run senza
       oggetti). */
    player->baseDamage = character ? character->baseDamage : 8.0f;
    player->baseFireDelay = character ? character->baseFireDelay : 0.23f;
    player->baseShotSpeed = character ? character->baseShotSpeed : 520.0f;
    player->baseShotRadius = character ? character->baseShotRadius : 5.0f;
    player->baseSpeed = character ? character->baseSpeed : 224.0f;
    player->baseMaxHp = character ? character->baseMaxHp : 6;
    /* Step C: la fortuna parte da zero (esplicita come le altre, perche' "da
       dove parte una statistica" si deve leggere qui e in nessun altro posto).
       M6a: un personaggio puo' spostare anche questo (Wayfinder parte
       fortunato, DEC-030). */
    player->baseLuck = character ? character->baseLuck : 0.0f;
    /* M6a (DEC-033): il tetto di salute BASE segue il personaggio; 12 resta
       il tetto STORICO quando nessun personaggio e' applicato (vedi il
       commento su Player.hpCap in core/game_types.h). */
    player->hpCap = character ? character->hpCap : 12;
    /* hp parte SEMPRE pieno al tetto di partenza del personaggio (baseMaxHp,
       non hpCap: un personaggio non inizia gia' "cresciuto"), esattamente
       come lo storico 6 di sempre quando non c'e' personaggio. */
    player->hp = player->baseMaxHp;
    /* M6b-3 (DEC-068): il colpo firmato del personaggio, se ne ha uno --
       vedi il commento su Player.characterShotType in core/game_types.h.
       'character' NULL o senza colpo firmato (la stragrande maggioranza dei
       casi: rosa base sempre, personaggio generato "spesso") azzera per
       intero, esattamente come ogni altro campo sopra quando non c'e' un
       personaggio applicato -- mai un residuo del personaggio/colpo
       PRECEDENTE che sopravvive a uno switch (vedi lo scenario 8d di
       GameFloorZeroTest, che esercita proprio questo switch per il trait
       Lua e ora anche per il colpo firmato). */
    if (character && character->signatureShot.active)
    {
        player->characterShotType = character->signatureShot;
        player->characterShotColor = character->palette;
    }
    else
    {
        memset(&player->characterShotType, 0, sizeof(player->characterShotType));
        player->characterShotColor = (Color){ 0, 0, 0, 0 };
    }
}

void GamePlayerResetBaseStats(Player *player)
{
    GamePlayerResetBaseStatsFor(player, NULL);
}

/* M6b-1 (DEC-014, prima fetta): vedi il commento su questa funzione in
   game_internal.h. */
const CharacterDef *GameResolveCharacterDef(const Game *game, int index)
{
    if (index == CHARACTER_COUNT)
        return game->generatedCharacterValid ? &game->generatedCharacter : NULL;
    if (index < 0 || index >= CHARACTER_COUNT) return NULL;
    return CharacterRosterGet(index);
}

int GameCharacterCardCount(const Game *game)
{
    return CHARACTER_COUNT + (game->generatedCharacterValid ? 1 : 0);
}

/* DEC-141: deriva il seed di gameplay ('rng') dal seed di run con uno
   splitmix64 (Steele/Lea/Flood) a costante di dominio propria ('GMPLAY'),
   cosi' lo stream di gameplay non e' mai lo stesso di quello di
   generazione anche quando entrambi partono dallo stesso runSeed (che
   RunContentLoad sotto riceve grezzo, senza mix: e' gia' il seed che
   melting-gen usa esternamente, nessun bisogno di separarlo ulteriormente
   da se stesso). Un solo giro del finalizzatore basta: qui serve un buon
   rimescolamento del seed iniziale, non un generatore completo (quello
   resta GameRngNext, invariato). */
static unsigned int GameplayRngSeedFromRunSeed(unsigned int runSeed)
{
    const unsigned long long domain = 0x474D504C41590001ULL;   /* 'GMPLAY' + costante di dominio */
    unsigned long long state = ((unsigned long long)runSeed ^ domain) + 0x9E3779B97F4A7C15ULL;
    unsigned long long z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z ^= (z >> 31);
    unsigned int mixed = (unsigned int)(z >> 32) ^ (unsigned int)z;
    return mixed ? mixed : 0xA341316Cu;   /* GameRngNext si autoripara da 0 comunque, ma un seed iniziale gia' non-zero e' piu' pulito */
}

void GameResetRunWithSeed(Game *game, unsigned int runSeed)
{
    GameUnloadAssets(game);
    memset(game, 0, sizeof(*game));
    game->runSeed = runSeed;
    game->rng = GameplayRngSeedFromRunSeed(runSeed);
    RunContentLoad(&game->content, runSeed);
    AssetsLoad(game);
    game->phase = PHASE_PLAY;
    GamePlayerResetBaseStats(&game->player);
    /* M6a: -1, MAI 0 (che il memset sopra scriverebbe da solo e che
       collide con "personaggio 0 scelto", vedi CharacterRosterGet) --
       questa funzione non sa nulla del Piano 0/della scelta del giocatore
       (mai passata come parametro, per non cambiare la firma storica di
       ogni chiamante esistente): resta "nessun personaggio applicato" per
       costruzione, coerente coi valori storici appena scritti sopra da
       GamePlayerResetBaseStats(NULL). Il case APP_FLOOR_ZERO in src/app/
       app.c (l'UNICO punto che sa quale personaggio e' stato scelto)
       sovrascrive questo campo SUBITO dopo la chiamata, quando davvero
       arriva da un attraversamento del Piano 0. */
    game->characterChosenIndex = -1;
    ScriptItemsInit(game, NULL);   /* nessun personaggio applicato per costruzione, vedi sopra */
    WorldStartFloor(game, 1);
}

void GameResetRun(Game *game)
{
    /* Wrapper storica: nessun seed di run scelto e' disponibile qui (l'Game
       provvisorio di avvio in AppRun, i binari *Test che non passano mai dal
       Piano 0) -- un valore orologio resta l'unica sorgente, esattamente
       come prima di DEC-141. Passa comunque da GameResetRunWithSeed (mai
       una propria copia della logica) cosi' anche questo cammino deriva
       'rng' con lo stesso splitmix64 a dominio invece di riusare il valore
       grezzo com'era storicamente: nessun test dipende dai bit esatti di un
       seed basato sull'orologio. */
    GameResetRunWithSeed(game, (unsigned int)time(NULL) ^ 0x514AACu);
}

void GameUpdate(Game *game, float dt, Vector2 mouseGame, bool mouseInsideGame)
{
    if (dt > 0.033f) dt = 0.033f;
    if (game->resetQueued)
    {
        /* Reset rapido R: GameResetRun azzera l'INTERO Game (compreso
           characterChosenIndex E generatedCharacter/generatedCharacterValid,
           M6b-1), quindi l'indice scelto va catturato PRIMA e riapplicato
           SUBITO dopo. Stessa regola di floorZeroExitCrossed in app.c -- la
           run continua col personaggio scelto, non ricade sul "nessun
           personaggio" storico che GameResetRun applicherebbe da sola.
           GameResetRun da sola NON cambia comportamento (resta storica:
           characterChosenIndex = -1). Se il personaggio scelto e' quello
           GENERATO (indice CHARACTER_COUNT), l'indice da solo non basta:
           serve una COPIA della sua CharacterDef presa PRIMA del memset,
           altrimenti generatedCharacterValid tornerebbe falso e
           GameResolveCharacterDef non troverebbe piu' nulla da applicare
           (stesso punto delicato del case APP_FLOOR_ZERO in app.c).
           DEC-141: stessa idea per 'runSeed' -- catturato PRIMA e riusato
           SUBITO dopo (GameResetRunWithSeed, non piu' GameResetRun) cosi'
           il reset rapido resta la STESSA run: stesso seed, quindi stessa
           sequenza di spawn/drop/combattimento, non una nuova random ad
           ogni pressione di R come prima del fix. */
        int chosenCharacter = game->characterChosenIndex;
        bool chosenIsGenerated = (chosenCharacter == CHARACTER_COUNT && game->generatedCharacterValid);
        CharacterDef savedGenerated = chosenIsGenerated ? game->generatedCharacter : (CharacterDef){ 0 };
        unsigned int savedRunSeed = game->runSeed;
        GameResetRunWithSeed(game, savedRunSeed);
        game->characterChosenIndex = chosenCharacter;
        if (chosenIsGenerated)
        {
            game->generatedCharacter = savedGenerated;
            game->generatedCharacterValid = true;
        }
        const CharacterDef *resolved = GameResolveCharacterDef(game, chosenCharacter);
        if (resolved)
        {
            GamePlayerResetBaseStatsFor(&game->player, resolved);
            ScriptItemsInit(game, resolved);
        }
    }
    if (game->messageTimer > 0.0f) game->messageTimer -= dt;

    if (game->phase == PHASE_GAME_OVER || game->phase == PHASE_WIN)
    {
        GameUpdateParticles(game, dt);
        return;
    }

    /* Rete di sicurezza del sistema delle cache (spec, sezione 7): consuma
       Game.statsDirty una volta per frame, PRIMA che CombatUpdatePlayer
       legga player.damage/fireDelay/shotSpeed/shotRadius/speed. In pratica
       CombatApplyItem la consuma gia' subito al momento del pickup (vedi
       combat.c): questa chiamata copre solo l'eventualita' che qualcos'altro
       in futuro sporchi la bandiera senza ricalcolare subito. */
    ScriptItemsProcessDirty(game);
    CombatUpdatePlayer(game, dt, mouseGame, mouseInsideGame);
    /* DEC-170: la telecamera e' stato di SIMULAZIONE, non di rendering --
       aggiornata qui, subito dopo l'unico passo che muove il giocatore, cosi'
       a parita' di passi simulati l'inquadratura e' identica (e un frame video
       che ne contiene due non la fa avanzare di piu' di uno che ne contiene
       uno). Per una stanza 1x1 e' un no-op: il bersaglio e' costante. */
    WorldUpdateCamera(game, dt);
    CombatUpdateEnemies(game, dt);
    CombatUpdateShots(game, dt);
    CombatUpdateBombs(game, dt);
    CombatUpdatePickups(game);
    GameUpdateParticles(game, dt);
    WorldCheckRoomClear(game);
}
