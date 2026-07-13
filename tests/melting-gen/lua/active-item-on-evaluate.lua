-- Uno stat-up travestito da oggetto attivo: definisce SOLO on_evaluate,
-- niente on_fire/on_hit/on_tick. Riproduce il caso reale spedito
-- dall'ultima run (floor5_item2.lua, un oggetto negozio): sintatticamente
-- valido e innocuo per la sandbox, ma un oggetto ATTIVO con on_evaluate
-- passa SOLO dal tetto GLOBALE a runtime (ScriptItemsRecomputeStats), MAI
-- dal tetto per-oggetto riservato agli ITEM_STATUP -- puo' quindi spostare
-- una statistica fino al doppio del budget di un boss reward in un colpo
-- solo. Deve essere rifiutato da GenLuaValidate in modalita' attiva
-- (statUpOnly=false, il default di --lua-check) con un errore che spiega
-- il problema, cosi' il ciclo di retry lo rimanda al modello.
function on_evaluate(stats)
  stats.damage = stats.damage * 1.5
end
