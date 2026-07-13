#include "melting_gen.h"

#include "cJSON.h"

#include <stdio.h>
#include <string.h>

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

static int WriteManifest(const GenRun *run, const char *outDir)
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
    fprintf(f, "atlas.path=generated/current_atlas.bmp\n");
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
            ScriptToText(item, text, sizeof(text));
            fprintf(f, "floor%d.item%d.script=%s\n", n, i + 1, text);
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

int GenWriteRunFiles(const GenRun *run, const char *outDir)
{
    if (GenEnsureDir(outDir) != 0) return -1;
    if (WriteManifest(run, outDir) != 0) return -1;

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
