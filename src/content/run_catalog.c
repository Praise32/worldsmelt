#include "content/run_catalog.h"

#include "gameplay/item_traits.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Percorso del catalogo per i test: se settato, overridea il percorso di default "catalog".
   Usato da RunCatalogAggregateFromPath per isolare il catalogo del test da quello reale. */
static const char *g_testCatalogPath = NULL;

/* Stesso schema "strstr fino a fine riga" di ReadManifestValue in
   src/content/run_content.c e src/content/character_proposal.c: una copia
   PRIVATA per modulo e' la convenzione gia' in uso nel progetto (vedi il
   commento su ParseHexColor in character_proposal.c -- "moduli diversi,
   ognuno coi propri file da leggere"), non una funzione condivisa. */
static void ReadManifestValue(const char *text, const char *key, char *out, int outSize)
{
    if (!text || !key || !out || outSize <= 0) return;
    const char *start = strstr(text, key);
    if (!start) return;
    start += strlen(key);
    int i = 0;
    while (start[i] && start[i] != '\r' && start[i] != '\n' && i < outSize - 1)
    {
        out[i] = start[i];
        i++;
    }
    out[i] = '\0';
}

/* M8 (DEC-045, RunCatalogAggregate sotto): come ReadManifestValue, ma la
   chiave e' ANCORATA all'inizio di una riga (un '\n' subito prima) invece di
   un semplice strstr ovunque nel testo. Necessario perche' alcune chiavi di
   questo formato sono la suffissatura ESATTA di un'altra -- "character.name="
   e "character.active=" sono entrambe un suffisso letterale di
   "shot.character.name="/"shot.character.active=" (vedi il blocco
   shot.character.* scritto da RunCatalogWriteRun sopra) -- un semplice
   strstr(text, "character.name=") troverebbe la PRIMA occorrenza ovunque
   compaia, cioe' dentro "shot.character.name=", non la riga "character.name="
   vera piu' avanti nel file. Ogni riga scritta da RunCatalogWriteRun e'
   sempre preceduta da un '\n' (compresa la seconda riga del file, "seed=":
   la primissima riga "catalogSchema=1" e' l'UNICA eccezione, letta a parte
   con ReadManifestValue prima che serva quest'ancora), quindi "\n"+chiave e'
   sempre corretto per tutto cio' che RunCatalogAggregate legge dopo lo
   schema. 'key' resta entro 63 char (il piu' lungo qui e'
   "character.signatureShot.pierceBonus=", ampio margine). */
static void ReadCatalogLineValue(const char *text, const char *key, char *out, int outSize)
{
    char anchored[80];
    snprintf(anchored, sizeof(anchored), "\n%s", key);
    ReadManifestValue(text, anchored, out, outSize);
}

/* Direzione OPPOSTA di SlotFromText/ItemKindFromText/RarityFromText
   (src/content/run_content.c): quel modulo legge un manifest, questo ne
   scrive uno nuovo. Stessi identici testi -- se cambiano LI', vanno
   cambiati anche QUI (stessa duplicazione dichiarata di ParseHexColor). */
static const char *SlotName(ItemSlot slot)
{
    switch (slot)
    {
        case SLOT_EYES: return "eyes";
        case SLOT_HAND: return "hand";
        case SLOT_BACK: return "back";
        case SLOT_BODY: return "body";
        case SLOT_AURA: return "aura";
        case SLOT_HAT: default: return "hat";
    }
}

/* Tassonomia a 4 categorie. "passive" e' il nome NUOVO della categoria che
   fino a questa fase il catalogo scriveva "active" (l'enum si chiamava cosi'
   ma significava passivo): i record gia' su disco restano leggibili senza
   conversioni, perche' ItemKindFromText (src/content/run_content.c) mappa la
   parola storica "active" su ITEM_PASSIVE quando l'oggetto non dichiara ne'
   cariche ne' cooldown -- e nessun record vecchio le dichiara. */
static const char *KindName(ItemKind kind)
{
    switch (kind)
    {
        case ITEM_STATUP: return "statup";
        case ITEM_ACTIVE: return "active";
        case ITEM_GRAFT: return "graft";
        case ITEM_PASSIVE: default: return "passive";
    }
}

static const char *RarityName(Rarity rarity)
{
    switch (rarity)
    {
        case RARITY_UNCOMMON: return "uncommon";
        case RARITY_RARE: return "rare";
        case RARITY_LEGENDARY: return "legendary";
        case RARITY_COMMON: default: return "common";
    }
}

/* Un valore di catalogo e' SEMPRE una riga sola (lo stesso schema
   chiave=valore/fino-a-fine-riga di ogni manifest del progetto): il sorgente
   Lua di un oggetto o del trait del personaggio non lo e' quasi mai. Questa
   funzione scrive il VALORE (mai la chiave: il chiamante l'ha gia' scritta
   con un fprintf ordinario) con '\\' raddoppiato e ogni newline sostituito
   dal letterale a due caratteri "\n" -- CR normalizzato via (rimosso, come
   ogni altro punto del progetto che tocca testo multipiattaforma). Reversibile
   con RunCatalogUnescapeText (run_catalog.h): il campo resta rileggibile da
   una futura riconvalida (DEC-069), non solo scritto e dimenticato. */
static void WriteEscapedValue(FILE *f, const char *text)
{
    for (const unsigned char *p = (const unsigned char *)text; *p; p++)
    {
        if (*p == '\\') fputs("\\\\", f);
        else if (*p == '\n') fputs("\\n", f);
        else if (*p == '\r') continue;
        else fputc((int)*p, f);
    }
    fputc('\n', f);
}

void RunCatalogUnescapeText(const char *escaped, char *out, int outSize)
{
    if (!out || outSize <= 0) return;
    out[0] = '\0';
    if (!escaped) return;
    int w = 0;
    for (const char *p = escaped; *p && w < outSize - 1; p++)
    {
        if (p[0] == '\\' && p[1] == 'n') { out[w++] = '\n'; p++; }
        else if (p[0] == '\\' && p[1] == '\\') { out[w++] = '\\'; p++; }
        else out[w++] = *p;
    }
    out[w] = '\0';
}

/* Il tema SCELTO della run (distinto dai floorN.theme dei singoli piani,
   spec M7 punto 2: "i floorN.theme dei piani raggiunti (+ il tema scelto
   della run)"): rilegge generated/chosen_theme.txt, lo STESSO file che
   AppWriteChosenThemeFile (src/app/app.c) scrive alla conferma nel Piano 0 e
   che sopravvive per tutta la run (mai riscritto ne' cancellato da nessun
   altro punto). Formato "<name> -- <blurb>\n", una riga sola -- se il
   separatore " -- " non c'e' (file forgiato a mano, o assente) ritorna
   false: nessuna sezione "world.*" nel record, mai un valore a meta'. */
static bool ReadChosenTheme(char *name, int nameSize, char *blurb, int blurbSize)
{
    char *text = LoadFileText("generated/chosen_theme.txt");
    if (!text) return false;
    char *sep = strstr(text, " -- ");
    bool ok = sep != NULL;
    if (ok)
    {
        int nameLen = (int)(sep - text);
        if (nameLen >= nameSize) nameLen = nameSize - 1;
        memcpy(name, text, (size_t)nameLen);
        name[nameLen] = '\0';

        const char *blurbStart = sep + 4;   /* dopo " -- " */
        int i = 0;
        while (blurbStart[i] && blurbStart[i] != '\r' && blurbStart[i] != '\n' && i < blurbSize - 1)
        {
            blurb[i] = blurbStart[i];
            i++;
        }
        blurb[i] = '\0';
    }
    UnloadFileText(text);
    return ok;
}

/* Il sorgente Lua del trait del personaggio generato -- STESSO percorso
   fisso di script_character.c (SCRIPT_CHARACTER_TRAIT_PATH, non esposto:
   questo modulo lo duplica invece di importarlo, stessa convenzione gia' in
   uso in src/tests/script_character_tests.c). Chiamata SOLO quando
   CharacterDef.traitHook non e' vuoto (il chiamante l'ha gia' verificato):
   un file assente/illeggibile a quel punto e' il caso anomalo esplicito
   della spec M6b-2 ("lua":true col file sparito) -- ritorna false, niente
   riga "character.traitLua=" nel record, mai un crash o un valore a meta'. */
static bool ReadCharacterTraitLua(char *out, int outSize)
{
    char *text = LoadFileText("generated/scripts/character_trait.lua");
    if (!text) return false;
    snprintf(out, (size_t)outSize, "%s", text);
    UnloadFileText(text);
    return true;
}

/* Il prossimo progressivo del nome file (spec M7 punto 1: "niente timestamp
   da orologio... un progressivo che scandisce i file esistenti va bene").
   Scandisce catalog/ per il "run-*.txt" col suffisso "-<N>.txt" piu' alto e
   ritorna N+1 (1 se la cartella e' vuota o non esiste ancora): GLOBALE alla
   cartella, non per seed/esito/piano -- due run con lo stesso seed ed esito
   restano comunque due file distinti. strrchr(base, '-') prende l'ULTIMO
   trattino del nome (subito prima di ".txt" nel pattern
   "run-<seed>-<esito>-p<piano>-<progressivo>.txt"): robusto perche' nessuno
   dei tre esiti (RUN_CATALOG_OUTCOME_*) contiene un trattino. Un file
   estraneo in catalog/ (senza trattino, o con un suffisso non numerico)
   contribuisce 0 al massimo, mai un crash. Usa il percorso di test se settato. */
static int NextProgressive(void)
{
    const char *catalogPath = g_testCatalogPath ? g_testCatalogPath : "catalog";
    if (!DirectoryExists(catalogPath)) return 1;
    FilePathList files = LoadDirectoryFilesEx(catalogPath, ".txt", false);
    int maxN = 0;
    for (unsigned int i = 0; i < files.count; i++)
    {
        const char *base = GetFileName(files.paths[i]);
        const char *dash = strrchr(base, '-');
        if (!dash) continue;
        int n = atoi(dash + 1);
        if (n > maxN) maxN = n;
    }
    UnloadDirectoryFiles(files);
    return maxN + 1;
}

int RunCatalogWriteRun(const Game *game, unsigned int seed, const char *outcome)
{
    if (!game || !outcome) return 0;

    /* Guardia principale (spec M7 punto 2, default v1): SOLO le run con
       contenuto DAVVERO generato entrano nel catalogo. 'source=' e' riletto
       QUI da generated/current_run.txt con ReadManifestValue -- MAI da
       provenance.txt (istruzione esplicita della spec): provenance.txt e'
       un file diagnostico per il debug della generazione (hash dei prompt,
       modelli usati), current_run.txt e' il manifest che il gioco stesso
       carica per giocare, la fonte di verita' su "questa run ha contenuto
       generato o no". Nessun manifest sul disco (run senza --generate mai
       partita, o cancellato a mano) e source=fallback ricadono entrambi su
       "niente da registrare", stesso trattamento. */
    char *manifest = LoadFileText("generated/current_run.txt");
    if (!manifest) return 0;
    char source[96];
    source[0] = '\0';
    ReadManifestValue(manifest, "source=", source, sizeof(source));
    UnloadFileText(manifest);
    if (!source[0] || strcmp(source, "fallback") == 0) return 0;

    /* Usa il percorso del catalogo di test se settato (isola il test dal
       catalogo reale), altrimenti usa il catalogo di produzione standard.
       Stesso pattern usato da RunCatalogAggregate per le letture. */
    const char *catalogPath = g_testCatalogPath ? g_testCatalogPath : "catalog";

    if (!DirectoryExists(catalogPath) && MakeDirectory(catalogPath) != 0)
    {
        fprintf(stderr, "RunCatalog: impossibile creare %s, catalogo non aggiornato per questa run\n", catalogPath);
        return 0;
    }

    int progressive = NextProgressive();
    char finalPath[192];
    char tmpPath[208];
    snprintf(finalPath, sizeof(finalPath), "%s/run-%u-%s-p%d-%d.txt", catalogPath, seed, outcome, game->floor, progressive);
    snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", finalPath);

    FILE *f = fopen(tmpPath, "w");
    if (!f)
    {
        fprintf(stderr, "RunCatalog: impossibile scrivere %s, catalogo non aggiornato per questa run\n", tmpPath);
        return 0;
    }

    int records = 0;

    /* Header (spec M7 punto 1): schema, seed, source, esito, piano raggiunto.
       Nessuna riga di versione: il progetto non ha ancora una costante di
       versione di gioco (vedi il vincolo esplicito della spec, "non
       inventare un sistema di versioni ora"). */
    fprintf(f, "catalogSchema=1\n");
    fprintf(f, "seed=%u\n", seed);
    fprintf(f, "source=%s\n", source);
    fprintf(f, "outcome=%s\n", outcome);
    fprintf(f, "floorReached=%d\n", game->floor);

    /* Il tema SCELTO della run (world), distinto dai floorN.theme sotto. */
    char worldName[48];
    char worldBlurb[200];
    if (ReadChosenTheme(worldName, sizeof(worldName), worldBlurb, sizeof(worldBlurb)))
    {
        fprintf(f, "world.name=%s\n", worldName);
        fprintf(f, "world.blurb=%s\n", worldBlurb);
        records++;
    }

    /* Temi/mondi e layout stanza dei piani RAGGIUNTI (spec M7 punto 2):
       game->floor e' 1-based e resta dentro [1, FLOOR_COUNT] per costruzione
       (WorldStartFloor/combat.c, mai chiamato da FloorZero -- il chiamante
       AppWriteRunCatalog gia' rifiuta floor<1), quindi i piani 1..floor sono
       esattamente quelli in cui il giocatore e' davvero entrato. */
    int floorCount = game->floor;
    if (floorCount > FLOOR_COUNT) floorCount = FLOOR_COUNT;
    fprintf(f, "floor.count=%d\n", floorCount);
    for (int i = 0; i < floorCount; i++)
    {
        const FloorContent *fc = &game->content.floors[i];
        int n = i + 1;

        fprintf(f, "floor%d.theme.name=%s\n", n, fc->theme.name);
        fprintf(f, "floor%d.theme.style=%s\n", n, fc->theme.style);
        fprintf(f, "floor%d.theme.bossName=%s\n", n, fc->theme.bossName);
        fprintf(f, "floor%d.theme.bg=#%02X%02X%02X\n", n, fc->theme.bg.r, fc->theme.bg.g, fc->theme.bg.b);
        fprintf(f, "floor%d.theme.floor=#%02X%02X%02X\n", n, fc->theme.floor.r, fc->theme.floor.g, fc->theme.floor.b);
        fprintf(f, "floor%d.theme.wall=#%02X%02X%02X\n", n, fc->theme.wall.r, fc->theme.wall.g, fc->theme.wall.b);
        fprintf(f, "floor%d.theme.accent=#%02X%02X%02X\n", n, fc->theme.accent.r, fc->theme.accent.g, fc->theme.accent.b);
        fprintf(f, "floor%d.theme.accent2=#%02X%02X%02X\n", n, fc->theme.accent2.r, fc->theme.accent2.g, fc->theme.accent2.b);
        fprintf(f, "floor%d.theme.enemyColor=#%02X%02X%02X\n", n, fc->theme.enemy.r, fc->theme.enemy.g, fc->theme.enemy.b);
        fprintf(f, "floor%d.theme.bossColor=#%02X%02X%02X\n", n, fc->theme.boss.r, fc->theme.boss.g, fc->theme.boss.b);
        records++;

        if (fc->roomLayout.active)
        {
            fprintf(f, "floor%d.room.name=%s\n", n, fc->roomLayout.name);
            fprintf(f, "floor%d.room.form=%s\n", n, RoomFormName(fc->roomLayout.form));
            fprintf(f, "floor%d.room.density=%.3f\n", n, (double)fc->roomLayout.density);
            records++;
        }

        /* Nemici e boss DAVVERO incontrati (spec M7 punto 2): il flag lo
           imposta world.c solo quando quel tipo compare in una stanza dove
           il giocatore e' presente per costruzione (vedi il commento su
           Game.enemyEncountered in core/game_types.h). */
        for (int slot = 0; slot < 2; slot++)
        {
            if (!game->enemyEncountered[i][slot]) continue;
            const EnemyTypeDef *e = &fc->enemies[slot];
            fprintf(f, "floor%d.enemy%d.name=%s\n", n, slot + 1, e->name);
            fprintf(f, "floor%d.enemy%d.form=%s\n", n, slot + 1, EnemyFormName(e->form));
            fprintf(f, "floor%d.enemy%d.move=%s\n", n, slot + 1, EnemyMoveName(e->move));
            fprintf(f, "floor%d.enemy%d.fire=%s\n", n, slot + 1, EnemyFireName(e->fire));
            fprintf(f, "floor%d.enemy%d.hp=%.3f\n", n, slot + 1, (double)e->hpMul);
            fprintf(f, "floor%d.enemy%d.speed=%.3f\n", n, slot + 1, (double)e->speedMul);
            fprintf(f, "floor%d.enemy%d.size=%.3f\n", n, slot + 1, (double)e->sizeMul);
            fprintf(f, "floor%d.enemy%d.rate=%.3f\n", n, slot + 1, (double)e->fireRate);
            fprintf(f, "floor%d.enemy%d.pellets=%d\n", n, slot + 1, e->pellets);
            records++;
        }

        if (game->bossEncountered[i])
        {
            const EnemyTypeDef *b = &fc->bossType;
            fprintf(f, "floor%d.boss.name=%s\n", n, b->name);
            fprintf(f, "floor%d.boss.form=%s\n", n, EnemyFormName(b->form));
            fprintf(f, "floor%d.boss.move=%s\n", n, EnemyMoveName(b->move));
            fprintf(f, "floor%d.boss.fire=%s\n", n, EnemyFireName(b->fire));
            fprintf(f, "floor%d.boss.hp=%.3f\n", n, (double)b->hpMul);
            fprintf(f, "floor%d.boss.speed=%.3f\n", n, (double)b->speedMul);
            fprintf(f, "floor%d.boss.size=%.3f\n", n, (double)b->sizeMul);
            fprintf(f, "floor%d.boss.rate=%.3f\n", n, (double)b->fireRate);
            fprintf(f, "floor%d.boss.pellets=%d\n", n, b->pellets);
            fprintf(f, "floor%d.boss.outcome=%s\n", n, game->bossDefeated[i] ? "sconfitto" : "incontrato");
            records++;
        }
    }

    /* Oggetti PRESI (spec M7 punto 2, default v1: solo visti in
       negozio/tesoro NON conta): Player.items[] e' esattamente la lista
       degli oggetti che il giocatore ha davvero raccolto in questa run. */
    const Player *p = &game->player;
    fprintf(f, "item.count=%d\n", p->itemCount);
    for (int i = 0; i < p->itemCount; i++)
    {
        const Item *it = &p->items[i];
        int k = i + 1;
        fprintf(f, "item%d.name=%s\n", k, it->name);
        fprintf(f, "item%d.slot=%s\n", k, SlotName(it->slot));
        fprintf(f, "item%d.kind=%s\n", k, KindName(it->kind));
        fprintf(f, "item%d.rarity=%s\n", k, RarityName(it->rarity));
        char traitsText[160];
        ItemTraitsToText(it->traits, traitsText, sizeof(traitsText));
        fprintf(f, "item%d.traits=%s\n", k, traitsText);
        fprintf(f, "item%d.color=#%02X%02X%02X\n", k, it->color.r, it->color.g, it->color.b);
        fprintf(f, "item%d.shape=%d\n", k, it->shape);
        if (it->script[0]) fprintf(f, "item%d.script=%s\n", k, it->script);
        if (it->luaSource[0])
        {
            fprintf(f, "item%d.lua=", k);
            WriteEscapedValue(f, it->luaSource);
        }
        if (it->shotType.active)
        {
            fprintf(f, "item%d.shotType.name=%s\n", k, it->shotType.name);
            fprintf(f, "item%d.shotType.form=%s\n", k, ShotFormName(it->shotType.form));
            fprintf(f, "item%d.shotType.speedMul=%.3f\n", k, (double)it->shotType.speedMul);
            fprintf(f, "item%d.shotType.damageMul=%.3f\n", k, (double)it->shotType.damageMul);
            fprintf(f, "item%d.shotType.radiusMul=%.3f\n", k, (double)it->shotType.radiusMul);
            fprintf(f, "item%d.shotType.lifeMul=%.3f\n", k, (double)it->shotType.lifeMul);
            fprintf(f, "item%d.shotType.pierceBonus=%d\n", k, it->shotType.pierceBonus);
            fprintf(f, "item%d.shotType.chain=%d\n", k, it->shotType.chain);
            fprintf(f, "item%d.shotType.pellets=%d\n", k, it->shotType.pellets);
        }
        records++;
    }

    /* Tipi di colpo (spec M7 punto 2): quello del piano se ADOTTATO (il
       giocatore porta ancora attivo il tipo di un oggetto raccolto -- gia'
       incluso per intero dentro "item%d.shotType.*" sopra, questa e' solo
       l'indice separato che la domanda aperta di open-questions.md chiede
       come "scheda a parte", v1 solo dati) e il colpo firmato del
       personaggio se attivo. */
    if (p->shotType.active)
    {
        fprintf(f, "shot.floor.active=1\n");
        fprintf(f, "shot.floor.name=%s\n", p->shotType.name);
        fprintf(f, "shot.floor.form=%s\n", ShotFormName(p->shotType.form));
        fprintf(f, "shot.floor.speedMul=%.3f\n", (double)p->shotType.speedMul);
        fprintf(f, "shot.floor.damageMul=%.3f\n", (double)p->shotType.damageMul);
        fprintf(f, "shot.floor.radiusMul=%.3f\n", (double)p->shotType.radiusMul);
        fprintf(f, "shot.floor.lifeMul=%.3f\n", (double)p->shotType.lifeMul);
        fprintf(f, "shot.floor.pierceBonus=%d\n", p->shotType.pierceBonus);
        fprintf(f, "shot.floor.chain=%d\n", p->shotType.chain);
        fprintf(f, "shot.floor.pellets=%d\n", p->shotType.pellets);
        records++;
    }
    if (p->characterShotType.active)
    {
        fprintf(f, "shot.character.active=1\n");
        fprintf(f, "shot.character.name=%s\n", p->characterShotType.name);
        fprintf(f, "shot.character.form=%s\n", ShotFormName(p->characterShotType.form));
        fprintf(f, "shot.character.speedMul=%.3f\n", (double)p->characterShotType.speedMul);
        fprintf(f, "shot.character.damageMul=%.3f\n", (double)p->characterShotType.damageMul);
        fprintf(f, "shot.character.radiusMul=%.3f\n", (double)p->characterShotType.radiusMul);
        fprintf(f, "shot.character.lifeMul=%.3f\n", (double)p->characterShotType.lifeMul);
        fprintf(f, "shot.character.pierceBonus=%d\n", p->characterShotType.pierceBonus);
        fprintf(f, "shot.character.chain=%d\n", p->characterShotType.chain);
        fprintf(f, "shot.character.pellets=%d\n", p->characterShotType.pellets);
        records++;
    }

    /* Personaggio generato, SOLO se scelto per questa run (spec M7 punto 2,
       default v1: proposto ma non scelto non si registra). */
    bool hasGeneratedCharacter = (game->characterChosenIndex == CHARACTER_COUNT && game->generatedCharacterValid);
    if (hasGeneratedCharacter)
    {
        const CharacterDef *c = &game->generatedCharacter;
        fprintf(f, "character.active=1\n");
        fprintf(f, "character.name=%s\n", c->name);
        fprintf(f, "character.role=%s\n", c->role);
        fprintf(f, "character.blurb=%s\n", c->blurb);
        fprintf(f, "character.baseDamage=%.3f\n", (double)c->baseDamage);
        fprintf(f, "character.baseFireDelay=%.3f\n", (double)c->baseFireDelay);
        fprintf(f, "character.baseShotSpeed=%.3f\n", (double)c->baseShotSpeed);
        fprintf(f, "character.baseShotRadius=%.3f\n", (double)c->baseShotRadius);
        fprintf(f, "character.baseSpeed=%.3f\n", (double)c->baseSpeed);
        fprintf(f, "character.baseMaxHp=%d\n", c->baseMaxHp);
        fprintf(f, "character.hpCap=%d\n", c->hpCap);
        fprintf(f, "character.baseLuck=%.3f\n", (double)c->baseLuck);
        fprintf(f, "character.palette=#%02X%02X%02X\n", c->palette.r, c->palette.g, c->palette.b);
        if (c->traitHook[0])
        {
            fprintf(f, "character.traitHook=%s\n", c->traitHook);
            char traitLua[SCRIPT_LUA_LEN];
            if (ReadCharacterTraitLua(traitLua, sizeof(traitLua)))
            {
                fprintf(f, "character.traitLua=");
                WriteEscapedValue(f, traitLua);
            }
        }
        if (c->signatureShot.active)
        {
            fprintf(f, "character.signatureShot.name=%s\n", c->signatureShot.name);
            fprintf(f, "character.signatureShot.form=%s\n", ShotFormName(c->signatureShot.form));
            fprintf(f, "character.signatureShot.speedMul=%.3f\n", (double)c->signatureShot.speedMul);
            fprintf(f, "character.signatureShot.damageMul=%.3f\n", (double)c->signatureShot.damageMul);
            fprintf(f, "character.signatureShot.radiusMul=%.3f\n", (double)c->signatureShot.radiusMul);
            fprintf(f, "character.signatureShot.lifeMul=%.3f\n", (double)c->signatureShot.lifeMul);
            fprintf(f, "character.signatureShot.pierceBonus=%d\n", c->signatureShot.pierceBonus);
            fprintf(f, "character.signatureShot.chain=%d\n", c->signatureShot.chain);
            fprintf(f, "character.signatureShot.pellets=%d\n", c->signatureShot.pellets);
        }
        records++;
    }

    fclose(f);
    if (rename(tmpPath, finalPath) != 0)
    {
        fprintf(stderr, "RunCatalog: rename fallita per %s, catalogo non aggiornato per questa run\n", finalPath);
        remove(tmpPath);
        return 0;
    }
    return records;
}

/* ============================================================
   M8 (DEC-045, vista Catalogo v1): l'aggregazione. Vedi il commento su
   RunCatalogAggregate in run_catalog.h per il contratto completo -- da qui
   in giu' solo l'implementazione.
   ============================================================ */

/* Trova la voce '(cat, name)' gia' presente in 'agg' (confronto esatto sul
   nome: due generazioni diverse con lo STESSO nome sono per costruzione la
   stessa voce di catalogo, spec M8 "aggrega per (categoria, nome)") o ne
   crea una nuova in coda. Ritorna l'indice, o -1 se la categoria ha gia'
   raggiunto RUN_CATALOG_ENTRY_MAX voci DISTINTE -- il chiamante incrementa
   overflowCount in quel caso (RecordOccurrence sotto), mai una scrittura
   fuori dall'array. */
static int FindOrCreateCatalogEntry(RunCatalogSummary *agg, RunCatalogCategory cat, const char *name)
{
    int count = agg->entryCount[cat];
    for (int i = 0; i < count; i++)
        if (strcmp(agg->entries[cat][i].name, name) == 0) return i;
    if (count >= RUN_CATALOG_ENTRY_MAX)
    {
        agg->overflowCount[cat]++;
        return -1;
    }
    RunCatalogEntry *e = &agg->entries[cat][count];
    memset(e, 0, sizeof(*e));
    snprintf(e->name, sizeof(e->name), "%s", name);
    agg->entryCount[cat] = count + 1;
    return count;
}

/* Registra UN'occorrenza di '(cat, name)' nel file corrente (identificato da
   'seed' e dal flag 'seenThisFile', locale a RunCatalogAggregateOneFile: una
   sola voce per file puo' incrementare runCount, quante volte vuole
   encounterCount -- vedi il commento su RunCatalogEntry in core/
   game_types.h). 'detail' e' scritto SOLO la prima volta che la voce viene
   creata/vista (mai sovrascritto dopo): la prima descrizione e' gia' buona
   quanto le successive per lo stesso nome (stesso oggetto/nemico/tema
   generato, i campi non cambiano fra un'occorrenza e l'altra). Ritorna
   l'indice della voce (per i chiamanti che devono aggiornare campi extra,
   es. RUN_CATALOG_CAT_BOSS/bossDefeated sotto), o -1 se name e' vuoto o la
   categoria e' in overflow -- in entrambi i casi non c'e' nulla in piu' da
   fare. */
static int RecordCatalogOccurrence(RunCatalogSummary *agg, bool seenThisFile[RUN_CATALOG_CATEGORY_COUNT][RUN_CATALOG_ENTRY_MAX],
                                    RunCatalogCategory cat, const char *name, const char *detail, unsigned int seed)
{
    if (!name || !name[0]) return -1;
    int idx = FindOrCreateCatalogEntry(agg, cat, name);
    if (idx < 0) return -1;
    RunCatalogEntry *e = &agg->entries[cat][idx];
    e->encounterCount++;
    if (detail && detail[0] && !e->detail[0]) snprintf(e->detail, sizeof(e->detail), "%s", detail);
    if (e->runCount == 0) e->firstSeed = seed;
    e->lastSeed = seed;
    if (!seenThisFile[cat][idx]) { seenThisFile[cat][idx] = true; e->runCount++; }
    return idx;
}

/* Un singolo file .txt di catalog/: legge ogni categoria con la STESSA tecnica
   "chiave formattata + ReadManifestValue" di RunCatalogWriteRun sopra (le
   chiavi che scrive quella funzione sono ESATTAMENTE quelle che questa
   rilegge -- stesso schema, direzione opposta, come SlotName/KindName/
   RarityName in cima al file). 'seenThisFile' e' locale a QUESTA chiamata
   (una voce puo' contare come "una run" al massimo una volta per file): un
   array sullo stack di RUN_CATALOG_CATEGORY_COUNT*RUN_CATALOG_ENTRY_MAX bool
   (~1.8 KB), innocuo da riazzerare ad ogni file. */
static void RunCatalogAggregateOneFile(RunCatalogSummary *out, const char *path)
{
    char *text = LoadFileText(path);
    if (!text)
    {
        fprintf(stderr, "RunCatalogAggregate: impossibile leggere %s, file saltato\n", path);
        out->filesSkipped++;
        return;
    }

    /* Guardia principale di robustezza (spec M8, requisito 1: "schema
       diversi: ignora il file con una riga di log, mai crash"). Un file
       corrotto/estraneo che non porta questa riga (o la porta con un valore
       diverso da "1") si ferma qui, PRIMA di leggere qualunque altro campo:
       non sappiamo interpretarne il resto in sicurezza. */
    char schema[8] = { 0 };
    ReadManifestValue(text, "catalogSchema=", schema, sizeof(schema));
    if (strcmp(schema, "1") != 0)
    {
        fprintf(stderr, "RunCatalogAggregate: %s ha catalogSchema='%s' (atteso 1), file ignorato\n", path, schema);
        out->filesSkipped++;
        UnloadFileText(text);
        return;
    }

    bool seenThisFile[RUN_CATALOG_CATEGORY_COUNT][RUN_CATALOG_ENTRY_MAX] = { { false } };

    char seedText[32] = { 0 };
    ReadCatalogLineValue(text, "seed=", seedText, sizeof(seedText));
    unsigned int seed = (unsigned int)strtoul(seedText, NULL, 10);

    char value[256];
    char key[64];

    /* Mondo scelto della run (spec M8: categoria "mondi/temi" copre SIA il
       tema scelto SIA i temi per-piano sotto -- lo stesso raggruppamento
       concettuale della KB). */
    ReadCatalogLineValue(text, "world.name=", value, sizeof(value));
    RecordCatalogOccurrence(out, seenThisFile, RUN_CATALOG_CAT_WORLD, value, "Il mondo scelto della run", seed);

    char floorCountText[16] = { 0 };
    ReadCatalogLineValue(text, "floor.count=", floorCountText, sizeof(floorCountText));
    int floorCount = atoi(floorCountText);
    if (floorCount < 0) floorCount = 0;
    if (floorCount > FLOOR_COUNT) floorCount = FLOOR_COUNT;   /* difensivo: un file forgiato non deve far leggere piani inesistenti */

    for (int f = 1; f <= floorCount; f++)
    {
        snprintf(key, sizeof(key), "floor%d.theme.name=", f);
        ReadCatalogLineValue(text, key, value, sizeof(value));
        RecordCatalogOccurrence(out, seenThisFile, RUN_CATALOG_CAT_WORLD, value, "Il tema di un piano raggiunto", seed);

        char roomName[64] = { 0 };
        snprintf(key, sizeof(key), "floor%d.room.name=", f);
        ReadCatalogLineValue(text, key, roomName, sizeof(roomName));
        if (roomName[0])
        {
            char roomForm[32] = { 0 };
            snprintf(key, sizeof(key), "floor%d.room.form=", f);
            ReadCatalogLineValue(text, key, roomForm, sizeof(roomForm));
            char detail[128];
            snprintf(detail, sizeof(detail), "Forma: %s", roomForm[0] ? roomForm : "?");
            RecordCatalogOccurrence(out, seenThisFile, RUN_CATALOG_CAT_LAYOUT, roomName, detail, seed);
        }

        for (int slot = 1; slot <= 2; slot++)
        {
            char ename[64] = { 0 };
            snprintf(key, sizeof(key), "floor%d.enemy%d.name=", f, slot);
            ReadCatalogLineValue(text, key, ename, sizeof(ename));
            if (!ename[0]) continue;   /* slot non incontrato in questa run: RunCatalogWriteRun non l'ha scritto */
            char form[32] = { 0 }, move[32] = { 0 };
            snprintf(key, sizeof(key), "floor%d.enemy%d.form=", f, slot); ReadCatalogLineValue(text, key, form, sizeof(form));
            snprintf(key, sizeof(key), "floor%d.enemy%d.move=", f, slot); ReadCatalogLineValue(text, key, move, sizeof(move));
            char detail[128];
            snprintf(detail, sizeof(detail), "Forma: %s -- Movimento: %s", form[0] ? form : "?", move[0] ? move : "?");
            RecordCatalogOccurrence(out, seenThisFile, RUN_CATALOG_CAT_ENEMY, ename, detail, seed);
        }

        char bossName[64] = { 0 };
        snprintf(key, sizeof(key), "floor%d.boss.name=", f);
        ReadCatalogLineValue(text, key, bossName, sizeof(bossName));
        if (bossName[0])
        {
            char form[32] = { 0 }, move[32] = { 0 }, outcome[32] = { 0 };
            snprintf(key, sizeof(key), "floor%d.boss.form=", f); ReadCatalogLineValue(text, key, form, sizeof(form));
            snprintf(key, sizeof(key), "floor%d.boss.move=", f); ReadCatalogLineValue(text, key, move, sizeof(move));
            snprintf(key, sizeof(key), "floor%d.boss.outcome=", f); ReadCatalogLineValue(text, key, outcome, sizeof(outcome));
            char detail[128];
            snprintf(detail, sizeof(detail), "Forma: %s -- Movimento: %s", form[0] ? form : "?", move[0] ? move : "?");
            int idx = RecordCatalogOccurrence(out, seenThisFile, RUN_CATALOG_CAT_BOSS, bossName, detail, seed);
            /* 'outcome' e' testo, non un enum (stesso principio di
               RUN_CATALOG_OUTCOME_* in cima a run_catalog.h): confronto per
               stringa contro l'ESATTO valore che RunCatalogWriteRun scrive
               sopra ("sconfitto"/"incontrato"), mai il contrario. */
            if (idx >= 0 && strcmp(outcome, "sconfitto") == 0) out->entries[RUN_CATALOG_CAT_BOSS][idx].bossDefeated = true;
        }
    }

    /* Oggetti presi (e il tipo di colpo che ciascuno conferisce, se lo
       conferisce: spec M8, categoria "tipi di colpo" separata da "oggetti"
       anche se il dato viaggia nello stesso record, esattamente come
       RunCatalogWriteRun lo scrive nello stesso item%d.*). */
    char itemCountText[16] = { 0 };
    ReadCatalogLineValue(text, "item.count=", itemCountText, sizeof(itemCountText));
    int itemCount = atoi(itemCountText);
    if (itemCount < 0) itemCount = 0;
    if (itemCount > MAX_ITEMS) itemCount = MAX_ITEMS;

    for (int i = 1; i <= itemCount; i++)
    {
        char iname[64] = { 0 };
        snprintf(key, sizeof(key), "item%d.name=", i);
        ReadCatalogLineValue(text, key, iname, sizeof(iname));
        if (!iname[0]) continue;

        char slot[32] = { 0 }, rarity[32] = { 0 }, traits[160] = { 0 };
        snprintf(key, sizeof(key), "item%d.slot=", i); ReadCatalogLineValue(text, key, slot, sizeof(slot));
        snprintf(key, sizeof(key), "item%d.rarity=", i); ReadCatalogLineValue(text, key, rarity, sizeof(rarity));
        snprintf(key, sizeof(key), "item%d.traits=", i); ReadCatalogLineValue(text, key, traits, sizeof(traits));
        char detail[128];
        snprintf(detail, sizeof(detail), "Slot: %s -- Rarita': %s -- Tratti: %s",
                 slot[0] ? slot : "?", rarity[0] ? rarity : "?", traits[0] ? traits : "nessuno");
        RecordCatalogOccurrence(out, seenThisFile, RUN_CATALOG_CAT_ITEM, iname, detail, seed);

        /* 'item%d.shotType.name=' NON collide con 'shot.floor.name='/
           'shot.character.name=' (prefissi diversi: "item1." vs "shot."),
           l'ancora a inizio riga serve comunque per coerenza e per non
           dipendere da un'assenza di collisione che un futuro campo
           potrebbe introdurre. */
        char shotName[64] = { 0 };
        snprintf(key, sizeof(key), "item%d.shotType.name=", i);
        ReadCatalogLineValue(text, key, shotName, sizeof(shotName));
        if (shotName[0])
        {
            char form[32] = { 0 };
            snprintf(key, sizeof(key), "item%d.shotType.form=", i); ReadCatalogLineValue(text, key, form, sizeof(form));
            char sdetail[96];
            snprintf(sdetail, sizeof(sdetail), "Forma: %s -- dall'oggetto %s", form[0] ? form : "?", iname);
            RecordCatalogOccurrence(out, seenThisFile, RUN_CATALOG_CAT_SHOT, shotName, sdetail, seed);
        }
    }

    /* Personaggio generato, SOLO se scelto per questa run (stessa guardia di
       RunCatalogWriteRun: "character.active=1" c'e' solo quando lo era).
       ATTENZIONE (bug corretto in fase di test): "character.active="/
       "character.name=" sono un suffisso LETTERALE di "shot.character.active="/
       "shot.character.name=" (le due righe del colpo firmato attivo, scritte
       PRIMA di questo blocco -- vedi RunCatalogWriteRun sopra) -- da qui
       ReadCatalogLineValue (ancorata a inizio riga) invece del semplice
       ReadManifestValue per OGNI chiave di questo blocco, non solo le due
       che collidono davvero: la stessa protezione vale gratis per tutte. */
    char charActive[8] = { 0 };
    ReadCatalogLineValue(text, "character.active=", charActive, sizeof(charActive));
    if (strcmp(charActive, "1") == 0)
    {
        char cname[32] = { 0 }, role[24] = { 0 }, hook[16] = { 0 };
        ReadCatalogLineValue(text, "character.name=", cname, sizeof(cname));
        ReadCatalogLineValue(text, "character.role=", role, sizeof(role));
        ReadCatalogLineValue(text, "character.traitHook=", hook, sizeof(hook));
        if (cname[0])
        {
            char detail[128];
            snprintf(detail, sizeof(detail), "Ruolo: %s%s%s", role[0] ? role : "?",
                     hook[0] ? " -- trait: " : "", hook[0] ? hook : "");
            RecordCatalogOccurrence(out, seenThisFile, RUN_CATALOG_CAT_CHARACTER, cname, detail, seed);
        }

        char sigName[64] = { 0 };
        ReadCatalogLineValue(text, "character.signatureShot.name=", sigName, sizeof(sigName));
        if (sigName[0])
        {
            char form[32] = { 0 };
            ReadCatalogLineValue(text, "character.signatureShot.form=", form, sizeof(form));
            char sdetail[96];
            snprintf(sdetail, sizeof(sdetail), "Forma: %s -- colpo firmato di %s", form[0] ? form : "?", cname[0] ? cname : "?");
            RecordCatalogOccurrence(out, seenThisFile, RUN_CATALOG_CAT_SHOT, sigName, sdetail, seed);
        }
    }

    UnloadFileText(text);
    out->filesRead++;
}

void RunCatalogAggregateFromPath(RunCatalogSummary *out, const char *path)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!path || !DirectoryExists(path)) return;   /* nessuna run ha ancora scritto nulla: aggregato vuoto, mai un errore */

    FilePathList files = LoadDirectoryFilesEx(path, ".txt", false);
    for (unsigned int i = 0; i < files.count; i++)
        RunCatalogAggregateOneFile(out, files.paths[i]);
    UnloadDirectoryFiles(files);
}

void RunCatalogAggregate(RunCatalogSummary *out)
{
    const char *path = g_testCatalogPath ? g_testCatalogPath : "catalog";
    RunCatalogAggregateFromPath(out, path);
}

/* Setter per il percorso del catalogo di test: usato dai test per isolare il catalogo reale. */
void RunCatalogSetTestPath(const char *path)
{
    g_testCatalogPath = path;
}

/* Getter per il percorso del catalogo di test. */
const char *RunCatalogGetTestPath(void)
{
    return g_testCatalogPath;
}
