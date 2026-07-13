-- Ignora shot_id/enemy_id ricevuti da on_hit e si inventa un handle a caso:
-- deve fallire la validazione dell'handle (indice/generazione), sia contro
-- l'API vera del gioco (script_api.c) sia contro lo stub di melting-gen.
function on_hit(shot_id, enemy_id)
  damage_enemy(999999, 10)
end
