#include "content/character_proposal.h"

#include "core/character_type.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Stesso HexDigit/ParseHexColor di src/content/run_content.c (ParseHexColor
 * li' e' static, nessuna condivisione voluta fra i due moduli -- stesso
 * principio gia' spiegato sul commento di ReadManifestValue in
 * run_content.c: moduli diversi, ognuno coi propri file da leggere). */
static int HexDigit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return 0;
}

static Color ParseHexColor(const char *text, Color fallback)
{
    if (!text || text[0] != '#' || strlen(text) < 7) return fallback;
    return (Color){
        (unsigned char)(HexDigit(text[1])*16 + HexDigit(text[2])),
        (unsigned char)(HexDigit(text[3])*16 + HexDigit(text[4])),
        (unsigned char)(HexDigit(text[5])*16 + HexDigit(text[6])),
        255
    };
}

/* Un byte fuori dall'ASCII stampabile (controllo, o >0x7E) diventa '?':
 * name/blurb finiscono su DrawText (raylib) senza mai passare da un
 * validatore di charset vero (il gioco non ne ha uno, DEC-052 lo impone solo
 * lato generazione) -- un file forgiato a mano con newline o byte alti non
 * deve poter rompere il layout della carta. */
static void SanitizeAscii(char *text)
{
    for (char *p = text; *p; p++)
        if ((unsigned char)*p < 0x20 || (unsigned char)*p > 0x7E) *p = '?';
}

/* M6b-2 (DEC-037): il nome della callback ("on_fire"/"on_hit"/"on_tick"/
 * "on_evaluate") che generated/scripts/character_trait.lua definisce -- UN
 * scan di testo, non un parse Lua vero (questo modulo non linka Lua, vedi
 * AGENTS.md: solo bin/melting-gen e il gioco a runtime lo fanno). Cerca
 * "function <hook>" nell'ordine sotto (lo stesso ordine di validazione di
 * GenLuaValidateCharacterTrait, tools/melting-gen/gen_lua.c: la generazione
 * garantisce ESATTAMENTE una corrispondenza, ma un file forgiato a mano
 * potrebbe averne piu' d'una -- prendere la prima e' una scelta onesta
 * quanto qualunque altra, mai un crash). 'out' resta stringa vuota se il
 * file non esiste/e' illeggibile o non contiene nessuno dei quattro nomi:
 * "niente di leggibile, niente riga sulla carta" (spec). Il caricamento VERO
 * del comportamento (src/script/script_character.c) rilegge lo stesso file
 * per conto suo, indipendentemente da questa funzione -- due letture
 * separate, mai una a cascata dell'altra. */
static void DetectTraitHook(char *out, size_t outSize)
{
    out[0] = '\0';
    char *lua = LoadFileText("generated/scripts/character_trait.lua");
    if (!lua) return;

    static const char *hooks[] = { "on_evaluate", "on_fire", "on_hit", "on_tick" };
    for (int i = 0; i < 4; i++)
    {
        char needle[32];
        snprintf(needle, sizeof(needle), "function %s", hooks[i]);
        if (strstr(lua, needle)) { snprintf(out, outSize, "%s", hooks[i]); break; }
    }
    UnloadFileText(lua);
}

/* Ritorna il puntatore SUBITO DOPO 'key' (letterale, virgolette e due punti
 * gia' dentro, es. "\"name\":\"" per una stringa o "\"damage\":" per un
 * numero), o NULL se 'key' non compare in 'text'. Stesso schema di
 * strstr-e-avanza di AppLoadThemeCards (src/app/app.c) -- cerca nell'INTERO
 * testo ogni volta, non avanza un cursore sequenziale: l'ordine delle chiavi
 * nel file non conta (lo controlla comunque il writer, gen_manifest.c, ma
 * questo lato non ne dipende). */
static const char *FindAfter(const char *text, const char *key)
{
    const char *p = strstr(text, key);
    return p ? p + strlen(key) : NULL;
}

/* Copia la stringa JSON che inizia in 'cursor' (gia' DOPO la virgoletta di
 * apertura) fino alla prossima virgoletta, troncando in sicurezza su
 * 'outSize'. Ritorna il puntatore dopo la virgoletta di chiusura, o NULL se
 * non la trova (stringa non terminata: JSON corrotto). */
static const char *CopyJsonString(const char *cursor, char *out, size_t outSize)
{
    const char *end = strchr(cursor, '"');
    if (!end) return NULL;
    size_t len = (size_t)(end - cursor);
    if (len >= outSize) len = outSize - 1;
    memcpy(out, cursor, len);
    out[len] = '\0';
    return end + 1;
}

/* Legge un numero JSON da 'cursor' (gia' dopo la chiave "\"campo\":", senza
 * virgolette: e' un numero) con strtod -- serve sapere DOVE si e' fermato
 * per distinguere "il numero manca" da "il numero e' zero", cosa che atof
 * (usato altrove nel progetto, run_content.c/script_vm.c) non permette.
 * Ritorna false se 'cursor' e' NULL o non c'e' nessuna cifra da leggere. */
static bool ReadJsonNumber(const char *cursor, double *out)
{
    if (!cursor) return false;
    char *endp = NULL;
    double v = strtod(cursor, &endp);
    if (endp == cursor) return false;
    *out = v;
    return true;
}

bool RunContentLoadCharacterProposal(const char *path, CharacterDef *out)
{
    char *text = LoadFileText(path);
    if (!text) return false;

    char name[CHARACTER_GEN_NAME_LEN];
    char blurb[CHARACTER_GEN_BLURB_LEN];
    char palette[8];
    name[0] = blurb[0] = palette[0] = '\0';

    const char *cursor = FindAfter(text, "\"name\":\"");
    bool ok = (cursor != NULL);
    if (ok) { cursor = CopyJsonString(cursor, name, sizeof(name)); ok = cursor && name[0]; }

    if (ok) { cursor = FindAfter(text, "\"blurb\":\""); ok = cursor != NULL; }
    if (ok) { cursor = CopyJsonString(cursor, blurb, sizeof(blurb)); ok = cursor && blurb[0]; }

    double damage = 0.0, fireDelay = 0.0, shotSpeed = 0.0, speed = 0.0, maxHp = 0.0, luck = 0.0;
    if (ok) ok = ReadJsonNumber(FindAfter(text, "\"damage\":"), &damage);
    if (ok) ok = ReadJsonNumber(FindAfter(text, "\"fireDelay\":"), &fireDelay);
    if (ok) ok = ReadJsonNumber(FindAfter(text, "\"shotSpeed\":"), &shotSpeed);
    if (ok) ok = ReadJsonNumber(FindAfter(text, "\"speed\":"), &speed);
    if (ok) ok = ReadJsonNumber(FindAfter(text, "\"maxHp\":"), &maxHp);
    if (ok) ok = ReadJsonNumber(FindAfter(text, "\"luck\":"), &luck);

    if (ok)
    {
        cursor = FindAfter(text, "\"palette\":\"");
        ok = cursor != NULL;
        if (ok)
        {
            cursor = CopyJsonString(cursor, palette, sizeof(palette));
            ok = cursor && palette[0] == '#' && strlen(palette) == 7;
        }
    }

    /* M6b-2 (DEC-037): il campo "lua" va letto QUI, mentre 'text' e' ancora
     * vivo (viene liberato subito sotto) -- vedi il commento sopra
     * DetectTraitHook per il perche' e' OPZIONALE (non entra nella catena
     * 'ok'). */
    bool hasLua = strstr(text, "\"lua\":true") != NULL;

    UnloadFileText(text);
    if (!ok) return false;

    SanitizeAscii(name);
    SanitizeAscii(blurb);

    CharacterGenDef def;
    memset(&def, 0, sizeof(def));
    snprintf(def.name, sizeof(def.name), "%s", name);
    snprintf(def.blurb, sizeof(def.blurb), "%s", blurb);
    def.damage = (float)damage;
    def.fireDelay = (float)fireDelay;
    def.shotSpeed = (float)shotSpeed;
    def.speed = (float)speed;
    def.maxHp = (int)maxHp;
    def.luck = (float)luck;
    snprintf(def.palette, sizeof(def.palette), "%s", palette);

    /* Seconda rete di clamp (spec, punto (d)): riporta OGNI numero dentro
       banda qualunque cosa dica il file, prima ancora di costruire la
       CharacterDef che il gioco applica davvero -- la prima rete e' in
       melting-gen, PRIMA di scrivere il json (vedi character_type.h). */
    CharacterGenDefClamp(&def);

    memset(out, 0, sizeof(*out));
    snprintf(out->name, sizeof(out->name), "%s", def.name);
    /* M6b-1, requisito 2: l'etichetta di origine e' il campo 'role' stesso
       -- "FORGED THIS RUN" al posto di "Explorer"/"Offensive"/"Defensive" --
       cosi' DrawCharacterCards (src/render/game_renderer.c) la disegna
       ESATTAMENTE come disegna il ruolo di ogni altra carta, senza bisogno
       di un secondo ramo di disegno, e il segnale di origine e' testo
       leggibile, mai affidato al solo colore (DEC-058). */
    snprintf(out->role, sizeof(out->role), "FORGED THIS RUN");
    snprintf(out->blurb, sizeof(out->blurb), "%s", def.blurb);
    out->baseDamage = def.damage;
    out->baseFireDelay = def.fireDelay;
    out->baseShotSpeed = def.shotSpeed;
    /* baseShotRadius: fuori scope di questa fetta (spec, sezione "Contesto e
       confini della fetta") -- resta il default storico del colpo base,
       come ogni personaggio della rosa curata (character_roster.c). */
    out->baseShotRadius = 5.0f;
    out->baseSpeed = def.speed;
    out->baseMaxHp = def.maxHp;
    out->hpCap = def.hpCap;
    out->baseLuck = def.luck;
    out->palette = ParseHexColor(def.palette, WHITE);

    /* M6b-2 (DEC-037): campo "lua" OPZIONALE (assente su una proposta di
     * un'M6b-1 vecchia, o su un file forgiato a mano senza trait) -- a
     * differenza di name/blurb/stats/palette sopra, la sua assenza NON
     * invalida l'intera proposta ('ok' resta quello gia' deciso sopra): un
     * personaggio generato senza trait e' comunque un personaggio valido
     * (le stats/la carta restano), semplicemente senza quella riga in piu'.
     * Se il campo dice true, si prova a leggere/scandire il file vero
     * (DetectTraitHook sopra): un "lua":true con il file assente o
     * illeggibile (caso anomalo esplicito della spec M6b-2) lascia
     * semplicemente traitHook vuoto, mai un fallimento di QUESTA funzione --
     * il caricamento VERO (script_character.c) e' quello che decide davvero
     * se il trait gira, e gia' fallisce in silenzio da solo. */
    out->traitHook[0] = '\0';
    if (hasLua) DetectTraitHook(out->traitHook, sizeof(out->traitHook));

    return true;
}
