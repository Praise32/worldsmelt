-- Ciclo che non ritorna mai dentro una callback di frame: deve sforare il
-- budget di istruzioni stretto (SCRIPT_SANDBOX_FRAME_BUDGET) ed essere ucciso.
function on_tick(dt)
  while true do
    dt = dt + 0
  end
end
