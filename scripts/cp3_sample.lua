-- CP3 — campione di produzione Worldsmelt (stile S1, 32px, palette Fucina).
-- Produce nel formato-contratto dell'engine (striscia orizzontale, righe =
-- animazioni, json a fianco):
--   assets/art/enemies/goblin-di-slag.png/.json  (walk 4f, hit 1f, death 4f)
--   assets/art/items/fiala-di-brace.png/.json    (idle 1f, glow 2f)
--   assets/art/shots/orb.png/.json               (fly 2f, impact 3f)
-- Piu' l'export training-ready: frame singoli 64x64 trasparenti centrati con
-- caption, in dataset/worldsmelt-style/<famiglia>/ (DEC-175).
-- Riproducibile: aseprite -b --script scripts/cp3_sample.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local function D(n) return string.rep(".", n) end
local function R(ch, n) return string.rep(ch, n) end

-- ------------------------------------------------------- goblin: frame walk
-- Corpo = goblin32 senza le tre righe delle gambe; gambe in 3 varianti;
-- nei frame di passaggio il corpo scende di 1px (ancheggiata).
local base = W.grids.goblin32
local BODY_ROWS = 27          -- righe 1..27 = corpo, 28..30 = gambe
local legsA = {
  D(10).."SSS"..D(6).."SSS"..D(10),
  D(10).."sss"..D(6).."sss"..D(10),
  D(9)..R("s",4)..D(6)..R("s",4)..D(9),
}
local legsL = {
  D(10).."SSS"..D(6).."SSS"..D(10),
  D(9)..R("s",4)..D(7).."sss"..D(9),
  D(19)..R("s",4)..D(9),
}
local legsR = {
  D(10).."SSS"..D(6).."SSS"..D(10),
  D(10).."sss"..D(7)..R("s",4)..D(8),
  D(9)..R("s",4)..D(19),
}

local function goblinFrame(legs, down)
  local g = {}
  if down then
    g[1] = D(32)
    for i = 1, BODY_ROWS - 1 do g[i + 1] = base[i] end
  else
    for i = 1, BODY_ROWS do g[i] = base[i] end
  end
  for i = 1, 3 do g[BODY_ROWS + i] = legs[i] end
  while #g < 32 do g[#g + 1] = D(32) end
  return g
end

local walk1 = goblinFrame(legsA, false)
local walk2 = goblinFrame(legsL, true)
local walk3 = goblinFrame(legsA, false)
local walk4 = goblinFrame(legsR, true)

-- hit: stesso frame di contatto, colori sbiancati dal colpo (mappa dedicata)
local hitMap = {
  s=C.fumo, S=C.cen_chiara, L=C.fumo, e=C.brace, p=C.brace_s,
  c=C.cen_chiara, C=C.cenere, t=C.bianco,
}

-- death: il goblin si scioglie in slag di verderame (4 frame autoriali)
local deathMap = {}
for k, v in pairs(W.maps.goblin) do deathMap[k] = v end
deathMap.q = C.ver; deathMap.Q = C.ver_s; deathMap.x = C.ver_c

local death1 = {
  D(32), D(32), D(32), D(32), D(32), D(32), D(32), D(32),
  D(10)..R("L",12)..D(10),
  D(8).."LL"..R("s",12).."LL"..D(8),
  D(7).."L"..R("s",16).."L"..D(7),
  D(6)..R("s",20)..D(6),
  D(1).."LL"..R("s",26).."LL"..D(1),
  D(1).."ss".."SS"..R("s",22).."SS".."ss"..D(1),
  D(3).."SS"..R("s",22).."SS"..D(3),
  D(5).."SS"..R("s",18).."SS"..D(5),
  D(6)..R("s",20)..D(6),
  D(6).."ss".."ppp"..R("s",10).."ppp".."sS"..D(6),
  D(6).."ss".."SSS"..R("s",10).."SSS".."sS"..D(6),
  D(6)..R("s",8).."SSSS"..R("s",7).."S"..D(6),
  D(7).."S"..R("s",16).."S"..D(7),
  D(9).."S"..R("s",12).."S"..D(9),
  D(7)..R("s",18)..D(7),
  D(4)..R("s",3).."S"..R("s",16).."S"..R("s",3)..D(4),
  D(5)..R("S",22)..D(5),
  D(8)..R("q",16)..D(8),
  D(10)..R("Q",12)..D(10),
  D(32), D(32), D(32), D(32), D(32),
}
local death2 = {
  D(32), D(32), D(32), D(32), D(32), D(32), D(32),
  D(32), D(32), D(32), D(32), D(32), D(32), D(32),
  D(10)..R("L",12)..D(10),
  D(8).."LL"..R("s",12).."LL"..D(8),
  D(7)..R("s",18)..D(7),
  D(3).."SS"..R("s",22).."SS"..D(3),
  D(5)..R("s",22)..D(5),
  D(6).."ss".."ppp"..R("s",10).."ppp".."ss"..D(6),
  D(6)..R("s",20)..D(6),
  D(7)..R("s",18)..D(7),
  D(4)..R("q",2)..R("s",20)..R("q",2)..D(4),
  D(3)..R("q",26)..D(3),
  D(2)..R("q",28)..D(2),
  D(3)..R("q",7).."x"..R("q",14).."x"..R("q",3)..D(3),
  D(4)..R("Q",24)..D(4),
  D(32), D(32), D(32), D(32), D(32),
}
local death3 = {
  D(32), D(32), D(32), D(32), D(32), D(32), D(32), D(32), D(32),
  D(32), D(32), D(32), D(32), D(32), D(32), D(32), D(32), D(32),
  D(8).."ss"..D(12).."ss"..D(8),
  D(7).."sss"..D(11).."sss"..D(8),
  D(7).."ss"..D(14).."ss"..D(7),
  D(4)..R("q",24)..D(4),
  D(2)..R("q",28)..D(2),
  D(1)..R("q",8).."ee"..R("q",8).."ee"..R("q",10)..D(1),
  D(2)..R("q",6).."x"..R("q",14).."x"..R("q",6)..D(2),
  D(3)..R("Q",26)..D(3),
  D(32), D(32), D(32), D(32), D(32), D(32),
}
local death4 = {
  D(32), D(32), D(32), D(32), D(32), D(32), D(32), D(32),
  D(32), D(32), D(32), D(32), D(32), D(32), D(32), D(32),
  D(32), D(32), D(32), D(32), D(32), D(32),
  D(6)..R("q",20)..D(6),
  D(4)..R("q",24)..D(4),
  D(5)..R("q",8).."x"..R("q",13)..D(5),
  D(6)..R("Q",20)..D(6),
  D(32), D(32), D(32), D(32), D(32), D(32),
}

-- ------------------------------------------------- fiala: idle + 2 glow
-- I frame glow sono la base piu' scintille/bagliori sovrapposti.
local glow1px = {
  {10,6,C.bagliore}, {23,15,C.bagliore}, {12,15,C.bagliore},
  {19,17,C.bagliore}, {15,3,C.oro_p},
}
local glow2px = {
  {9,5,C.bagliore}, {24,14,C.bagliore}, {13,14,C.bagliore},
  {20,16,C.bagliore}, {15,2,C.bianco}, {16,2,C.bianco},
  {5,10,C.bagliore}, {27,20,C.bagliore}, {6,22,C.bagliore},
}

-- ------------------------------------------------- colpo orb: 2f + impatto
local orbMap = { c=C.bagliore, f=C.fiamma, t=C.brace }
local orb1 = {
  D(16), D(16), D(16), D(16), D(16),
  D(6).."ffff"..D(6),
  D(5).."f".."cccc".."f"..D(5),
  D(5).."f".."cccc".."f"..D(5),
  D(6).."ffff"..D(6),
  D(16), D(16), D(16), D(16), D(16), D(16), D(16),
}
local orb2 = {
  D(16), D(16), D(16), D(16),
  D(6).."ffff"..D(6),
  D(5).."f".."cccc".."f"..D(5),
  D(4).."f".."cccccc".."f"..D(4),
  D(4).."f".."cccccc".."f"..D(4),
  D(5).."f".."cccc".."f"..D(5),
  D(6).."ffff"..D(6),
  D(16), D(16), D(16), D(16), D(16), D(16),
}
local imp1 = {
  D(16), D(16), D(16), D(16), D(16), D(16),
  D(7).."cc"..D(7),
  D(6).."cccc"..D(6),
  D(6).."cccc"..D(6),
  D(7).."cc"..D(7),
  D(16), D(16), D(16), D(16), D(16), D(16),
}
local imp2 = {
  D(16), D(16), D(16), D(16),
  D(6).."ffff"..D(6),
  D(5).."f"..D(1).."cc"..D(1).."f"..D(5),
  D(5).."f"..D(1).."cc"..D(1).."f"..D(5),
  D(6).."ffff"..D(6),
  D(16), D(16), D(16), D(16), D(16), D(16), D(16), D(16),
}
local imp3 = {
  D(16), D(16), D(16), D(16),
  D(5).."t"..D(4).."t"..D(5),
  D(16),
  D(3).."t"..D(8).."t"..D(3),
  D(16),
  D(6).."f"..D(2).."t"..D(6),
  D(16),
  D(4).."t"..D(7).."t"..D(3),
  D(16), D(16), D(16), D(16), D(16),
}

-- ------------------------------------------------------------ infrastruttura
local function mkPalette()
  local pal = Palette(#W.PAL)
  for i, p in ipairs(W.PAL) do
    pal:setColor(i - 1, Color{ r = p[1], g = p[2], b = p[3] })
  end
  return pal
end

-- Un "frame" = { grid=..., map=..., ox=0, extra={ {x,y,idx}... } }
-- Una "sheet" = { id, dir, fam, fw, fh, anchor={x,y}, rows={ {name,fps,loop,frames={...}} } }
local function buildSheet(sheet)
  local cols = 0
  for _, row in ipairs(sheet.rows) do
    if #row.frames > cols then cols = #row.frames end
  end
  local sw, sh = cols * sheet.fw, #sheet.rows * sheet.fh
  local img = Image(sw, sh, ColorMode.RGB)
  for ri, row in ipairs(sheet.rows) do
    for fi, fr in ipairs(row.frames) do
      local ox = (fi - 1) * sheet.fw + (fr.ox or 0)
      local oy = (ri - 1) * sheet.fh
      W.renderS1(img, fr.grid, fr.map, ox, oy, 1)
      for _, e in ipairs(fr.extra or {}) do
        W.px(img, ox + e[1], oy + e[2], e[3])
      end
    end
  end
  local spr = Sprite(sw, sh, ColorMode.RGB)
  spr:setPalette(mkPalette())
  spr.layers[1].name = sheet.id
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveAs(ROOT .. "/assets/art-src/" .. sheet.dir .. "/" .. sheet.id .. ".aseprite")
  spr:saveCopyAs(ROOT .. "/assets/art/" .. sheet.dir .. "/" .. sheet.id .. ".png")
  -- json a contratto
  local j = { '{"frame_w":' .. sheet.fw .. ',"frame_h":' .. sheet.fh ..
              ',"anchor":[' .. sheet.anchor[1] .. ',' .. sheet.anchor[2] .. '],"anims":{' }
  local parts = {}
  for ri, row in ipairs(sheet.rows) do
    parts[#parts + 1] = '"' .. row.name .. '":{"row":' .. (ri - 1) ..
      ',"frames":' .. #row.frames .. ',"fps":' .. row.fps ..
      ',"loop":' .. tostring(row.loop) .. '}'
  end
  j[#j + 1] = table.concat(parts, ",") .. "}}"
  local f = io.open(ROOT .. "/assets/art/" .. sheet.dir .. "/" .. sheet.id .. ".json", "w")
  f:write(table.concat(j) .. "\n")
  f:close()
  print(sheet.id .. ": sheet " .. sw .. "x" .. sh .. " ok")
end

-- Export training-ready: frame singolo 64x64 trasparente centrato + caption.
local function datasetFrame(fam, name, fr, fw, fh, caption)
  local spr = Sprite(64, 64, ColorMode.RGB)
  spr:setPalette(mkPalette())
  local img = Image(64, 64, ColorMode.RGB)
  local ox = math.floor((64 - fw) / 2)
  local oy = math.floor((64 - fh) / 2)
  W.renderS1(img, fr.grid, fr.map, ox, oy, 1)
  for _, e in ipairs(fr.extra or {}) do
    W.px(img, ox + e[1], oy + e[2], e[3])
  end
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  local base_ = ROOT .. "/dataset/worldsmelt-style/" .. fam .. "/" .. name
  spr:saveCopyAs(base_ .. ".png")
  local f = io.open(base_ .. ".txt", "w")
  f:write(caption .. "\n")
  f:close()
end

-- GIF di anteprima a x4 (per il checkpoint, non per l'engine).
local function previewGif(path, frames, map, fw, fh, durMs, bgIdx)
  local spr = Sprite(fw * 4, fh * 4, ColorMode.RGB)
  spr:setPalette(mkPalette())
  while #spr.frames < #frames do spr:newEmptyFrame() end
  for i, fr in ipairs(frames) do
    local img = Image(fw * 4, fh * 4, ColorMode.RGB)
    W.fillRect(img, 0, 0, fw * 4, fh * 4, bgIdx)
    W.renderS1(img, fr.grid, fr.map or map, (fr.ox or 0) * 4, 0, 4)
    for _, e in ipairs(fr.extra or {}) do
      W.fillRect(img, (fr.ox or 0)*4 + e[1]*4, e[2]*4, 4, 4, e[3])
    end
    local cel = spr:newCel(spr.layers[1], i)
    cel.image = img
    cel.position = Point(0, 0)
    spr.frames[i].duration = durMs / 1000.0
  end
  spr:saveCopyAs(path)
  print("gif: " .. path)
end

-- ------------------------------------------------------------------- fogli
local GM = W.maps.goblin
local goblinSheet = {
  id = "goblin-di-slag", dir = "enemies", fam = "enemies",
  fw = 32, fh = 32, anchor = { 16, 30 },
  rows = {
    { name = "walk", fps = 8, loop = true, frames = {
      { grid = walk1, map = GM }, { grid = walk2, map = GM },
      { grid = walk3, map = GM }, { grid = walk4, map = GM } } },
    { name = "hit", fps = 10, loop = false, frames = {
      { grid = walk1, map = hitMap, ox = 1 } } },
    { name = "death", fps = 10, loop = false, frames = {
      { grid = death1, map = deathMap }, { grid = death2, map = deathMap },
      { grid = death3, map = deathMap }, { grid = death4, map = deathMap } } },
  },
}

local PM = W.maps.potion
local potionSheet = {
  id = "fiala-di-brace", dir = "items", fam = "items",
  fw = 32, fh = 32, anchor = { 16, 23 },
  rows = {
    { name = "idle", fps = 1, loop = true, frames = {
      { grid = W.grids.potion32, map = PM } } },
    { name = "glow", fps = 6, loop = true, frames = {
      { grid = W.grids.potion32, map = PM, extra = glow1px },
      { grid = W.grids.potion32, map = PM, extra = glow2px } } },
  },
}

local orbSheet = {
  id = "orb", dir = "shots", fam = "shots",
  fw = 16, fh = 16, anchor = { 8, 8 },
  rows = {
    { name = "fly", fps = 8, loop = true, frames = {
      { grid = orb1, map = orbMap }, { grid = orb2, map = orbMap } } },
    { name = "impact", fps = 14, loop = false, frames = {
      { grid = imp1, map = orbMap }, { grid = imp2, map = orbMap },
      { grid = imp3, map = orbMap } } },
  },
}

buildSheet(goblinSheet)
buildSheet(potionSheet)
buildSheet(orbSheet)

-- ------------------------------------------------------------------ dataset
local CAP = "pixel art, worldsmelt style, black 1px outline, flat shading, fucina palette, "
local function cap(rest) return CAP .. rest end

for _, sh in ipairs({ goblinSheet, potionSheet, orbSheet }) do
  for _, row in ipairs(sh.rows) do
    for fi, fr in ipairs(row.frames) do
      local subj
      if sh.id == "goblin-di-slag" then
        subj = "goblin enemy, green skin, loincloth, " .. row.name ..
               " animation frame " .. fi .. " of " .. #row.frames
        if row.name == "death" then subj = subj .. ", melting into green slag" end
      elseif sh.id == "fiala-di-brace" then
        subj = "potion item pickup, glass vial with molten ember liquid, " ..
               row.name .. " animation frame " .. fi .. " of " .. #row.frames
      else
        subj = "orb projectile shot, molten glow, " .. row.name ..
               " animation frame " .. fi .. " of " .. #row.frames
      end
      datasetFrame(sh.fam, sh.id .. "_" .. row.name .. "_" .. fi, fr,
                   sh.fw, sh.fh, cap(subj .. ", " .. sh.fw .. "x" .. sh.fh ..
                   ", transparent background"))
    end
  end
end

-- --------------------------------------------------------------- anteprime
local PV = ROOT .. "/assets/art-src/preview-cp3/"
previewGif(PV .. "goblin-walk.gif", goblinSheet.rows[1].frames, GM, 32, 32, 125, C.ard_s)
previewGif(PV .. "goblin-death.gif", goblinSheet.rows[3].frames, deathMap, 32, 32, 220, C.ard_s)
previewGif(PV .. "fiala-glow.gif", {
  { grid = W.grids.potion32, map = PM },
  { grid = W.grids.potion32, map = PM, extra = glow1px },
  { grid = W.grids.potion32, map = PM, extra = glow2px },
}, PM, 32, 32, 170, C.ard_s)
previewGif(PV .. "orb-fly-impact.gif", {
  { grid = orb1, map = orbMap }, { grid = orb2, map = orbMap },
  { grid = orb1, map = orbMap }, { grid = orb2, map = orbMap },
  { grid = imp1, map = orbMap }, { grid = imp2, map = orbMap },
  { grid = imp3, map = orbMap },
}, orbMap, 16, 16, 110, C.ard_s)

print("CP3 OK")
