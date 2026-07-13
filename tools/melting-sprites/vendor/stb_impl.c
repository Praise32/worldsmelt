/* Traduzione unica per le implementazioni "header-only" di stb_image e
   stb_image_write (vendorizzate, non modificate: vedi stb_image.h e
   stb_image_write.h in questa stessa cartella). Le macro *_IMPLEMENTATION
   vanno definite una volta sola in tutto il link, quindi vivono isolate
   qui: il resto del progetto include gli header senza definirle. */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
