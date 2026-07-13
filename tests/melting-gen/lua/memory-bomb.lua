-- Alloca tabelle senza fine nel corpo di primo livello (budget di istruzioni
-- generoso, SCRIPT_SANDBOX_LOAD_BUDGET): deve sforare il tetto di memoria
-- della sandbox (SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP) prima di finire il ciclo.
local bomb = {}
local i = 0
while true do
  i = i + 1
  bomb[i] = { i, i, i, i, i, i, i, i }
end
