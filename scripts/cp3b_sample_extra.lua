-- CP3b — estensione del campione su richiesta del proprietario:
--   1) le 4 forme di colpo restanti del motore (spike, beam, arc, blade)
--      -> con orb (cp3_sample.lua) TUTTE le SHOT_FORM_* hanno sprite;
--   2) due creature NON umanoidi che attaccano A CONTATTO (niente spari),
--      con anim "attack" dedicata oltre a walk/hit/death:
--      - melma-di-brace: blob fuso, camminata a balzi, lancio in avanti
--      - ragno-di-cenere: aracnide di cenere, zampettio, balzo con zanne
-- Stesso formato-contratto e stesso doppio export dataset di cp3_sample.lua.
-- Riproducibile: aseprite -b --script scripts/cp3b_sample_extra.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local function D(n) return string.rep(".", n) end
local function R(ch, n) return string.rep(ch, n) end

-- --------------------------------------------------- costruttore di griglie
local function blank(n)
  local g = {}
  for y = 1, n do
    g[y] = {}
    for x = 1, n do g[y][x] = "." end
  end
  g.n = n
  return g
end
local function set(g, x, y, ch)
  if x >= 1 and y >= 1 and x <= g.n and y <= g.n then g[y][x] = ch end
end
local function seg(g, x1, y1, x2, y2, ch)
  local steps = math.max(math.abs(x2 - x1), math.abs(y2 - y1))
  if steps == 0 then set(g, x1, y1, ch) return end
  for i = 0, steps do
    local x = math.floor(x1 + (x2 - x1) * i / steps + 0.5)
    local y = math.floor(y1 + (y2 - y1) * i / steps + 0.5)
    set(g, x, y, ch)
  end
end
local function ellipseFill(g, cx, cy, rx, ry, ch)
  for y = cy - ry, cy + ry do
    for x = cx - rx, cx + rx do
      local dx, dy = (x - cx) / rx, (y - cy) / ry
      if dx * dx + dy * dy <= 1.0 then set(g, x, y, ch) end
    end
  end
end
local function toRows(g)
  local rows = {}
  for y = 1, g.n do rows[y] = table.concat(g[y]) end
  return rows
end

-- ------------------------------------------------------- melma di brace
-- Cupola a base piatta parametrica: w/h/lean variano per frame.
local function slime(w, h, opts)
  opts = opts or {}
  local g = blank(32)
  local bottom = opts.bottom or 29
  local top = bottom - h + 1
  for i = 0, h - 1 do
    local t = (h == 1) and 1 or i / (h - 1)
    local half = math.floor((w / 2) * math.sqrt(1 - (1 - t) ^ 2) + 0.5)
    if half < 1 then half = 1 end
    local lean = math.floor((opts.lean or 0) * (1 - t) + 0.5)
    local y = top + i
    for x = 17 - half, 16 + half do
      local ch = "b"
      if i <= 1 then ch = "f"                                -- luce in cima
      elseif x > 16 + half - 2 then ch = "B" end             -- ombra a destra
      if y == bottom then ch = "B" end                       -- base in ombra
      set(g, x + lean, y, ch)
    end
  end
  if opts.eyes then
    local ey = top + math.floor(h * 0.45)
    local lean = math.floor((opts.lean or 0) * 0.5 + 0.5)
    for _, ex in ipairs({ 12, 19 }) do
      set(g, ex + lean, ey, "p"); set(g, ex + 1 + lean, ey, "p")
      set(g, ex + lean, ey + 1, "p"); set(g, ex + 1 + lean, ey + 1, "p")
    end
  end
  if opts.core then
    local cy2 = bottom - 3
    for x = 14, 17 do set(g, x, cy2, "g"); set(g, x, cy2 + 1, "g") end
  end
  if opts.drips then
    set(g, 9, bottom + 1, "B"); set(g, 16, bottom + 1, "B")
    set(g, 24, bottom + 1, "B")
  end
  return toRows(g)
end

local slimeMap = {
  b=C.brace, B=C.brace_s, f=C.fiamma, g=C.bagliore, p=C.slag_scuro,
}
local slimeHitMap = {
  b=C.fumo, B=C.cen_chiara, f=C.bianco, g=C.bianco, p=C.brace_s,
}

local slWalk = {
  slime(24, 13, { eyes = true, core = true }),
  slime(27, 10, { eyes = true, core = true, drips = true }),
  slime(18, 17, { eyes = true, core = true, bottom = 27 }),
  slime(25, 12, { eyes = true, core = true, drips = true }),
}
local slAttack = {
  slime(26, 11, { eyes = true, core = true, lean = -4 }),
  slime(21, 16, { eyes = true, core = true, lean = 8, drips = true }),
  slime(24, 13, { eyes = true, core = true, lean = 2 }),
}
local slDeath = {
  slime(26, 9, { eyes = true }),
  slime(28, 6, { drips = true }),
  slime(28, 4, { core = true }),
  slime(26, 2, {}),
}

-- ------------------------------------------------------- ragno di cenere
local function spiderBody(g, dy, ry)
  ellipseFill(g, 16, 19 + dy, 7, ry or 4, "a")      -- addome
  ellipseFill(g, 16, 14 + dy, 5, 3, "a")            -- capo
  for x = 9, 23 do set(g, x, 19 + dy + (ry or 4), "A") end
  set(g, 13, 14 + dy, "e"); set(g, 14, 14 + dy, "e")
  set(g, 18, 14 + dy, "e"); set(g, 19, 14 + dy, "e")
end
local function mirror(x) return 33 - x end
local function spiderLegs(g, dy, pose)
  -- tre zampe per lato; pose = tabella di 3 endpoint {x,y} (lato sinistro)
  local hips = { { 10, 15 }, { 9, 18 }, { 10, 21 } }
  for i, hp in ipairs(hips) do
    local e = pose[i]
    seg(g, hp[1], hp[2] + dy, e[1], e[2] + dy, "a")
    seg(g, mirror(hp[1]), hp[2] + dy, mirror(e[1]), e[2] + dy, "a")
  end
end
local poseA = { { 4, 10 }, { 2, 18 }, { 5, 26 } }
local poseB = { { 3, 13 }, { 2, 21 }, { 8, 27 } }

local function spiderFrame(pose, dy, opts)
  opts = opts or {}
  local g = blank(32)
  spiderBody(g, dy, opts.ry)
  if pose then spiderLegs(g, dy, pose) end
  if opts.fangs then
    set(g, 14, 17 + dy, "t"); set(g, 17, 17 + dy, "t")
    set(g, 14, 18 + dy, "t"); set(g, 17, 18 + dy, "t")
  end
  if opts.extra then opts.extra(g) end
  return toRows(g)
end

local spiderMap = { a=C.cen_scura, A=C.cen_nera, e=C.fiamma, t=C.bianco }
local spiderHitMap = { a=C.fumo, A=C.cen_chiara, e=C.brace, t=C.bianco }

local spWalk = {
  spiderFrame(poseA, 0),
  spiderFrame(poseB, 1),
  spiderFrame(poseA, 0),
  spiderFrame({ { 3, 13 }, { 2, 15 }, { 6, 27 } }, 1),
}
local spAttack = {
  -- impennata: corpo su, zampe davanti alzate
  spiderFrame({ { 3, 4 }, { 2, 16 }, { 5, 26 } }, -2, { fangs = true }),
  -- balzo: corpo schiacciato avanti, zampe distese, zanne fuori
  spiderFrame({ { 1, 17 }, { 2, 20 }, { 4, 27 } }, 3, { ry = 3, fangs = true }),
  spiderFrame(poseA, 0, { fangs = true }),
}
local spDeath = {
  -- zampe che si ripiegano
  spiderFrame({ { 6, 12 }, { 5, 18 }, { 7, 24 } }, 1),
  -- palla raggomitolata con moncherini
  spiderFrame({ { 8, 13 }, { 7, 18 }, { 8, 22 } }, 2, { ry = 3 }),
  -- mucchietto di cenere
  (function()
    local g = blank(32)
    for i = 0, 3 do
      local half = 9 - i * 2
      for x = 16 - half, 16 + half do set(g, x, 28 - i, i == 0 and "A" or "a") end
    end
    set(g, 12, 26, "c"); set(g, 19, 25, "c"); set(g, 16, 27, "c")
    return toRows(g)
  end)(),
  (function()
    local g = blank(32)
    for i = 0, 1 do
      local half = 7 - i * 3
      for x = 16 - half, 16 + half do set(g, x, 28 - i, i == 0 and "A" or "a") end
    end
    set(g, 14, 27, "c")
    return toRows(g)
  end)(),
}
spiderMap.c = C.cenere
spiderHitMap.c = C.cenere

-- --------------------------------------------------------------- colpi
-- spike: dardo orientato (la velocita' lo ruota nel motore)
local spikeMap = { t=C.brace, f=C.fiamma, c=C.bagliore }
local sp1 = {
  D(16), D(16), D(16), D(16), D(16), D(16),
  D(12).."c"..D(3),
  D(3).."ttt".."ffffff".."cc"..D(2),
  D(3).."ttt".."ffffff".."cc"..D(2),
  D(12).."c"..D(3),
  D(16), D(16), D(16), D(16), D(16), D(16),
}
local sp2 = {
  D(16), D(16), D(16), D(16), D(16), D(16),
  D(12).."c"..D(3),
  D(4).."tt".."ffffff".."ccc"..D(1),
  D(4).."tt".."ffffff".."ccc"..D(1),
  D(12).."c"..D(3),
  D(16), D(16), D(16), D(16), D(16), D(16),
}
local spImp = {
  { D(16), D(16), D(16), D(16), D(16), D(16),
    D(7).."cc"..D(7), D(6).."cccc"..D(6), D(6).."cccc"..D(6),
    D(7).."cc"..D(7), D(16), D(16), D(16), D(16), D(16), D(16) },
  { D(16), D(16), D(16),
    D(3).."f"..D(8).."f"..D(3),
    D(4).."c"..D(6).."c"..D(4),
    D(16),
    D(7).."cc"..D(7),
    D(7).."cc"..D(7),
    D(16),
    D(4).."c"..D(6).."c"..D(4),
    D(3).."f"..D(8).."f"..D(3),
    D(16), D(16), D(16), D(16), D(16) },
  { D(16), D(16), D(16), D(16),
    D(5).."t"..D(4).."t"..D(5),
    D(16),
    D(3).."t"..D(8).."t"..D(3),
    D(16),
    D(6).."f"..D(2).."t"..D(6),
    D(16),
    D(4).."t"..D(7).."t"..D(3),
    D(16), D(16), D(16), D(16), D(16) },
}

-- beam: segmento di raggio con scia (il motore lo allunga/ripete)
local beamMap = { t=C.brace, f=C.fiamma, c=C.bagliore }
local bm1 = {
  D(16), D(16), D(16), D(16), D(16), D(16),
  D(1).."t".."ff".."cccccccc".."ff".."t"..D(1),
  D(1).."t".."ff".."cccccccc".."ff".."t"..D(1),
  D(16), D(16), D(16), D(16), D(16), D(16), D(16), D(16),
}
local bm2 = {
  D(16), D(16), D(16), D(16), D(16),
  D(4).."tt"..D(4).."tt"..D(4),
  D(1).."t".."ff".."cccccccc".."ff".."t"..D(1),
  D(4).."tt"..D(4).."tt"..D(4),
  D(16), D(16), D(16), D(16), D(16), D(16), D(16), D(16),
}
local bmImp = {
  { D(16), D(16), D(16),
    D(6).."ffff"..D(6),
    D(7).."cc"..D(7), D(7).."cc"..D(7), D(7).."cc"..D(7),
    D(7).."cc"..D(7), D(7).."cc"..D(7), D(7).."cc"..D(7),
    D(6).."ffff"..D(6),
    D(16), D(16), D(16), D(16), D(16) },
  { D(16), D(16), D(16),
    D(4).."f"..D(6).."f"..D(4),
    D(5).."c"..D(4).."c"..D(5),
    D(6).."c"..D(2).."c"..D(6),
    D(7).."cc"..D(7),
    D(6).."c"..D(2).."c"..D(6),
    D(5).."c"..D(4).."c"..D(5),
    D(4).."f"..D(6).."f"..D(4),
    D(16), D(16), D(16), D(16), D(16), D(16) },
  { D(16), D(16), D(16), D(16),
    D(5).."t"..D(4).."t"..D(5),
    D(16),
    D(3).."t"..D(8).."t"..D(3),
    D(16),
    D(6).."f"..D(2).."t"..D(6),
    D(16),
    D(4).."t"..D(7).."t"..D(3),
    D(16), D(16), D(16), D(16), D(16) },
}

-- arc: scarica a zig-zag (fredda: ardesia/bianco)
local arcMap = { c=C.bianco, f=C.ard_p, t=C.ard_c }
local function zig(points)
  local g = blank(16)
  for i = 1, #points - 1 do
    seg(g, points[i][1], points[i][2], points[i+1][1], points[i+1][2], "c")
    seg(g, points[i][1], points[i][2] + 1, points[i+1][1], points[i+1][2] + 1, "f")
  end
  return toRows(g)
end
local ar1 = zig({ { 2, 10 }, { 6, 5 }, { 9, 10 }, { 14, 5 } })
local ar2 = zig({ { 2, 5 }, { 6, 10 }, { 9, 5 }, { 14, 10 } })
local arImp = {
  (function()
    local g = blank(16)
    seg(g, 8, 4, 8, 12, "c"); seg(g, 4, 8, 12, 8, "c")
    return toRows(g)
  end)(),
  (function()
    local g = blank(16)
    seg(g, 4, 4, 12, 12, "c"); seg(g, 12, 4, 4, 12, "c")
    seg(g, 8, 2, 8, 5, "f"); seg(g, 8, 11, 8, 14, "f")
    seg(g, 2, 8, 5, 8, "f"); seg(g, 11, 8, 14, 8, "f")
    return toRows(g)
  end)(),
  (function()
    local g = blank(16)
    set(g, 4, 4, "t"); set(g, 12, 5, "t"); set(g, 3, 11, "t")
    set(g, 13, 12, "t"); set(g, 8, 3, "f"); set(g, 7, 13, "t")
    return toRows(g)
  end)(),
}

-- blade: lama che ruota (quadrato <-> rombo, metallo freddo)
local bladeMap = { f=C.cen_chiara, t=C.cenere, c=C.bianco }
local bl1 = {
  D(16), D(16), D(16), D(16), D(16),
  D(4)..R("f",8)..D(4),
  D(4).."f"..R("t",6).."f"..D(4),
  D(4).."f".."tt".."cc".."tt".."f"..D(4),
  D(4).."f".."tt".."cc".."tt".."f"..D(4),
  D(4).."f"..R("t",6).."f"..D(4),
  D(4)..R("f",8)..D(4),
  D(16), D(16), D(16), D(16), D(16),
}
local bl2 = (function()
  local g = blank(16)
  for i = 0, 5 do
    -- rombo cavo di raggio 5 centrato in (8,8)
    set(g, 8 - i, 3 + i, "f"); set(g, 8 + i, 3 + i, "f")
    set(g, 8 - i, 13 - i, "f"); set(g, 8 + i, 13 - i, "f")
  end
  ellipseFill(g, 8, 8, 2, 2, "t")
  set(g, 8, 8, "c"); set(g, 9, 8, "c")
  return toRows(g)
end)()
local blImp = {
  { D(16), D(16), D(16), D(16), D(16), D(16),
    D(7).."cc"..D(7), D(6).."cccc"..D(6), D(6).."cccc"..D(6),
    D(7).."cc"..D(7), D(16), D(16), D(16), D(16), D(16), D(16) },
  (function()
    local g = blank(16)
    for _, q in ipairs({ { 4, 4 }, { 11, 3 }, { 3, 11 }, { 12, 11 } }) do
      set(g, q[1], q[2], "f"); set(g, q[1] + 1, q[2], "f")
      set(g, q[1], q[2] + 1, "f"); set(g, q[1] + 1, q[2] + 1, "f")
    end
    ellipseFill(g, 8, 8, 1, 1, "c")
    return toRows(g)
  end)(),
  (function()
    local g = blank(16)
    set(g, 3, 5, "t"); set(g, 12, 4, "t"); set(g, 4, 12, "t")
    set(g, 13, 11, "t"); set(g, 8, 8, "t")
    return toRows(g)
  end)(),
}

-- --------------------------------------------- riuso dell'infrastruttura CP3
-- (stesse funzioni di cp3_sample.lua: qui duplicate in piccolo per tenere i
-- due script indipendenti e rilanciabili da soli)
local function mkPalette()
  local pal = Palette(#W.PAL)
  for i, p in ipairs(W.PAL) do
    pal:setColor(i - 1, Color{ r = p[1], g = p[2], b = p[3] })
  end
  return pal
end

local function buildSheet(sheet)
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
  spr:saveAs(ROOT .. "/assets/art-src/" .. sheet.dir .. "/" .. sheet.id .. ".aseprite")
  spr:saveCopyAs(ROOT .. "/assets/art/" .. sheet.dir .. "/" .. sheet.id .. ".png")
  local parts = {}
  for ri, row in ipairs(sheet.rows) do
    parts[#parts + 1] = '"' .. row.name .. '":{"row":' .. (ri - 1) ..
      ',"frames":' .. #row.frames .. ',"fps":' .. row.fps ..
      ',"loop":' .. tostring(row.loop) .. '}'
  end
  local f = io.open(ROOT .. "/assets/art/" .. sheet.dir .. "/" .. sheet.id .. ".json", "w")
  f:write('{"frame_w":' .. sheet.fw .. ',"frame_h":' .. sheet.fh ..
          ',"anchor":[' .. sheet.anchor[1] .. ',' .. sheet.anchor[2] ..
          '],"anims":{' .. table.concat(parts, ",") .. "}}\n")
  f:close()
  print(sheet.id .. ": sheet " .. sw .. "x" .. sh .. " ok")
end

local function datasetFrame(fam, name, fr, fw, fh, caption)
  local spr = Sprite(64, 64, ColorMode.RGB)
  spr:setPalette(mkPalette())
  local img = Image(64, 64, ColorMode.RGB)
  W.renderS1(img, fr.grid, fr.map, math.floor((64 - fw) / 2),
             math.floor((64 - fh) / 2), 1)
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

local function previewGif(path, frames, fw, fh, durMs, bgIdx)
  local spr = Sprite(fw * 4, fh * 4, ColorMode.RGB)
  spr:setPalette(mkPalette())
  while #spr.frames < #frames do spr:newEmptyFrame() end
  for i, fr in ipairs(frames) do
    local img = Image(fw * 4, fh * 4, ColorMode.RGB)
    W.fillRect(img, 0, 0, fw * 4, fh * 4, bgIdx)
    W.renderS1(img, fr.grid, fr.map, 0, 0, 4)
    local cel = spr:newCel(spr.layers[1], i)
    cel.image = img
    cel.position = Point(0, 0)
    spr.frames[i].duration = (fr.dur or durMs) / 1000.0
  end
  spr:saveCopyAs(path)
  print("gif: " .. path)
end

-- ------------------------------------------------------------------- fogli
local function frames(list, map)
  local out = {}
  for _, g in ipairs(list) do out[#out + 1] = { grid = g, map = map } end
  return out
end

local slimeSheet = {
  id = "melma-di-brace", dir = "enemies", fam = "enemies",
  fw = 32, fh = 32, anchor = { 16, 30 },
  rows = {
    { name = "walk", fps = 8, loop = true, frames = frames(slWalk, slimeMap) },
    { name = "attack", fps = 10, loop = false, frames = frames(slAttack, slimeMap) },
    { name = "hit", fps = 10, loop = false, frames = {
      { grid = slWalk[1], map = slimeHitMap, ox = 1 } } },
    { name = "death", fps = 9, loop = false, frames = frames(slDeath, slimeMap) },
  },
}
local spiderSheet = {
  id = "ragno-di-cenere", dir = "enemies", fam = "enemies",
  fw = 32, fh = 32, anchor = { 16, 29 },
  rows = {
    { name = "walk", fps = 10, loop = true, frames = frames(spWalk, spiderMap) },
    { name = "attack", fps = 10, loop = false, frames = frames(spAttack, spiderMap) },
    { name = "hit", fps = 10, loop = false, frames = {
      { grid = spWalk[1], map = spiderHitMap, ox = 1 } } },
    { name = "death", fps = 9, loop = false, frames = frames(spDeath, spiderMap) },
  },
}
local function shotSheet(id, fly1, fly2, imp, map)
  return {
    id = id, dir = "shots", fam = "shots", fw = 16, fh = 16, anchor = { 8, 8 },
    rows = {
      { name = "fly", fps = 10, loop = true, frames = frames({ fly1, fly2 }, map) },
      { name = "impact", fps = 14, loop = false, frames = frames(imp, map) },
    },
  }
end
local spikeSheet = shotSheet("spike", sp1, sp2, spImp, spikeMap)
local beamSheet = shotSheet("beam", bm1, bm2, bmImp, beamMap)
local arcSheet = shotSheet("arc", ar1, ar2, arImp, arcMap)
local bladeSheet = shotSheet("blade", bl1, bl2, blImp, bladeMap)

local ALL = { slimeSheet, spiderSheet, spikeSheet, beamSheet, arcSheet, bladeSheet }
for _, sh in ipairs(ALL) do buildSheet(sh) end

-- ------------------------------------------------------------------ dataset
local CAP = "pixel art, worldsmelt style, black 1px outline, flat shading, fucina palette, "
local SUBJ = {
  ["melma-di-brace"] = "molten slag slime enemy, amorphous ember blob, melee contact attacker",
  ["ragno-di-cenere"] = "ash spider enemy, non-humanoid crawler, ember eyes, melee contact attacker",
  ["spike"] = "spike dart projectile, elongated molten shard",
  ["beam"] = "beam ray projectile segment, molten streak",
  ["arc"] = "arc lightning projectile, zigzag discharge, slate blue",
  ["blade"] = "spinning blade projectile, rotating steel square",
}
for _, sh in ipairs(ALL) do
  for _, row in ipairs(sh.rows) do
    for fi, fr in ipairs(row.frames) do
      datasetFrame(sh.fam, sh.id .. "_" .. row.name .. "_" .. fi, fr, sh.fw, sh.fh,
        CAP .. SUBJ[sh.id] .. ", " .. row.name .. " animation frame " .. fi ..
        " of " .. #row.frames .. ", " .. sh.fw .. "x" .. sh.fh ..
        ", transparent background")
    end
  end
end

-- --------------------------------------------------------------- anteprime
local PV = ROOT .. "/assets/art-src/preview-cp3/"
local function showcase(sheet, map)
  local seqs = {}
  local byName = {}
  for _, row in ipairs(sheet.rows) do byName[row.name] = row.frames end
  for _ = 1, 2 do
    for _, fr in ipairs(byName.walk) do seqs[#seqs + 1] = fr end
  end
  for _, fr in ipairs(byName.attack or {}) do
    seqs[#seqs + 1] = { grid = fr.grid, map = fr.map, dur = 140 }
  end
  seqs[#seqs + 1] = { grid = byName.hit[1].grid, map = byName.hit[1].map, dur = 120 }
  for _, fr in ipairs(byName.death) do
    seqs[#seqs + 1] = { grid = fr.grid, map = fr.map, dur = 200 }
  end
  previewGif(PV .. sheet.id .. "-showcase.gif", seqs, 32, 32, 125, C.ard_s)
end
showcase(slimeSheet)
showcase(spiderSheet)
for _, sh in ipairs({ spikeSheet, beamSheet, arcSheet, bladeSheet }) do
  local seqs = {}
  for _ = 1, 3 do
    for _, fr in ipairs(sh.rows[1].frames) do seqs[#seqs + 1] = fr end
  end
  for _, fr in ipairs(sh.rows[2].frames) do
    seqs[#seqs + 1] = { grid = fr.grid, map = fr.map, dur = 90 }
  end
  previewGif(PV .. sh.id .. "-fly-impact.gif", seqs, 16, 16, 110, C.ard_s)
end

print("CP3b OK")
