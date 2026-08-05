-- Lumaca calligrafa: traccia una riga d'inchiostro, poi la punteggia con
-- satelliti. Lo status e la geometria sono gameplay; l'inchiostro e' visuale.
snail_phase = 0
snail_timer = 0
snail_locked_aim = 0

function on_tick(dt, self_handle)
  snail_timer = snail_timer + dt

  if snail_phase == 0 then
    local a = aim_at_player()
    set_velocity(self_handle, math.cos(a + 1.5707963) * 20, math.sin(a + 1.5707963) * 20)
    if snail_timer >= 1.35 then
      snail_phase = 1
      snail_timer = 0
      snail_locked_aim = aim_at_player()
      set_velocity(self_handle, 0, 0)
      telegraph_beam(self_x(), self_y(), snail_locked_aim, 360, 15, 0.62, VIS_CALLIGRAPHY_INK)
    end
  elseif snail_phase == 1 then
    set_velocity(self_handle, 0, 0)
    if snail_timer >= 0.62 then
      emit_beam(self_handle, self_x(), self_y(), snail_locked_aim, 360, 15, 7, 0.18, VIS_CALLIGRAPHY_INK)
      emit_orbit(self_handle, self_x(), self_y(), 5, 54, 2.2, 3, 5, 2.4, VIS_CALLIGRAPHY_INK)
      add_status(PLAYER_HANDLE, STATUS_INKED, 0.55, 1.4)
      snail_phase = 2
      snail_timer = 0
    end
  else
    if snail_timer >= 0.72 then
      snail_phase = 0
      snail_timer = 0
    end
  end
end
