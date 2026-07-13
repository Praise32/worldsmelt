-- 'io' non e' mai installato nell'_ENV della sandbox (niente libreria io):
-- indicizzarlo solleva "attempt to index a nil value" a runtime, dentro la
-- callback che il dry-run chiama davvero.
function on_fire(x, y, dx, dy)
  local f = io.open("/tmp/melting-run-pwned", "w")
  if f then
    f:write("non dovrebbe mai arrivare qui")
    f:close()
  end
end
