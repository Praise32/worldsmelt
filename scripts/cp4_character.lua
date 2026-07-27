-- CP4 — personaggio giocante: la Fonditrice (32px, stile S1, palette Fucina).
-- Copertura del brief: walk 4 direzioni x 4 frame (una riga per direzione,
-- left = specchio di right pre-cotto), idle 2f, hit 1f, death 4f (si fonde,
-- in tema col crogiolo). Contratto spritesheet + json + doppio export dataset.
-- Riproducibile: aseprite -b --script scripts/cp4_character.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local blank, set, seg, ell, rows = W.blank, W.gset, W.gseg, W.gellipse, W.grows
local function D(n) return string.rep(".", n) end
local function R(ch, n) return string.rep(ch, n) end

local heroMap = {}
for k, v in pairs(W.maps.hero) do heroMap[k] = v end
local heroHitMap = {
  H=C.fumo, V=C.bianco, A=C.cen_chiara, B=C.cenere, P=C.fumo, m=C.bianco,
}
local deathMap = {}
for k, v in pairs(heroMap) do deathMap[k] = v end
deathMap.q = C.ard; deathMap.Q = C.ard_s; deathMap.x = C.oro

-- ------------------------------------------------ fronte: corpo + gambe
local front = W.grids.hero32
local BODY_N = 20                       -- righe 1..20 = fino ai fianchi
local legsN = {}
for i = 21, 27 do legsN[#legsN + 1] = front[i] end
local legsUp = {}
for i = 3, 7 do legsUp[#legsUp + 1] = legsN[i] end
legsUp[6] = D(32)
legsUp[7] = D(32)
local function halfMix(a, b)            -- sinistra da a, destra da b
  local out = {}
  for i = 1, 7 do out[i] = a[i]:sub(1, 16) .. b[i]:sub(17, 32) end
  return out
end
local legsL = halfMix(legsUp, legsN)
local legsR = halfMix(legsN, legsUp)

local function stack(body, legs, down)
  local g = {}
  if down then
    g[1] = D(32)
    for i = 1, BODY_N - 1 do g[i + 1] = body[i] end
  else
    for i = 1, BODY_N do g[i] = body[i] end
  end
  for i = 1, 7 do g[BODY_N + i] = legs[i] end
  while #g < 32 do g[#g + 1] = D(32) end
  return g
end

-- ------------------------------------------------ retro (walk_up)
local back = {}
for i = 1, BODY_N do back[i] = front[i] end
back[5] = D(8)..R("H",7).."PP"..R("H",7)..D(8)
back[6] = D(7)..R("H",8).."PP"..R("H",8)..D(7)
back[7] = D(7)..R("H",8).."PP"..R("H",8)..D(7)
back[8] = D(7)..R("H",8).."PP"..R("H",8)..D(7)
back[9] = D(7)..R("H",8).."PP"..R("H",8)..D(7)
back[15] = D(5).."PP"..R("A",18).."PP"..D(5)
back[16] = D(5).."PP"..R("A",18).."PP"..D(5)
back[19] = D(6)..R("m",20)..D(6)

-- ------------------------------------------------ profilo (walk_right)
-- Costruito col builder geometrico: elmo con visiera avanti, torso stretto,
-- braccio visibile, gambe in falcata.
local function sideFrame(stride, bob)
  local g = blank(32)
  local dy = bob and 1 or 0
  -- elmo di profilo: calotta piatta, visiera in avanti (destra)
  for y = 4 + dy, 10 + dy do
    local half = (y == 4 + dy or y == 10 + dy) and 5 or 6
    for x = 15 - half, 15 + half do set(g, x, y, "H") end
  end
  for x = 16, 21 do set(g, x, 7 + dy, "V"); set(g, x, 8 + dy, "V") end
  -- torso
  for y = 12 + dy, 18 + dy do
    for x = 11, 20 do set(g, x, y, "A") end
    set(g, 20, y, "B")
  end
  -- spallaccio e braccio
  for x = 12, 17 do set(g, x, 12 + dy, "P"); set(g, x, 13 + dy, "P") end
  for y = 14 + dy, 17 + dy do set(g, 15, y, "P"); set(g, 16, y, "P") end
  -- cintura
  for x = 11, 20 do set(g, x, 19 + dy, "m") end
  set(g, 17, 19 + dy, "V")
  -- gambe: stride = -1 (contatto A), 0 (passaggio), 1 (contatto B)
  local hipY = 20 + dy
  local footY = 27
  if stride == 0 then
    -- gambe affiancate: quella dietro in ombra, quella davanti piena
    for y = hipY, footY - 2 do
      for x = 12, 14 do set(g, x, y, "B") end
      for x = 15, 18 do set(g, x, y, "A") end
    end
    for x = 11, 14 do set(g, x, footY - 1, "m"); set(g, x, footY, "m") end
    for x = 15, 19 do set(g, x, footY - 1, "m"); set(g, x, footY, "m") end
  else
    -- stessa geometria per i due contatti, cambia QUALE gamba e' davanti
    local fwd = (stride == 1) and "A" or "B"
    local bck = (stride == 1) and "B" or "A"
    for o = 0, 2 do
      seg(g, 15 + o, hipY, 19 + o, footY - 1, fwd)
      seg(g, 12 + o, hipY, 8 + o, footY - 1, bck)
    end
    for x = 19, 23 do set(g, x, footY, "m") end
    for x = 7, 11 do set(g, x, footY, "m") end
  end
  return rows(g)
end
local sideWalk = {
  sideFrame(1, false),
  sideFrame(0, true),
  sideFrame(-1, false),
  sideFrame(0, true),
}
local function mirrorGrid(grid)
  local out = {}
  for i, row in ipairs(grid) do out[i] = row:reverse() end
  return out
end
local leftWalk = {}
for i, g in ipairs(sideWalk) do leftWalk[i] = mirrorGrid(g) end

-- ------------------------------------------------ morte: la fonditrice si fonde
local death1 = (function()
  local g = blank(32)
  ell(g, 16, 12, 8, 6, "H")
  for x = 10, 22 do set(g, x, 12, "V") end
  for y = 17, 24 do
    for x = 10, 22 do set(g, x, y, "A") end
    set(g, 22, y, "B")
  end
  for x = 10, 22 do set(g, x, 20, "m") end
  for x = 8, 12 do set(g, x, 25, "m"); set(g, x, 26, "m") end
  for x = 20, 24 do set(g, x, 25, "m"); set(g, x, 26, "m") end
  ell(g, 16, 28, 7, 1, "q")
  return rows(g)
end)()
local death2 = (function()
  local g = blank(32)
  ell(g, 16, 17, 8, 6, "H")
  for x = 10, 22 do set(g, x, 17, "V") end
  for y = 21, 25 do
    for x = 11, 21 do set(g, x, y, "A") end
  end
  ell(g, 16, 28, 10, 2, "q")
  set(g, 8, 27, "Q"); set(g, 24, 28, "Q")
  return rows(g)
end)()
local death3 = (function()
  local g = blank(32)
  ell(g, 16, 28, 12, 3, "q")
  ell(g, 16, 24, 6, 4, "H")
  for x = 12, 19 do set(g, x, 25, "V") end
  set(g, 6, 27, "x"); set(g, 25, 29, "x")
  for x = 5, 27 do set(g, x, 31, "Q") end
  return rows(g)
end)()
local death4 = (function()
  local g = blank(32)
  ell(g, 16, 29, 12, 2, "q")
  for x = 13, 17 do set(g, x, 28, "V") end
  set(g, 8, 29, "x"); set(g, 22, 30, "x")
  for x = 6, 26 do set(g, x, 31, "Q") end
  return rows(g)
end)()

-- ------------------------------------------------ frame front/back/idle/hit
local wd = {
  stack(front, legsN, false), stack(front, legsL, true),
  stack(front, legsN, false), stack(front, legsR, true),
}
local wu = {
  stack(back, legsN, false), stack(back, legsL, true),
  stack(back, legsN, false), stack(back, legsR, true),
}
local idle = { stack(front, legsN, false), stack(front, legsN, true) }
local hitF = stack(front, legsN, false)

-- ------------------------------------------------------------ infrastruttura
local function mkPalette()
  local pal = Palette(#W.PAL)
  for i, p in ipairs(W.PAL) do
    pal:setColor(i - 1, Color{ r = p[1], g = p[2], b = p[3] })
  end
  return pal
end
local function frames(list, map)
  local out = {}
  for _, g in ipairs(list) do out[#out + 1] = { grid = g, map = map } end
  return out
end
local sheet = {
  id = "fonditrice", dir = "character", fam = "character",
  fw = 32, fh = 32, anchor = { 16, 28 },
  rows = {
    { name = "walk_down", fps = 8, loop = true, frames = frames(wd, heroMap) },
    { name = "walk_up", fps = 8, loop = true, frames = frames(wu, heroMap) },
    { name = "walk_right", fps = 8, loop = true, frames = frames(sideWalk, heroMap) },
    { name = "walk_left", fps = 8, loop = true, frames = frames(leftWalk, heroMap) },
    { name = "idle", fps = 2, loop = true, frames = frames(idle, heroMap) },
    { name = "hit", fps = 10, loop = false, frames = {
      { grid = hitF, map = heroHitMap, ox = 1 } } },
    { name = "death", fps = 8, loop = false, frames = frames(
      { death1, death2, death3, death4 }, deathMap) },
  },
}

local cols = 0
for _, row in ipairs(sheet.rows) do
  if #row.frames > cols then cols = #row.frames end
end
local sw, sh = cols * sheet.fw, #sheet.rows * sheet.fh
local img = Image(sw, sh, ColorMode.RGB)
for ri, row in ipairs(sheet.rows) do
  for fi, fr in ipairs(row.frames) do
    W.renderS1(img, fr.grid, fr.map, (fi - 1) * sheet.fw + (fr.ox or 0),
               (ri - 1) * sheet.fh, 1)
  end
end
local spr = Sprite(sw, sh, ColorMode.RGB)
spr:setPalette(mkPalette())
spr.layers[1].name = sheet.id
local cel = spr.cels[1]
if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
cel.image = img
cel.position = Point(0, 0)
spr:saveAs(ROOT .. "/assets/art-src/character/fonditrice.aseprite")
spr:saveCopyAs(ROOT .. "/assets/art/character/fonditrice.png")
local parts = {}
for ri, row in ipairs(sheet.rows) do
  parts[#parts + 1] = '"' .. row.name .. '":{"row":' .. (ri - 1) ..
    ',"frames":' .. #row.frames .. ',"fps":' .. row.fps ..
    ',"loop":' .. tostring(row.loop) .. '}'
end
local f = io.open(ROOT .. "/assets/art/character/fonditrice.json", "w")
f:write('{"frame_w":32,"frame_h":32,"anchor":[16,28],"anims":{' ..
        table.concat(parts, ",") .. "}}\n")
f:close()
print("fonditrice: sheet " .. sw .. "x" .. sh .. " ok")

-- dataset
local CAP = "pixel art, worldsmelt style, black 1px outline, flat shading, " ..
  "fucina palette, player character, forge diver heroine, bronze helmet " ..
  "with gold visor, slate armor, "
for _, row in ipairs(sheet.rows) do
  for fi, fr in ipairs(row.frames) do
    local spr2 = Sprite(64, 64, ColorMode.RGB)
    spr2:setPalette(mkPalette())
    local img2 = Image(64, 64, ColorMode.RGB)
    W.renderS1(img2, fr.grid, fr.map, 16, 16, 1)
    local cel2 = spr2.cels[1]
    if cel2 == nil then cel2 = spr2:newCel(spr2.layers[1], 1) end
    cel2.image = img2
    cel2.position = Point(0, 0)
    local base_ = ROOT .. "/dataset/worldsmelt-style/character/fonditrice_" ..
                  row.name .. "_" .. fi
    spr2:saveCopyAs(base_ .. ".png")
    local f2 = io.open(base_ .. ".txt", "w")
    f2:write(CAP .. row.name .. " animation frame " .. fi .. " of " ..
             #row.frames .. ", 32x32, transparent background\n")
    f2:close()
  end
end

-- anteprime gif
local PV = ROOT .. "/assets/art-src/preview-cp3/"
local function gif(path, list, map, durMs)
  local spr3 = Sprite(128, 128, ColorMode.RGB)
  spr3:setPalette(mkPalette())
  while #spr3.frames < #list do spr3:newEmptyFrame() end
  for i, fr in ipairs(list) do
    local img3 = Image(128, 128, ColorMode.RGB)
    W.fillRect(img3, 0, 0, 128, 128, C.ard_s)
    W.renderS1(img3, fr.grid, fr.map, 0, 0, 4)
    local cel3 = spr3:newCel(spr3.layers[1], i)
    cel3.image = img3
    cel3.position = Point(0, 0)
    spr3.frames[i].duration = durMs / 1000.0
  end
  spr3:saveCopyAs(path)
  print("gif: " .. path)
end
gif(PV .. "fonditrice-walk-down.gif", frames(wd, heroMap), heroMap, 125)
gif(PV .. "fonditrice-walk-up.gif", frames(wu, heroMap), heroMap, 125)
gif(PV .. "fonditrice-walk-right.gif", frames(sideWalk, heroMap), heroMap, 125)
gif(PV .. "fonditrice-death.gif", frames({ death1, death2, death3, death4 },
    deathMap), deathMap, 220)
print("CP4 OK")
