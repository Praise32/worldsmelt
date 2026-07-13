-- Ogni terzo colpo si sdoppia con un frammento che insegue il nemico piu' vicino.
-- Usa solo l'API documentata (cheat-sheet in prompts/lua_system.txt).
shot_count = 0

function on_fire(x, y, dx, dy)
  shot_count = shot_count + 1
  if shot_count % 3 == 0 then
    local id = nearest_enemy(x, y)
    if id ~= nil then
      local ex, ey = enemy_x(id), enemy_y(id)
      local hx, hy = ex - x, ey - y
      local len = math.sqrt(hx*hx + hy*hy)
      if len > 0.0001 then
        hx = hx/len
        hy = hy/len
        spawn_shot(x, y, hx, hy, 380, 3, 4, TRAIT_HOMING)
      end
    end
  end
end
