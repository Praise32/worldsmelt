-- B2 — sprite dei 14 nemici del catalogo curato (enemies.txt), stile S1+.
-- Quattro costruttori di forma (blob/spiky/armored/floater) parametrizzati su
-- taglia (size del catalogo), palette dai tag e bocca da tiro per fire!=none.
-- Anim: walk 4f, attack 3f, hit 1f, death 4f. Doppio binario come B1.
-- Riproducibile: aseprite -b --script scripts/b2_catalog_enemies.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local blank, set, seg, ell, rows = W.blank, W.gset, W.gseg, W.gellipse, W.grows

-- ------------------------------------------------------------- primitive
local function dome(g, cx, w, h, bottom, mat)
  local top = bottom - h + 1
  for i = 0, h - 1 do
    local t = (h == 1) and 1 or i / (h - 1)
    local half = math.floor((w / 2) * math.sqrt(1 - (1 - t) ^ 2) + 0.5)
    if half < 1 then half = 1 end
    for x = cx - half + 1, cx + half do set(g, x, top + i, mat) end
  end
end
local function eyes(g, cy, gap, glow)
  for _, ex in ipairs({ 16 - gap - 2, 16 + gap }) do
    for dy = 0, 2 do
      for dx = 0, 2 do set(g, ex + dx, cy + dy, "O") end
    end
    set(g, ex, cy, glow and "G" or "s"); set(g, ex + 2, cy, "s")
    set(g, ex, cy + 2, "s"); set(g, ex + 2, cy + 2, "s")
    set(g, ex + 1, cy + 1, glow and "G" or "E")
    if not glow then set(g, ex + 1, cy, "E") end
  end
end
local function fireMouth(g, cy, big, flash)
  local r = big and 3 or 2
  ell(g, 16, cy, r, r - 1, "O")
  if flash then ell(g, 16, cy, r - 1, math.max(1, r - 2), "G") end
end

-- ------------------------------------------------------------- forme
-- pose: w1..w4, a1..a3, d1..d4 (hit = w1 con mappa sbiancata)
local FORM = {}

function FORM.blob(e, pose)
  local g = blank(32)
  local r = e.r
  local w2, h2, dy = 2.1 * r, 1.5 * r, 0
  if pose == "w2" then w2, h2 = 2.4 * r, 1.1 * r
  elseif pose == "w3" then w2, h2, dy = 1.7 * r, 1.9 * r, -2
  elseif pose == "w4" then w2, h2 = 2.2 * r, 1.35 * r
  elseif pose == "a1" then w2, h2 = 2.4 * r, 1.15 * r
  elseif pose == "a2" then w2, h2, dy = 1.8 * r, 2.0 * r, -3
  elseif pose == "a3" then w2, h2 = 2.2 * r, 1.4 * r
  elseif pose == "d1" then w2, h2 = 2.3 * r, 1.0 * r
  elseif pose == "d2" then w2, h2 = 2.5 * r, 0.7 * r
  elseif pose == "d3" then w2, h2 = 2.6 * r, 0.4 * r
  elseif pose == "d4" then w2, h2 = 2.2 * r, 0.2 * r end
  h2 = math.max(2, math.floor(h2))
  dome(g, 16, math.floor(w2), h2, 29 + dy, "s")
  W.shade3(g, "s", "L", "S")
  if not pose:find("^d") or pose == "d1" then
    local ey = 29 + dy - h2 + math.floor(h2 * 0.35)
    eyes(g, ey, 2, false)
    if e.fire ~= "none" then
      fireMouth(g, ey + 5, e.fire == "ring", pose == "a2" or pose == "a3")
    end
  end
  if pose == "d3" or pose == "d4" then
    set(g, 8, 30, "S"); set(g, 24, 30, "S")
  end
  set(g, 10, 30, "S"); set(g, 22, 30, "S")
  return g
end

function FORM.spiky(e, pose)
  local g = blank(32)
  local r = math.floor(e.r * 0.9)
  local dy = (pose == "w2" or pose == "w4") and -1 or 0
  if pose == "a2" then dy = -2 end
  local cy = 20 + dy
  local spikeLen = 3
  if pose == "a1" then spikeLen = 2 end
  if pose == "a2" or pose == "a3" then spikeLen = 5 end
  if pose == "d1" then spikeLen = 2 end
  if pose == "d2" then spikeLen = 1 end
  local nsp = (pose == "d3" or pose == "d4") and 0 or 6
  local phase = (pose == "w2" or pose == "w4") and 0.5 or 0
  for k = 0, nsp - 1 do
    local ang = k * math.pi * 2 / 6 + phase - math.pi / 2
    local ux, uy = math.cos(ang), 0.9 * math.sin(ang)
    local px_, py_ = -uy, ux                    -- perpendicolare
    for d = 0, spikeLen + 1 do
      local wdt = (d <= 1) and 1 or ((d <= spikeLen - 1) and 1 or 0)
      if d <= 1 then wdt = 2 end
      local bx = 16 + (r - 1 + d) * ux
      local by = cy + (r - 1 + d) * uy
      for o = -wdt + 1, wdt - 1 do
        set(g, math.floor(bx + px_ * o * 0.7 + 0.5),
            math.floor(by + py_ * o * 0.7 + 0.5), "s")
      end
    end
  end
  if pose == "d3" then ell(g, 16, cy + 2, r - 1, math.floor((r - 1) * 0.8), "s")
  elseif pose == "d4" then dome(g, 16, 2 * r, 3, 29, "s")
  else ell(g, 16, cy, r, math.floor(r * 0.9), "s") end
  W.shade3(g, "s", "L", "S")
  if pose ~= "d3" and pose ~= "d4" then
    eyes(g, cy - 2, 2, false)
    if e.fire ~= "none" then
      fireMouth(g, cy + 3, e.fire == "ring" or e.fire == "spread",
                pose == "a2" or pose == "a3")
    end
  end
  return g
end

function FORM.armored(e, pose)
  local g = blank(32)
  local r = e.r
  local dy = (pose == "w2" or pose == "w4") and 1 or 0
  local lx = (pose == "w2") and 1 or ((pose == "w4") and -1 or 0)
  if pose:find("^d") then
    local stage = tonumber(pose:sub(2))
    dome(g, 16, 2 * r, math.max(2, math.floor(r * (2.2 - stage * 0.45))), 29, "s")
    for i = 0, stage do
      set(g, 10 + i * 4, 30, "S"); set(g, 12 + i * 3, 28, "L")
    end
    W.shade3(g, "s", "L", "S")
    if stage <= 1 then eyes(g, 20, 2, true) end
    return g
  end
  -- gambe
  dome(g, 12 + lx, 4, 3, 30, "s"); dome(g, 20 - lx, 4, 3, 30, "s")
  -- corpo a piastre
  for y = 14 + dy, 27 do
    for x = 16 - r, 15 + r do set(g, x, y, "s") end
  end
  set(g, 16 - r, 14 + dy, "."); set(g, 15 + r, 14 + dy, ".")
  -- testa staccata dal corpo da un collo in ombra
  dome(g, 16, r + 1, 5, 12 + dy, "s")
  -- braccia
  local ay = (pose == "a2" or pose == "a3") and 12 + dy or 17 + dy
  for y2 = ay, ay + 4 do
    set(g, 15 - r, y2, "s"); set(g, 14 - r, y2, "s")
    set(g, 16 + r, y2, "s"); set(g, 17 + r, y2, "s")
  end
  W.shade3(g, "s", "L", "S")
  -- collo e giunture delle piastre
  for x = 16 - math.floor(r / 2), 15 + math.floor(r / 2) do set(g, x, 13 + dy, "S") end
  for x = 17 - r, 14 + r do set(g, x, 18 + dy, "S"); set(g, x, 23 + dy, "S") end
  eyes(g, 9 + dy, 1, pose == "a2" or pose == "a3")
  return g
end

function FORM.floater(e, pose)
  local g = blank(32)
  local r = e.r
  local hover = ({ w1 = 0, w2 = -1, w3 = -2, w4 = -1 })[pose] or 0
  if pose == "a2" then hover = -2 end
  local fade = pose:find("^d") and tonumber(pose:sub(2)) or 0
  local cy = 15 + hover + fade * 2
  local rr = r - math.floor(fade * r / 5)
  if rr >= 2 then
    ell(g, 16, cy, rr, math.floor(rr * 0.9), "s")
    -- gonna ondulata
    local ph = (pose == "w2" or pose == "w4") and 1.6 or 0
    local spread = (pose == "a2" or pose == "a3") and 3 or 0
    for x = 16 - rr - math.floor(spread / 2), 15 + rr + math.floor(spread / 2) do
      local yb = cy + math.floor(rr * 0.9) +
                 math.floor(2 * math.abs(math.sin((x - 16) * 0.9 + ph)))
      for y2 = cy, yb do
        if g[y2] and g[y2][x] == "." then
          local dx = (x - 16) / (rr + spread * 0.5)
          if dx > -1.15 and dx < 1.15 then set(g, x, y2, "s") end
        end
      end
    end
    W.shade3(g, "s", "L", "S")
    if fade <= 1 then
      eyes(g, cy - 2, 2, true)
      if e.fire ~= "none" then
        fireMouth(g, cy + 3, e.fire == "ring", pose == "a2" or pose == "a3")
      end
    end
  end
  -- volute che si disperdono
  if fade >= 2 then
    set(g, 10, 12 + fade, "L"); set(g, 22, 10 + fade, "L")
    set(g, 16, 8 + fade, "s"); set(g, 13, 16 + fade, "S")
    set(g, 20, 18 + fade, "L")
  end
  return g
end

-- ----------------------------------------------------------- catalogo
local function P(b, l, s2, extra)
  local m = { s=b, L=l, S=s2, O=C.slag_nero, E=C.bianco, G=extra or C.bagliore }
  return m
end
local ENEMIES = {
  { id="salt-sniper", form="spiky", size=0.9, fire="spread", tags="beast",
    map=P(C.cenere, C.fumo, C.cen_scura, C.brace) },
  { id="root-lurker", form="blob", size=1.6, fire="none", tags="ooze",
    map=P(C.ver, C.ver_c, C.ver_s) },
  { id="mimic-cluster", form="blob", size=1.8, fire="none", tags="ooze",
    map=P(C.pru, C.pru_c, C.pru_s) },
  { id="mold-creeper", form="floater", size=1.2, fire="none", tags="ooze",
    map=P(C.ver_s, C.ver, C.slag_scuro, C.patina) },
  { id="ice-sprite", form="floater", size=1.0, fire="none", tags="undead,flying",
    map=P(C.ard_p, C.fumo, C.ard_c) },
  { id="frost-fang", form="spiky", size=1.7, fire="spread", tags="beast",
    map=P(C.ard_c, C.ard_p, C.ard, C.bianco) },
  { id="root-crawler", form="blob", size=1.4, fire="none", tags="ooze",
    map=P(C.terra, C.bronzo_s, C.slag_caldo, C.ver_c) },
  { id="murk-phantom", form="floater", size=1.1, fire="none", tags="undead,flying",
    map=P(C.pru_s, C.pru, C.slag_scuro, C.pru_c) },
  { id="vine-snapper", form="spiky", size=1.5, fire="single", tags="beast",
    map=P(C.ver, C.ver_c, C.ver_s, C.patina) },
  { id="sapling-stalker", form="blob", size=1.3, fire="none", tags="beast,neutral",
    map=P(C.ver_c, C.patina, C.ver) },
  { id="stone-golems", form="armored", size=1.2, fire="none", tags="humanoid,guard",
    map=P(C.cenere, C.cen_chiara, C.cen_scura, C.brace) },
  { id="silent-echoes", form="floater", size=0.6, fire="ring", tags="undead,flying",
    map=P(C.fumo, C.bianco, C.cen_chiara, C.ard_p) },
  { id="coglings", form="spiky", size=0.6, fire="spread", tags="mechanical",
    map=P(C.bronzo, C.bronzo_c, C.bronzo_s, C.oro) },
  { id="shell-striders", form="spiky", size=0.7, fire="spread", tags="beast,reptile",
    map=P(C.terra, C.bronzo_s, C.slag_caldo, C.oro_p) },
}

local hitMap = { s=C.fumo, L=C.bianco, S=C.cen_chiara, O=C.brace_s, E=C.bianco, G=C.brace }

-- ------------------------------------------------------------------ output
local CAP = "pixel art, worldsmelt style, black 1px outline, cluster shading, fucina palette, "
local contact = {}
for _, e in ipairs(ENEMIES) do
  e.r = math.max(5, math.min(12, math.floor(8 * e.size + 0.5)))
  local build = FORM[e.form]
  local poses = { "w1","w2","w3","w4","a1","a2","a3","d1","d2","d3","d4" }
  local G = {}
  for _, p in ipairs(poses) do G[p] = rows(build(e, p)) end
  local function fr(list, map)
    local out = {}
    for _, p in ipairs(list) do out[#out + 1] = { grid = G[p], map = map } end
    return out
  end
  local sheet = {
    id = e.id, dir = "enemies", fw = 32, fh = 32, anchor = { 16, 29 },
    rows = {
      { name = "walk", fps = 7, loop = true, frames = fr({ "w1","w2","w3","w4" }, e.map) },
      { name = "attack", fps = 10, loop = false, frames = fr({ "a1","a2","a3" }, e.map) },
      { name = "hit", fps = 10, loop = false, frames = { { grid = G.w1, map = hitMap, ox = 1 } } },
      { name = "death", fps = 9, loop = false, frames = fr({ "d1","d2","d3","d4" }, e.map) },
    },
  }
  W.buildSheet(ROOT, sheet)
  -- fotogramma singolo per il manifest della demo
  local spr = Sprite(32, 32, ColorMode.RGB)
  spr:setPalette(W.mkPalette())
  local img = Image(32, 32, ColorMode.RGB)
  W.renderS1(img, G.w1, e.map, 0, 0, 1)
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveCopyAs(ROOT .. "/assets/curated/worldsmelt/" .. e.id .. ".png")
  -- dataset (un frame per anim, varieta' senza ridondanza)
  for _, p in ipairs({ "w1", "a2", "d2" }) do
    W.datasetFrame(ROOT, "enemies", e.id .. "_" .. p, { grid = G[p], map = e.map },
      32, 32, CAP .. e.form .. " enemy, " .. e.id:gsub("%-", " ") .. ", " ..
      e.tags .. ", pose " .. p .. ", 32x32, transparent background")
  end
  contact[#contact + 1] = { G.w1, e.map }
  print(e.id .. " ok")
end

-- foglio-contatto
local csz = 34
local cols = 5
local rowsN = math.ceil(#contact / cols)
local cimg = Image(csz * cols + 8, csz * rowsN + 8, ColorMode.RGB)
W.fillRect(cimg, 0, 0, cimg.width, cimg.height, C.ard_s)
for i, e2 in ipairs(contact) do
  W.renderS1(cimg, e2[1], e2[2], 4 + ((i - 1) % cols) * csz + 1,
             4 + math.floor((i - 1) / cols) * csz + 1, 1)
end
local cspr = Sprite(cimg.width, cimg.height, ColorMode.RGB)
cspr:setPalette(W.mkPalette())
local ccel = cspr.cels[1]
if ccel == nil then ccel = cspr:newCel(cspr.layers[1], 1) end
ccel.image = cimg
ccel.position = Point(0, 0)
cspr:saveCopyAs(ROOT .. "/assets/art-src/preview-b/enemies-contact.png")
print("B2 OK")
