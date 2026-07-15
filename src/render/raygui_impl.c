/* Unica unita' di traduzione che compila l'IMPLEMENTAZIONE di raygui (fase 4, GUI
   completa). raygui e' una libreria a singolo header: RAYGUI_IMPLEMENTATION va
   definito in ESATTAMENTE un .c del progetto, altrimenti i simboli si duplicano al
   link. Tutti gli altri file che usano i widget includono raygui.h SENZA quella
   macro (solo le dichiarazioni). Vendorizzata in deps/raygui/ (vedi -Ideps/raygui
   nel Makefile), versione 4.5.0, compatibile con la raylib 6.0 gia' linkata. */
#include "raylib.h"

/* raygui.h e' terzo-parte: i suoi warning (parametri inutilizzati, fgets senza
   controllo) non sono nostri e non vanno corretti nel vendor. Silenziati solo per
   questa TU, cosi' la nostra build resta pulita senza toccare l'header. */
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-result"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
