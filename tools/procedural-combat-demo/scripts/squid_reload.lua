-- Ricarica inventata: il "caricatore" non deve esistere nello sprite.
-- La seppia raccoglie proiettili/energia in orbita, li comprime e li rilascia
-- come munizioni-eco lungo la mira che il giocatore aveva all'inizio.
reload_phase = 0
reload_timer = 0
reload_locked_aim = 0

function on_tick(dt, self_handle)
  reload_timer = reload_timer + dt

  if reload_phase == 0 then
    if reload_timer >= 1.65 then
      reload_phase = 1
      reload_timer = 0
      reload_locked_aim = aim_snapshot()
      capture_radius(self_handle, self_x(), self_y(), 118, 1.8, 7, 0.72, VIS_RELOAD_ORBIT)
      emit_orbit(self_handle, self_x(), self_y(), 7, 76, 3.0, 0, 4, 0.72, VIS_RELOAD_ORBIT)
    end
  elseif reload_phase == 1 then
    if reload_timer >= 0.72 then
      reload_phase = 2
      reload_timer = 0
      emit_orbit(self_handle, self_x(), self_y(), 7, 38, 6.2, 0, 5, 0.26, VIS_RELOAD_ORBIT)
      telegraph_beam(self_x(), self_y(), reload_locked_aim, 240, 10, 0.26, VIS_VOID_ECHO)
    end
  else
    if reload_timer >= 0.26 then
      release_echoes(self_handle, self_x(), self_y(), reload_locked_aim, 7, 440, 4, 0.82, 1.45, VIS_VOID_ECHO)
      reload_phase = 0
      reload_timer = 0
    end
  end
end
