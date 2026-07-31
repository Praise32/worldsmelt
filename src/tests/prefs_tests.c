/* DEC-189/190 (docs/design/governance/decision-log.md;
   docs/design/ui/options-and-accessibility.md "Volumi"): LE PREFERENZE DEL
   GIOCATORE (prefs/settings.txt, src/app/prefs.c) -- oggi solo i tre volumi
   audio.

   Cinque blocchi:
   (a) nessun file -> PrefsLoad torna il default 1.0/1.0/1.0, senza crearlo;
   (b) andata e ritorno -- salva 0.6/0.3/0.9, ricarica, confronta -- e il
       file si crea al primo salvataggio;
   (c) file corrotto (schema estraneo), troncato o senza riga di schema ->
       sempre il default 1.0, mai un crash;
   (d) valori fuori banda anche da un file manomesso a mano (2.5 -> 1.0,
       -3.0 -> 0.0), clamp anche in SCRITTURA;
   (e) l'integrazione vera attraverso UpdateApp: i DUE punti d'ingresso di
       APP_OPTIONS (da MainMenu e da PauseMenu) prendono la propria
       istantanea, "una scrittura per visita SOLO se qualcosa e' cambiato".

   Come GameSuspendTest, gira dopo InitWindow e usa 'game' per davvero (il
   blocco (e) esercita UpdateApp) ma non disegna nulla. Il file delle
   preferenze vive in una cartella temporanea (PrefsSetTestPath): questo test
   non tocca mai prefs/ vero. */

#include "tests/game_tests.h"

#include "app/app.h"
#include "app/app_internal.h"
#include "app/prefs.h"
#include "audio/audio.h"
#include "game/game_internal.h"

#include "raylib.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#define PREFS_EPS 0.0005f

static bool g_fail = false;

#define PREFS_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GamePrefsTest: %s\n", (msg)); g_fail = true; } } while (0)

/* Copia privata di CreateTempDir (src/tests/suspend_tests.c/catalog_tests.c):
   stessa convenzione del progetto, ogni modulo di test si porta la propria. */
static char *CreateTempDir(char *pathBuf, size_t pathBufSize, const char *namePrefix)
{
    const char *base = getenv("TMPDIR");
#ifdef _WIN32
    if (!base) base = getenv("TEMP");
    if (!base) base = getenv("TMP");
    if (!base) base = ".";
    for (int attempt = 0; attempt < 64; attempt++)
    {
        snprintf(pathBuf, pathBufSize, "%s\\%s-%d-%d", base, namePrefix, rand(), attempt);
        if (_mkdir(pathBuf) == 0) return pathBuf;
    }
    return NULL;
#else
    if (!base) base = "/tmp";
    snprintf(pathBuf, pathBufSize, "%s/%s-XXXXXX", base, namePrefix);
    return mkdtemp(pathBuf);
#endif
}

static void RemoveTempDir(const char *path)
{
#ifdef _WIN32
    _rmdir(path);
#else
    rmdir(path);
#endif
}

/* Scrive un file di preferenze ARBITRARIO nel percorso di test: serve al
   blocco (c)/(d), stesso ruolo di WriteRawSuspendFile in suspend_tests.c. */
static void WriteRawPrefsFile(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    if (!f) return;
    fputs(content, f);
    fclose(f);
}

static AppInput InputNone(void)    { AppInput in = { 0 }; return in; }
static AppInput InputConfirm(void) { AppInput in = { 0 }; in.confirm = true; return in; }
static AppInput InputBack(void)    { AppInput in = { 0 }; in.back = true; return in; }
static AppInput InputRight(void)   { AppInput in = { 0 }; in.right = true; return in; }

bool GamePrefsTest(Game *game)
{
    g_fail = false;
    char dirBuf[256] = { 0 };
    char *dir = CreateTempDir(dirBuf, sizeof(dirBuf), "melting-test-prefs");
    if (!dir)
    {
        fprintf(stderr, "GamePrefsTest: impossibile creare la cartella temporanea\n");
        return false;
    }
    PrefsSetTestPath(dir);

    char path[300];
    snprintf(path, sizeof(path), "%s/settings.txt", dir);

    /* Stato audio reale salvato/ripristinato: questo test lo muta parecchio
       (blocco e), e non deve influenzare la suite che gira dopo. */
    float savedMaster = AudioGetMasterVolume();
    float savedMusic = AudioGetMusicVolume();
    float savedSfx = AudioGetSfxVolume();

    /* ---- (a) nessun file: default 1.0/1.0/1.0, MAI creato dalla sola lettura ---- */
    {
        PlayerPrefs p = { 0.0f, 0.0f, 0.0f };
        PrefsLoad(&p);
        PREFS_CHECK(p.masterVolume == 1.0f && p.musicVolume == 1.0f && p.sfxVolume == 1.0f,
                    "il default senza file non e' 1.0/1.0/1.0");
        PREFS_CHECK(!FileExists(path), "PrefsLoad ha creato un file da un semplice tentativo di lettura");
    }

    /* ---- (b) andata e ritorno, e il file si crea al primo salvataggio ---- */
    {
        PlayerPrefs p = { 0.6f, 0.3f, 0.9f };
        PREFS_CHECK(PrefsSave(&p), "PrefsSave ha rifiutato una scrittura valida");
        PREFS_CHECK(FileExists(path), "il file non esiste dopo il primo salvataggio");

        PlayerPrefs reloaded = { 0.0f, 0.0f, 0.0f };
        PrefsLoad(&reloaded);
        PREFS_CHECK(fabsf(reloaded.masterVolume - 0.6f) < PREFS_EPS &&
                    fabsf(reloaded.musicVolume - 0.3f) < PREFS_EPS &&
                    fabsf(reloaded.sfxVolume - 0.9f) < PREFS_EPS,
                    "l'andata/ritorno non torna 0.6/0.3/0.9");
    }

    /* ---- (c) file corrotto/troncato/senza schema -> sempre il default, mai un crash ---- */
    {
        WriteRawPrefsFile(path, "prefsSchema=99\nmasterVolume=0.2\nmusicVolume=0.2\nsfxVolume=0.2\n");
        PlayerPrefs p = { 0.0f, 0.0f, 0.0f };
        PrefsLoad(&p);
        PREFS_CHECK(p.masterVolume == 1.0f && p.musicVolume == 1.0f && p.sfxVolume == 1.0f,
                    "uno schema estraneo (99) non ricade sul default 1.0");

        WriteRawPrefsFile(path, "prefsSchema=1\nmasterVo");   /* troncato a meta' chiave */
        p = (PlayerPrefs){ 0.0f, 0.0f, 0.0f };
        PrefsLoad(&p);
        PREFS_CHECK(p.masterVolume == 1.0f && p.musicVolume == 1.0f && p.sfxVolume == 1.0f,
                    "un file troncato a meta' chiave (schema valido) non ricade sul default per i campi mancanti");

        WriteRawPrefsFile(path, "non e' nemmeno un formato\nchiave=valore\n");
        p = (PlayerPrefs){ 0.0f, 0.0f, 0.0f };
        PrefsLoad(&p);
        PREFS_CHECK(p.masterVolume == 1.0f && p.musicVolume == 1.0f && p.sfxVolume == 1.0f,
                    "un file senza riga di schema non ricade sul default");

        /* Schema valido, ma un VALORE non numerico ("abc"): un parser che
           confonde "non e' un numero" con "vale zero" (atof) produrrebbe
           0.0 -- gioco muto -- invece del default 1.0 promesso da un campo
           corrotto quanto uno mancante. */
        WriteRawPrefsFile(path, "prefsSchema=1\nmasterVolume=abc\nmusicVolume=1.0\nsfxVolume=1.0\n");
        p = (PlayerPrefs){ 0.0f, 0.0f, 0.0f };
        PrefsLoad(&p);
        PREFS_CHECK(p.masterVolume == 1.0f && p.musicVolume == 1.0f && p.sfxVolume == 1.0f,
                    "masterVolume=abc (valore non numerico, schema valido) non ricade sul default 1.0 (torna 0.0 come se il volume fosse muto per scelta)");
    }

    /* ---- (d) fuori banda anche da un file manomesso, clamp anche in scrittura ---- */
    {
        WriteRawPrefsFile(path, "prefsSchema=1\nmasterVolume=2.5\nmusicVolume=-3.0\nsfxVolume=0.5\n");
        PlayerPrefs p = { 0.0f, 0.0f, 0.0f };
        PrefsLoad(&p);
        PREFS_CHECK(p.masterVolume == 1.0f, "masterVolume=2.5 nel file non e' stato clampato a 1.0");
        PREFS_CHECK(p.musicVolume == 0.0f, "musicVolume=-3.0 nel file non e' stato clampato a 0.0");
        PREFS_CHECK(fabsf(p.sfxVolume - 0.5f) < PREFS_EPS, "sfxVolume=0.5 (gia' in banda) e' stato alterato");

        PlayerPrefs bad = { 3.0f, -1.0f, 0.5f };
        PREFS_CHECK(PrefsSave(&bad), "PrefsSave ha rifiutato una scrittura con valori fuori banda");

        /* Il TESTO GREZZO su disco, non ripassato da PrefsLoad: PrefsLoad
           clampa ANCHE in lettura, quindi rileggere con PrefsLoad qui
           maschererebbe l'assenza del clamp in SCRITTURA (il file potrebbe
           contenere "masterVolume=3.000000" e il test non se ne accorgerebbe
           mai). Il clamp di scrittura e' una garanzia separata (prefs.h,
           "mai fidandosi di un chiamante che dimentichi di clampare") e va
           verificata leggendo esattamente cio' che PrefsSave ha scritto. */
        char *rawWritten = LoadFileText(path);
        PREFS_CHECK(rawWritten != NULL, "il file scritto da PrefsSave non e' leggibile come testo grezzo");
        if (rawWritten)
        {
            PREFS_CHECK(strstr(rawWritten, "masterVolume=1.000000") != NULL,
                        "PrefsSave non ha clampato masterVolume=3.0 a 1.0 PRIMA di scrivere su disco (testo grezzo, non ripassato da PrefsLoad)");
            PREFS_CHECK(strstr(rawWritten, "musicVolume=0.000000") != NULL,
                        "PrefsSave non ha clampato musicVolume=-1.0 a 0.0 PRIMA di scrivere su disco (testo grezzo, non ripassato da PrefsLoad)");
            UnloadFileText(rawWritten);
        }

        PlayerPrefs reloaded = { 0.0f, 0.0f, 0.0f };
        PrefsLoad(&reloaded);
        PREFS_CHECK(reloaded.masterVolume == 1.0f && reloaded.musicVolume == 0.0f,
                    "l'andata/ritorno dopo una scrittura fuori banda non torna 1.0/0.0 (clamp end-to-end)");

        /* NaN/inf (bocciatura del giudice, 31/07): strtod ACCETTA "nan" e
           "inf" consumando l'intero token (endptr non li ferma) e NaN buca
           PrefsClamp01 (NaN<0 e NaN>1 sono entrambi falsi): senza il gate
           isfinite in ReadFloat il volume uscirebbe fuori banda, PrefsSave
           riscriverebbe "nan" per sempre (il file non guarisce) e il
           confronto di AppSaveVolumePrefs (NaN != NaN) scriverebbe a ogni
           visita di Options. Qui il campo corrotto DEVE ricadere sul
           default 1.0 (mai 0.0: un file rotto non ammutolisce il gioco) e i
           campi sani DEVONO sopravvivere. isnan/isinf come controprova
           esplicita, non solo il confronto (NaN != 1.0 renderebbe il primo
           check gia' vero, ma il messaggio deve dire il PERCHE'). */
        WriteRawPrefsFile(path, "prefsSchema=1\nmasterVolume=nan\nmusicVolume=inf\nsfxVolume=0.9\n");
        PlayerPrefs poisoned = { 0.0f, 0.0f, 0.0f };
        PrefsLoad(&poisoned);
        PREFS_CHECK(!isnan(poisoned.masterVolume) && poisoned.masterVolume == 1.0f,
                    "masterVolume=nan nel file non ricade sul default 1.0 (NaN buca il clamp)");
        PREFS_CHECK(isfinite(poisoned.musicVolume) && poisoned.musicVolume == 1.0f,
                    "musicVolume=inf nel file non ricade sul default 1.0");
        PREFS_CHECK(fabsf(poisoned.sfxVolume - 0.9f) < PREFS_EPS,
                    "il campo sano sfxVolume=0.9 non sopravvive accanto a nan/inf");
        PREFS_CHECK(PrefsSave(&poisoned), "PrefsSave dopo un file avvelenato ha rifiutato la scrittura");
        char *healed = LoadFileText(path);
        PREFS_CHECK(healed != NULL, "il file riscritto dopo nan/inf non e' leggibile");
        if (healed)
        {
            PREFS_CHECK(strstr(healed, "nan") == NULL && strstr(healed, "inf") == NULL,
                        "il file NON guarisce: PrefsSave ha riscritto nan/inf su disco");
            UnloadFileText(healed);
        }
    }

    /* ---- (e) integrazione vera attraverso UpdateApp ---- */
    {
        remove(path);
        AudioSetMasterVolume(1.0f);
        AudioSetMusicVolume(1.0f);
        AudioSetSfxVolume(1.0f);

        AppGen gen = { 0 };
        AppUi ui = { 0 };
        ui.prefsEnabled = true;
        AppMode mode = APP_MAIN_MENU;

        /* (e1) MainMenu -> Opzioni (focus 2, nessuna riga "Continua" senza
           sospensione) -> ESC senza toccare nulla -> nessuna scrittura. */
        ui.focus = 2;
        AppInput confirm = InputConfirm();
        UpdateApp(game, &mode, &gen, &ui, &confirm);
        PREFS_CHECK(mode == APP_OPTIONS, "'Opzioni' da MainMenu non apre APP_OPTIONS");

        AppInput back = InputBack();
        UpdateApp(game, &mode, &gen, &ui, &back);
        PREFS_CHECK(mode == APP_MAIN_MENU, "ESC da Options non torna a MainMenu");
        PREFS_CHECK(!FileExists(path), "uscire da Options SENZA cambiare nulla ha comunque scritto il file");

        /* (e2) PauseMenu -> Opzioni (focus 3) -> ESC senza toccare nulla ->
           ancora nessuna scrittura: verifica DAVVERO la snapshot presa dal
           SECONDO punto d'ingresso, non solo quella (gia' testata sopra) del
           primo. Prima di entrare, si cambia un volume FUORI da Options (mai
           attraverso lo slider): se il secondo ingresso non prendesse una
           snapshot fresca, quella stantia di (e1) (1.0/1.0/1.0) differirebbe
           da questo nuovo valore e produrrebbe una scrittura spuria che il
           controllo sotto scoprirebbe. */
        remove(path);
        AudioSetMusicVolume(0.4f);
        mode = APP_PAUSE_MENU;
        ui.focus = 3;
        UpdateApp(game, &mode, &gen, &ui, &confirm);
        PREFS_CHECK(mode == APP_OPTIONS, "'Opzioni' da PauseMenu non apre APP_OPTIONS");
        UpdateApp(game, &mode, &gen, &ui, &back);
        PREFS_CHECK(mode == APP_PAUSE_MENU, "ESC da Options non torna a PauseMenu");
        PREFS_CHECK(!FileExists(path),
                    "uscire da Options (aperta da PauseMenu) SENZA cambiare nulla ha comunque scritto il file");

        /* (e3) PauseMenu -> Opzioni -> alza il volume generale -> ESC ->
           ORA il file deve esistere coi valori applicati. */
        AudioSetMasterVolume(0.5f);
        mode = APP_PAUSE_MENU;
        ui.focus = 3;
        UpdateApp(game, &mode, &gen, &ui, &confirm);
        PREFS_CHECK(mode == APP_OPTIONS, "il secondo ingresso in Options da PauseMenu e' fallito");
        ui.focus = 0;   /* riga "Volume generale" */
        AppInput right = InputRight();
        UpdateApp(game, &mode, &gen, &ui, &right);
        PREFS_CHECK(AudioGetMasterVolume() > 0.5f + PREFS_EPS,
                    "DESTRA sulla riga 'Volume generale' non ha davvero alzato il volume (precondizione)");
        UpdateApp(game, &mode, &gen, &ui, &back);
        PREFS_CHECK(mode == APP_PAUSE_MENU, "ESC dopo una modifica non torna a PauseMenu");
        PREFS_CHECK(FileExists(path), "uscire da Options DOPO aver cambiato un volume non ha scritto il file");

        PlayerPrefs written = { 0.0f, 0.0f, 0.0f };
        PrefsLoad(&written);
        PREFS_CHECK(fabsf(written.masterVolume - AudioGetMasterVolume()) < PREFS_EPS,
                    "il file scritto non riflette il volume generale applicato");
        PREFS_CHECK(fabsf(written.musicVolume - AudioGetMusicVolume()) < PREFS_EPS &&
                    fabsf(written.sfxVolume - AudioGetSfxVolume()) < PREFS_EPS,
                    "il file scritto non riflette musica/effetti (mai toccati in questo blocco)");

        /* (e4) navigare SENZA mai entrare in Options non deve scrivere nulla. */
        remove(path);
        AppInput none = InputNone();
        for (int i = 0; i < 5; i++) UpdateApp(game, &mode, &gen, &ui, &none);
        PREFS_CHECK(!FileExists(path), "un giro di UpdateApp senza mai aprire Options ha scritto il file");
    }

    AudioSetMasterVolume(savedMaster);
    AudioSetMusicVolume(savedMusic);
    AudioSetSfxVolume(savedSfx);
    PrefsSetTestPath(NULL);
    RemoveTempDir(dir);

    if (!g_fail) printf("  andata/ritorno=ok | file corrotto/troncato/schema estraneo=default 1.0 | clamp [0,1] in lettura e scrittura=ok | integrazione UpdateApp (2 punti d'ingresso, scrittura solo se cambiato)=ok\n");
    else fprintf(stderr, "GamePrefsTest: FALLITO -- vedi i messaggi sopra\n");
    return !g_fail;
}
