-- CP1 — prove di stile Worldsmelt (palette «Fucina di Worldsmelt», 31 colori)
-- Genera 5 fogli di prova (S1..S5) con GLI STESSI soggetti: goblin 16/24/32,
-- pozione 24, cuore HUD 24, cornice slot 24. Cambia SOLO il trattamento di stile.
-- Riproducibile: aseprite -b --script scripts/cp1_style_tests.lua
-- Output: assets/art-src/style-tests/S*.aseprite (preview PNG via export a parte)

local ROOT = "/home/meri/progetti/melting-run-gpu"
local OUT = ROOT .. "/assets/art-src/style-tests/"

-- ---------------------------------------------------------------- palette
local PAL = {
  {20,16,14},{36,26,22},{58,38,32},{85,53,42},{122,74,43},{156,101,38},
  {201,138,46},{232,183,74},{245,223,143},{126,34,22},{177,58,30},
  {224,91,35},{247,145,62},{255,196,107},{43,43,49},{74,74,85},
  {115,115,130},{167,167,181},{216,216,224},{244,242,236},{30,77,68},
  {58,125,99},{109,179,136},{168,220,168},{38,48,63},{64,84,107},
  {106,134,160},{157,182,201},{91,42,77},{148,64,110},{207,111,150},
}
local C = {
  slag_nero=1, slag_scuro=2, slag_caldo=3, terra=4, bronzo_s=5, bronzo=6,
  bronzo_c=7, oro=8, oro_p=9, brace_s=10, brace=11, fiamma=12, fiamma_c=13,
  bagliore=14, cen_nera=15, cen_scura=16, cenere=17, cen_chiara=18, fumo=19,
  bianco=20, ver_s=21, ver=22, ver_c=23, patina=24, ard_s=25, ard=26,
  ard_c=27, ard_p=28, pru_s=29, pru=30, pru_c=31,
}

local function D(n) return string.rep(".", n) end
local function R(ch, n) return string.rep(ch, n) end

-- ---------------------------------------------------------------- griglie
-- Materiali: s/S/L pelle base/ombra/luce, e occhio, p pupilla, c/C stoffa,
-- t denti/artigli, k/K tappo, g vetro, a aria nel vetro, h riflesso vetro,
-- G ombra fondo vetro, l/M liquido base/ombra, b bolla, r/R/x/w cuore
-- base/ombra/luce/lucina, f/F/v cornice base/ombra/luce, i/I incavo, d rivetto.

local goblin24 = {
  D(24), D(24),
  D(8)..R("L",8)..D(8),
  D(6).."LL"..R("s",8).."LL"..D(6),
  D(5)..R("s",14)..D(5),
  D(1).."LL"..R("s",18).."LL"..D(1),
  D(2).."S"..R("s",18).."S"..D(2),
  D(3).."SS"..R("s",14).."SS"..D(3),
  D(5).."ss".."ee"..R("s",6).."ee".."sS"..D(5),
  D(5).."ss".."ep"..R("s",6).."pe".."sS"..D(5),
  D(5).."ss".."SS"..R("s",6).."SS".."sS"..D(5),
  D(5)..R("s",4).."StSStS"..R("s",3).."S"..D(5),
  D(6)..R("s",11).."S"..D(6),
  D(7).."S"..R("s",8).."S"..D(7),
  D(7)..R("s",10)..D(7),
  D(5).."LL"..R("s",10).."LL"..D(5),
  D(5).."sS"..R("s",10).."Ss"..D(5),
  D(5).."sS"..R("c",10).."Ss"..D(5),
  D(5).."t."..R("c",4).."CC"..R("c",4)..".t"..D(5),
  D(8).."SS"..D(4).."SS"..D(8),
  D(8).."ss"..D(4).."ss"..D(8),
  D(7).."sss"..D(4).."sss"..D(7),
  D(24), D(24),
}

local goblin16 = {
  D(16), D(16),
  D(4)..R("L",8)..D(4),
  D(1)..R("s",14)..D(1),
  D(2).."S"..R("s",10).."S"..D(2),
  D(4).."s".."ee".."ss".."ee".."s"..D(4),
  D(4).."s".."ep".."ss".."pe".."s"..D(4),
  D(4).."ss".."SttS".."ss"..D(4),
  D(5)..R("s",6)..D(5),
  D(3)..R("s",10)..D(3),
  D(3).."sS"..R("s",6).."Ss"..D(3),
  D(3).."t.".."cc".."CC".."cc"..".t"..D(3),
  D(5).."SS"..D(2).."SS"..D(5),
  D(4).."sss"..D(2).."sss"..D(4),
  D(16), D(16),
}

local goblin32 = {
  D(32), D(32), D(32),
  D(10)..R("L",12)..D(10),
  D(8).."LL"..R("s",12).."LL"..D(8),
  D(7).."L"..R("s",16).."L"..D(7),
  D(6)..R("s",20)..D(6),
  D(1).."LL"..R("s",26).."LL"..D(1),
  D(1).."ss".."SS"..R("s",22).."SS".."ss"..D(1),
  D(3).."SS"..R("s",22).."SS"..D(3),
  D(5).."SS"..R("s",18).."SS"..D(5),
  D(6)..R("s",20)..D(6),
  D(6).."ss".."eee"..R("s",10).."eee".."sS"..D(6),
  D(6).."ss".."epp"..R("s",10).."ppe".."sS"..D(6),
  D(6).."ss".."SSS"..R("s",10).."SSS".."sS"..D(6),
  D(6)..R("s",8).."S".."ss".."S"..R("s",7).."S"..D(6),
  D(6).."ss".."StSSStSSSStSSStS".."sS"..D(6),
  D(7).."S"..R("s",16).."S"..D(7),
  D(9).."S"..R("s",12).."S"..D(9),
  D(10)..R("s",12)..D(10),
  D(7).."LL"..R("s",14).."LL"..D(7),
  D(6)..R("s",20)..D(6),
  D(6).."ss".."S"..R("s",14).."S".."ss"..D(6),
  D(6).."ss".."S"..R("s",14).."S".."ss"..D(6),
  D(6).."ss".."S"..R("c",14).."S".."ss"..D(6),
  D(6).."t."..R("c",4).."CC"..R("c",4).."CC"..R("c",4)..".t"..D(6),
  D(8)..R("C",16)..D(8),
  D(10).."SSS"..D(6).."SSS"..D(10),
  D(10).."sss"..D(6).."sss"..D(10),
  D(9)..R("s",4)..D(6)..R("s",4)..D(9),
  D(32), D(32),
}

local potion24 = {
  D(24), D(24),
  D(10)..R("k",4)..D(10),
  D(10).."kkKK"..D(10),
  D(9).."g".."KKKK".."g"..D(9),
  D(9).."ghaaag"..D(9),
  D(9).."ghaaag"..D(9),
  D(8).."gh"..R("a",5).."g"..D(8),
  D(7).."gh"..R("a",7).."g"..D(7),
  D(6).."gh"..R("a",9).."g"..D(6),
  D(5).."gh"..R("a",11).."g"..D(5),
  D(5).."g"..R("M",12).."g"..D(5),
  D(5).."g".."ll".."b"..R("l",9).."g"..D(5),
  D(5).."g"..R("l",7).."b"..R("l",4).."g"..D(5),
  D(5).."g"..R("l",10).."MM".."g"..D(5),
  D(6).."g"..R("l",8).."MM".."g"..D(6),
  D(7).."g"..R("M",8).."g"..D(7),
  D(8)..R("G",8)..D(8),
  D(24), D(24), D(24), D(24), D(24), D(24),
}

local heart24 = {
  D(24), D(24), D(24), D(24),
  D(5)..R("x",5)..D(4)..R("x",5)..D(5),
  D(4).."xx"..R("r",5)..D(2)..R("r",6).."R"..D(4),
  D(3).."xx"..R("r",7).."RR"..R("r",5).."RR"..D(3),
  D(2).."x".."rr".."ww"..R("r",13).."RR"..D(2),
  D(2).."rrr".."ww"..R("r",13).."RR"..D(2),
  D(2)..R("r",18).."RR"..D(2),
  D(2)..R("r",17).."RRR"..D(2),
  D(3)..R("r",15).."RRR"..D(3),
  D(4)..R("r",13).."RRR"..D(4),
  D(5)..R("r",11).."RRR"..D(5),
  D(6)..R("r",9).."RRR"..D(6),
  D(7)..R("r",7).."RRR"..D(7),
  D(8)..R("r",5).."RRR"..D(8),
  D(9)..R("r",3).."RRR"..D(9),
  D(10).."rr".."RR"..D(10),
  D(11).."rR"..D(11),
  D(24), D(24), D(24), D(24),
}

local slot24 = {
  D(24),
  D(1)..R("v",22)..D(1),
  D(1).."v".."dd"..R("f",16).."dd".."F"..D(1),
  D(1).."v".."dd"..R("f",16).."dd".."F"..D(1),
  D(1).."vff"..R("I",16).."ffF"..D(1),
}
for _ = 5, 19 do
  slot24[#slot24+1] = D(1).."vff".."I"..R("i",15).."ffF"..D(1)
end
slot24[#slot24+1] = D(1).."v".."dd"..R("f",16).."dd".."F"..D(1)
slot24[#slot24+1] = D(1).."v".."dd"..R("f",16).."dd".."F"..D(1)
slot24[#slot24+1] = D(1)..R("F",22)..D(1)
slot24[#slot24+1] = D(24)

-- ---------------------------------------------------------------- stili
local baseMap = {
  s=C.ver, S=C.ver_s, L=C.ver_c, e=C.bagliore, p=C.brace_s, c=C.terra,
  C=C.slag_caldo, t=C.bianco, k=C.bronzo, K=C.bronzo_s, g=C.ard_c,
  a=C.ard_p, h=C.fumo, G=C.ard, M=C.brace, l=C.fiamma, b=C.bagliore,
  r=C.brace, R=C.brace_s, x=C.fiamma, w=C.bagliore, f=C.bronzo,
  F=C.bronzo_s, v=C.bronzo_c, i=C.cen_nera, I=C.slag_nero, d=C.oro,
}
local function mapWith(over)
  local m = {}
  for k2, v2 in pairs(baseMap) do m[k2] = v2 end
  for k2, v2 in pairs(over or {}) do m[k2] = v2 end
  return m
end

local outlineColored = {
  s=C.ver_s, S=C.ver_s, L=C.ver_s, e=C.ver_s, p=C.ver_s,
  c=C.slag_caldo, C=C.slag_caldo, t=C.ver_s,
  k=C.terra, K=C.terra, g=C.ard_s, a=C.ard_s, h=C.ard_s, G=C.ard_s,
  l=C.brace_s, M=C.brace_s, b=C.brace_s,
  r=C.brace_s, R=C.brace_s, x=C.brace_s, w=C.brace_s,
  f=C.slag_caldo, F=C.slag_caldo, v=C.slag_caldo, d=C.slag_caldo,
  i=C.slag_caldo, I=C.slag_caldo,
}
local rimMap = {
  s=C.patina, L=C.patina, e=C.bianco, c=C.bronzo, t=C.bianco,
  k=C.bronzo_c, g=C.ard_p, h=C.ard_p, a=C.ard_p, l=C.bagliore,
  M=C.bagliore, b=C.bianco, r=C.fiamma_c, x=C.fiamma_c, w=C.bianco,
  f=C.oro, v=C.oro, d=C.oro_p,
}
local edgeShadowMap = {
  s=C.ver_s, L=C.ver_s, S=C.ver_s, c=C.slag_caldo, C=C.slag_caldo,
  k=C.bronzo_s, K=C.bronzo_s, g=C.ard_s, G=C.ard_s, M=C.brace_s,
  l=C.brace, r=C.brace_s, R=C.brace_s, x=C.brace_s,
  f=C.bronzo_s, F=C.bronzo_s, v=C.bronzo_s,
}
local ditherPairs = { {"s","S"}, {"c","C"}, {"l","M"}, {"r","R"} }

local STYLES = {
  { name="S1-outline-nero",
    map=mapWith{ L=C.ver, x=C.brace, h=C.ard_c, v=C.bronzo },
    outline="black" },
  { name="S2-outline-selettivo", map=mapWith(), outline="colored" },
  { name="S3-silhouette-rim", map=mapWith(), outline="none", rim=true },
  { name="S4-2bit-contrasto",
    map={ s=C.brace, S=C.slag_nero, L=C.oro, e=C.bianco, p=C.slag_nero,
          c=C.oro, C=C.brace, t=C.bianco, k=C.brace, K=C.slag_nero,
          g=C.brace, a=C.slag_nero, h=C.bianco, G=C.slag_nero,
          M=C.brace, l=C.oro, b=C.bianco, r=C.brace, R=C.slag_nero,
          x=C.oro, w=C.bianco, f=C.brace, F=C.slag_nero, v=C.oro,
          i=C.slag_nero, I=C.slag_nero, d=C.bianco },
    outline="black" },
  { name="S5-dither-retro", map=mapWith(), outline="black", dither=true },
}

-- ---------------------------------------------------------------- motore
local function toMatrix(grid)
  local w = #grid[1]
  local m = {}
  for y, row in ipairs(grid) do
    assert(#row == w, "riga " .. y .. " lunga " .. #row .. " != " .. w)
    m[y] = {}
    for x = 1, w do m[y][x] = row:sub(x, x) end
  end
  return m, w, #grid
end

local function applyDither(m, w, h)
  local out = {}
  for y = 1, h do
    out[y] = {}
    for x = 1, w do out[y][x] = m[y][x] end
  end
  local function at(x, y)
    if x < 1 or y < 1 or x > w or y > h then return "." end
    return m[y][x]
  end
  for y = 1, h do
    for x = 1, w do
      local ch = m[y][x]
      for _, pr in ipairs(ditherPairs) do
        local b2, sh = pr[1], pr[2]
        local n = { at(x+1,y), at(x-1,y), at(x,y+1), at(x,y-1) }
        local function has(t)
          for _, v2 in ipairs(n) do if v2 == t then return true end end
          return false
        end
        if ch == b2 and has(sh) and (x + y) % 2 == 1 then out[y][x] = sh end
        if ch == sh and has(b2) and (x + y) % 2 == 0 then out[y][x] = b2 end
      end
    end
  end
  return out
end

-- Ritorna: colori[y][x] = indice palette (o nil), piu' lista pixel outline.
local function renderSprite(grid, style)
  local m, w, h = toMatrix(grid)
  if style.dither then m = applyDither(m, w, h) end
  local function at(x, y)
    if x < 1 or y < 1 or x > w or y > h then return "." end
    return m[y][x]
  end
  local col = {}
  for y = 1, h do
    col[y] = {}
    for x = 1, w do
      local ch = m[y][x]
      if ch ~= "." then
        local idx = style.map[ch]
        assert(idx, "materiale senza colore: " .. ch)
        col[y][x] = idx
      end
    end
  end
  if style.rim then
    for y = 1, h do
      for x = 1, w do
        local ch = m[y][x]
        if ch ~= "." then
          local top = at(x, y-1) == "."
          local bottom, right = at(x, y+1) == ".", at(x+1, y) == "."
          if (bottom or right) and edgeShadowMap[ch] then
            col[y][x] = edgeShadowMap[ch]
          end
          if top and rimMap[ch] then
            col[y][x] = rimMap[ch]
          end
        end
      end
    end
  end
  local outline = {}
  if style.outline ~= "none" then
    for y = 1, h do
      for x = 1, w do
        if m[y][x] == "." then
          local nbr, onlyBelow = nil, true
          for dy = -1, 1 do
            for dx = -1, 1 do
              if not (dx == 0 and dy == 0) and at(x+dx, y+dy) ~= "." then
                if nbr == nil or dy == 1 then nbr = at(x+dx, y+dy) end
                if dy ~= 1 then onlyBelow = false end
              end
            end
          end
          if nbr then
            if style.outline == "black" then
              outline[#outline+1] = { x, y, C.slag_nero }
            elseif style.outline == "colored" and not onlyBelow then
              outline[#outline+1] = { x, y, outlineColored[nbr] or C.slag_scuro }
            end
          end
        end
      end
    end
  end
  return col, outline, w, h
end

local function lastContentRow(grid)
  for y = #grid, 1, -1 do
    if grid[y]:find("[^.]") then return y end
  end
  return #grid
end

-- ---------------------------------------------------------------- fogli
local W, H = 200, 48
local BASELINE = 41
local SPRITES = {
  { grid=goblin16, x=8 },
  { grid=goblin24, x=32 },
  { grid=goblin32, x=64 },
  { grid=potion24, x=104 },
  { grid=heart24, x=136 },
  { grid=slot24, x=168 },
}

local function rgba(idx)
  local p = PAL[idx]
  return app.pixelColor.rgba(p[1], p[2], p[3], 255)
end

for _, style in ipairs(STYLES) do
  local spr = Sprite(W, H, ColorMode.RGB)
  local pal = Palette(#PAL)
  for i2, p in ipairs(PAL) do
    pal:setColor(i2 - 1, Color{ r=p[1], g=p[2], b=p[3] })
  end
  spr:setPalette(pal)
  spr.layers[1].name = "sheet"
  local img = Image(W, H, ColorMode.RGB)
  for y = 0, H - 1 do
    local bg = (y >= 42) and C.ard_s or C.cen_nera
    for x = 0, W - 1 do img:putPixel(x, y, rgba(bg)) end
  end
  for _, sp in ipairs(SPRITES) do
    local col, outline, gw, gh = renderSprite(sp.grid, style)
    local oy = BASELINE - lastContentRow(sp.grid)
    for y = 1, gh do
      for x = 1, gw do
        if col[y][x] then
          img:putPixel(sp.x + x - 1, oy + y - 1, rgba(col[y][x]))
        end
      end
    end
    for _, o in ipairs(outline) do
      img:putPixel(sp.x + o[1] - 1, oy + o[2] - 1, rgba(o[3]))
    end
  end
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  -- verifica: ogni pixel deve appartenere alla palette Fucina
  local legal = {}
  for _, p in ipairs(PAL) do legal[p[1] .. "," .. p[2] .. "," .. p[3]] = true end
  local bad = 0
  for y = 0, H - 1 do
    for x = 0, W - 1 do
      local v2 = img:getPixel(x, y)
      local key = app.pixelColor.rgbaR(v2) .. "," .. app.pixelColor.rgbaG(v2)
        .. "," .. app.pixelColor.rgbaB(v2)
      if not legal[key] then bad = bad + 1 end
    end
  end
  local path = OUT .. style.name .. ".aseprite"
  spr:saveAs(path)
  print(style.name .. " salvato, pixel fuori palette: " .. bad)
end
print("CP1 OK")
