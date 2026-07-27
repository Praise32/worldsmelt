/* Test di DEC-172 (docs/design/content/audio-and-feedback.md,
   docs/engineering/architecture.md): il modulo audio (src/audio/audio.c)
   deve restare un fallback silenzioso TOTALE -- mai un crash -- sia quando
   il dispositivo audio manca (il caso REALE di questa suite: gira sotto
   Xvfb, senza alcun backend audio, esattamente come make test) sia PRIMA
   che AudioInit venga mai chiamata. Come GameEconomyTest/GameDiscoveryTest
   (src/tests/game_tests.c), gira dopo InitWindow e usa 'game' per davvero,
   ma non disegna nulla. */

#include "tests/game_tests.h"

#include "audio/audio.h"
#include "game/game.h"

#include <stdio.h>

/* (1) Mappatura PURA stato->traccia (nessun device, nessuna chiamata
   raylib): la fonte di verita' che AudioSyncMusic usa davvero. */
static bool AudioTestTrackMapping(void)
{
    bool ok = true;

    if (AudioTrackForState(APP_MAIN_MENU, 0, false) != AUDIO_TRACK_MAIN_MENU)
    {
        fprintf(stderr, "AudioTest: APP_MAIN_MENU doveva mappare su AUDIO_TRACK_MAIN_MENU\n");
        ok = false;
    }
    /* RunSetup non ha un tema proprio nel documento: condivide la traccia del menu. */
    if (AudioTrackForState(APP_RUN_SETUP, 0, false) != AUDIO_TRACK_MAIN_MENU)
    {
        fprintf(stderr, "AudioTest: APP_RUN_SETUP doveva mappare su AUDIO_TRACK_MAIN_MENU\n");
        ok = false;
    }
    if (AudioTrackForState(APP_FLOOR_ZERO, 0, false) != AUDIO_TRACK_FLOOR_ZERO)
    {
        fprintf(stderr, "AudioTest: APP_FLOOR_ZERO doveva mappare su AUDIO_TRACK_FLOOR_ZERO\n");
        ok = false;
    }
    if (AudioTrackForState(APP_GAMEPLAY, 1, false) != AUDIO_TRACK_GAMEPLAY_1)
    {
        fprintf(stderr, "AudioTest: piano 1 non-boss doveva mappare su AUDIO_TRACK_GAMEPLAY_1\n");
        ok = false;
    }
    if (AudioTrackForState(APP_GAMEPLAY, 2, false) != AUDIO_TRACK_GAMEPLAY_1)
    {
        fprintf(stderr, "AudioTest: piano 2 non-boss doveva mappare su AUDIO_TRACK_GAMEPLAY_1\n");
        ok = false;
    }
    if (AudioTrackForState(APP_GAMEPLAY, 3, false) != AUDIO_TRACK_GAMEPLAY_2)
    {
        fprintf(stderr, "AudioTest: piano 3 non-boss doveva mappare su AUDIO_TRACK_GAMEPLAY_2\n");
        ok = false;
    }
    if (AudioTrackForState(APP_GAMEPLAY, 5, false) != AUDIO_TRACK_GAMEPLAY_2)
    {
        fprintf(stderr, "AudioTest: piano 5 non-boss doveva mappare su AUDIO_TRACK_GAMEPLAY_2\n");
        ok = false;
    }
    /* La stanza boss vince SEMPRE, a qualunque piano (anche il piano 1: una
       run corta sintetica non deve mai perdere il tema boss). */
    if (AudioTrackForState(APP_GAMEPLAY, 1, true) != AUDIO_TRACK_BOSS)
    {
        fprintf(stderr, "AudioTest: la stanza boss al piano 1 doveva mappare su AUDIO_TRACK_BOSS\n");
        ok = false;
    }
    if (AudioTrackForState(APP_GAMEPLAY, 5, true) != AUDIO_TRACK_BOSS)
    {
        fprintf(stderr, "AudioTest: la stanza boss al piano 5 doveva mappare su AUDIO_TRACK_BOSS\n");
        ok = false;
    }
    if (AudioTrackForState(APP_RUN_RESULTS, 5, false) != AUDIO_TRACK_RESULTS)
    {
        fprintf(stderr, "AudioTest: APP_RUN_RESULTS doveva mappare su AUDIO_TRACK_RESULTS\n");
        ok = false;
    }
    /* Overlay senza traccia propria: -1, mai una traccia a caso. */
    AppMode overlays[] = { APP_PAUSE_MENU, APP_OPTIONS, APP_BUILD_SCREEN, APP_EXIT_CONFIRM };
    for (int i = 0; i < 4; i++)
    {
        if (AudioTrackForState(overlays[i], 3, false) != -1)
        {
            fprintf(stderr, "AudioTest: lo stato overlay #%d doveva mappare su -1 (nessun cambio traccia)\n", i);
            ok = false;
        }
    }
    return ok;
}

/* (2) Volumi: clampati in [0,1] a prescindere da cosa arriva, e il default
   e' 1.0 su tutti e tre i canali (lo stesso valore prima ancora di
   chiamare AudioInit, vedi il commento su s_audio in audio.c). */
static bool AudioTestVolumeClamp(void)
{
    bool ok = true;

    AudioSetMasterVolume(5.0f);
    if (AudioGetMasterVolume() != 1.0f) { fprintf(stderr, "AudioTest: master 5.0 doveva clampare a 1.0, e' %f\n", (double)AudioGetMasterVolume()); ok = false; }
    AudioSetMasterVolume(-2.0f);
    if (AudioGetMasterVolume() != 0.0f) { fprintf(stderr, "AudioTest: master -2.0 doveva clampare a 0.0, e' %f\n", (double)AudioGetMasterVolume()); ok = false; }
    AudioSetMusicVolume(3.0f);
    if (AudioGetMusicVolume() != 1.0f) { fprintf(stderr, "AudioTest: musica 3.0 doveva clampare a 1.0, e' %f\n", (double)AudioGetMusicVolume()); ok = false; }
    AudioSetSfxVolume(-1.0f);
    if (AudioGetSfxVolume() != 0.0f) { fprintf(stderr, "AudioTest: sfx -1.0 doveva clampare a 0.0, e' %f\n", (double)AudioGetSfxVolume()); ok = false; }

    /* Ripristina i default per non influenzare il resto della suite (SFX
       udibili solo se un device reale e' aperto, ma il volume resta uno
       stato condiviso). */
    AudioSetMasterVolume(1.0f);
    AudioSetMusicVolume(1.0f);
    AudioSetSfxVolume(1.0f);
    return ok;
}

/* Martella ogni AudioPlaySfx/AudioSyncMusic su tutti gli stati (compresa la
   stanza boss sintetica e un Game NULL): l'unica cosa che questa funzione
   verifica e' "non e' andato in crash" -- se il processo arriva alla
   printf finale, il fallback ha tenuto. Usata sia PRIMA sia DOPO AudioInit
   (vedi GameAudioTest sotto): il fallback deve essere lo STESSO in entrambi
   i casi (device assente == modulo mai inizializzato, stesso ramo interno,
   commento su s_audio in audio.c). */
static void AudioTestHammer(Game *game)
{
    for (int i = 0; i < AUDIO_SFX_COUNT; i++) AudioPlaySfx((AudioSfx)i);

    AudioSyncMusic(game, APP_MAIN_MENU, 0.016f);
    AudioSyncMusic(game, APP_RUN_SETUP, 0.016f);
    AudioSyncMusic(game, APP_FLOOR_ZERO, 0.016f);

    game->floor = 1;
    game->rooms[game->roomY][game->roomX].kind = ROOM_COMBAT;
    AudioSyncMusic(game, APP_GAMEPLAY, 0.016f);

    game->floor = 5;
    AudioSyncMusic(game, APP_GAMEPLAY, 0.7f);   /* oltre AUDIO_MUSIC_FADE_SECONDS: chiude un eventuale crossfade */

    game->rooms[game->roomY][game->roomX].kind = ROOM_BOSS;
    AudioSyncMusic(game, APP_GAMEPLAY, 0.7f);

    AudioSyncMusic(game, APP_PAUSE_MENU, 0.016f);    /* duck */
    AudioSyncMusic(game, APP_OPTIONS, 0.016f);
    AudioSyncMusic(game, APP_BUILD_SCREEN, 0.016f);
    AudioSyncMusic(game, APP_EXIT_CONFIRM, 0.016f);
    AudioSyncMusic(game, APP_RUN_RESULTS, 0.7f);

    /* Nessun Game (es. un chiamante ipotetico prima che una run esista):
       floor/stanza-boss ricadono sui default (0/false), mai un crash. */
    AudioSyncMusic(NULL, APP_GAMEPLAY, 0.016f);
    AudioSyncMusic(NULL, APP_MAIN_MENU, 0.016f);
}

bool GameAudioTest(Game *game)
{
    bool ok = true;
    const unsigned int seed = 20260728u;
    GameResetRunWithSeed(game, seed);

    bool mappingOk = AudioTestTrackMapping();
    if (!mappingOk) fprintf(stderr, "GameAudioTest: la mappatura stato->traccia e' sbagliata (vedi sopra)\n");

    bool volumeOk = AudioTestVolumeClamp();
    if (!volumeOk) fprintf(stderr, "GameAudioTest: il clamp dei volumi e' sbagliato (vedi sopra)\n");

    /* (3) PRIMA di AudioInit (o dopo un AudioShutdown): il modulo deve
       comportarsi ESATTAMENTE come con un device assente -- AudioShutdown()
       e' idempotente per contratto, sicura anche a modulo mai inizializzato. */
    AudioShutdown();
    bool readyBeforeInit = AudioIsDeviceReady();
    if (readyBeforeInit)
    {
        fprintf(stderr, "GameAudioTest: AudioIsDeviceReady doveva essere falso subito dopo AudioShutdown\n");
        ok = false;
    }
    AudioTestHammer(game);   /* nessuna asserzione: se non va in crash, il fallback ha tenuto */

    /* (4) AudioInit tentera' un device reale: in questo ambiente headless
       (make test/--audio-test girano senza backend audio, vedi Makefile,
       TEST_RUNNER/xvfb-run) e' atteso che fallisca -- ma il test NON deve
       assumerlo (una macchina con un device reale lo aprirebbe con
       successo): in ENTRAMBI i casi il martellamento successivo non deve
       mai andare in crash, ed e' l'unica cosa che si verifica qui. */
    AudioInit();
    printf("  device audio dopo AudioInit: %s\n", AudioIsDeviceReady() ? "pronto" : "assente (fallback silenzioso atteso)");
    AudioTestHammer(game);

    /* Richiamare AudioInit senza uno Shutdown esplicito prima deve restare
       sicuro (AudioInit parte gia' da un ciclo di vita pulito, vedi
       AudioReleaseAll in audio.c) -- nessun leak osservabile, nessun crash. */
    AudioInit();
    AudioTestHammer(game);

    /* Chiusura pulita, due volte di fila: AudioShutdown deve restare
       idempotente anche a modulo gia' spento. */
    AudioShutdown();
    AudioShutdown();
    if (AudioIsDeviceReady())
    {
        fprintf(stderr, "GameAudioTest: AudioIsDeviceReady doveva essere falso dopo un doppio AudioShutdown\n");
        ok = false;
    }
    AudioTestHammer(game);   /* di nuovo a modulo spento: stesso fallback di (3) */

    printf("  mappatura stato->traccia=%s | clamp volumi=%s | ciclo di vita (init/shutdown ripetuti, con/senza device, con/senza Game)=nessun crash\n",
           mappingOk ? "ok" : "NO", volumeOk ? "ok" : "NO");

    ok = ok && mappingOk && volumeOk;
    if (!ok) fprintf(stderr, "GameAudioTest: FALLITO -- vedi i messaggi sopra\n");
    return ok;
}
