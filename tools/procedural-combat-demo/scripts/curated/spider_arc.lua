-- Ragno falciante: lo slash e' un arco di collisione vero, non una texture.
-- La mira viene congelata durante il preavviso: il giocatore puo' schivare.
spider_phase = 0
spider_timer = 0
spider_locked_aim = 0

function on_tick(dt, self_handle)
  spider_timer = spider_timer + dt

  if spider_phase == 0 then
    local a = aim_at_player()
    set_velocity(self_handle, math.cos(a) * 34, math.sin(a) * 34)
    if spider_timer >= 1.05 then
      spider_phase = 1
      spider_timer = 0
      spider_locked_aim = aim_at_player()
      set_velocity(self_handle, 0, 0)
      telegraph_arc(self_x(), self_y(), spider_locked_aim, 92, 20, 1.72, 0.46, VIS_VIOLET_CUT)
    end
  elseif spider_phase == 1 then
    set_velocity(self_handle, 0, 0)
    if spider_timer >= 0.46 then
      emit_arc(self_handle, self_x(), self_y(), spider_locked_aim, 92, 20, 1.72, 10, 0.14, VIS_VIOLET_CUT)
      spider_phase = 2
      spider_timer = 0
    end
  else
    local a = spider_locked_aim + 3.14159265
    set_velocity(self_handle, math.cos(a) * 58, math.sin(a) * 58)
    if spider_timer >= 0.38 then
      spider_phase = 0
      spider_timer = 0
    end
  end
end
