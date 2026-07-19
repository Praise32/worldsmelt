#include "melting_gen.h"

#include "cJSON.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Vedi il commento su "shot" in RunToJson piu' sotto: serve a tenere il JSON del
   writer dentro la regola 'mul' di run.gbnf (al massimo 2 decimali). */
static double RoundTo2(float value)
{
    return floor((double)value*100.0 + 0.5)/100.0;
}

static void ScriptToText(const GenItem *item, char *out, size_t outSize)
{
    out[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < item->opCount; i++)
    {
        const GenScriptOp *op = &item->ops[i];
        int n = snprintf(out + used, outSize - used, "%s%s:%s,%g,%g,%s",
                         i > 0 ? "|" : "", op->trigger, op->op, op->a, op->b, op->trait);
        if (n < 0 || (size_t)n >= outSize - used) break;
        used += (size_t)n;
    }
}

static void TraitsToText(const GenItem *item, char *out, size_t outSize)
{
    out[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < item->traitCount; i++)
    {
        int n = snprintf(out + used, outSize - used, "%s%s", i > 0 ? "," : "", item->traits[i]);
        if (n < 0 || (size_t)n >= outSize - used) break;
        used += (size_t)n;
    }
}

/* Fase 3a-L3: se l'oggetto ha uno script Lua VALIDATO (item->lua non vuoto,
 * riempito da gen_lua.c SOLO dopo GenLuaValidate, mai dal JSON grezzo del
 * modello), lo scrive nel proprio file (tmp+rename, stesso pattern atomico
 * di GenPublishFile usato per tutto il resto). Un oggetto senza Lua non
 * scrive nessun file: la mini-VM (gia' scritta nel manifest come .script=)
 * resta l'unico comportamento, esattamente come un oggetto di oggi.
 *
 * Il percorso nel TESTO del manifest (scritto da WriteManifest, non da
 * questa funzione: vedi sotto) e' SEMPRE il letterale
 * "generated/scripts/floorN_itemM.lua", MAI derivato da 'outDir': stesso
 * ragionamento di "atlas.path" sopra (vedi il commento li'). Il FILE pero'
 * va scritto nella vera 'outDir' passata dal chiamante (che nei test e' una
 * directory temporanea), cosi' i test restano isolati da generated/ vera.
 *
 * IMPORTANTE (trovato in review): questa funzione va chiamata SOLO nell'ultimo
 * giro stretto di WriteManifest, appena prima di GenPublishFile sul manifest
 * stesso, MAI intrecciata nel ciclo piani/oggetti che scrive il testo. Se
 * ogni oggetto scrivesse subito il proprio file mentre il testo del
 * manifest e' ancora in costruzione, un SIGTERM (timeout o ESC-annullamento,
 * src/app/app.c) a meta' di quel ciclo potrebbe sovrascrivere un
 * floorN_itemM.lua di una run PRECEDENTE con contenuto della run NUOVA,
 * mentre il manifest vecchio (mai sostituito, perche' la sua stessa
 * pubblicazione atomica non e' ancora avvenuta) continua a referenziare
 * quel percorso per un oggetto con nome/trait diversi: mismatch silenzioso,
 * non un crash, ma una vera incoerenza. Chiamandola solo qui, immediatamente
 * prima del rename del manifest, la finestra di rischio si restringe al
 * minimo indispensabile (pochi rename consecutivi) invece di estendersi a
 * tutto il tempo di scrittura del testo: se il processo muore PRIMA di
 * questo punto, NESSUN file .lua viene toccato, e i file di una run
 * precedente restano coerenti col manifest precedente che li referenzia
 * ancora. Non e' un'unica transazione multi-file vera (lo stesso limite,
 * preesistente a questa fase, vale gia' per current_atlas.bmp rispetto a
 * current_run.txt: vedi GenWriteRunFiles sotto), ma e' il massimo
 * restringimento ragionevole senza riscrivere l'intera pipeline di
 * pubblicazione. */
/* 'itemTag' e' il pezzo che distingue il percorso di un oggetto dall'altro
 * ("item1".."item3" per gli attivi, "bossItem" per lo stat-up del piano,
 * fase 3): stringa invece di un intero da quando bossItem non e' piu' un
 * indice di items[], vedi il commento su FloorContent.bossItem in
 * core/game_types.h. */
static void WriteItemLua(const GenItem *item, const char *outDir, int floorNum, const char *itemTag)
{
    if (item->lua[0] == '\0') return;

    char scriptDir[280];
    snprintf(scriptDir, sizeof(scriptDir), "%s/scripts", outDir);
    if (GenEnsureDir(scriptDir) != 0) return;

    /* tmpPath costruito dagli STESSI componenti di finalPath (mai
       "%s.tmp", finalPath"), come il resto di questo file (vedi
       WriteManifest sotto): concatenare un %s letto da un array a
       dimensione dichiarata (finalPath[512]) dentro un altro array a
       dimensione dichiarata fa scattare un falso -Wformat-truncation,
       perche' gcc ragiona sul caso pessimo dell'intero buffer sorgente. */
    char finalPath[512], tmpPath[512];
    snprintf(finalPath, sizeof(finalPath), "%s/floor%d_%s.lua", scriptDir, floorNum, itemTag);
    snprintf(tmpPath, sizeof(tmpPath), "%s/floor%d_%s.lua.tmp", scriptDir, floorNum, itemTag);
    FILE *lf = fopen(tmpPath, "w");
    if (!lf) return;
    fputs(item->lua, lf);
    /* Se GenPublishFile fallisce (disco pieno, rename impossibile...) il
     * manifest referenzia comunque il percorso (la riga .lua= e' gia' stata
     * scritta da WriteManifest prima di questo giro finale): run_content.c
     * degrada silenziosamente a mini-VM su un file mancante, non e' un
     * errore fatale per il resto della run. */
    GenPublishFile(lf, tmpPath, finalPath);
}

/* Step B2: l'atlas.path gia' scritto nel manifest esistente, se c'e'.
 * ESISTE PER UN MOTIVO PRECISO: melting-sprites, quando il passo sprite va a
 * buon fine, RISCRIVE quella riga facendola puntare al PNG generato
 * (SpritesUpdateManifestAtlasPath). Una ripresa in sottofondo (--resume, che
 * riscrive il manifest per aggiungere le righe .lua dei piani 2-5 mentre si
 * gioca) che rimettesse il letterale "generated/current_atlas.bmp" butterebbe via
 * quel puntamento: il gioco, al prossimo caricamento, tornerebbe all'atlas
 * procedurale di riserva pur avendo gli sprite veri sul disco. Nemmeno
 * PreferPngAtlasIfFresh (run_content.c) salverebbe la situazione, perche' quel
 * controllo confronta le DATE (manifest appena riscritto = piu' recente del PNG
 * -> il PNG viene ignorato). Preservare la riga e' l'unico modo corretto.
 * Ritorna false se non c'e' nessun manifest o nessuna riga atlas.path: il
 * chiamante usa allora il letterale di sempre. */
static bool ReadExistingAtlasPath(const char *outDir, char *out, size_t outSize)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/current_run.txt", outDir);
    char *text = GenReadFile(path);
    if (!text) return false;

    const char *key = "atlas.path=";
    const char *start = strstr(text, key);
    if (!start) { free(text); return false; }
    start += strlen(key);

    size_t i = 0;
    while (start[i] && start[i] != '\n' && start[i] != '\r' && i < outSize - 1)
    {
        out[i] = start[i];
        i++;
    }
    out[i] = '\0';
    free(text);
    return i > 0;
}

/* 'preserveAtlasPath' (step B2): vedi ReadExistingAtlasPath sopra. Falso = il
 * comportamento di sempre (atlas.path = il letterale del BMP), che e' quello
 * giusto per una generazione NUOVA (l'atlas BMP viene riscritto insieme al
 * manifest, e un eventuale PNG di una run precedente e' ormai stantio). */
static int WriteManifest(const GenRun *run, const char *outDir, bool preserveAtlasPath)
{
    /* Scrittura su file temporaneo + rename atomico alla fine (vedi
       GenPublishFile in gen_util.c): se il gioco manda SIGTERM per timeout o
       ESC-cancel a meta' scrittura, o il disco e' pieno, current_run.txt
       resta quello valido di prima invece di un file troncato o vuoto. */
    char tmpPath[512], finalPath[512];
    snprintf(finalPath, sizeof(finalPath), "%s/current_run.txt", outDir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/current_run.txt.tmp", outDir);
    FILE *f = fopen(tmpPath, "w");
    if (!f) return -1;
    fprintf(f, "# Generated by melting-gen\n");
    fprintf(f, "source=%s\n", run->source);
    fprintf(f, "seed=%u\n", run->seed);
    /* Percorso fisso, non derivato da outDir: stesso valore letterale del
       fallback in llm/run_content.mjs (runToManifest / writePlayableAtlas),
       che scrive sempre "generated/current_atlas.bmp" perche' Node non ha
       un equivalente di --out. Interpolare outDir qui romperebbe sia la
       parita' di formato col generatore Node sia il test di determinismo
       di scripts/test-gen.sh, che confronta due run con lo stesso seed
       scritte in --out diversi e si aspetta manifest identici byte-a-byte. */
    char atlasLine[256];
    if (!preserveAtlasPath || !ReadExistingAtlasPath(outDir, atlasLine, sizeof(atlasLine)))
    {
        snprintf(atlasLine, sizeof(atlasLine), "generated/current_atlas.bmp");
    }
    fprintf(f, "atlas.path=%s\n", atlasLine);
    for (int fl = 0; fl < GEN_FLOORS; fl++)
    {
        const GenFloor *floor = &run->floors[fl];
        int n = fl + 1;
        fprintf(f, "floor%d.theme=%s\n", n, floor->theme);
        fprintf(f, "floor%d.style=%s\n", n, floor->style);
        fprintf(f, "floor%d.boss=%s\n", n, floor->boss);
        fprintf(f, "floor%d.bg=%s\n", n, floor->bg);
        fprintf(f, "floor%d.floor=%s\n", n, floor->floorColor);
        fprintf(f, "floor%d.wall=%s\n", n, floor->wall);
        fprintf(f, "floor%d.accent=%s\n", n, floor->accent);
        fprintf(f, "floor%d.accent2=%s\n", n, floor->accent2);
        fprintf(f, "floor%d.enemy=%s\n", n, floor->enemy);
        fprintf(f, "floor%d.bossColor=%s\n", n, floor->bossColor);
        /* Tipi di nemico del piano (fase 3b): due normali + il boss. La chiave
           "name" fa da sentinella lato gioco (run_content.c, ReadEnemyType): se
           manca, il piano usa i nemici storici -- back-compat totale con ogni
           manifest scritto prima di questa fase. */
        for (int en = 0; en <= 2; en++)   /* <= 2: l'ultimo giro e' il boss */
        {
            const EnemyTypeDef *foe = (en < 2) ? &floor->enemies[en] : &floor->bossType;
            if (!foe->active) continue;
            char prefix[32];
            if (en < 2) snprintf(prefix, sizeof(prefix), "floor%d.enemy%d", n, en + 1);
            else snprintf(prefix, sizeof(prefix), "floor%d.bossType", n);
            fprintf(f, "%s.name=%s\n", prefix, foe->name);
            fprintf(f, "%s.form=%s\n", prefix, EnemyFormName(foe->form));
            fprintf(f, "%s.move=%s\n", prefix, EnemyMoveName(foe->move));
            fprintf(f, "%s.fire=%s\n", prefix, EnemyFireName(foe->fire));
            fprintf(f, "%s.hp=%.2f\n", prefix, (double)foe->hpMul);
            fprintf(f, "%s.speed=%.2f\n", prefix, (double)foe->speedMul);
            fprintf(f, "%s.size=%.2f\n", prefix, (double)foe->sizeMul);
            fprintf(f, "%s.rate=%.2f\n", prefix, (double)foe->fireRate);
            fprintf(f, "%s.pellets=%d\n", prefix, foe->pellets);
        }

        /* Layout della stanza del piano (fase 3c). La chiave "room.name" fa da
           sentinella lato gioco; un layout OPEN non si scrive (stanza vuota). */
        if (floor->roomLayout.active && floor->roomLayout.form != ROOM_LAYOUT_OPEN)
        {
            fprintf(f, "floor%d.room.name=%s\n", n, floor->roomLayout.name);
            fprintf(f, "floor%d.room.form=%s\n", n, RoomFormName(floor->roomLayout.form));
            fprintf(f, "floor%d.room.density=%.2f\n", n, (double)floor->roomLayout.density);
        }

        for (int i = 0; i < GEN_ITEMS; i++)
        {
            const GenItem *item = &floor->items[i];
            char text[256];
            fprintf(f, "floor%d.item%d.name=%s\n", n, i + 1, item->name);
            fprintf(f, "floor%d.item%d.slot=%s\n", n, i + 1, item->slot);
            TraitsToText(item, text, sizeof(text));
            fprintf(f, "floor%d.item%d.traits=%s\n", n, i + 1, text);
            fprintf(f, "floor%d.item%d.color=%s\n", n, i + 1, item->color);
            fprintf(f, "floor%d.item%d.kind=%s\n", n, i + 1, item->kind);
            fprintf(f, "floor%d.item%d.rarity=%s\n", n, i + 1, item->rarity);
            ScriptToText(item, text, sizeof(text));
            fprintf(f, "floor%d.item%d.script=%s\n", n, i + 1, text);
            /* Tipo di colpo (step C): scritto SOLO sull'oggetto che lo porta
               (floor->shotItem, 1..3, scelto dal modello), mai su tutti e tre --
               un piano ha UN modo di sparare nuovo, non tre. La chiave "shotName"
               fa da sentinella lato gioco (run_content.c, ReadItemShotType): se
               manca, l'oggetto non cambia il modo di sparare, e un manifest
               scritto prima di questa fase resta valido esattamente com'e'.
               %.2f e non %g per i moltiplicatori: due decimali bastano
               (ShotTypeBalance lavora su questa scala) e il formato resta
               identico byte-per-byte a parita' di seed su qualunque libc, che e'
               cio' che il test di determinismo di scripts/test-gen.sh confronta. */
            if (floor->shotItem == i + 1 && floor->shot.active)
            {
                const ShotTypeDef *shot = &floor->shot;
                fprintf(f, "floor%d.item%d.shotName=%s\n", n, i + 1, shot->name);
                fprintf(f, "floor%d.item%d.shotForm=%s\n", n, i + 1, ShotFormName(shot->form));
                fprintf(f, "floor%d.item%d.shotSpeed=%.2f\n", n, i + 1, (double)shot->speedMul);
                fprintf(f, "floor%d.item%d.shotDamage=%.2f\n", n, i + 1, (double)shot->damageMul);
                fprintf(f, "floor%d.item%d.shotSize=%.2f\n", n, i + 1, (double)shot->radiusMul);
                fprintf(f, "floor%d.item%d.shotLife=%.2f\n", n, i + 1, (double)shot->lifeMul);
                fprintf(f, "floor%d.item%d.shotPierce=%d\n", n, i + 1, shot->pierceBonus);
                fprintf(f, "floor%d.item%d.shotChain=%d\n", n, i + 1, shot->chain);
                fprintf(f, "floor%d.item%d.shotPellets=%d\n", n, i + 1, shot->pellets);
            }
            /* La riga .lua= si scrive QUI (testo), il FILE che referenzia si
               scrive PIU' TARDI, in un giro a parte subito sotto: vedi il
               commento lungo su WriteItemLua sopra per il perche'. */
            if (item->lua[0] != '\0')
            {
                fprintf(f, "floor%d.item%d.lua=generated/scripts/floor%d_item%d.lua\n", n, i + 1, n, i + 1);
            }
        }

        /* Oggetto stat-up del piano (fase 3, ricompensa del boss): stesse
           chiavi degli oggetti attivi sopra MA senza ".script=" (nessun
           comportamento mini-VM, vedi il commento su GenItem.bossItem in
           melting_gen.h) e col prefisso "bossItem" invece di "itemN", per
           tenerlo inconfondibile da un quarto oggetto attivo quando si legge
           il manifest a mano. */
        const GenItem *boss = &floor->bossItem;
        char bossText[256];
        fprintf(f, "floor%d.bossItem.name=%s\n", n, boss->name);
        fprintf(f, "floor%d.bossItem.slot=%s\n", n, boss->slot);
        TraitsToText(boss, bossText, sizeof(bossText));
        fprintf(f, "floor%d.bossItem.traits=%s\n", n, bossText);
        fprintf(f, "floor%d.bossItem.color=%s\n", n, boss->color);
        fprintf(f, "floor%d.bossItem.kind=%s\n", n, boss->kind);
        fprintf(f, "floor%d.bossItem.rarity=%s\n", n, boss->rarity);
        if (boss->lua[0] != '\0')
        {
            fprintf(f, "floor%d.bossItem.lua=generated/scripts/floor%d_bossItem.lua\n", n, n);
        }
    }

    /* Ultimo giro, il piu' vicino possibile alla pubblicazione atomica del
       manifest (subito sotto): vedi il commento su WriteItemLua per il
       perche' NON e' stato fatto dentro il ciclo sopra. */
    for (int fl = 0; fl < GEN_FLOORS; fl++)
    {
        const GenFloor *floor = &run->floors[fl];
        char tag[16];
        for (int i = 0; i < GEN_ITEMS; i++)
        {
            snprintf(tag, sizeof(tag), "item%d", i + 1);
            WriteItemLua(&floor->items[i], outDir, fl + 1, tag);
        }
        WriteItemLua(&floor->bossItem, outDir, fl + 1, "bossItem");
    }

    return GenPublishFile(f, tmpPath, finalPath);
}

/* Un tipo di nemico in JSON, nell'ordine di chiavi della regola 'foe' di run.gbnf
   (fase 3b). */
static cJSON *EnemyTypeToJson(const EnemyTypeDef *foe)
{
    cJSON *j = cJSON_CreateObject();
    cJSON_AddStringToObject(j, "name", foe->name);
    cJSON_AddStringToObject(j, "form", EnemyFormName(foe->form));
    cJSON_AddStringToObject(j, "move", EnemyMoveName(foe->move));
    cJSON_AddStringToObject(j, "fire", EnemyFireName(foe->fire));
    cJSON_AddNumberToObject(j, "hp", RoundTo2(foe->hpMul));
    cJSON_AddNumberToObject(j, "speed", RoundTo2(foe->speedMul));
    cJSON_AddNumberToObject(j, "size", RoundTo2(foe->sizeMul));
    cJSON_AddNumberToObject(j, "rate", RoundTo2(foe->fireRate));
    cJSON_AddNumberToObject(j, "pellets", foe->pellets);
    return j;
}

/* Ordine di inserimento = ordine chiavi di run.gbnf: la coppia writer/grammatica
   viene verificata da test-gen (Task 5). Volutamente NON include
   floor->bossItem (fase 3): quel campo non fa parte di cosa il modello
   scrive in JSON (run.gbnf/system.txt restano quelli di sempre, 3 oggetti
   per piano), quindi non ha senso nel JSON di debug/nel campione usato dal
   validatore GBNF (--emit-llm-json, scripts/test-gen.sh) che verifica
   ESATTAMENTE che l'output di questa funzione rispetti quella grammatica.
   Vedi il commento su GenFloor.bossItem in melting_gen.h per il perche'
   dell'intera scelta. */
static cJSON *RunToJson(const GenRun *run)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *floors = cJSON_AddArrayToObject(root, "floors");
    for (int fl = 0; fl < GEN_FLOORS; fl++)
    {
        const GenFloor *floor = &run->floors[fl];
        cJSON *jf = cJSON_CreateObject();
        cJSON_AddStringToObject(jf, "theme", floor->theme);
        cJSON_AddStringToObject(jf, "style", floor->style);
        cJSON_AddStringToObject(jf, "boss", floor->boss);
        cJSON_AddStringToObject(jf, "bg", floor->bg);
        cJSON_AddStringToObject(jf, "floor", floor->floorColor);
        cJSON_AddStringToObject(jf, "wall", floor->wall);
        cJSON_AddStringToObject(jf, "accent", floor->accent);
        cJSON_AddStringToObject(jf, "accent2", floor->accent2);
        cJSON_AddStringToObject(jf, "enemy", floor->enemy);
        cJSON_AddStringToObject(jf, "bossColor", floor->bossColor);

        /* Tipo di colpo del piano (step C): fa parte di cio' che il MODELLO
           scrive (a differenza di kind/rarity), quindi va nel JSON e nella
           grammatica. I moltiplicatori sono arrotondati a 2 decimali PRIMA di
           finire in cJSON: un float come 1.45f vale 1.4500000476837158 come
           double, e cJSON lo stamperebbe con tutte le sue cifre ("1.45000004768372"),
           che la regola 'mul' di run.gbnf (al massimo 2 decimali) rifiuterebbe --
           facendo fallire il test di coerenza writer<->grammatica di
           scripts/test-gen.sh, non il gioco. Arrotondare qui tiene le due cose
           allineate per costruzione. */
        cJSON *jshot = cJSON_AddObjectToObject(jf, "shot");
        cJSON_AddStringToObject(jshot, "name", floor->shot.name);
        cJSON_AddStringToObject(jshot, "form", ShotFormName(floor->shot.form));
        cJSON_AddNumberToObject(jshot, "speed",  RoundTo2(floor->shot.speedMul));
        cJSON_AddNumberToObject(jshot, "damage", RoundTo2(floor->shot.damageMul));
        cJSON_AddNumberToObject(jshot, "size",   RoundTo2(floor->shot.radiusMul));
        cJSON_AddNumberToObject(jshot, "life",   RoundTo2(floor->shot.lifeMul));
        cJSON_AddNumberToObject(jshot, "pierce",  floor->shot.pierceBonus);
        cJSON_AddNumberToObject(jshot, "chain",   floor->shot.chain);
        cJSON_AddNumberToObject(jshot, "pellets", floor->shot.pellets);
        cJSON_AddNumberToObject(jf, "shotItem", floor->shotItem);

        /* Tipi di nemico (fase 3b): li scrive il modello, quindi stanno nel JSON e
           nella grammatica. Stesso arrotondamento a 2 decimali dei moltiplicatori
           del tipo di colpo, e per lo stesso motivo (la regola 'mul' di run.gbnf). */
        cJSON *jfoes = cJSON_AddArrayToObject(jf, "enemies");
        for (int en = 0; en < 2; en++) cJSON_AddItemToArray(jfoes, EnemyTypeToJson(&floor->enemies[en]));
        cJSON_AddItemToObject(jf, "bossType", EnemyTypeToJson(&floor->bossType));

        /* Layout della stanza (fase 3c): fa parte di cio' che il modello scrive. */
        cJSON *jroom = cJSON_AddObjectToObject(jf, "room");
        cJSON_AddStringToObject(jroom, "name", floor->roomLayout.name);
        cJSON_AddStringToObject(jroom, "form", RoomFormName(floor->roomLayout.form));
        cJSON_AddNumberToObject(jroom, "density", RoundTo2(floor->roomLayout.density));

        cJSON *items = cJSON_AddArrayToObject(jf, "items");
        for (int i = 0; i < GEN_ITEMS; i++)
        {
            const GenItem *item = &floor->items[i];
            cJSON *ji = cJSON_CreateObject();
            cJSON_AddStringToObject(ji, "name", item->name);
            cJSON_AddStringToObject(ji, "slot", item->slot);
            cJSON *traits = cJSON_AddArrayToObject(ji, "traits");
            for (int t = 0; t < item->traitCount; t++)
            {
                cJSON_AddItemToArray(traits, cJSON_CreateString(item->traits[t]));
            }
            cJSON_AddStringToObject(ji, "color", item->color);
            cJSON *script = cJSON_AddArrayToObject(ji, "script");
            for (int s = 0; s < item->opCount; s++)
            {
                const GenScriptOp *op = &item->ops[s];
                cJSON *jo = cJSON_CreateObject();
                cJSON_AddStringToObject(jo, "trigger", op->trigger);
                cJSON_AddStringToObject(jo, "op", op->op);
                cJSON_AddNumberToObject(jo, "a", op->a);
                cJSON_AddNumberToObject(jo, "b", op->b);
                cJSON_AddStringToObject(jo, "trait", op->trait);
                cJSON_AddItemToArray(script, jo);
            }
            cJSON_AddItemToArray(items, ji);
        }
        cJSON_AddItemToArray(floors, jf);
    }
    return root;
}

int GenWriteLlmJson(const GenRun *run, const char *path)
{
    cJSON *root = RunToJson(run);
    char *text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!text) return -1;
    FILE *f = fopen(path, "w");
    if (!f) { cJSON_free(text); return -1; }
    fputs(text, f);
    fclose(f);
    cJSON_free(text);
    return 0;
}

/* Step B2: pubblicazione di una RIPRESA (manifest + file .lua soltanto). Non
 * tocca l'atlas BMP (esiste gia' su disco dalla generazione che ha aperto la run,
 * riscriverlo sarebbe lavoro sprecato e ne cambierebbe la data) e PRESERVA
 * atlas.path (vedi ReadExistingAtlasPath). Chiamata dopo OGNI piano completato,
 * mentre il giocatore sta gia' giocando: e' quindi il percorso piu' delicato del
 * file, ed e' anche il motivo per cui ogni scrittura qui dentro passa da
 * tmp+rename (GenPublishFile) -- il gioco puo' leggere il manifest in qualunque
 * momento e non deve mai vederne uno a meta'. */
int GenWriteRunFilesResume(const GenRun *run, const char *outDir)
{
    if (GenEnsureDir(outDir) != 0) return -1;
    return WriteManifest(run, outDir, true);
}

int GenWriteRunFiles(const GenRun *run, const char *outDir)
{
    if (GenEnsureDir(outDir) != 0) return -1;
    if (WriteManifest(run, outDir, false) != 0) return -1;

    char atlasPath[300];
    snprintf(atlasPath, sizeof(atlasPath), "%s/current_atlas.bmp", outDir);

    /* current_run.json e current_atlas.json sono output di debug/diagnostica:
       nulla in src/ li legge (il gioco legge solo current_run.txt, con
       atlas.path fisso a "generated/current_atlas.bmp", vedi il commento in
       WriteManifest). Per questo qui sotto il campo "path" interpola
       volutamente atlasPath/outDir invece del letterale hardcoded: e' comodo
       per ispezionare a mano l'output di un --out non standard, e non c'e'
       nessun consumatore che possa notare l'inconsistenza. */
    cJSON *root = RunToJson(run);
    cJSON_AddStringToObject(root, "source", run->source);
    cJSON_AddNumberToObject(root, "seed", run->seed);
    cJSON *atlas = cJSON_AddObjectToObject(root, "atlas");
    cJSON_AddStringToObject(atlas, "path", atlasPath);
    cJSON_AddNumberToObject(atlas, "cellSize", 128);
    cJSON_AddNumberToObject(atlas, "columns", 8);
    char *text = cJSON_Print(root);
    cJSON_Delete(root);
    if (!text) return -1;

    char tmpPath[512], finalPath[512];
    snprintf(finalPath, sizeof(finalPath), "%s/current_run.json", outDir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/current_run.json.tmp", outDir);
    FILE *f = fopen(tmpPath, "w");
    if (!f) { cJSON_free(text); return -1; }
    fprintf(f, "%s\n", text);
    cJSON_free(text);
    if (GenPublishFile(f, tmpPath, finalPath) != 0) return -1;

    snprintf(finalPath, sizeof(finalPath), "%s/current_atlas.json", outDir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/current_atlas.json.tmp", outDir);
    f = fopen(tmpPath, "w");
    if (!f) return -1;
    fprintf(f,
        "{\n"
        "  \"path\": \"%s\",\n"
        "  \"width\": 1024,\n  \"height\": 1024,\n"
        "  \"cellSize\": 128,\n  \"columns\": 8,\n  \"rows\": 8,\n"
        "  \"sprites\": {\n"
        "    \"player\": [0, 0], \"enemy_chaser\": [1, 0], \"enemy_shooter\": [2, 0],\n"
        "    \"enemy_tank\": [3, 0], \"boss\": [4, 0], \"item\": [5, 0], \"heart\": [6, 0],\n"
        "    \"coin\": [7, 0], \"bomb\": [0, 1], \"key\": [1, 1], \"exit\": [2, 1], \"shot\": [3, 1]\n"
        "  }\n"
        "}\n",
        atlasPath);
    return GenPublishFile(f, tmpPath, finalPath);
}

/* RunBundle v1 (roadmap 16/07/2026 settimana 4): vedi il commento lungo su
 * questa funzione in melting_gen.h -- MAI chiamata sul percorso --resume
 * (main.c semplicemente non la invoca li'). Formato chiave=valore, come
 * current_run.txt, cosi' si legge e si fa il grep allo stesso modo. */
int GenWriteProvenance(const GenRun *run, const char *outDir, const char *promptsDir,
                        const char *modelJsonField, const char *modelLuaField,
                        const char *chosenThemeField)
{
    if (GenEnsureDir(outDir) != 0) return -1;

    /* Un promptsDir mancante non deve far fallire l'intera generazione per un
       campo diagnostico: si scrive comunque il file, con un hash a zero, e si
       logga il perche' (GenLogLine, mai silenzioso). */
    unsigned long long fnv = 0;
    if (GenPromptsFnv(promptsDir, &fnv) != 0)
    {
        GenLogLine("provenance: promptsFnv non calcolabile (cartella prompt mancante o vuota: %s)",
                   promptsDir ? promptsDir : "(null)");
    }

    char tmpPath[512], finalPath[512];
    snprintf(finalPath, sizeof(finalPath), "%s/provenance.txt", outDir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/provenance.txt.tmp", outDir);
    FILE *f = fopen(tmpPath, "w");
    if (!f) return -1;
    fprintf(f, "bundleSchema=1\n");
    fprintf(f, "seed=%u\n", run->seed);
    fprintf(f, "source=%s\n", run->source);
    fprintf(f, "modelJson=%s\n", modelJsonField ? modelJsonField : "fallback");
    fprintf(f, "modelLua=%s\n", modelLuaField ? modelLuaField : "-");
    /* M5, requisito 6: SEMPRE una riga (mai omessa quando --theme-file manca),
       stesso trattamento di modelJson/modelLua sopra -- "none" e' un valore
       da fare grep, non un buco nel formato. */
    fprintf(f, "chosenTheme=%s\n", chosenThemeField ? chosenThemeField : "none");
    /* Costante, non una define: vive qui e in AGENTS.md/scripts/setup-deps.sh
       (LLAMA_TAG) come lo stesso valore tenuto sincronizzato a mano -- non
       vale la pena di una define condivisa per un singolo letterale che
       cambia solo quando si aggiorna deliberatamente la dipendenza. */
    fprintf(f, "llamaTag=b9979\n");
    fprintf(f, "promptsFnv=%016llx\n", fnv);
    fprintf(f, "createdAt=%lld\n", (long long)time(NULL));
    return GenPublishFile(f, tmpPath, finalPath);
}

/* M5 (DEC-005): generated/theme_proposals.json, letto dal gioco SENZA cJSON
 * (AGENTS.md: il binario del gioco non linka mai cJSON) -- schema fisso e
 * volutamente minimo, {"proposals":[{"name":...,"blurb":...}...],"source":...},
 * charset ASCII puro senza virgolette/backslash interni (propose.gbnf lo
 * impone sia sul percorso modello sia -- per costruzione -- su quello
 * procedurale), cosi' il gioco puo' spezzarlo con un semplice strstr invece
 * di un parser JSON vero. Stesso pattern tmp+rename di ogni altro output. */
int GenWriteThemeProposals(const GenThemeProposal *proposals, int count, const char *source, const char *outDir)
{
    if (GenEnsureDir(outDir) != 0) return -1;
    if (count < 1) count = 1;
    if (count > GEN_THEME_PROPOSALS) count = GEN_THEME_PROPOSALS;

    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(root, "proposals");
    for (int i = 0; i < count; i++)
    {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddStringToObject(p, "name", proposals[i].name);
        cJSON_AddStringToObject(p, "blurb", proposals[i].blurb);
        cJSON_AddItemToArray(arr, p);
    }
    cJSON_AddStringToObject(root, "source", source ? source : "fallback");
    char *text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!text) return -1;

    char tmpPath[512], finalPath[512];
    snprintf(finalPath, sizeof(finalPath), "%s/theme_proposals.json", outDir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/theme_proposals.json.tmp", outDir);
    FILE *f = fopen(tmpPath, "w");
    if (!f) { cJSON_free(text); return -1; }
    fputs(text, f);
    cJSON_free(text);
    return GenPublishFile(f, tmpPath, finalPath);
}

/* M6b-2 (DEC-037): generated/scripts/character_trait.lua, scritto PRIMA di
 * character_proposal.json che lo referenzia (campo "lua", vedi
 * GenWriteCharacterProposal sotto) -- STESSA garanzia d'ordine di
 * WriteItemLua sopra per il manifest degli oggetti: un lettore (il gioco)
 * non deve mai trovare un json che dice "lua":true con il file ancora
 * assente per una scrittura a meta'. Nessun 'itemTag'/floor a differenza di
 * WriteItemLua: UN solo file per personaggio generato (un solo trait per
 * run), non uno per oggetto. Tmp+rename come ogni altro file pubblicato qui. */
int GenWriteCharacterTraitLua(const char *lua, const char *outDir)
{
    if (!lua || !lua[0]) return -1;

    char scriptDir[280];
    snprintf(scriptDir, sizeof(scriptDir), "%s/scripts", outDir);
    if (GenEnsureDir(scriptDir) != 0) return -1;

    char finalPath[512], tmpPath[512];
    snprintf(finalPath, sizeof(finalPath), "%s/character_trait.lua", scriptDir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/character_trait.lua.tmp", scriptDir);
    FILE *lf = fopen(tmpPath, "w");
    if (!lf) return -1;
    fputs(lua, lf);
    return GenPublishFile(lf, tmpPath, finalPath);
}

/* M6b-1 (DEC-014, prima fetta): generated/character_proposal.json, letto dal
 * gioco SENZA cJSON (src/content/character_proposal.c) -- stesso spirito di
 * GenWriteThemeProposals sopra: schema fisso, charset ASCII senza
 * virgolette/backslash interni (character.gbnf lo garantisce sia sul
 * percorso modello sia -- per costruzione -- su ogni scrittore di questo
 * file), tmp+rename. 'def' e' gia' CLAMPATO da chi chiama
 * (CharacterGenDefClamp, prima rete di sicurezza -- la seconda gira alla
 * lettura lato gioco).
 * 'hasLua' (M6b-2, DEC-037): vero SOLO quando il chiamante ha GIA' scritto
 * generated/scripts/character_trait.lua con successo (GenWriteCharacterTraitLua
 * sopra, chiamata PRIMA di questa funzione) -- scrive il campo booleano
 * "lua" nel json, che il gioco legge per decidere se vale la pena provare a
 * caricare il file (src/content/character_proposal.c). Il chiamante reale
 * (RunProposeCharacter, main.c) non arriva mai qui con hasLua=false: se il
 * trait non valida, la proposta INTERA non si scrive (KB: trait invalido =
 * personaggio invalido) -- il parametro esiste comunque esplicito, non un
 * default nascosto, cosi' questa funzione non deve indovinare la regola di
 * dominio di chi la chiama.
 * M6b-3 (DEC-068): il colpo firmato OPZIONALE viaggia dentro 'def' stesso
 * (def->hasShot/def->signatureShot, gia' CLAMPATO/bilanciato dal chiamante
 * con CharacterGenDefClamp -- niente parametro a parte come 'hasLua': a
 * differenza del trait, che vive in un file separato scritto da un'altra
 * funzione, il colpo firmato e' gia' tutto dentro CharacterGenDef). Scritto
 * come sotto-oggetto "shot" SOLO quando def->hasShot, omesso del tutto
 * altrimenti -- il gioco (src/content/character_proposal.c) lo rileva
 * cercando la sottostringa letterale "\"shot\":{" nel testo, stesso stile
 * sentinella di "\"lua\":true" per il trait. */
int GenWriteCharacterProposal(const CharacterGenDef *def, const char *source, const char *outDir, bool hasLua)
{
    if (!def) return -1;
    if (GenEnsureDir(outDir) != 0) return -1;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", def->name);
    cJSON_AddStringToObject(root, "blurb", def->blurb);
    cJSON *stats = cJSON_AddObjectToObject(root, "stats");
    cJSON_AddNumberToObject(stats, "damage", def->damage);
    cJSON_AddNumberToObject(stats, "fireDelay", def->fireDelay);
    cJSON_AddNumberToObject(stats, "shotSpeed", def->shotSpeed);
    cJSON_AddNumberToObject(stats, "speed", def->speed);
    cJSON_AddNumberToObject(stats, "maxHp", def->maxHp);
    cJSON_AddNumberToObject(stats, "luck", def->luck);
    cJSON_AddStringToObject(root, "palette", def->palette);
    cJSON_AddStringToObject(root, "source", source ? source : "local:unknown");
    cJSON_AddBoolToObject(root, "lua", hasLua);
    if (def->hasShot)
    {
        cJSON *jshot = cJSON_AddObjectToObject(root, "shot");
        cJSON_AddStringToObject(jshot, "name", def->signatureShot.name);
        cJSON_AddStringToObject(jshot, "form", ShotFormName(def->signatureShot.form));
        cJSON_AddNumberToObject(jshot, "speed",  RoundTo2(def->signatureShot.speedMul));
        cJSON_AddNumberToObject(jshot, "damage", RoundTo2(def->signatureShot.damageMul));
        cJSON_AddNumberToObject(jshot, "size",   RoundTo2(def->signatureShot.radiusMul));
        cJSON_AddNumberToObject(jshot, "life",   RoundTo2(def->signatureShot.lifeMul));
        cJSON_AddNumberToObject(jshot, "pierce",  def->signatureShot.pierceBonus);
        cJSON_AddNumberToObject(jshot, "chain",   def->signatureShot.chain);
        cJSON_AddNumberToObject(jshot, "pellets", def->signatureShot.pellets);
    }
    char *text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!text) return -1;

    char tmpPath[512], finalPath[512];
    snprintf(finalPath, sizeof(finalPath), "%s/character_proposal.json", outDir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/character_proposal.json.tmp", outDir);
    FILE *f = fopen(tmpPath, "w");
    if (!f) { cJSON_free(text); return -1; }
    fputs(text, f);
    cJSON_free(text);
    return GenPublishFile(f, tmpPath, finalPath);
}
