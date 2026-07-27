-- CP7 — tileset ambiente a griglia 32px per i 5 temi fallback della demo
-- (RunContentMakeFallbackThemeCards): Neon Cellar, Moldy Library, Lunar
-- Forge, Radioactive Aquarium, Cathedral of Sugar. Per ogni tema:
--   pavimento base + 3 varianti; pareti (4 lati, 4 angoli esterni, 4 angoli
--   interni, blocco angolo-mancante della L); porte integrate 4 lati x 3
--   stati; ostacoli per famiglia ROOM_LAYOUT_* (pillar/corridor/arena/
--   scatter); vuoto fuori stanza; variante di escalation (DEC-024): floor/
--   wall/void degradati con crepe di brace.
-- Output: assets/art/tiles/<tema>.png + manifest json ruolo->cella
-- (contratto documentato in docs/ai-production/08-PIPELINE-SPRITE-ANIMAZIONI.md).
-- Riproducibile: aseprite -b --script scripts/cp7_tilesets.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local blank, set, seg, ell, rows = W.blank, W.gset, W.gseg, W.gellipse, W.grows

-- --------------------------------------------------------------- temi
-- fB pavimento base, fD scuro, fL chiaro, wB parete, wD parete scura,
-- wL parete chiara, ac accento (motivo), ac2 accento raro, motivo: id stampo
local THEMES = {
  { slug = "neon-cellar", fB=C.ard_s, fD=C.cen_nera, fL=C.ard, wB=C.cen_scura,
    wD=C.cen_nera, wL=C.cenere, ac=C.ver, ac2=C.pru, motif="moss" },
  { slug = "moldy-library", fB=C.slag_caldo, fD=C.slag_scuro, fL=C.terra,
    wB=C.bronzo_s, wD=C.slag_scuro, wL=C.bronzo, ac=C.ver_c, ac2=C.oro,
    motif="book" },
  { slug = "lunar-forge", fB=C.cen_scura, fD=C.cen_nera, fL=C.cenere,
    wB=C.ard_s, wD=C.cen_nera, wL=C.ard, ac=C.fiamma, ac2=C.bagliore,
    motif="spark" },
  { slug = "radioactive-aquarium", fB=C.ard, fD=C.ard_s, fL=C.ard_c,
    wB=C.ard_s, wD=C.cen_nera, wL=C.ard, ac=C.patina, ac2=C.ver_c,
    motif="bubble" },
  { slug = "cathedral-of-sugar", fB=C.cen_chiara, fD=C.cenere, fL=C.fumo,
    wB=C.cenere, wD=C.cen_scura, wL=C.cen_chiara, ac=C.pru_c, ac2=C.oro_p,
    motif="crystal" },
}

-- posizioni deterministiche dei dettagli (niente RNG negli script)
local SPECKS1 = { {7,9},{21,5},{14,19},{26,27},{4,24} }
local SPECKS2 = { {11,4},{25,14},{6,17},{17,26},{28,8},{9,28} }

local function stampMotif(g, t, x, y)
  if t.motif == "moss" then
    set(g, x, y, t.ac); set(g, x+1, y, t.ac); set(g, x, y+1, t.ac)
  elseif t.motif == "book" then
    for i = 0, 3 do set(g, x+i, y, t.ac2) end
    set(g, x+1, y-1, t.ac)
  elseif t.motif == "spark" then
    set(g, x, y, t.ac2); set(g, x-1, y, t.ac); set(g, x+1, y, t.ac)
    set(g, x, y-1, t.ac); set(g, x, y+1, t.ac)
  elseif t.motif == "bubble" then
    set(g, x, y, t.ac); set(g, x+1, y, t.ac)
    set(g, x, y+1, t.ac); set(g, x+1, y+1, t.ac); set(g, x, y, t.ac2)
  else -- crystal
    set(g, x, y-1, t.ac2); set(g, x-1, y, t.ac); set(g, x, y, t.ac2)
    set(g, x+1, y, t.ac); set(g, x, y+1, t.ac)
  end
end

-- Nota: qui i colori sono INDICI diretti (niente mappa materiali): le griglie
-- contengono gia' indici. Renderer dedicato senza outline (i tile affiancano).
local function tileImgDraw(img, tile, ox, oy)
  for y = 1, 32 do
    for x = 1, 32 do
      local idx = tile[y][x]
      if idx then W.px(img, ox + x - 1, oy + y - 1, idx) end
    end
  end
end
local function newTile(fill)
  local t = {}
  for y = 1, 32 do
    t[y] = {}
    for x = 1, 32 do t[y][x] = fill end
  end
  return t
end
local function tset(t, x, y, idx)
  if x >= 1 and y >= 1 and x <= 32 and y <= 32 then t[y][x] = idx end
end
local function rot90(t)
  local o = {}
  for y = 1, 32 do
    o[y] = {}
    for x = 1, 32 do o[y][x] = t[33 - x][y] end
  end
  return o
end

-- ------------------------------------------------------------ pavimenti
local function floorBase(t)
  local g = newTile(t.fB)
  for _, p in ipairs(SPECKS1) do tset(g, p[1], p[2], t.fD) end
  return g
end
local function floorVar1(t)
  local g = floorBase(t)
  local sg = { n = 32 }  -- adattatore per stampMotif su tile di indici
  local proxy = setmetatable({ n = 32 }, { __index = function() return nil end })
  -- stampo il motivo direttamente con tset
  local mx, my = 12, 14
  local saveSet = nil
  -- versione tile-nativa dello stampo
  if t.motif == "moss" then
    tset(g, mx, my, t.ac); tset(g, mx+1, my, t.ac); tset(g, mx, my+1, t.ac)
  elseif t.motif == "book" then
    for i = 0, 3 do tset(g, mx+i, my, t.ac2) end
    tset(g, mx+1, my-1, t.ac)
  elseif t.motif == "spark" then
    tset(g, mx, my, t.ac2); tset(g, mx-1, my, t.ac); tset(g, mx+1, my, t.ac)
    tset(g, mx, my-1, t.ac); tset(g, mx, my+1, t.ac)
  elseif t.motif == "bubble" then
    tset(g, mx, my, t.ac2); tset(g, mx+1, my, t.ac)
    tset(g, mx, my+1, t.ac); tset(g, mx+1, my+1, t.ac)
  else
    tset(g, mx, my-1, t.ac2); tset(g, mx-1, my, t.ac); tset(g, mx, my, t.ac2)
    tset(g, mx+1, my, t.ac); tset(g, mx, my+1, t.ac)
  end
  tset(g, 24, 24, t.fD); tset(g, 25, 24, t.fD)
  return g
end
local function floorVar2(t)
  local g = floorBase(t)
  -- crepa sottile
  local path = { {5,26},{9,22},{13,21},{18,17},{21,13},{24,12} }
  for i = 1, #path - 1 do
    local a, b = path[i], path[i+1]
    local steps = math.max(math.abs(b[1]-a[1]), math.abs(b[2]-a[2]))
    for s = 0, steps do
      tset(g, math.floor(a[1] + (b[1]-a[1])*s/steps + 0.5),
           math.floor(a[2] + (b[2]-a[2])*s/steps + 0.5), t.fD)
    end
  end
  return g
end
local function floorVar3(t)
  local g = floorBase(t)
  for _, p in ipairs(SPECKS2) do tset(g, p[1], p[2], t.fL) end
  tset(g, 20, 7, t.fD); tset(g, 21, 7, t.fD); tset(g, 20, 8, t.fD)
  return g
end

-- ----------------------------------------------------------- pareti
-- wall_n: parete piena in alto che scende sul pavimento (bordo su lato S)
local function wallN(t)
  local g = newTile(t.wB)
  for x = 1, 32 do
    tset(g, x, 1, t.wL); tset(g, x, 2, t.wL)
    tset(g, x, 28, t.wD); tset(g, x, 29, t.wD)
    tset(g, x, 30, C.slag_nero); tset(g, x, 31, C.slag_nero)
    tset(g, x, 32, C.slag_nero)
  end
  for _, p in ipairs(SPECKS1) do
    if p[2] < 26 then tset(g, p[1], p[2], t.wD) end
  end
  -- giunti di pietra orizzontali
  for x = 1, 32 do tset(g, x, 10, t.wD); tset(g, x, 19, t.wD) end
  for y = 3, 9 do tset(g, 11, y, t.wD) end
  for y = 11, 18 do tset(g, 22, y, t.wD) end
  for y = 20, 27 do tset(g, 6, y, t.wD) end
  return g
end
-- angolo esterno NW: pareti che girano (lati S ed E aperti sul pavimento)
local function cornerNW(t)
  local g = wallN(t)
  for y = 1, 32 do
    tset(g, 1, y, t.wL); tset(g, 2, y, t.wL)
  end
  for y = 28, 32 do
    for x = 1, 5 do tset(g, x, y, (y >= 30) and C.slag_nero or t.wD) end
  end
  return g
end
-- angolo interno NW (la stanza gira attorno: solo lo spigolo e' parete)
local function innerNW(t)
  local g = newTile(t.fB)
  for _, p in ipairs(SPECKS1) do tset(g, p[1], p[2], t.fD) end
  for y = 1, 12 do
    for x = 1, 12 do
      local idx = t.wB
      if x >= 10 or y >= 10 then idx = t.wD end
      if x == 12 or y == 12 then idx = C.slag_nero end
      tset(g, x, y, idx)
    end
  end
  tset(g, 1, 1, t.wL); tset(g, 2, 1, t.wL); tset(g, 1, 2, t.wL)
  return g
end
-- blocco pieno (angolo mancante della L, oggi un blocco solido)
local function wallFull(t)
  local g = newTile(t.wB)
  for i = 1, 32 do
    tset(g, i, 1, t.wL); tset(g, 1, i, t.wL)
    tset(g, i, 32, t.wD); tset(g, 32, i, t.wD)
  end
  for x = 1, 32 do tset(g, x, 11, t.wD); tset(g, x, 22, t.wD) end
  for y = 1, 10 do tset(g, 16, y, t.wD) end
  for y = 12, 21 do tset(g, 8, y, t.wD); tset(g, 25, y, t.wD) end
  for y = 23, 32 do tset(g, 17, y, t.wD) end
  return g
end

-- ------------------------------------------------------------- porte (lato N)
local function doorN(t, state)
  local g = wallN(t)
  -- apertura centrale con stipiti bronzo
  for y = 1, 29 do
    for x = 9, 24 do tset(g, x, y, nil) end
  end
  for y = 1, 29 do
    tset(g, 7, y, C.bronzo_s); tset(g, 8, y, C.bronzo_c)
    tset(g, 25, y, C.bronzo_c); tset(g, 26, y, C.bronzo_s)
  end
  for y = 30, 32 do
    for x = 7, 26 do tset(g, x, y, nil) end
  end
  if state == "aperta" then
    for y = 1, 27 do
      for x = 9, 24 do tset(g, x, y, C.cen_nera) end
    end
    for y = 1, 6 do
      for x = 9, 24 do tset(g, x, y, C.slag_nero) end
    end
  else
    for y = 1, 29 do
      for x = 9, 24 do
        local idx = C.terra
        if x == 13 or x == 19 then idx = C.slag_caldo end
        tset(g, x, y, idx)
      end
    end
    tset(g, 22, 16, C.bronzo_c); tset(g, 22, 17, C.bronzo_c)
    if state == "bloccata" then
      for x = 7, 26 do
        tset(g, x, 13, C.brace_s); tset(g, x, 14, C.brace); tset(g, x, 15, C.brace_s)
      end
      for y = 10, 18 do tset(g, 15, y, C.brace); tset(g, 16, y, C.brace_s) end
    end
  end
  return g
end

-- --------------------------------------------------------- ostacoli e vuoto
local function obstPillar(t)
  local g = floorBase(t)
  for y = 4, 28 do
    for x = 8, 24 do
      local idx = t.wB
      if x <= 9 or y <= 5 then idx = t.wL end
      if x >= 23 or y >= 27 then idx = t.wD end
      tset(g, x, y, idx)
    end
  end
  for x = 8, 24 do tset(g, x, 29, C.slag_nero) end
  for x = 6, 26 do tset(g, x, 3, t.wL) end
  return g
end
local function obstCorridor(t)
  local g = floorBase(t)
  for y = 10, 24 do
    for x = 1, 32 do
      local idx = t.wB
      if y <= 11 then idx = t.wL end
      if y >= 23 then idx = t.wD end
      tset(g, x, y, idx)
    end
  end
  for x = 1, 32 do tset(g, x, 25, C.slag_nero) end
  for x = 1, 32 do tset(g, x, 17, t.wD) end
  return g
end
local function obstArena(t)
  local g = floorBase(t)
  ell({ n = 32 }, 0, 0, 0, 0, ".")  -- no-op per chiarezza
  for dy = -7, 7 do
    for dx = -8, 8 do
      if (dx*dx)/64.0 + (dy*dy)/49.0 <= 1.0 then
        local idx = t.wB
        if dy < -4 then idx = t.wL end
        if dy > 4 or dx > 5 then idx = t.wD end
        tset(g, 16 + dx, 17 + dy, idx)
      end
    end
  end
  for dx = -6, 6 do tset(g, 16 + dx, 25, C.slag_nero) end
  return g
end
local function obstScatter(t)
  local g = floorBase(t)
  local rocks = { {8,9,2},{22,7,1},{14,20,3},{26,23,2},{5,26,1} }
  for _, r in ipairs(rocks) do
    for dy = -r[3], r[3] do
      for dx = -r[3], r[3] do
        if dx*dx + dy*dy <= r[3]*r[3] then
          tset(g, r[1] + dx, r[2] + dy, (dy > 0) and t.wD or t.wB)
        end
      end
    end
    tset(g, r[1] - r[3] + 1, r[2] - r[3] + 1, t.wL)
  end
  return g
end
local function voidTile(t)
  local g = newTile(C.slag_nero)
  tset(g, 9, 12, C.slag_scuro); tset(g, 22, 25, C.slag_scuro)
  tset(g, 27, 6, C.slag_scuro)
  return g
end

-- ------------------------------------------------- escalation (DEC-024)
local function degrade(g, t)
  -- crepe incandescenti: la degenerazione e' il collasso verso il crogiolo
  local cracks = { {3,29},{7,25},{12,24},{16,20},{20,19},{25,15},{28,11} }
  for i = 1, #cracks - 1 do
    local a, b = cracks[i], cracks[i+1]
    local steps = math.max(math.abs(b[1]-a[1]), math.abs(b[2]-a[2]))
    for s = 0, steps do
      tset(g, math.floor(a[1] + (b[1]-a[1])*s/steps + 0.5),
           math.floor(a[2] + (b[2]-a[2])*s/steps + 0.5), C.brace)
    end
  end
  tset(g, 12, 24, C.bagliore); tset(g, 20, 19, C.bagliore)
  tset(g, 6, 6, C.brace_s); tset(g, 27, 27, C.brace_s)
  return g
end

-- ------------------------------------------------------------------- build
local ROLES = {}  -- ordine stabile dei ruoli nel foglio
local function addRole(list, name, tile) list[#list + 1] = { name, tile } end

for _, t in ipairs(THEMES) do
  local tiles = {}
  addRole(tiles, "floor", floorBase(t))
  addRole(tiles, "floor_var1", floorVar1(t))
  addRole(tiles, "floor_var2", floorVar2(t))
  addRole(tiles, "floor_var3", floorVar3(t))
  local wn = wallN(t)
  addRole(tiles, "wall_n", wn)
  addRole(tiles, "wall_e", rot90(wn))
  addRole(tiles, "wall_s", rot90(rot90(wn)))
  addRole(tiles, "wall_w", rot90(rot90(rot90(wn))))
  local cnw = cornerNW(t)
  addRole(tiles, "corner_nw", cnw)
  addRole(tiles, "corner_ne", rot90(cnw))
  addRole(tiles, "corner_se", rot90(rot90(cnw)))
  addRole(tiles, "corner_sw", rot90(rot90(rot90(cnw))))
  local inw = innerNW(t)
  addRole(tiles, "inner_nw", inw)
  addRole(tiles, "inner_ne", rot90(inw))
  addRole(tiles, "inner_se", rot90(rot90(inw)))
  addRole(tiles, "inner_sw", rot90(rot90(rot90(inw))))
  addRole(tiles, "l_block", wallFull(t))
  for _, st in ipairs({ "aperta", "chiusa", "bloccata" }) do
    local dn = doorN(t, st)
    addRole(tiles, "door_n_" .. st, dn)
    addRole(tiles, "door_e_" .. st, rot90(dn))
    addRole(tiles, "door_s_" .. st, rot90(rot90(dn)))
    addRole(tiles, "door_w_" .. st, rot90(rot90(rot90(dn))))
  end
  addRole(tiles, "obst_pillar", obstPillar(t))
  addRole(tiles, "obst_corridor", obstCorridor(t))
  addRole(tiles, "obst_arena", obstArena(t))
  addRole(tiles, "obst_scatter", obstScatter(t))
  addRole(tiles, "void", voidTile(t))
  addRole(tiles, "floor_deg", degrade(floorBase(t), t))
  addRole(tiles, "wall_deg", degrade(wallN(t), t))
  addRole(tiles, "void_deg", degrade(voidTile(t), t))

  local cols = 8
  local rowsN = math.ceil(#tiles / cols)
  local img = Image(cols * 32, rowsN * 32, ColorMode.RGB)
  local meta = {}
  for i, entry in ipairs(tiles) do
    local cx = (i - 1) % cols
    local cy = math.floor((i - 1) / cols)
    tileImgDraw(img, entry[2], cx * 32, cy * 32)
    meta[#meta + 1] = '"' .. entry[1] .. '":[' .. cx .. ',' .. cy .. ']'
  end
  local spr = Sprite(cols * 32, rowsN * 32, ColorMode.RGB)
  spr:setPalette(W.mkPalette())
  spr.layers[1].name = t.slug
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveAs(ROOT .. "/assets/art-src/tiles/" .. t.slug .. ".aseprite")
  spr:saveCopyAs(ROOT .. "/assets/art/tiles/" .. t.slug .. ".png")
  local f = io.open(ROOT .. "/assets/art/tiles/" .. t.slug .. ".json", "w")
  f:write('{"tile_w":32,"tile_h":32,"grid":[' .. cols .. ',' .. rowsN ..
          '],"tiles":{' .. table.concat(meta, ",") .. "}}\n")
  f:close()
  print(t.slug .. ": tileset " .. #tiles .. " tile ok")

  -- dataset: ogni tile come frame 64x64 (tile centrata, sfondo trasparente)
  for _, entry in ipairs(tiles) do
    local spr2 = Sprite(64, 64, ColorMode.RGB)
    spr2:setPalette(W.mkPalette())
    local img2 = Image(64, 64, ColorMode.RGB)
    tileImgDraw(img2, entry[2], 16, 16)
    local cel2 = spr2.cels[1]
    if cel2 == nil then cel2 = spr2:newCel(spr2.layers[1], 1) end
    cel2.image = img2
    cel2.position = Point(0, 0)
    local base_ = ROOT .. "/dataset/worldsmelt-style/tiles/" .. t.slug .. "_" .. entry[1]
    spr2:saveCopyAs(base_ .. ".png")
    local f2 = io.open(base_ .. ".txt", "w")
    f2:write("pixel art, worldsmelt style, flat shading, fucina palette, " ..
             "environment tile, " .. t.slug:gsub("%-", " ") .. " theme, " ..
             entry[1]:gsub("_", " ") .. ", 32x32 tile, top-down dungeon\n")
    f2:close()
  end
end
print("CP7 OK")
