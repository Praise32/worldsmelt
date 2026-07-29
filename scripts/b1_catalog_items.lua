-- B1 — sprite dei 25 oggetti del catalogo curato (assets/curated-content/
-- items.txt), stile S1+ (outline nero + shade3), palette Fucina, 32px.
-- Doppio output per ogni oggetto:
--   assets/art/items/<id>.png/.json     (idle 1f + glow 2f, per W8)
--   assets/curated/worldsmelt/<id>.png  (fotogramma singolo per il ponte
--                                        manifest della demo attuale)
-- Piu' dataset training-ready e foglio-contatto di anteprima.
-- Riproducibile: aseprite -b --script scripts/b1_catalog_items.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local blank, set, seg, ell, rows = W.blank, W.gset, W.gseg, W.gellipse, W.grows

-- --------------------------------------------------------------- costruttori
-- Ogni costruttore disegna su griglia 32 con materiali generici:
-- a=base, b=secondario, d=dettaglio/accento, poi shade3 su a ("l"/"s") e
-- su b ("m"/"n"). La mappa colori arriva dalla voce di catalogo.
local function newG() return blank(32) end
local function finish(g)
  W.shade3(g, "a", "l", "s")
  W.shade3(g, "b", "m", "n")
  return rows(g)
end

local BUILD = {}

function BUILD.hammer(g)
  for y = 8, 14 do
    for x = 8, 24 do set(g, x, y, "a") end
  end
  set(g, 8, 8, "."); set(g, 24, 8, "."); set(g, 8, 14, "."); set(g, 24, 14, ".")
  for y = 15, 26 do set(g, 15, y, "b"); set(g, 16, y, "b"); set(g, 17, y, "b") end
  set(g, 12, 11, "d"); set(g, 20, 11, "d")
end
function BUILD.goggles(g)
  ell(g, 11, 16, 5, 4, "a"); ell(g, 21, 16, 5, 4, "a")
  ell(g, 11, 16, 3, 2, "d"); ell(g, 21, 16, 3, 2, "d")
  set(g, 15, 15, "b"); set(g, 16, 15, "b"); set(g, 17, 15, "b")
  seg(g, 6, 14, 4, 12, "b"); seg(g, 26, 14, 28, 12, "b")
end
function BUILD.shieldRound(g)
  ell(g, 16, 16, 9, 10, "a")
  ell(g, 16, 16, 4, 5, "b")
  set(g, 16, 16, "d"); set(g, 16, 15, "d")
  set(g, 10, 10, "d"); set(g, 22, 10, "d"); set(g, 10, 22, "d"); set(g, 22, 22, "d")
end
function BUILD.husk(g)
  ell(g, 16, 17, 8, 7, "a")
  set(g, 15, 9, "d"); set(g, 16, 9, "d"); set(g, 17, 9, "d")
  for _, sx in ipairs({ 13, 16, 19 }) do
    for y = 14, 20 do set(g, sx, y, "d") end
  end
  set(g, 16, 15, "D")
end
function BUILD.shard(g, p)
  local wdt = p and p.w or 5
  local tip, base_, mid = 6, 26, 16
  for y = tip, base_ do
    local t = (y - tip) / (base_ - tip)
    local half = math.floor(wdt * math.sin(t * math.pi * 0.9) + 0.6)
    for x = mid - half, mid + half do set(g, x, y, "a") end
  end
  seg(g, mid - 1, tip + 3, mid - 2, base_ - 5, "b")
  seg(g, mid + 2, tip + 5, mid + 2, base_ - 7, "d")
end
function BUILD.pebbles(g)
  ell(g, 16, 24, 8, 4, "a")
  ell(g, 15, 17, 6, 3, "b")
  ell(g, 17, 11, 4, 3, "a")
  set(g, 16, 10, "d")
end
function BUILD.pick(g)
  seg(g, 10, 24, 22, 12, "b"); seg(g, 11, 25, 23, 13, "b")
  for i = 0, 8 do
    set(g, 14 + i, 8 + math.floor(i * 0.4), "a")
    set(g, 14 + i, 9 + math.floor(i * 0.4), "a")
  end
  for i = 0, 5 do set(g, 13 - i, 10 + i, "a"); set(g, 14 - i, 11 + i, "a") end
  set(g, 24, 11, "d")
end
function BUILD.vent(g)
  for y = 10, 24 do
    for x = 9, 23 do set(g, x, y, "a") end
  end
  set(g, 9, 10, "."); set(g, 23, 10, "."); set(g, 9, 24, "."); set(g, 23, 24, ".")
  for _, vy in ipairs({ 13, 17, 21 }) do
    for x = 12, 20 do set(g, x, vy, "d") end
  end
  set(g, 16, 26, "d"); set(g, 16, 27, "D")
end
function BUILD.boots(g)
  for y = 10, 22 do
    for x = 9, 13 do set(g, x, y, "a") end
    for x = 18, 22 do set(g, x, y, "a") end
  end
  for x = 9, 16 do set(g, x, 23, "a"); set(g, x, 24, "a") end
  for x = 18, 25 do set(g, x, 23, "a"); set(g, x, 24, "a") end
  set(g, 11, 13, "d"); set(g, 20, 13, "d"); set(g, 11, 17, "d"); set(g, 20, 17, "d")
  for x = 9, 13 do set(g, x, 10, "b") end
  for x = 18, 22 do set(g, x, 10, "b") end
end
function BUILD.pad(g)
  ell(g, 16, 17, 10, 8, "a")
  seg(g, 16, 9, 20, 15, ".")
  seg(g, 17, 9, 21, 15, ".")
  ell(g, 14, 17, 2, 2, "d")
  seg(g, 10, 13, 13, 16, "b"); seg(g, 20, 21, 23, 22, "b")
end
function BUILD.lanternWing(g)
  for y = 12, 22 do
    for x = 12, 20 do set(g, x, y, "a") end
  end
  set(g, 12, 12, "."); set(g, 20, 12, "."); set(g, 12, 22, "."); set(g, 20, 22, ".")
  for y = 14, 20 do
    for x = 14, 18 do set(g, x, y, "d") end
  end
  set(g, 15, 10, "b"); set(g, 16, 10, "b"); set(g, 17, 10, "b")
  for i = 0, 5 do
    seg(g, 21 + i, 15 - math.floor(i * 0.8), 21 + i, 18 - i, "b")
  end
end
function BUILD.sprout(g)
  ell(g, 16, 22, 5, 4, "b")
  seg(g, 16, 18, 16, 12, "a"); seg(g, 15, 18, 15, 13, "a")
  ell(g, 12, 10, 3, 2, "a"); ell(g, 20, 9, 3, 2, "a")
  set(g, 16, 7, "d"); set(g, 15, 8, "d"); set(g, 17, 8, "d")
end
function BUILD.bell(g)
  set(g, 15, 7, "b"); set(g, 16, 7, "b")
  for dy = 0, 8 do
    local half = 2 + math.floor(dy * 0.7)
    for x = 16 - half, 16 + half do set(g, x, 9 + dy, "a") end
  end
  for x = 9, 23 do set(g, x, 18, "b") end
  set(g, 16, 20, "d"); set(g, 16, 21, "d")
end
function BUILD.coil(g)
  for i = 0, 66 do
    local ang = i * 0.21
    local r = 2 + i * 0.11
    local x = 16 + r * math.cos(ang)
    local y = 16 + r * 0.8 * math.sin(ang)
    set(g, math.floor(x + 0.5), math.floor(y + 0.5), "a")
    set(g, math.floor(x + 1.5), math.floor(y + 0.5), "a")
  end
  set(g, 16, 16, "d"); set(g, 17, 16, "d")
end
function BUILD.tendril(g)
  for i = 0, 40 do
    local t = i / 40
    local x = 10 + t * 12
    local y = 25 - t * 16 + 3 * math.sin(t * 9)
    set(g, math.floor(x + 0.5), math.floor(y + 0.5), "a")
    set(g, math.floor(x + 0.5), math.floor(y + 1.5), "a")
  end
  set(g, 22, 8, "d"); set(g, 23, 9, "d"); set(g, 22, 10, "d")
end
function BUILD.tablet(g)
  for y = 8, 25 do
    for x = 10, 22 do set(g, x, y, "a") end
  end
  set(g, 10, 8, "."); set(g, 22, 8, "."); set(g, 10, 25, "."); set(g, 22, 25, ".")
  seg(g, 14, 8, 17, 15, "s"); seg(g, 17, 15, 15, 22, "s")
  set(g, 13, 12, "d"); set(g, 19, 12, "d"); set(g, 16, 19, "d"); set(g, 13, 20, "d")
end
function BUILD.prism(g)
  for dy = 0, 14 do
    local half = math.floor(dy * 0.6)
    for x = 16 - half, 16 + half do set(g, x, 8 + dy, "a") end
  end
  seg(g, 16, 8, 12, 22, "b")
  seg(g, 16, 10, 19, 22, "d")
end
function BUILD.die(g, p)
  for y = 10, 22 do
    for x = 10, 22 do set(g, x, y, "a") end
  end
  set(g, 10, 10, "."); set(g, 22, 10, "."); set(g, 10, 22, "."); set(g, 22, 22, ".")
  set(g, 13, 13, "d"); set(g, 14, 13, "d")
  set(g, 18, 16, "d"); set(g, 19, 16, "d")
  set(g, 13, 19, "d"); set(g, 14, 19, "d")
  if p and p.coil then
    for i = 0, 16 do
      local ang = i * 0.4
      set(g, math.floor(16 + (11 - i * 0.2) * math.cos(ang) + 0.5),
          math.floor(16 + (9 - i * 0.2) * math.sin(ang) + 0.5), "b")
    end
  end
end
function BUILD.bubbles(g)
  ell(g, 12, 12, 4, 4, "a"); ell(g, 21, 15, 3, 3, "a")
  ell(g, 14, 21, 3, 3, "a"); ell(g, 22, 23, 2, 2, "a")
  set(g, 10, 10, "d"); set(g, 20, 13, "d"); set(g, 13, 19, "d")
end
function BUILD.stoneBubbles(g)
  ell(g, 16, 18, 8, 6, "a")
  ell(g, 13, 16, 1, 1, "d"); ell(g, 18, 20, 1, 1, "d")
  set(g, 20, 15, "d"); set(g, 14, 21, "d")
  ell(g, 19, 10, 2, 2, "b"); ell(g, 12, 8, 1, 1, "b")
end
function BUILD.conduit(g)
  for y = 8, 25 do
    for x = 13, 19 do set(g, x, y, "a") end
  end
  for i = 0, 30 do
    local t = i / 30
    local y = 9 + t * 15
    set(g, math.floor(16 + 4 * math.sin(t * 7) + 0.5), math.floor(y + 0.5), "b")
  end
  set(g, 15, 6, "d"); set(g, 16, 6, "d"); set(g, 17, 6, "d")
  set(g, 16, 27, "d")
end

-- ----------------------------------------------------------- voci catalogo
-- colori: a/l/s base+luce+ombra, b/m/n secondario, d accento, D accento scuro
local function pal(a, l, s2, b2, m2, n2, d, D2)
  return { a=a, l=l, s=s2, b=b2, m=m2, n=n2, d=d, D=D2 or C.slag_scuro }
end
local ITEMS = {
  { id="hammerhead-bounce", builder="hammer", tags={"weapon","blunt"},
    map=pal(C.cenere, C.cen_chiara, C.cen_scura, C.terra, C.bronzo_s, C.slag_caldo, C.fiamma_c) },
  { id="lens-goggles", builder="goggles", tags={"accessory"},
    map=pal(C.bronzo, C.bronzo_c, C.bronzo_s, C.terra, C.bronzo_s, C.slag_caldo, C.brace) },
  { id="reflector-shield", builder="shieldRound", tags={"armor","shield"},
    map=pal(C.bronzo, C.bronzo_c, C.bronzo_s, C.oro, C.oro_p, C.bronzo_c, C.oro_p) },
  { id="ember-husk", builder="husk", tags={"gadget"},
    map=pal(C.terra, C.bronzo_s, C.slag_caldo, C.terra, C.terra, C.terra, C.fiamma, C.bagliore) },
  { id="glowstone-fragment", builder="shard", p={w=6}, tags={"gem"},
    map=pal(C.oro, C.oro_p, C.bronzo_c, C.bronzo_c, C.oro, C.bronzo, C.bianco) },
  { id="pebble-pylon", builder="pebbles", tags={"gem"},
    map=pal(C.cenere, C.cen_chiara, C.cen_scura, C.cen_chiara, C.fumo, C.cenere, C.bianco) },
  { id="shattering-icepick", builder="pick", tags={"weapon","dagger"},
    map=pal(C.fumo, C.bianco, C.ard_p, C.terra, C.bronzo_s, C.slag_caldo, C.patina) },
  { id="crystal-shard", builder="shard", p={w=5}, tags={"gem"},
    map=pal(C.ard_p, C.fumo, C.ard_c, C.ard_c, C.ard_p, C.ard, C.bianco) },
  { id="leeching-vent", builder="vent", tags={"gadget"},
    map=pal(C.cen_scura, C.cenere, C.cen_nera, C.cen_scura, C.cenere, C.cen_nera, C.pru, C.pru_s) },
  { id="mud-boots", builder="boots", tags={"armor"},
    map=pal(C.terra, C.bronzo_s, C.slag_caldo, C.slag_caldo, C.terra, C.slag_scuro, C.ver_c) },
  { id="pool-stone", builder="stoneBubbles", tags={"gem"},
    map=pal(C.ard, C.ard_c, C.ard_s, C.pru, C.pru_c, C.pru_s, C.pru_c) },
  { id="lily-pad-shield", builder="pad", tags={"armor","shield"},
    map=pal(C.ver, C.ver_c, C.ver_s, C.patina, C.patina, C.ver_c, C.oro) },
  { id="lantern-wing", builder="lanternWing", tags={"gadget","light"},
    map=pal(C.bronzo, C.bronzo_c, C.bronzo_s, C.fumo, C.bianco, C.cen_chiara, C.oro_p) },
  { id="seed-sprout", builder="sprout", tags={"accessory"},
    map=pal(C.ver_c, C.patina, C.ver, C.terra, C.bronzo_s, C.slag_caldo, C.pru_c) },
  { id="wind-bell", builder="bell", tags={"gadget"},
    map=pal(C.cen_chiara, C.fumo, C.cenere, C.bronzo, C.bronzo_c, C.bronzo_s, C.oro) },
  { id="spindle-coil", builder="coil", tags={"gadget"},
    map=pal(C.oro, C.oro_p, C.bronzo_c, C.bronzo, C.bronzo, C.bronzo, C.bronzo_s) },
  { id="root-tendril", builder="tendril", tags={"accessory"},
    map=pal(C.ver, C.ver_c, C.ver_s, C.terra, C.terra, C.terra, C.pru_c) },
  { id="glass-splinter", builder="shard", p={w=3}, tags={"gem"},
    map=pal(C.fumo, C.bianco, C.ard_p, C.ard_p, C.fumo, C.ard_c, C.brace_s) },
  { id="cracked-clay-ward", builder="tablet", tags={"armor","shield"},
    map=pal(C.terra, C.bronzo_s, C.slag_caldo, C.terra, C.terra, C.terra, C.pru_c) },
  { id="prism-shard", builder="prism", tags={"gem","accessory"},
    map=pal(C.fiamma_c, C.bagliore, C.fiamma, C.oro_p, C.oro_p, C.oro, C.bianco) },
  { id="dice-coil", builder="die", p={coil=true}, tags={"gadget"},
    map=pal(C.fumo, C.bianco, C.cen_chiara, C.oro, C.oro_p, C.bronzo_c, C.pru_c) },
  { id="bubble-stone", builder="stoneBubbles", tags={"gem"},
    map=pal(C.pru_s, C.pru, C.slag_scuro, C.ard_c, C.ard_p, C.ard, C.ard_p) },
  { id="dice-core", builder="die", tags={"gem"},
    map=pal(C.patina, C.bianco, C.ver_c, C.ver, C.ver, C.ver, C.slag_scuro) },
  { id="root-conduit", builder="conduit", tags={"gadget"},
    map=pal(C.bronzo, C.bronzo_c, C.bronzo_s, C.ver, C.ver_c, C.ver_s, C.oro_p) },
  { id="bubble-rain", builder="bubbles", tags={"accessory"},
    map=pal(C.ard_c, C.ard_p, C.ard, C.ard, C.ard, C.ard, C.bianco) },
}

-- scintille deterministiche dall'id (niente RNG)
local function sparkles(id, n)
  local h = 0
  for i = 1, #id do h = (h * 31 + id:byte(i)) % 65536 end
  local out = {}
  for k = 1, n do
    h = (h * 1103515245 + 12345) % 65536
    local x = 5 + (h % 22)
    h = (h * 1103515245 + 12345) % 65536
    local y = 4 + (h % 12)
    out[#out + 1] = { x, y, (k % 2 == 0) and C.bagliore or C.bianco }
  end
  return out
end

-- ------------------------------------------------------------------ output
local function frames(list, map)
  local out = {}
  for _, g in ipairs(list) do out[#out + 1] = { grid = g, map = map } end
  return out
end
local function stillPng(grid, map, path, n)
  local spr = Sprite(n, n, ColorMode.RGB)
  spr:setPalette(W.mkPalette())
  local img = Image(n, n, ColorMode.RGB)
  W.renderS1(img, grid, map, 0, 0, 1)
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveCopyAs(path)
end

local CAP = "pixel art, worldsmelt style, black 1px outline, cluster shading, fucina palette, "
local sheetGrids = {}
for _, it in ipairs(ITEMS) do
  local g = newG()
  BUILD[it.builder](g, it.p)
  local base = finish(g)
  local sp = sparkles(it.id, 3)
  local glow1 = { grid = base, map = it.map, extra = { sp[1] } }
  local glow2 = { grid = base, map = it.map, extra = { sp[2], sp[3] } }
  local sheet = {
    id = it.id, dir = "items", fw = 32, fh = 32, anchor = { 16, 26 },
    rows = {
      { name = "idle", fps = 1, loop = true, frames = { { grid = base, map = it.map } } },
      { name = "glow", fps = 4, loop = true, frames = { glow1, glow2 } },
    },
  }
  -- buildSheet della lib non gestisce extra: disegno le scintille inline
  local img = Image(64, 64, ColorMode.RGB)
  W.renderS1(img, base, it.map, 0, 0, 1)
  W.renderS1(img, base, it.map, 0, 32, 1)
  W.renderS1(img, base, it.map, 32, 32, 1)
  W.px(img, sp[1][1], 32 + sp[1][2], sp[1][3])
  W.px(img, 32 + sp[2][1], 32 + sp[2][2], sp[2][3])
  W.px(img, 32 + sp[3][1], 32 + sp[3][2], sp[3][3])
  local spr = Sprite(64, 64, ColorMode.RGB)
  spr:setPalette(W.mkPalette())
  spr.layers[1].name = it.id
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveAs(ROOT .. "/assets/art-src/items/" .. it.id .. ".aseprite")
  spr:saveCopyAs(ROOT .. "/assets/art/items/" .. it.id .. ".png")
  local f = io.open(ROOT .. "/assets/art/items/" .. it.id .. ".json", "w")
  f:write('{"frame_w":32,"frame_h":32,"anchor":[16,26],"anims":{' ..
    '"idle":{"row":0,"frames":1,"fps":1,"loop":true},' ..
    '"glow":{"row":1,"frames":2,"fps":4,"loop":true}}}\n')
  f:close()
  -- fotogramma singolo per il ponte manifest della demo
  stillPng(base, it.map, ROOT .. "/assets/curated/worldsmelt/" .. it.id .. ".png", 32)
  -- dataset
  W.datasetFrame(ROOT, "items", it.id .. "_idle_1", { grid = base, map = it.map },
    32, 32, CAP .. "item pickup, " .. it.id:gsub("%-", " ") .. ", " ..
    table.concat(it.tags, ", ") .. ", 32x32, transparent background")
  sheetGrids[#sheetGrids + 1] = { base, it.map }
  print(it.id .. " ok")
end

-- foglio-contatto 5x5
local csz = 34
local cimg = Image(csz * 5 + 8, csz * 5 + 8, ColorMode.RGB)
W.fillRect(cimg, 0, 0, cimg.width, cimg.height, C.ard_s)
for i, e in ipairs(sheetGrids) do
  local cx = (i - 1) % 5
  local cy = math.floor((i - 1) / 5)
  W.renderS1(cimg, e[1], e[2], 4 + cx * csz + 1, 4 + cy * csz + 1, 1)
end
local cspr = Sprite(cimg.width, cimg.height, ColorMode.RGB)
cspr:setPalette(W.mkPalette())
local ccel = cspr.cels[1]
if ccel == nil then ccel = cspr:newCel(cspr.layers[1], 1) end
ccel.image = cimg
ccel.position = Point(0, 0)
cspr:saveCopyAs(ROOT .. "/assets/art-src/preview-b/items-contact.png")
print("B1 OK")
