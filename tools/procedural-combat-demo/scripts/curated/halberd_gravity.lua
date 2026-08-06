-- Sinergia player: l'alabarda descrive un arco fisico, cattura cio' che
-- attraversa il campo gravitazionale e rilascia echi nella mira congelata.
-- Tutta la sequenza temporale e' qui; il C applica collisioni e limiti.
halberd_phase = 0
halberd_timer = 0
halberd_locked_aim = 0

function on_tick(dt, self_handle)
  halberd_timer = halberd_timer + dt

  if halberd_phase == 0 then
    if halberd_timer >= 1.20 then
      halberd_phase = 1
      halberd_timer = 0
      halberd_locked_aim = aim_snapshot()
      telegraph_arc(self_x(), self_y(), halberd_locked_aim, 112, 24, 2.35, 0.34, VIS_GRAVITY)
    end
  elseif halberd_phase == 1 then
    if halberd_timer >= 0.34 then
      melee_sweep(self_handle, self_x(), self_y(), halberd_locked_aim, 112, 24, 2.35, 12, 0.18, VIS_GRAVITY)
      capture_radius(self_handle, self_x(), self_y(), 132, 2.2, 8, 0.46, VIS_GRAVITY)
      add_status(self_handle, STATUS_GRAVITY, 0.35, 0.46)
      halberd_phase = 2
      halberd_timer = 0
    end
  else
    if halberd_timer >= 0.46 then
      release_echoes(self_handle, self_x(), self_y(), halberd_locked_aim, 5, 360, 5, 1.15, 1.6, VIS_VOID_ECHO)
      halberd_phase = 0
      halberd_timer = 0
    end
  end
end
