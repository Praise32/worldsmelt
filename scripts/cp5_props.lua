-- CP5 — prop del mondo (produzione di massa, stile S1, palette Fucina):
-- piedistallo (vuoto/pieno, DEC-117), porta (aperta/chiusa/bloccata),
-- pickup valuta e Flux, cassa e vaso distruttibili, roccia-ostacolo.
-- Riproducibile: aseprite -b --script scripts/cp5_props.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local blank, set, seg, ell, rows = W.blank, W.gset, W.gseg, W.gellipse, W.grows
local function D(n) return string.rep(".", n) end
local function R(ch, n) return string.rep(ch, n) end
local function frames(list, map)
  local out = {}
  for _, g in ipairs(list) do out[#out + 1] = { grid = g, map = map } end
  return out
end

-- ------------------------------------------------------------- piedistallo
local pedMap = { s=C.cenere, S=C.cen_scura, d=C.cen_nera, o=C.oro, b=C.bagliore }
local function pedestal(full, sparkle)
  local g = blank(32)
  for x = 8, 23 do set(g, x, 13, "s"); set(g, x, 14, "s"); set(g, x, 15, "S") end
  for y = 16, 24 do
    for x = 11, 20 do set(g, x, y, (x >= 19) and "d" or "S") end
  end
  for x = 9, 22 do set(g, x, 25, "s"); set(g, x, 26, "S"); set(g, x, 27, "d") end
  if full then
    for dy2 = -3, 3 do
      local half = 3 - math.abs(dy2)
      for x = 15 - half, 16 + half do set(g, x, 8 + dy2, "o") end
    end
    if sparkle then
      set(g, 12, 5, "b"); set(g, 20, 7, "b"); set(g, 15, 3, "b")
    end
  end
  return rows(g)
end

-- ------------------------------------------------------------------- porta
local doorMap = {
  f=C.bronzo, F=C.bronzo_s, v=C.bronzo_c, i=C.cen_nera, p=C.terra,
  l=C.brace, L=C.brace_s, s=C.slag_caldo,
}
local function doorFrame(state)
  local g = blank(32)
  -- stipiti e architrave
  for y = 4, 29 do
    for x = 3, 6 do set(g, x, y, (x == 3) and "v" or "f") end
    for x = 25, 28 do set(g, x, y, (x == 28) and "F" or "f") end
  end
  for x = 3, 28 do set(g, x, 4, "v"); set(g, x, 5, "f"); set(g, x, 6, "F") end
  if state == "aperta" then
    for y = 7, 29 do
      for x = 7, 24 do set(g, x, y, "i") end
    end
  else
    -- anta chiusa a doghe
    for y = 7, 29 do
      for x = 7, 24 do
        local ch = "p"
        if x == 11 or x == 17 or x == 23 then ch = "s" end
        set(g, x, y, ch)
      end
    end
    set(g, 21, 18, "v"); set(g, 21, 19, "v")     -- maniglia
    if state == "bloccata" then
      for x = 5, 26 do set(g, x, 15, "L"); set(g, x, 16, "l"); set(g, x, 17, "L") end
      for y = 12, 20 do set(g, 14, y, "l"); set(g, 15, y, "L") end
    end
  end
  return rows(g)
end

-- ------------------------------------------------------------- pickup 16px
local ingotMap = { o=C.oro, O=C.bronzo_c, w=C.oro_p }
local ing1 = {
  D(16), D(16), D(16), D(16), D(16),
  D(5).."w"..R("o",5)..D(5),
  D(4)..R("o",8)..D(4),
  D(3)..R("o",10)..D(3),
  D(3)..R("O",10)..D(3),
  D(16), D(16), D(16), D(16), D(16), D(16), D(16),
}
local ing2 = {
  D(16), D(16), D(16), D(16), D(16),
  D(5)..R("o",5).."w"..D(5),
  D(4)..R("o",8)..D(4),
  D(3)..R("o",10)..D(3),
  D(3)..R("O",10)..D(3),
  D(16), D(16), D(16), D(16), D(16), D(16), D(16),
}
local fluxMap = { l=C.fiamma, b=C.bagliore, u=C.cen_scura }
local function fluxPickup(dy)
  local g = blank(16)
  set(g, 8, 2 + dy, "l")
  for x = 7, 9 do set(g, x, 3 + dy, "l") end
  for x = 7, 9 do set(g, x, 4 + dy, "l") end
  set(g, 8, 3 + dy, "b")
  for x = 4, 12 do set(g, x, 8, "u") end
  for x = 4, 12 do set(g, x, 9, (x > 5 and x < 11) and "l" or "u") end
  for x = 5, 11 do set(g, x, 10, "u") end
  for x = 6, 10 do set(g, x, 11, "u") end
  return rows(g)
end

-- ------------------------------------------------------------ cassa e vaso
local crateMap = { p=C.terra, P=C.slag_caldo, m=C.bronzo_s, v=C.bronzo, d=C.cen_nera }
local function crate(state)
  local g = blank(32)
  if state == 0 then
    for y = 8, 27 do
      for x = 6, 25 do
        local ch = "p"
        if y == 8 or y == 27 or x == 6 or x == 25 then ch = "m"
        elseif y == 17 or y == 18 then ch = "P"
        elseif x == 15 or x == 16 then ch = "P" end
        set(g, x, y, ch)
      end
    end
    for _, q in ipairs({ {7,9},{24,9},{7,26},{24,26} }) do
      set(g, q[1], q[2], "v")
    end
  elseif state == 1 then
    for y = 8, 27 do
      for x = 6, 25 do
        local ch = "p"
        if y == 8 or y == 27 or x == 6 or x == 25 then ch = "m"
        elseif y == 17 or y == 18 or x == 15 or x == 16 then ch = "P" end
        set(g, x, y, ch)
      end
    end
    seg(g, 10, 9, 20, 26, ".")
    seg(g, 11, 9, 21, 26, ".")
    seg(g, 20, 10, 12, 25, ".")
  elseif state == 2 then
    for y = 16, 27 do
      for x = 6, 14 do set(g, x - 2, y + 2, (y == 27) and "m" or "p") end
      for x = 17, 25 do set(g, x + 2, y + 2, (y == 27) and "m" or "p") end
    end
    set(g, 14, 20, "P"); set(g, 18, 22, "P")
  else
    for _, q in ipairs({ {6,28},{10,29},{15,27},{20,29},{24,28},{13,30},{18,30} }) do
      set(g, q[1], q[2], "p"); set(g, q[1] + 1, q[2], "P")
    end
  end
  return rows(g)
end
local vaseMap = { a=C.ard, A=C.ard_s, c=C.ard_c, t=C.patina }
local function vase(state)
  local g = blank(32)
  if state == 0 then
    for x = 13, 18 do set(g, x, 8, "c") end
    for x = 14, 17 do set(g, x, 9, "a") end
    for dy2 = 0, 10 do
      local half = math.floor(6 * math.sin((dy2 / 10) * math.pi * 0.85 + 0.4) + 1.5)
      for x = 16 - half, 15 + half do
        set(g, x, 10 + dy2, (x >= 14 + half) and "A" or "a")
      end
    end
    for x = 12, 19 do set(g, x, 21, "A") end
    set(g, 13, 13, "t"); set(g, 14, 13, "t"); set(g, 17, 15, "t"); set(g, 18, 15, "t")
  elseif state == 1 then
    for dy2 = 0, 10 do
      local half = math.floor(6 * math.sin((dy2 / 10) * math.pi * 0.85 + 0.4) + 1.5)
      for x = 16 - half, 15 + half do set(g, x, 10 + dy2, "a") end
    end
    seg(g, 14, 10, 17, 20, ".")
    seg(g, 15, 10, 18, 20, ".")
    set(g, 13, 13, "t"); set(g, 18, 16, "t")
  else
    for _, q in ipairs({ {8,27},{12,28},{16,26},{20,28},{23,27},{14,29},{18,29} }) do
      set(g, q[1], q[2], "a"); set(g, q[1] + 1, q[2], "A")
    end
    set(g, 11, 28, "t"); set(g, 21, 28, "t")
  end
  return rows(g)
end

-- ------------------------------------------------------------------ roccia
local rockMap = { r=C.cen_scura, Rr=nil, s=C.cenere, d=C.cen_nera, q=C.slag_caldo }
rockMap.R = C.cen_nera
local rock = (function()
  local g = blank(32)
  ell(g, 15, 20, 10, 8, "r")
  ell(g, 20, 16, 6, 5, "r")
  for x = 6, 25 do set(g, x, 28, "R") end
  for x = 8, 14 do set(g, x, 13, "s") end
  set(g, 10, 16, "s"); set(g, 18, 12, "s")
  set(g, 13, 22, "q"); set(g, 19, 24, "q"); set(g, 22, 20, "d")
  return rows(g)
end)()

-- ------------------------------------------------------------------ fogli
local SHEETS = {
  { id = "piedistallo", dir = "props", fw = 32, fh = 32, anchor = { 16, 27 },
    rows = {
      { name = "vuoto", fps = 1, loop = true, frames = frames({ pedestal(false) }, pedMap) },
      { name = "pieno", fps = 3, loop = true, frames = frames(
        { pedestal(true, false), pedestal(true, true) }, pedMap) },
    } },
  { id = "porta", dir = "props", fw = 32, fh = 32, anchor = { 16, 29 },
    rows = {
      { name = "aperta", fps = 1, loop = true, frames = frames({ doorFrame("aperta") }, doorMap) },
      { name = "chiusa", fps = 1, loop = true, frames = frames({ doorFrame("chiusa") }, doorMap) },
      { name = "bloccata", fps = 1, loop = true, frames = frames({ doorFrame("bloccata") }, doorMap) },
    } },
  { id = "pickup-lingotto", dir = "props", fw = 16, fh = 16, anchor = { 8, 9 },
    rows = {
      { name = "idle", fps = 3, loop = true, frames = frames({ ing1, ing2 }, ingotMap) },
    } },
  { id = "pickup-flux", dir = "props", fw = 16, fh = 16, anchor = { 8, 11 },
    rows = {
      { name = "idle", fps = 3, loop = true, frames = frames(
        { fluxPickup(0), fluxPickup(1) }, fluxMap) },
    } },
  { id = "cassa", dir = "props", fw = 32, fh = 32, anchor = { 16, 27 },
    rows = {
      { name = "idle", fps = 1, loop = true, frames = frames({ crate(0) }, crateMap) },
      { name = "break", fps = 10, loop = false, frames = frames(
        { crate(1), crate(2), crate(3) }, crateMap) },
    } },
  { id = "vaso", dir = "props", fw = 32, fh = 32, anchor = { 16, 21 },
    rows = {
      { name = "idle", fps = 1, loop = true, frames = frames({ vase(0) }, vaseMap) },
      { name = "break", fps = 10, loop = false, frames = frames(
        { vase(1), vase(2) }, vaseMap) },
    } },
  { id = "roccia", dir = "props", fw = 32, fh = 32, anchor = { 16, 28 },
    rows = {
      { name = "idle", fps = 1, loop = true, frames = frames({ rock }, rockMap) },
    } },
}
for _, sh in ipairs(SHEETS) do W.buildSheet(ROOT, sh) end

-- dataset
local CAP = "pixel art, worldsmelt style, black 1px outline, flat shading, fucina palette, "
local SUBJ = {
  ["piedistallo"] = "stone pedestal prop for active items",
  ["porta"] = "forge door prop with bronze frame",
  ["pickup-lingotto"] = "gold ingot currency pickup",
  ["pickup-flux"] = "flux catalyst pickup, molten drop over crucible",
  ["cassa"] = "wooden crate destructible prop",
  ["vaso"] = "slate amphora destructible prop with patina decor",
  ["roccia"] = "slag boulder obstacle prop",
}
for _, sh in ipairs(SHEETS) do
  for _, row in ipairs(sh.rows) do
    for fi, fr in ipairs(row.frames) do
      W.datasetFrame(ROOT, "props", sh.id .. "_" .. row.name .. "_" .. fi, fr,
        sh.fw, sh.fh, CAP .. SUBJ[sh.id] .. ", " .. row.name ..
        " frame " .. fi .. " of " .. #row.frames .. ", " .. sh.fw .. "x" ..
        sh.fh .. ", transparent background")
    end
  end
end
print("CP5 OK")
