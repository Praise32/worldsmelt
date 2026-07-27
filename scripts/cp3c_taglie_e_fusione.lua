-- CP3c — risposte al feedback del proprietario sul campione:
--   1) ragno-di-cenere RIFATTO: zampe snodate a due segmenti (andatura a
--      tripode), 4 occhi di brace, crepe di brace sull'addome, zanne;
--   2) TIER DI TAGLIA disegnati (le taglie runtime restano del motore, che
--      scala via sizeMul/radiusMul; i tier tengono la griglia pixel pulita):
--      - enemies/melma-di-brace-grande: stessa melma a 48px, set completo
--      - shots/orb-grande: orb a 24px (fly 2f + impact 3f)
--   3) esempio di COLPO NATO DA FUSIONE, composto con la regola del motore
--      (nome dai genitori, tratti da entrambi i gruppi): "Meteora di Chiodi"
--      = orb dominante + spike -> nucleo orbitale con schegge che ruotano.
-- Riproducibile: aseprite -b --script scripts/cp3c_taglie_e_fusione.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local blank, set, seg, ell, rows = W.blank, W.gset, W.gseg, W.gellipse, W.grows

-- ------------------------------------------------- ragno di cenere (rifatto)
local spiderMap = {
  a=C.cen_scura, A=C.cen_nera, e=C.fiamma, E=C.bagliore, t=C.bianco,
  r=C.brace, c=C.cenere,
}
local spiderHitMap = {
  a=C.fumo, A=C.cen_chiara, e=C.brace, E=C.brace, t=C.bianco,
  r=C.brace_s, c=C.cenere,
}
local function mir(x) return 33 - x end

-- pose zampe: 3 zampe per lato, ognuna {knee={x,y}, foot={x,y}} (lato sx)
local legA = {
  { hip = { 11, 16 }, knee = { 6, 10 }, foot = { 3, 14 } },
  { hip = { 10, 18 }, knee = { 3, 15 }, foot = { 1, 21 } },
  { hip = { 10, 21 }, knee = { 4, 24 }, foot = { 2, 28 } },
}
local legB = {
  { hip = { 11, 16 }, knee = { 5, 12 }, foot = { 2, 17 } },
  { hip = { 10, 18 }, knee = { 4, 18 }, foot = { 2, 24 } },
  { hip = { 10, 21 }, knee = { 5, 26 }, foot = { 4, 29 } },
}
local function drawLegs(g, pose, side, dy)
  for _, l in ipairs(pose) do
    local hx, hy = l.hip[1], l.hip[2] + dy
    local kx, ky = l.knee[1], l.knee[2] + dy
    local fx, fy = l.foot[1], l.foot[2] + dy
    if side == "right" then hx, kx, fx = mir(hx), mir(kx), mir(fx) end
    seg(g, hx, hy, kx, ky, "a")
    seg(g, kx, ky, fx, fy, "a")
  end
end
local function spiderBody(g, hdY, abY, abRy, opts)
  opts = opts or {}
  ell(g, 16, abY, 7, abRy, "a")                       -- addome
  ell(g, 16, hdY, 4, 3, "a")                          -- capo
  seg(g, 14, hdY + 2, 14, abY - abRy, "a")            -- collo
  seg(g, 18, hdY + 2, 18, abY - abRy, "a")
  for x = 10, 22 do set(g, x, abY + abRy, "A") end    -- ombra a terra
  -- crepe di brace sull'addome
  set(g, 14, abY - 1, "r"); set(g, 18, abY + 1, "r")
  set(g, 16, abY + 2, "r"); set(g, 13, abY + 1, "r")
  -- 4 occhi: due grandi di fiamma, due piccoli di bagliore
  set(g, 14, hdY, "e"); set(g, 15, hdY, "e")
  set(g, 18, hdY, "e"); set(g, 19, hdY, "e")
  set(g, 16, hdY - 1, "E"); set(g, 17, hdY - 1, "E")
  if opts.fangs then
    set(g, 14, hdY + 3, "t"); set(g, 15, hdY + 3, "t")
    set(g, 18, hdY + 3, "t"); set(g, 19, hdY + 3, "t")
    if opts.bigFangs then
      set(g, 14, hdY + 4, "t"); set(g, 19, hdY + 4, "t")
    end
  end
end
local function spiderFrame(poseL, poseR, dy, opts)
  opts = opts or {}
  local g = blank(32)
  drawLegs(g, poseL, "left", dy)
  drawLegs(g, poseR, "right", dy)
  spiderBody(g, (opts.hdY or 12) + dy, (opts.abY or 20) + dy,
             opts.abRy or 5, opts)
  if opts.extra then opts.extra(g) end
  return rows(g)
end

local spWalk = {
  spiderFrame(legA, legB, 0),
  spiderFrame(legB, legA, 1),
  spiderFrame(legA, legB, 0, { extra = function(g) set(g, 3, 15, "a") end }),
  spiderFrame(legB, legA, 1, { extra = function(g) set(g, 30, 18, "a") end }),
}
local legRear = {
  { hip = { 12, 14 }, knee = { 6, 5 }, foot = { 3, 9 } },
  { hip = { 10, 18 }, knee = { 3, 14 }, foot = { 1, 20 } },
  { hip = { 10, 21 }, knee = { 4, 24 }, foot = { 2, 28 } },
}
local legPounce = {
  { hip = { 11, 15 }, knee = { 4, 11 }, foot = { 1, 16 } },
  { hip = { 10, 17 }, knee = { 2, 17 }, foot = { 1, 23 } },
  { hip = { 10, 19 }, knee = { 4, 24 }, foot = { 1, 28 } },
}
local spAttack = {
  spiderFrame(legRear, legRear, 0, { hdY = 8, abY = 21, fangs = true }),
  spiderFrame(legPounce, legPounce, 2, { hdY = 14, abY = 20, abRy = 3,
                                         fangs = true, bigFangs = true }),
  spiderFrame(legA, legB, 0, { fangs = true }),
}
local legCurl = {
  { hip = { 11, 16 }, knee = { 7, 12 }, foot = { 9, 15 } },
  { hip = { 10, 18 }, knee = { 6, 17 }, foot = { 9, 20 } },
  { hip = { 10, 21 }, knee = { 7, 25 }, foot = { 10, 23 } },
}
local spDeath = {
  spiderFrame(legCurl, legCurl, 1),
  (function()
    local g = blank(32)
    ell(g, 16, 22, 6, 5, "a")
    for _, p in ipairs({ {9,19},{8,23},{12,27},{20,27},{24,20},{23,25} }) do
      seg(g, p[1], p[2], p[1] + 1, p[2] + 1, "a")
    end
    for x = 11, 21 do set(g, x, 27, "A") end
    set(g, 14, 21, "r"); set(g, 18, 23, "r")
    return rows(g)
  end)(),
  (function()
    local g = blank(32)
    for i = 0, 3 do
      local half = 9 - i * 2
      for x = 16 - half, 16 + half do set(g, x, 28 - i, i == 0 and "A" or "a") end
    end
    set(g, 12, 26, "c"); set(g, 19, 25, "c"); set(g, 16, 27, "r")
    return rows(g)
  end)(),
  (function()
    local g = blank(32)
    for i = 0, 1 do
      local half = 7 - i * 3
      for x = 16 - half, 16 + half do set(g, x, 28 - i, i == 0 and "A" or "a") end
    end
    set(g, 14, 27, "c")
    return rows(g)
  end)(),
}

-- --------------------------------------- melma grande (tier di taglia 48px)
local function slimeN(n, w, h, opts)
  opts = opts or {}
  local g = blank(n)
  local cx = math.floor(n / 2)
  local bottom = opts.bottom or (n - 3)
  local top = bottom - h + 1
  for i = 0, h - 1 do
    local t = (h == 1) and 1 or i / (h - 1)
    local half = math.floor((w / 2) * math.sqrt(1 - (1 - t) ^ 2) + 0.5)
    if half < 1 then half = 1 end
    local lean = math.floor((opts.lean or 0) * (1 - t) + 0.5)
    local y = top + i
    for x = cx + 1 - half, cx + half do
      local ch = "b"
      if i <= 1 then ch = "f"
      elseif x > cx + half - 2 then ch = "B" end
      if y == bottom then ch = "B" end
      set(g, x + lean, y, ch)
    end
  end
  if opts.eyes then
    local ey = top + math.floor(h * 0.45)
    local lean = math.floor((opts.lean or 0) * 0.5 + 0.5)
    local off = math.floor(w * 0.16)
    for _, ex in ipairs({ cx - off - 1, cx + off }) do
      for dy2 = 0, 1 do
        set(g, ex + lean, ey + dy2, "p"); set(g, ex + 1 + lean, ey + dy2, "p")
      end
    end
  end
  if opts.core then
    for x = cx - 2, cx + 2 do
      set(g, x, bottom - 4, "g"); set(g, x, bottom - 3, "g")
    end
  end
  if opts.drips then
    set(g, cx - 10, bottom + 1, "B"); set(g, cx, bottom + 1, "B")
    set(g, cx + 11, bottom + 1, "B")
  end
  return rows(g)
end
local slimeMap = {
  b=C.brace, B=C.brace_s, f=C.fiamma, g=C.bagliore, p=C.slag_scuro,
}
local slimeHitMap = {
  b=C.fumo, B=C.cen_chiara, f=C.bianco, g=C.bianco, p=C.brace_s,
}
local sgWalk = {
  slimeN(48, 36, 20, { eyes = true, core = true }),
  slimeN(48, 41, 15, { eyes = true, core = true, drips = true }),
  slimeN(48, 27, 26, { eyes = true, core = true, bottom = 42 }),
  slimeN(48, 38, 18, { eyes = true, core = true, drips = true }),
}
local sgAttack = {
  slimeN(48, 39, 17, { eyes = true, core = true, lean = -6 }),
  slimeN(48, 32, 24, { eyes = true, core = true, lean = 12, drips = true }),
  slimeN(48, 36, 20, { eyes = true, core = true, lean = 3 }),
}
local sgDeath = {
  slimeN(48, 39, 13, { eyes = true }),
  slimeN(48, 42, 9, { drips = true }),
  slimeN(48, 42, 6, { core = true }),
  slimeN(48, 39, 3, {}),
}

-- ------------------------------------------- orb grande (tier colpo 24px)
local shotMap = { c=C.bagliore, f=C.fiamma, t=C.brace }
local function orb24(coreR, ringR)
  local g = blank(24)
  ell(g, 12, 12, ringR, ringR, "f")
  ell(g, 12, 12, coreR, coreR, "c")
  return rows(g)
end
local og1 = orb24(3, 5)
local og2 = orb24(4, 6)
local ogImp = {
  (function()
    local g = blank(24)
    ell(g, 12, 12, 4, 4, "c")
    return rows(g)
  end)(),
  (function()
    local g = blank(24)
    ell(g, 12, 12, 7, 7, "f")
    ell(g, 12, 12, 5, 5, ".")
    ell(g, 12, 12, 2, 2, "c")
    return rows(g)
  end)(),
  (function()
    local g = blank(24)
    for _, p in ipairs({ {5,6},{18,5},{4,16},{19,17},{12,3},{11,20},{7,12},{17,11} }) do
      set(g, p[1], p[2], "t")
    end
    set(g, 12, 12, "f")
    return rows(g)
  end)(),
}

-- ------------------- colpo di fusione: "Meteora di Chiodi" (orb + spike)
-- Regola del motore (item-fusion.md): nome = prima parola della dominante +
-- una parola dell'altra; tratti max 3 dai gruppi di entrambe. Qui la resa:
-- nucleo orbitale (orb, dominante) con schegge-chiodo che gli orbitano
-- attorno (spike) e un impatto che esplode in chiodi.
local function meteora(angleStep)
  local g = blank(24)
  ell(g, 12, 12, 5, 5, "f")
  ell(g, 12, 12, 3, 3, "c")
  for k = 0, 2 do
    local ang = (k * 120 + angleStep) * math.pi / 180
    local sx = math.floor(12 + 8 * math.cos(ang) + 0.5)
    local sy = math.floor(12 + 8 * math.sin(ang) + 0.5)
    local tx = math.floor(12 + 10 * math.cos(ang) + 0.5)
    local ty = math.floor(12 + 10 * math.sin(ang) + 0.5)
    seg(g, sx, sy, tx, ty, "t")
    set(g, tx, ty, "c")
  end
  return rows(g)
end
local mt1 = meteora(0)
local mt2 = meteora(60)
local mtImp = {
  (function()
    local g = blank(24)
    ell(g, 12, 12, 5, 5, "c")
    return rows(g)
  end)(),
  (function()
    local g = blank(24)
    ell(g, 12, 12, 8, 8, "f")
    ell(g, 12, 12, 6, 6, ".")
    ell(g, 12, 12, 2, 2, "c")
    for k = 0, 5 do
      local ang = k * 60 * math.pi / 180
      seg(g, math.floor(12 + 9 * math.cos(ang) + 0.5),
          math.floor(12 + 9 * math.sin(ang) + 0.5),
          math.floor(12 + 11 * math.cos(ang) + 0.5),
          math.floor(12 + 11 * math.sin(ang) + 0.5), "t")
    end
    return rows(g)
  end)(),
  (function()
    local g = blank(24)
    for k = 0, 5 do
      local ang = (k * 60 + 30) * math.pi / 180
      local px2 = math.floor(12 + 10 * math.cos(ang) + 0.5)
      local py2 = math.floor(12 + 10 * math.sin(ang) + 0.5)
      seg(g, px2, py2, px2 + 1, py2, "t")
    end
    set(g, 12, 12, "f"); set(g, 8, 8, "t"); set(g, 16, 15, "t")
    return rows(g)
  end)(),
  (function()
    local g = blank(24)
    for _, p in ipairs({ {4,5},{19,4},{3,17},{20,18},{12,2},{11,21} }) do
      set(g, p[1], p[2], "t")
    end
    return rows(g)
  end)(),
}

-- ------------------------------------------------------------ infrastruttura
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
local function previewGif(path, frames, fw, fh, durMs, bgIdx, scale)
  scale = scale or 4
  local spr = Sprite(fw * scale, fh * scale, ColorMode.RGB)
  spr:setPalette(mkPalette())
  while #spr.frames < #frames do spr:newEmptyFrame() end
  for i, fr in ipairs(frames) do
    local img = Image(fw * scale, fh * scale, ColorMode.RGB)
    W.fillRect(img, 0, 0, fw * scale, fh * scale, bgIdx)
    W.renderS1(img, fr.grid, fr.map, 0, 0, scale)
    local cel = spr:newCel(spr.layers[1], i)
    cel.image = img
    cel.position = Point(0, 0)
    spr.frames[i].duration = (fr.dur or durMs) / 1000.0
  end
  spr:saveCopyAs(path)
  print("gif: " .. path)
end
local function frames(list, map)
  local out = {}
  for _, g in ipairs(list) do out[#out + 1] = { grid = g, map = map } end
  return out
end

-- ------------------------------------------------------------------- fogli
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
local slimeBigSheet = {
  id = "melma-di-brace-grande", dir = "enemies", fam = "enemies",
  fw = 48, fh = 48, anchor = { 24, 45 },
  rows = {
    { name = "walk", fps = 7, loop = true, frames = frames(sgWalk, slimeMap) },
    { name = "attack", fps = 9, loop = false, frames = frames(sgAttack, slimeMap) },
    { name = "hit", fps = 10, loop = false, frames = {
      { grid = sgWalk[1], map = slimeHitMap, ox = 1 } } },
    { name = "death", fps = 8, loop = false, frames = frames(sgDeath, slimeMap) },
  },
}
local orbBigSheet = {
  id = "orb-grande", dir = "shots", fam = "shots",
  fw = 24, fh = 24, anchor = { 12, 12 },
  rows = {
    { name = "fly", fps = 10, loop = true, frames = frames({ og1, og2 }, shotMap) },
    { name = "impact", fps = 14, loop = false, frames = frames(ogImp, shotMap) },
  },
}
local meteoraSheet = {
  id = "fusione-meteora-di-chiodi", dir = "shots", fam = "shots",
  fw = 24, fh = 24, anchor = { 12, 12 },
  rows = {
    { name = "fly", fps = 10, loop = true, frames = frames({ mt1, mt2 }, shotMap) },
    { name = "impact", fps = 14, loop = false, frames = frames(mtImp, shotMap) },
  },
}

local ALL = { spiderSheet, slimeBigSheet, orbBigSheet, meteoraSheet }
for _, sh in ipairs(ALL) do buildSheet(sh) end

-- dataset
local CAP = "pixel art, worldsmelt style, black 1px outline, flat shading, fucina palette, "
local SUBJ = {
  ["ragno-di-cenere"] = "ash spider enemy, jointed legs, four ember eyes, ember cracks, melee contact attacker",
  ["melma-di-brace-grande"] = "large molten slag slime enemy, 48px size tier, melee contact attacker",
  ["orb-grande"] = "large orb projectile, 24px size tier, molten glow",
  ["fusione-meteora-di-chiodi"] = "fused projectile born from item fusion, orb core with orbiting spike shards",
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

-- anteprime
local PV = ROOT .. "/assets/art-src/preview-cp3/"
local function showcase(sheet, fw, scale)
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
  previewGif(PV .. sheet.id .. "-showcase.gif", seqs, fw, fw, 120, C.ard_s, scale)
end
showcase(spiderSheet, 32, 4)
showcase(slimeBigSheet, 48, 4)
for _, sh in ipairs({ orbBigSheet, meteoraSheet }) do
  local seqs = {}
  for _ = 1, 3 do
    for _, fr in ipairs(sh.rows[1].frames) do seqs[#seqs + 1] = fr end
  end
  for _, fr in ipairs(sh.rows[2].frames) do
    seqs[#seqs + 1] = { grid = fr.grid, map = fr.map, dur = 90 }
  end
  previewGif(PV .. sh.id .. "-fly-impact.gif", seqs, 24, 24, 110, C.ard_s, 5)
end

print("CP3c OK")
