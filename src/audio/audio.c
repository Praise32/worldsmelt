#include "audio/audio.h"

#include "world/world.h"

#include "raylib.h"

#include <string.h>

/* Quanti alias per AudioSfx (LoadSoundAlias, raylib): ogni alias condivide i
   dati campionati della Sound "base" ma ha il proprio cursore di
   riproduzione, cosi' un evento puo' sovrapporsi a se stesso (es. due pickup
   nello stesso frame, o i pallettoni di Sciame che colpiscono nemici diversi
   in rapida sequenza) senza che il secondo PlaySound tagli via il primo --
   PlaySound su una Sound gia' in riproduzione la riavvia da capo, non la
   accoda. 3 basta ampiamente per gli SFX brevi (<2s) di questo pacchetto: non
   e' un vero limite di polifonia globale, solo per-evento. */
#define AUDIO_SFX_POOL 3

/* Durata del crossfade fra due tracce musicali (default proposto, stile
   DEC-019: nessun documento fissa un numero, audio-and-feedback.md chiede
   solo "transizioni con breve fade"). */
#define AUDIO_MUSIC_FADE_SECONDS 0.6f

/* Moltiplicatore di volume mentre PauseMenu e' aperto (duck, default
   proposto stile DEC-019): abbassa, non silenzia -- la musica resta
   riconoscibile mentre il giocatore consulta il menu. */
#define AUDIO_MUSIC_DUCK_LEVEL 0.35f

/* Soglia di piano fra le due tracce di Gameplay (default proposto, stile
   DEC-019, asse "audio" dell'escalation DEC-024): piani 1-2 = tema
   iniziale, 3-5 = tema intensificato, verso il boss del piano 5. */
#define AUDIO_GAMEPLAY_1_MAX_FLOOR 2

/* Percorsi del pacchetto (assets/audio/, DEC-172): TABELLA STATICA in C,
   nello stesso ordine degli enum in audio.h -- manifest.json resta SOLO per
   gli umani (regola AGENTS.md, "niente parsing JSON nel motore"). */
static const char *const kMusicFiles[AUDIO_TRACK_COUNT] = {
    "assets/audio/music/main_menu.ogg",
    "assets/audio/music/floor_zero.ogg",
    "assets/audio/music/gameplay_1.ogg",
    "assets/audio/music/gameplay_2.ogg",
    "assets/audio/music/boss.ogg",
    "assets/audio/music/results.ogg"
};

static const char *const kSfxFiles[AUDIO_SFX_COUNT] = {
    "assets/audio/sfx/shot_base.ogg",
    "assets/audio/sfx/hit_enemy.ogg",
    "assets/audio/sfx/hit_player.ogg",
    "assets/audio/sfx/pickup_item.ogg",
    "assets/audio/sfx/door_open.ogg",
    "assets/audio/sfx/fusion_complete.ogg",
    "assets/audio/sfx/discovery_card.ogg",
    "assets/audio/sfx/ui_move.ogg",
    "assets/audio/sfx/ui_confirm.ogg",
    "assets/audio/sfx/ui_cancel.ogg"
};

typedef struct AudioMusicSlot {
    Music stream;
    bool loaded;   /* IsMusicValid(stream) catturato al caricamento, mai ricontrollato dopo */
} AudioMusicSlot;

typedef struct AudioSfxSlot {
    Sound base;
    Sound aliases[AUDIO_SFX_POOL];
    bool loaded;
    int nextAlias;
} AudioSfxSlot;

typedef struct AudioState {
    bool deviceReady;
    AudioMusicSlot music[AUDIO_TRACK_COUNT];
    AudioSfxSlot sfx[AUDIO_SFX_COUNT];

    /* -1 = nessuna traccia. 'previousTrack' e' valorizzata SOLO durante un
       crossfade (mai a riposo): vedi AudioStartCrossfade/AudioSyncMusic. */
    int currentTrack;
    int previousTrack;
    float fadeTimer;
    bool ducked;

    float masterVolume;
    float musicVolume;
    float sfxVolume;
} AudioState;

/* Zero-inizializzata di default (durata statica) PIU' i tre volumi a 1.0:
   cosi' AudioGetMasterVolume/... rispondono col default corretto (documento,
   "tutti a 1.0") anche se AudioInit non e' mai stata chiamata -- lo stesso
   vale per currentTrack/previousTrack a -1, mai 0 (che sarebbe una traccia
   vera). deviceReady resta false per costruzione finche' AudioInit non
   riesce: ogni altra funzione la controlla per prima, quindi lo stato
   "prima di AudioInit" e "device assente" sono lo stesso identico ramo di
   codice, mai due casi distinti da testare a parte. */
static AudioState s_audio = {
    .currentTrack = -1,
    .previousTrack = -1,
    .masterVolume = 1.0f,
    .musicVolume = 1.0f,
    .sfxVolume = 1.0f
};

static float AudioClamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

/* Riporta il modulo a "nessuna risorsa caricata", rilasciando quel che
   c'era se il device era pronto. Condivisa da AudioInit (riparte pulita
   anche se richiamata) e AudioShutdown: NON tocca i tre volumi, che sono
   una preferenza del giocatore indipendente dal ciclo di vita del device. */
static void AudioReleaseAll(void)
{
    if (s_audio.deviceReady)
    {
        for (int i = 0; i < AUDIO_TRACK_COUNT; i++)
        {
            if (s_audio.music[i].loaded) UnloadMusicStream(s_audio.music[i].stream);
        }
        for (int i = 0; i < AUDIO_SFX_COUNT; i++)
        {
            if (!s_audio.sfx[i].loaded) continue;
            for (int a = 0; a < AUDIO_SFX_POOL; a++) UnloadSoundAlias(s_audio.sfx[i].aliases[a]);
            UnloadSound(s_audio.sfx[i].base);
        }
        CloseAudioDevice();
    }
    memset(s_audio.music, 0, sizeof(s_audio.music));
    memset(s_audio.sfx, 0, sizeof(s_audio.sfx));
    s_audio.deviceReady = false;
    s_audio.currentTrack = -1;
    s_audio.previousTrack = -1;
    s_audio.fadeTimer = 0.0f;
    s_audio.ducked = false;
}

void AudioInit(void)
{
    AudioReleaseAll();   /* ciclo di vita pulito anche se AudioInit viene richiamata */

    InitAudioDevice();
    s_audio.deviceReady = IsAudioDeviceReady();
    if (!s_audio.deviceReady) return;   /* fallback silenzioso: nessun file viene nemmeno tentato (DEC-172) */

    for (int i = 0; i < AUDIO_TRACK_COUNT; i++)
    {
        Music m = LoadMusicStream(kMusicFiles[i]);
        if (!IsMusicValid(m)) continue;   /* file mancante/rotto: la traccia resta silenziosa, mai un crash */
        m.looping = true;
        s_audio.music[i].stream = m;
        s_audio.music[i].loaded = true;
    }
    for (int i = 0; i < AUDIO_SFX_COUNT; i++)
    {
        Sound base = LoadSound(kSfxFiles[i]);
        if (!IsSoundValid(base)) continue;
        AudioSfxSlot *slot = &s_audio.sfx[i];
        slot->base = base;
        for (int a = 0; a < AUDIO_SFX_POOL; a++) slot->aliases[a] = LoadSoundAlias(base);
        slot->loaded = true;
    }
}

void AudioShutdown(void)
{
    AudioReleaseAll();
}

bool AudioIsDeviceReady(void)
{
    return s_audio.deviceReady;
}

int AudioTrackForState(AppMode mode, int floor, bool bossRoom)
{
    switch (mode)
    {
        /* RunSetup non ha un tema proprio nel documento: resta sulla stessa
           traccia del menu principale da cui si accede. */
        case APP_MAIN_MENU:
        case APP_RUN_SETUP:
            return AUDIO_TRACK_MAIN_MENU;
        case APP_FLOOR_ZERO:
            return AUDIO_TRACK_FLOOR_ZERO;
        case APP_GAMEPLAY:
            if (bossRoom) return AUDIO_TRACK_BOSS;
            return (floor <= AUDIO_GAMEPLAY_1_MAX_FLOOR) ? AUDIO_TRACK_GAMEPLAY_1 : AUDIO_TRACK_GAMEPLAY_2;
        case APP_RUN_RESULTS:
            return AUDIO_TRACK_RESULTS;
        /* Overlay senza traccia propria: la musica sottostante continua
           (vedi il commento su AudioSyncMusic in audio.h). Elencati uno per
           uno (niente 'default') cosi' un decimo AppMode futuro sia un
           -Wswitch, non un silenzio -- stessa disciplina di UpdateApp
           (src/app/app.c). */
        case APP_PAUSE_MENU:
        case APP_OPTIONS:
        case APP_BUILD_SCREEN:
        case APP_EXIT_CONFIRM:
            return -1;
    }
    return -1;
}

/* Avvia il passaggio a 'target': la traccia corrente (se c'e') diventa
   quella in dissolvenza uscente e riparte da capo il timer di fade. Se un
   secondo cambio arriva prima che il primo fade sia finito, la traccia
   uscente STALE si ferma di scatto (mai tre tracce vive insieme): caso raro
   (richiederebbe un cambio di stato piu' veloce del fade stesso), il costo
   e' un piccolo scatto invece di generalizzare a N tracce per uno scenario
   che nessuno stato reale del gioco produce. */
static void AudioStartCrossfade(int target)
{
    if (s_audio.previousTrack >= 0)
    {
        AudioMusicSlot *stale = &s_audio.music[s_audio.previousTrack];
        if (stale->loaded) StopMusicStream(stale->stream);
    }
    s_audio.previousTrack = s_audio.currentTrack;
    s_audio.fadeTimer = 0.0f;
    s_audio.currentTrack = target;

    AudioMusicSlot *next = &s_audio.music[target];
    if (next->loaded)
    {
        StopMusicStream(next->stream);   /* riparte dall'inizio, mai a meta' di una posizione lasciata da un giro precedente */
        PlayMusicStream(next->stream);
    }
}

void AudioSyncMusic(const Game *game, AppMode mode, float dt)
{
    if (!s_audio.deviceReady) return;

    int floor = 0;
    bool bossRoom = false;
    if (game != NULL)
    {
        floor = game->floor;
        /* La stanza corrente si legge SOLO in Gameplay: negli altri stati
           game->roomX/roomY non descrivono necessariamente una stanza di
           combattimento vera (es. FloorZero e' sempre floor==0, hub). */
        if (mode == APP_GAMEPLAY) bossRoom = GameCurrentRoom(game)->kind == ROOM_BOSS;
    }

    int target = AudioTrackForState(mode, floor, bossRoom);
    if (target >= 0 && target != s_audio.currentTrack) AudioStartCrossfade(target);

    s_audio.ducked = (mode == APP_PAUSE_MENU);

    if (dt < 0.0f) dt = 0.0f;
    if (s_audio.previousTrack >= 0)
    {
        s_audio.fadeTimer += dt;
        if (s_audio.fadeTimer >= AUDIO_MUSIC_FADE_SECONDS)
        {
            AudioMusicSlot *prev = &s_audio.music[s_audio.previousTrack];
            if (prev->loaded) StopMusicStream(prev->stream);
            s_audio.previousTrack = -1;
        }
    }

    float t = (s_audio.previousTrack >= 0) ? AudioClamp01(s_audio.fadeTimer/AUDIO_MUSIC_FADE_SECONDS) : 1.0f;
    float duckMul = s_audio.ducked ? AUDIO_MUSIC_DUCK_LEVEL : 1.0f;
    float baseVolume = AudioClamp01(s_audio.masterVolume*s_audio.musicVolume)*duckMul;

    if (s_audio.currentTrack >= 0)
    {
        AudioMusicSlot *cur = &s_audio.music[s_audio.currentTrack];
        if (cur->loaded)
        {
            SetMusicVolume(cur->stream, baseVolume*t);
            UpdateMusicStream(cur->stream);
        }
    }
    if (s_audio.previousTrack >= 0)
    {
        AudioMusicSlot *prev = &s_audio.music[s_audio.previousTrack];
        if (prev->loaded)
        {
            SetMusicVolume(prev->stream, baseVolume*(1.0f - t));
            UpdateMusicStream(prev->stream);
        }
    }
}

void AudioPlaySfx(AudioSfx sfx)
{
    if (!s_audio.deviceReady) return;
    if (sfx < 0 || sfx >= AUDIO_SFX_COUNT) return;
    AudioSfxSlot *slot = &s_audio.sfx[sfx];
    if (!slot->loaded) return;

    Sound voice = slot->aliases[slot->nextAlias];
    slot->nextAlias = (slot->nextAlias + 1)%AUDIO_SFX_POOL;
    SetSoundVolume(voice, AudioClamp01(s_audio.masterVolume*s_audio.sfxVolume));
    PlaySound(voice);
}

void AudioSetMasterVolume(float volume) { s_audio.masterVolume = AudioClamp01(volume); }
void AudioSetMusicVolume(float volume)  { s_audio.musicVolume  = AudioClamp01(volume); }
void AudioSetSfxVolume(float volume)    { s_audio.sfxVolume    = AudioClamp01(volume); }
float AudioGetMasterVolume(void) { return s_audio.masterVolume; }
float AudioGetMusicVolume(void)  { return s_audio.musicVolume; }
float AudioGetSfxVolume(void)    { return s_audio.sfxVolume; }
