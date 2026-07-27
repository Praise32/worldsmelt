#ifndef MELTING_RUN_ITEM_POOL_H
#define MELTING_RUN_ITEM_POOL_H

#include "core/game_types.h"

/* Estrazione dai pool di oggetti (docs/design/systems/items-pools-and-
 * rarity.md, "Pool" + "Rarita' e peso" + "Correzione di fortuna"): pesi di
 * rarita' DEC-019, garanzia di copertura del pool curato minimo DEC-144,
 * correzione di fortuna con soglia N ridotta dalla statistica Fortuna
 * DEC-145. Prefisso 'ItemPool' come vuole AGENTS.md (niente simboli globali
 * generici); modulo gemello di item_slots.h/item_traits.h in questa stessa
 * cartella.
 *
 * Tutto qui dentro e' PURO rispetto all'RNG: ogni funzione che estrae
 * riceve 'unsigned int *rng' e usa solo GameRngNext/GameRngRange
 * (core/game_math.h) -- MAI time()/rand() globali. Chi chiama passa sempre
 * '&game->rng' (l'RNG di gameplay derivato dal seed di run, DEC-141): e'
 * cosi' che l'intero sistema resta deterministico dal seed, requisito
 * esplicito di questo modulo. */

#define ITEM_POOL_RARITY_COUNT 4   /* RARITY_COMMON..RARITY_LEGENDARY, core/game_types.h */

/* Pesi standard di rarita' (DEC-019, sezione "Rarita' e peso"): comune 55,
 * non-comune 30, rara 12, leggendaria 3. Draft di bilanciamento (il
 * documento lo dichiara esplicitamente), ma la FORMA della tabella --
 * quattro pesi indicizzati come l'enum Rarity -- e' quella che questo
 * modulo assume ovunque. */
extern const int ItemPoolWeightsStandard[ITEM_POOL_RARITY_COUNT];

/* Pesi del pool boss (DEC-019): 0, 0, 70, 30 -- MAI comune o non-comune per
 * costruzione. E' la ragione per cui la correzione di fortuna (sotto) resta
 * definita ma superflua su questo pool, vedi il commento su
 * ItemPoolLuckCorrectionActive. */
extern const int ItemPoolWeightsBoss[ITEM_POOL_RARITY_COUNT];

/* DEC-144 -- "pool curato minimo: almeno un oggetto per rarita'". Dato un
 * pool di 'poolSize' oggetti e una tabella di pesi, calcola quanti oggetti
 * di CIASCUNA rarita' quel pool dovrebbe contenere: si applicano i pesi per
 * proporzione (arrotondamento per difetto), poi ogni rarita' con peso > 0
 * che risulterebbe a zero per arrotondamento viene portata a 1 -- MAI una
 * rarita' con peso 0 (il pool boss non deve mai "guadagnare" una copia
 * comune/non-comune solo perche' il conteggio e' vuoto: quello e' vuoto per
 * costruzione, non per arrotondamento). L'eccedenza necessaria per la
 * garanzia si toglie prima dal residuo di arrotondamento, poi -- se non
 * basta -- dalle rarita' PIU' comuni (si parte dalla comune e si risale):
 * il totale di 'outCounts' resta SEMPRE 'poolSize', mai un oggetto in piu'
 * o in meno (DEC-087).
 *
 * Esempio normativo del documento: poolSize=20, pesi standard ->
 * {11, 6, 2, 1}. Verificato esattamente da questa funzione (vedi
 * GameItemPoolTest, src/tests/game_tests.c). Con un pool piccolo (es. i tre
 * oggetti di un piano, vedi run_content.c) la comune puo' arrivare a 0: e'
 * la stessa regola applicata a un caso piu' stretto, non un ramo diverso. */
void ItemPoolMinimumCounts(int poolSize, const int weights[ITEM_POOL_RARITY_COUNT], int outCounts[ITEM_POOL_RARITY_COUNT]);

/* DEC-019 -- tira UNA rarita' pesata secondo 'weights' (indicizzata come
 * l'enum Rarity). Le rarita' a peso 0 non possono mai uscire. Se tutti i
 * pesi sono <= 0 (tabella malformata) ricade su RARITY_COMMON: un difetto
 * di contenuto non deve mai bloccare l'estrazione ne' regalare una rarita'
 * alta per sbaglio (stessa filosofia di RarityFromText, src/content/
 * run_content.c). */
Rarity ItemPoolRollRarity(unsigned int *rng, const int weights[ITEM_POOL_RARITY_COUNT]);

/* DEC-145 -- soglia N di estrazioni comuni consecutive prima che la
 * correzione di fortuna scatti. N_BASE e il divisore sono DEFAULT PROPOSTI
 * DALL'IMPLEMENTAZIONE (stile DEC-019: valori di playtest, non ancora
 * decisi dal documento di design, vedi "Domande aperte residue" in
 * items-pools-and-rarity.md): N base 4, ridotta di 1 ogni 4 punti di
 * Fortuna, clampata a un MINIMO di 1 (richiesto dal documento: "la
 * riduzione non elimina mai del tutto la soglia") e a un MASSIMO pari alla
 * base (scelta conservativa non specificata dal documento: una Fortuna
 * negativa non deve peggiorare la sequenza sfortunata oltre il default,
 * solo una Fortuna POSITIVA la accorcia -- vedi la domanda aperta annotata
 * in scratchpad/questions-night.md). */
#define ITEM_POOL_LUCK_THRESHOLD_BASE 4
#define ITEM_POOL_LUCK_DIVISOR 4.0f
int ItemPoolLuckThreshold(float luck);

/* Vero se 'streak' (estrazioni comuni consecutive gia' viste da questo
 * pool) ha raggiunto la soglia ridotta dalla Fortuna: la PROSSIMA
 * estrazione da questo pool deve garantire almeno non-comune. Sul pool
 * boss questa condizione non puo' mai diventare vera A CAUSA della
 * correzione stessa (non contiene comuni da cui far crescere 'streak',
 * pesi {0,0,70,30}): la correzione resta definita anche li' (questa stessa
 * funzione la calcolerebbe) ma vi si applica solo in teoria, la garanzia
 * strutturale del pool la rende gia' superflua in pratica (DEC-145, nota
 * sul pool boss). */
bool ItemPoolLuckCorrectionActive(int streak, float luck);

/* Estrazione + correzione di fortuna su un pool di CANDIDATI GIA' ESISTENTI
 * (i tre oggetti di un piano, gli oggetti di un negozio, ecc.): 'rarities' e'
 * la rarita' di ciascun candidato (count elementi), gia' assegnata a monte
 * secondo i pesi DEC-019 da chi ha generato quei candidati -- melting-gen
 * (GenRollRarity, stessa tabella) per una run vera, ItemPoolMinimumCounts
 * per il contenuto di ripiego. La scelta FRA i candidati e' percio' sempre
 * UNIFORME, mai pesata di nuovo per rarita': applicare i pesi una seconda
 * volta qui equivarrebbe a elevarli al quadrato (rara e leggendaria
 * dimezzate o quasi azzerate, regressione misurata su 2.000.000 di
 * estrazioni prima di questa correzione), perche' i candidati che
 * condividono una rarita' piu' comune ricadrebbero MULTIPLE volte nella
 * stessa somma pesata. Una scelta uniforme fra k campioni gia' distribuiti
 * secondo i pesi standard riproduce quei pesi ESATTAMENTE sul candidato
 * estratto (fatto di probabilita', non un'approssimazione): e' cosi' che
 * DEC-019 resta rispettato "salvo intervento della correzione di fortuna"
 * (items-pools-and-rarity.md, Scenario 1). 'weights' resta un parametro
 * SOLO per un filtro di ammissione difensivo (un candidato la cui rarita'
 * ha peso <= 0 nella tabella passata non e' mai scelto, a meno che
 * escluderlo lasci l'insieme vuoto) -- mai per pesare la scelta.
 *
 * Se la correzione di fortuna e' attiva (vedi sopra) E almeno un candidato
 * ha rarita' > RARITY_COMMON, l'estrazione si limita a quei candidati
 * (garanzia soddisfatta); altrimenti procede come un'estrazione uniforme
 * normale (degrado dichiarato: "senza garantire sempre la soluzione
 * perfetta", items-pools-and-rarity.md). Aggiorna '*streak' in base alla
 * rarita' REALMENTE estratta (incrementa su comune, azzera altrimenti) --
 * riflette cio' che e' successo davvero, non la promessa della
 * correzione. Ritorna l'indice estratto in [0, count), o -1 se count <= 0. */
int ItemPoolDrawIndex(unsigned int *rng, const Rarity *rarities, int count,
                       const int weights[ITEM_POOL_RARITY_COUNT], float luck, int *streak);

#endif
