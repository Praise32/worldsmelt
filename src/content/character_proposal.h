#ifndef MELTING_RUN_CHARACTER_PROPOSAL_H
#define MELTING_RUN_CHARACTER_PROPOSAL_H

#include "core/game_types.h"

/* M6b-1 (DEC-014, prima fetta): legge generated/character_proposal.json,
 * scritto da melting-gen dentro la STESSA sessione modello di
 * --propose-themes (tools/melting-gen/main.c, RunProposeThemes -- mai un
 * secondo caricamento, vedi il commento su AppStopLazyGeneration in
 * src/app/app.c). Scanner a mano stile AppLoadThemeCards (src/app/app.c):
 * il gioco non linka mai cJSON (AGENTS.md). Vive in src/content (non in
 * app.c come AppLoadThemeCards) perche' produce direttamente una
 * CharacterDef "generata", pronta per GamePlayerResetBaseStatsFor -- non
 * una struttura-ponte intermedia che qualcun altro deve ancora interpretare.
 *
 * Seconda rete di clamp (la prima e' in melting-gen, PRIMA di scrivere il
 * file: CharacterGenDefClamp, src/core/character_type.h): un
 * character_proposal.json forgiato a mano fuori banda -- o un bug futuro
 * nella prima rete -- non puo' MAI produrre una CharacterDef fuori banda
 * qui, perche' la stessa funzione di clamp gira una seconda volta qui
 * dentro, idempotente.
 *
 * Ritorna false (contenuto di '*out' non garantito) su file assente, JSON
 * malformato, o un campo mancante/del tipo sbagliato: il chiamante
 * (src/app/app.c) tratta questo ESATTAMENTE come "nessuna proposta per
 * questa run" -- il fallback canonico del personaggio generato e' l'ASSENZA
 * della carta (characters.md, "Fallback"), mai un personaggio curato di
 * riserva. */
bool RunContentLoadCharacterProposal(const char *path, CharacterDef *out);

#endif
