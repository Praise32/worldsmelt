-- B3 — sprite dei 5 boss del catalogo curato (bosses.txt), 64px, stile S1+.
-- Anim: idle 4f, attack 4f, hit 1f, death 6f (taglia maggiore, DEC-176/177).
-- Doppio binario come B1/B2. Riproducibile:
--   aseprite -b --script scripts/b3_catalog_bosses.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local blank, set, seg, ell, rows = W.blank, W.gset, W.gseg, W.gellipse, W.grows

local function bob(pose)
  return ({ i1 = 0, i2 = -1, i3 = -2, i4 = -1 })[pose] or 0
end
local function dstage(pose)
  return pose:find("^d") and tonumber(pose:sub(2)) or 0
end

local BOSS = {}

-- The Frozen Maw: guscio corazzato di ghiaccio con fauci enormi
BOSS["frozen-maw"] = {
  map = { s=C.ard_c, L=C.ard_p, S=C.ard, O=C.slag_nero, t=C.bianco,
          e=C.brace, G=C.bagliore, q=C.fumo },
  build = function(pose)
    local g = blank(64)
    local dy = bob(pose)
    local st = dstage(pose)
    if st >= 5 then return g end
    local mouthH = ({ a1 = 9, a2 = 14, a3 = 17, a4 = 11 })[pose] or 6
    local split = (st >= 3) and (st - 2) * 3 or 0
    -- due meta' del guscio (si separano nella morte)
    for _, side in ipairs({ -1, 1 }) do
      local cx = 32 + side * split
      for y = 14 + dy + st, 50 do
        local t2 = (y - 14 - dy - st) / 36
        local half = math.floor(22 * math.sin(math.min(1, t2 + 0.15) * math.pi * 0.62) + 2)
        local x0 = (side < 0) and cx - half or cx + 1
        local x1 = (side < 0) and cx - 1 or cx + half
        for x = x0, x1 do set(g, x, y, "s") end
      end
    end
    -- bocca
    local my = 34 + dy + st
    for y = my, my + mouthH do
      local half = 16 - math.floor(math.abs(y - my - mouthH / 2) * 0.8)
      for x = 32 - half, 32 + half do set(g, x, y, "O") end
    end
    -- zanne di ghiaccio
    for _, fx in ipairs({ 22, 28, 36, 42 }) do
      for d = 0, 3 do
        set(g, fx, my + d, "t"); set(g, fx + 1, my + d, "t")
        if d < 3 then set(g, fx, my + mouthH - d, "t") end
      end
    end
    W.shade3(g, "s", "L", "S")
    -- placche
    for x = 16, 48 do set(g, x, 22 + dy + st, "S") end
    seg(g, 20, 16 + dy + st, 26, 20 + dy + st, "S")
    seg(g, 44, 16 + dy + st, 38, 20 + dy + st, "S")
    -- crepe di morte
    if st >= 1 then seg(g, 32, 14 + st, 28, 30 + st, "O") end
    if st >= 2 then seg(g, 36, 18 + st, 40, 34 + st, "O") end
    -- occhi
    if st <= 2 then
      set(g, 25, 28 + dy + st, "e"); set(g, 26, 28 + dy + st, "e")
      set(g, 38, 28 + dy + st, "e"); set(g, 39, 28 + dy + st, "e")
      if pose == "a2" or pose == "a3" then
        set(g, 25, 27 + dy, "G"); set(g, 38, 27 + dy, "G")
      end
    end
    return g
  end,
  rubble = { C.ard_c, C.ard, C.bianco },
}

-- The Mire Weaver: sfera spinosa di rovi con zampe, anello di fuoco
BOSS["mire-weaver"] = {
  map = { s=C.ver, L=C.ver_c, S=C.ver_s, O=C.slag_nero, t=C.terra,
          e=C.brace, G=C.bagliore, q=C.patina },
  build = function(pose)
    local g = blank(64)
    local dy = bob(pose)
    local st = dstage(pose)
    if st >= 5 then return g end
    local r = 15 - st * 2
    local cy = 30 + dy + st * 3
    local spikeLen = ({ a1 = 3, a2 = 8, a3 = 9, a4 = 6 })[pose] or 5
    if st > 0 then spikeLen = math.max(0, 4 - st) end
    -- zampe storte
    if st <= 2 then
      local curl = st * 3
      seg(g, 20, cy + 8, 10 + curl, 52 - curl, "t")
      seg(g, 26, cy + 10, 20 + curl, 54 - curl, "t")
      seg(g, 44, cy + 8, 54 - curl, 52 - curl, "t")
      seg(g, 38, cy + 10, 44 - curl, 54 - curl, "t")
    end
    -- spine
    local phase = (pose == "i2" or pose == "i4") and 0.35 or 0
    for k = 0, 7 do
      local ang = k * math.pi / 4 + phase
      local ux, uy = math.cos(ang), 0.9 * math.sin(ang)
      for d = 0, spikeLen do
        local wdt = (d <= 2) and 1 or 0
        for o = -wdt, wdt do
          set(g, math.floor(32 + (r - 1 + d) * ux - uy * o + 0.5),
              math.floor(cy + (r - 1 + d) * uy + ux * o + 0.5), "s")
        end
      end
    end
    ell(g, 32, cy, r, math.floor(r * 0.9), "s")
    W.shade3(g, "s", "L", "S")
    -- rosone di brace centrale (anello di fuoco)
    if st <= 1 then
      ell(g, 32, cy + 3, 3, 2, "O")
      if pose:find("^a") then ell(g, 32, cy + 3, 2, 1, "G") end
      -- occhi multipli da tessitrice
      set(g, 26, cy - 5, "e"); set(g, 30, cy - 7, "e")
      set(g, 34, cy - 7, "e"); set(g, 38, cy - 5, "e")
      set(g, 28, cy - 3, "e"); set(g, 36, cy - 3, "e")
    end
    -- ragnatela nell'attacco
    if pose == "a3" or pose == "a4" then
      for k = 0, 7 do
        local ang = k * math.pi / 4 + 0.4
        set(g, math.floor(32 + (r + spikeLen + 3) * math.cos(ang) + 0.5),
            math.floor(cy + (r + spikeLen + 3) * 0.9 * math.sin(ang) + 0.5), "q")
      end
    end
    return g
  end,
  rubble = { C.ver, C.terra, C.ver_s },
}

-- The Statue Warden: idolo di pietra con occhi cavi che si accendono
BOSS["statue-warden"] = {
  map = { s=C.cenere, L=C.cen_chiara, S=C.cen_scura, O=C.slag_nero,
          e=C.brace, G=C.bagliore, t=C.ard, q=C.cen_nera },
  build = function(pose)
    local g = blank(64)
    local dy = bob(pose)
    local st = dstage(pose)
    if st >= 5 then return g end
    local lean = ({ a2 = 2, a3 = 4, a4 = 2 })[pose] or 0
    local headDrop = (st >= 2) and st * 5 or 0
    local headSlide = (st >= 2) and st * 4 or 0
    -- corpo a blocco
    if st <= 3 then
      for y = 30 + dy + st * 2, 56 do
        local half = 14 - math.floor((y - 30) * 0.1)
        for x = 32 - half + lean, 32 + half + lean do set(g, x, y, "s") end
      end
      -- braccia colonnari
      for y2 = 32 + dy + st * 2, 50 do
        set(g, 14 + lean, y2, "s"); set(g, 15 + lean, y2, "s")
        set(g, 49 + lean, y2, "s"); set(g, 50 + lean, y2, "s")
      end
    end
    -- testa monolitica (nella morte si stacca e cade di lato)
    if st <= 4 then
      local hx = 32 + lean * 2 + headSlide
      local hy = 16 + dy + headDrop
      for y = hy, hy + 16 do
        for x = hx - 10, hx + 10 do set(g, x, y, "s") end
      end
      set(g, hx - 10, hy, "."); set(g, hx + 10, hy, ".")
      -- naso lungo da idolo
      for y2 = hy + 7, hy + 12 do set(g, hx, y2, "S"); set(g, hx - 1, y2, "S") end
      -- occhi cavi
      local blaze = (pose == "a2" or pose == "a3" or pose == "a4")
      for _, ex in ipairs({ hx - 6, hx + 3 }) do
        for dy2 = 0, 2 do
          for dx = 0, 2 do set(g, ex + dx, hy + 5 + dy2, "O") end
        end
        set(g, ex + 1, hy + 6, blaze and "G" or "e")
        if blaze then set(g, ex + 1, hy + 5, "G") end
      end
    end
    W.shade3(g, "s", "L", "S")
    -- crepe
    if st >= 1 then
      seg(g, 24, 34 + st, 30, 46 + st, "O")
      seg(g, 40, 32 + st, 36, 44 + st, "O")
    end
    if pose == "a3" then
      set(g, 12, 54, "G"); set(g, 52, 54, "G"); set(g, 32, 58, "G")
    end
    return g
  end,
  rubble = { C.cenere, C.cen_scura, C.ard },
}

-- The Clockwork Mole: talpa meccanica con trivella e corona di ingranaggi
BOSS["clockwork-mole"] = {
  map = { s=C.bronzo, L=C.bronzo_c, S=C.bronzo_s, O=C.slag_nero,
          e=C.brace, G=C.bagliore, t=C.cenere, q=C.cen_scura },
  build = function(pose)
    local g = blank(64)
    local dy = bob(pose)
    local st = dstage(pose)
    if st >= 5 then return g end
    local r = 16 - st * 2
    local cy = 30 + dy + st * 2
    -- corona di denti-ingranaggio
    local phase = (pose == "i2" or pose == "i4" or pose == "a2" or pose == "a4")
                  and 0.26 or 0
    if st <= 2 then
      for k = 0, 11 do
        local ang = k * math.pi / 6 + phase
        local tx = 32 + (r + 2) * math.cos(ang)
        local ty = cy + (r + 2) * 0.95 * math.sin(ang)
        for o = 0, 1 do
          set(g, math.floor(tx + 0.5) + o, math.floor(ty + 0.5), "t")
          set(g, math.floor(tx + 0.5) + o, math.floor(ty + 0.5) + 1, "t")
        end
      end
    end
    ell(g, 32, cy, r, math.floor(r * 0.95), "s")
    -- trivella (si estende nell'attacco)
    local drill = ({ a1 = 6, a2 = 12, a3 = 14, a4 = 9 })[pose] or 4
    if st <= 1 then
      for d = 0, drill do
        local half = math.max(0, math.floor(4 - d * 4 / math.max(1, drill)))
        for x = 32 - half, 32 + half do set(g, x, cy + r - 2 + d, "q") end
      end
      for d = 1, drill - 1, 2 do
        set(g, 31, cy + r - 2 + d, "t"); set(g, 33, cy + r - 2 + d, "t")
      end
    end
    W.shade3(g, "s", "L", "S")
    -- oblo'-occhi
    if st <= 2 then
      for _, ex in ipairs({ 25, 36 }) do
        for dy2 = 0, 2 do
          for dx = 0, 2 do set(g, ex + dx, cy - 6 + dy2, "O") end
        end
        set(g, ex + 1, cy - 5, (pose:find("^a")) and "G" or "e")
      end
      -- rivetti
      set(g, 20, cy + 4, "S"); set(g, 44, cy + 4, "S"); set(g, 32, cy - 10, "S")
    end
    -- molle e pezzi nella morte
    if st >= 3 then
      seg(g, 22, cy - 4, 18, cy - 12, "t"); seg(g, 42, cy - 2, 48, cy - 10, "t")
      set(g, 26, cy - 14, "t"); set(g, 40, cy - 16, "t")
    end
    return g
  end,
  rubble = { C.bronzo, C.cenere, C.bronzo_s },
}

-- The Spectral Heron: airone spettrale, collo lungo e ali di nebbia
BOSS["spectral-heron"] = {
  map = { s=C.fumo, L=C.bianco, S=C.ard_p, O=C.slag_nero,
          e=C.pru_c, G=C.pru_c, t=C.cen_chiara, q=C.ard_c },
  build = function(pose)
    local g = blank(64)
    local dy = bob(pose) * 2
    local st = dstage(pose)
    local rise = st * 3
    local alpha = 6 - st
    if st >= 6 then return g end
    local wing = ({ a1 = 4, a2 = 12, a3 = 14, a4 = 8 })[pose] or 2
    local cy = 34 + dy - rise
    -- corpo fantasma con gonna
    if alpha >= 2 then
      ell(g, 32, cy, 11 - st, 9 - st, "s")
      for x = 22 + st, 41 - st do
        local yb = cy + 8 - st + math.floor(2 * math.abs(math.sin((x - 32) * 0.7)))
        for y2 = cy, yb do
          if g[y2] and g[y2][x] == "." then set(g, x, y2, "s") end
        end
      end
    end
    -- ali
    if alpha >= 3 then
      for o = 0, wing do
        local wy = cy - 2 - math.floor(o * 0.7)
        set(g, 20 - o, wy, "s"); set(g, 20 - o, wy + 1, "s")
        set(g, 43 + o, wy, "s"); set(g, 43 + o, wy + 1, "s")
      end
    end
    -- collo lungo e testa a becco
    if alpha >= 2 then
      local nx = 32 + (({ a2 = 4, a3 = 6, a4 = 2 })[pose] or 0)
      local ny = cy - 22 + (({ a2 = 6, a3 = 9 })[pose] or 0)
      seg(g, 32, cy - 6, nx, ny + 6, "s")
      seg(g, 33, cy - 6, nx + 1, ny + 6, "s")
      ell(g, nx, ny + 3, 4, 3, "s")
      seg(g, nx + 4, ny + 3, nx + 9, ny + 4, "t")
      set(g, nx - 1, ny + 2, "O"); set(g, nx, ny + 2, "e")
    end
    W.shade3(g, "s", "L", "S")
    -- volute spettrali che salgono nella morte
    if st >= 2 then
      set(g, 24, cy - 10 - st, "q"); set(g, 40, cy - 14 - st, "q")
      set(g, 32, cy - 18 - st, "t"); set(g, 28, cy + 2, "q")
    end
    return g
  end,
  rubble = { C.fumo, C.ard_p, C.ard_c },
}

-- ------------------------------------------------------------------ output
local IDS = { "frozen-maw", "mire-weaver", "statue-warden", "clockwork-mole",
              "spectral-heron" }
local TAGS = { ["frozen-maw"]="boss,reptile", ["mire-weaver"]="boss,nature",
  ["statue-warden"]="boss,humanoid", ["clockwork-mole"]="boss,mechanical",
  ["spectral-heron"]="boss,undead,flying" }
local CAP = "pixel art, worldsmelt style, black 1px outline, cluster shading, fucina palette, "
local contact = {}

-- macerie comuni per gli ultimi frame di morte (d5/d6)
local function rubbleGrid(cols)
  local g = blank(64)
  for i, cx in ipairs({ 18, 28, 38, 48, 24, 42 }) do
    local rr = (i <= 4) and 4 or 3
    ell(g, cx, 54, rr, math.floor(rr * 0.6), (i % 2 == 0) and "s" or "S")
  end
  W.shade3(g, "s", "L", "S")
  return g
end
local function rubbleSmall()
  local g = blank(64)
  for _, p in ipairs({ {22,55,3},{34,56,2},{44,55,2} }) do
    ell(g, p[1], p[2], p[3], math.max(1, math.floor(p[3] * 0.6)), "S")
  end
  return g
end

local hitMapBase = { s=C.fumo, L=C.bianco, S=C.cen_chiara, O=C.brace_s,
  e=C.brace, G=C.bianco, t=C.bianco, q=C.cen_chiara }

for _, id in ipairs(IDS) do
  local B = BOSS[id]
  local poses = { "i1","i2","i3","i4","a1","a2","a3","a4","d1","d2","d3","d4" }
  local G = {}
  for _, p in ipairs(poses) do G[p] = rows(B.build(p)) end
  G.d5 = rows(rubbleGrid())
  G.d6 = rows(rubbleSmall())
  local function fr(list)
    local out = {}
    for _, p in ipairs(list) do out[#out + 1] = { grid = G[p], map = B.map } end
    return out
  end
  local sheet = {
    id = id, dir = "bosses", fw = 64, fh = 64, anchor = { 32, 56 },
    rows = {
      { name = "idle", fps = 6, loop = true, frames = fr({ "i1","i2","i3","i4" }) },
      { name = "attack", fps = 8, loop = false, frames = fr({ "a1","a2","a3","a4" }) },
      { name = "hit", fps = 10, loop = false, frames = {
        { grid = G.i1, map = hitMapBase, ox = 1 } } },
      { name = "death", fps = 7, loop = false, frames = fr({ "d1","d2","d3","d4","d5","d6" }) },
    },
  }
  W.buildSheet(ROOT, sheet)
  local spr = Sprite(64, 64, ColorMode.RGB)
  spr:setPalette(W.mkPalette())
  local img = Image(64, 64, ColorMode.RGB)
  W.renderS1(img, G.i1, B.map, 0, 0, 1)
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveCopyAs(ROOT .. "/assets/curated/worldsmelt/" .. id .. ".png")
  for _, p in ipairs({ "i1", "a3", "d2", "d5" }) do
    W.datasetFrame(ROOT, "bosses", id .. "_" .. p, { grid = G[p], map = B.map },
      64, 64, CAP .. "boss enemy, " .. id:gsub("%-", " ") .. ", " .. TAGS[id] ..
      ", pose " .. p .. ", 64x64, transparent background")
  end
  contact[#contact + 1] = { G.i1, B.map }
  print(id .. " ok")
end

local csz = 66
local cimg = Image(csz * 5 + 8, csz + 8, ColorMode.RGB)
W.fillRect(cimg, 0, 0, cimg.width, cimg.height, C.ard_s)
for i, e2 in ipairs(contact) do
  W.renderS1(cimg, e2[1], e2[2], 4 + (i - 1) * csz + 1, 5, 1)
end
local cspr = Sprite(cimg.width, cimg.height, ColorMode.RGB)
cspr:setPalette(W.mkPalette())
local ccel = cspr.cels[1]
if ccel == nil then ccel = cspr:newCel(cspr.layers[1], 1) end
ccel.image = cimg
ccel.position = Point(0, 0)
cspr:saveCopyAs(ROOT .. "/assets/art-src/preview-b/bosses-contact.png")
print("B3 OK")
