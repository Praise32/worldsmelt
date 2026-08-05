-- Falena di vetro: due telegraph incrociati annunciano una rosa di schegge.
-- La traiettoria e' orbitale, non un'animazione codificata nello sprite.
moth_phase = 0
moth_timer = 0
moth_locked_aim = 0

function on_tick(dt, self_handle)
  moth_timer = moth_timer + dt

  if moth_phase == 0 then
    local a = aim_at_player()
    set_velocity(self_handle, math.cos(a - 1.5707963) * 46, math.sin(a - 1.5707963) * 46)
    if moth_timer >= 1.10 then
      moth_phase = 1
      moth_timer = 0
      moth_locked_aim = aim_at_player()
      telegraph_beam(self_x(), self_y(), moth_locked_aim, 270, 8, 0.48, VIS_GLASS_PRISM)
      telegraph_beam(self_x(), self_y(), moth_locked_aim + 1.5707963, 270, 8, 0.48, VIS_GLASS_PRISM)
    end
  elseif moth_phase == 1 then
    set_velocity(self_handle, 0, 0)
    if moth_timer >= 0.48 then
      emit_ring(self_handle, self_x(), self_y(), 10, 235, 4, 4, 2.8, VIS_GLASS_PRISM)
      emit_orbit(self_handle, self_x(), self_y(), 3, 38, -3.6, 2, 4, 1.25, VIS_GLASS_PRISM)
      add_status(PLAYER_HANDLE, STATUS_GLASS_MARK, 0.45, 1.1)
      moth_phase = 2
      moth_timer = 0
    end
  else
    if moth_timer >= 0.58 then
      moth_phase = 0
      moth_timer = 0
    end
  end
end
