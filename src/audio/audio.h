#ifndef MELTING_RUN_AUDIO_H
#define MELTING_RUN_AUDIO_H

#include "core/game_types.h"

/* Modulo audio del motore (DEC-172): legge SOLO il pacchetto pre-generato
   offline di assets/audio/ (musica/ambience con Stable Audio 3 Small, SFX
   con rFXGen/ripiego, vedi assets/audio/README.md) -- nessun modello si
   carica mai a runtime, coerente con AGENTS.md ("il motore C indipendente
   da rete, chiavi API e modelli AI"). manifest.json e' SOLO per gli umani
   (regola AGENTS.md, "niente parsing JSON nel motore"): i percorsi dei file
   sono una TABELLA STATICA in audio.c, non letti da li'.

   Fallback silenzioso TOTALE, garantito dal design (docs/design/content/
   audio-and-feedback.md, "il gioco DEVE funzionare identico se il device
   audio manca o i file mancano"): dispositivo assente, singolo file
   mancante/rotto, o AudioInit mai chiamata affatto rendono ogni funzione
   qui sotto un no-op sicuro, mai un crash -- vedi AudioSelfTest
   (src/tests/audio_tests.c, --audio-test) per gli scenari verificati
   davvero, headless compreso (make test gira sotto Xvfb, senza backend
   audio reale). */

/* Tracce musicali in streaming, una per stato applicativo con un proprio
   tema (docs/design/content/audio-and-feedback.md, DEC-172): MainMenu e
   RunSetup condividono la stessa (nessuno dei due ha un tema proprio nel
   documento), Gameplay ne ha due (piano iniziale/avanzato, asse "audio"
   dell'escalation DEC-024) piu' la stanza boss dedicata. PauseMenu/Options/
   BuildScreen/ExitConfirm sono overlay senza traccia propria: vedi
   AudioTrackForState sotto. */
typedef enum AudioTrack {
    AUDIO_TRACK_MAIN_MENU,
    AUDIO_TRACK_FLOOR_ZERO,
    AUDIO_TRACK_GAMEPLAY_1,
    AUDIO_TRACK_GAMEPLAY_2,
    AUDIO_TRACK_BOSS,
    AUDIO_TRACK_RESULTS,
    AUDIO_TRACK_COUNT
} AudioTrack;

/* Effetti a evento (stesso documento, "Eventi prioritari"): un ID per file
   del pacchetto in assets/audio/sfx/. Vedi i punti di aggancio reali in
   src/gameplay/combat.c, src/world/world.c, src/game/game.c e
   src/app/app.c. */
typedef enum AudioSfx {
    AUDIO_SFX_SHOT,              /* sparo del giocatore, UNA volta per colpo (non per pallettone): CombatFirePlayer */
    AUDIO_SFX_HIT_ENEMY,         /* colpo che raggiunge un nemico: CombatDamageEnemy */
    AUDIO_SFX_HIT_PLAYER,        /* danno subito dal giocatore: CombatDamagePlayer */
    AUDIO_SFX_PICKUP,            /* oggetto/valuta/Flux raccolti: CombatPickup */
    AUDIO_SFX_DOOR_OPEN,         /* porta che si apre fra stanze: WorldTryEnterRoom */
    AUDIO_SFX_FUSION_COMPLETE,   /* fusione completata (DEC-118, priorita' massima dedicata): AppFusionConfirm */
    AUDIO_SFX_DISCOVERY_CARD,    /* card di scoperta mostrata: GameQueueDiscoveryCard */
    AUDIO_SFX_UI_MOVE,           /* navigazione nei menu (frecce) */
    AUDIO_SFX_UI_CONFIRM,        /* conferma nei menu (ENTER/SPACE o click) */
    AUDIO_SFX_UI_CANCEL,         /* annulla/indietro nei menu (ESC) */
    AUDIO_SFX_COUNT
} AudioSfx;

/* Ciclo di vita (AGENTS.md, "ogni nuova responsabilita' una cartella
   dedicata"). AudioInit va chiamata DOPO InitWindow (mai prima), UNA volta
   per avvio reale del gioco; e' pero' sicura da richiamare piu' volte (i
   test la esercitano ripetutamente): riparte sempre da un ciclo di vita
   pulito. Non fallisce mai in modo osservabile dal chiamante: se
   InitAudioDevice non trova un dispositivo, il modulo resta silenziosamente
   spento (AudioIsDeviceReady() false) e ogni altra funzione diventa un
   no-op. AudioShutdown e' idempotente, sicura anche se AudioInit non e'
   mai stata chiamata. */
void AudioInit(void);
void AudioShutdown(void);

/* Vero solo se il dispositivo audio e' stato aperto con successo: SOLO per
   diagnostica/test -- nessun chiamante di gioco deve mai ramificare su
   questo valore, il fallback e' gia' silenzioso in ogni funzione sotto. */
bool AudioIsDeviceReady(void);

/* La scelta PURA di traccia da AppMode + piano + "si e' nella stanza boss":
   nessuna chiamata raylib, nessun bisogno di device -- usata da
   AudioSyncMusic e riesposta per il test di mappatura (src/tests/
   audio_tests.c). Torna -1 per gli stati overlay senza traccia propria
   (PauseMenu/Options/BuildScreen/ExitConfirm): AudioSyncMusic in quel caso
   NON cambia la musica sottostante. */
int AudioTrackForState(AppMode mode, int floor, bool bossRoom);

/* Sincronizza la musica con lo stato applicativo corrente: chiamata UNA
   volta per frame dal ciclo principale (src/app/app.c), stesso spirito di
   "UpdateMusicStream nel ciclo". Decide la traccia bersaglio con
   AudioTrackForState (game->floor + la stanza corrente, letta SOLO in
   Gameplay) e copre un cambio con un breve crossfade (default proposto,
   stile DEC-019: AUDIO_MUSIC_FADE_SECONDS in audio.c). PauseMenu abbassa la
   musica sottostante invece di cambiarla (duck, stesso default). 'dt' e' il
   passo di frame GIA' calcolato/clampato dal chiamante (mai un nuovo
   GetFrameTime qui: un solo orologio di frame per il gioco intero). */
void AudioSyncMusic(const Game *game, AppMode mode, float dt);

/* Effetto a evento: no-op sicuro se il device manca o il file non ha
   caricato. Ogni AudioSfx pesca da un piccolo pool di alias (LoadSoundAlias)
   cosi' puo' sovrapporsi a se stesso nello stesso frame (es. piu' pickup in
   fila) senza ritagliarsi a meta' -- vedi il commento sul pool in audio.c. */
void AudioPlaySfx(AudioSfx sfx);

/* Volumi 0..1 (clampati qui, mai fuori banda). Master moltiplica sia
   musica sia SFX; i tre canali restano indipendenti. Default: tutti a 1.0
   (questo modulo non li persiste da solo -- lo stato qui sotto e' solo di
   PROCESSO, azzerato ad ogni AudioInit/riavvio del binario).
   Da W8 Options li espone come tre righe-slider (APP_OPTIONS, src/app/app.c e
   DrawOptionsOverlay in src/render/game_renderer.c) a passi di
   OPTIONS_VOLUME_STEP.
   PERSISTONO fra un avvio e l'altro DA DEC-189/190 (WP-PREFS): src/app/app.c
   (AppRun) li carica da prefs/settings.txt (src/app/prefs.c) subito dopo
   AudioInit chiamando i tre setter sotto, e li risalva all'uscita da Options
   e alla chiusura del gioco. Questo modulo non sa nulla del file -- resta
   proprietario solo del VALORE in memoria e del suo clamp, come sempre;
   la persistenza e' un livello sopra, di proposito (src/app possiede i file,
   non src/audio). */

/* Passo e numero di caselle degli slider di Options (default proposto, stile
   DEC-019: il documento elenca "audio" fra le categorie minime senza fissare
   ne' passo ne' etichette). Dieci caselle da 10%: abbastanza fine per
   accordare, abbastanza grossa perche' una freccia sia un cambiamento
   udibile. Stanno qui e non nel renderer perche' il passo e' una proprieta'
   del canale audio, e chi disegna e chi modifica devono leggere lo stesso
   numero. */
#define OPTIONS_VOLUME_CELLS 10
#define OPTIONS_VOLUME_STEP (1.0f/(float)OPTIONS_VOLUME_CELLS)
void AudioSetMasterVolume(float volume);
void AudioSetMusicVolume(float volume);
void AudioSetSfxVolume(float volume);
float AudioGetMasterVolume(void);
float AudioGetMusicVolume(void);
float AudioGetSfxVolume(void);

#endif
