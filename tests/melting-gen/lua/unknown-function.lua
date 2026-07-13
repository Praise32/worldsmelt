-- get_target_enemy non fa parte dell'API di gioco (non e' nel cheat-sheet):
-- e' un global inesistente, quindi nil; chiamarlo solleva
-- "attempt to call a nil value" a runtime.
function on_tick(dt)
  local id = get_target_enemy()
end
