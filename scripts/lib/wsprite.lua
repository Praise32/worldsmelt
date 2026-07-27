-- wsprite.lua — libreria condivisa della produzione pixel-art Worldsmelt.
-- Stile ufficiale S1 «outline nero» (DEC CP1): outline nero 1px, shading piatto.
-- Palette «Fucina di Worldsmelt» (DEC-173), 31 colori, niente neon.
-- Usata dagli script batch in scripts/ via dofile.

local M = {}

M.PAL = {
  {20,16,14},{36,26,22},{58,38,32},{85,53,42},{122,74,43},{156,101,38},
  {201,138,46},{232,183,74},{245,223,143},{126,34,22},{177,58,30},
  {224,91,35},{247,145,62},{255,196,107},{43,43,49},{74,74,85},
  {115,115,130},{167,167,181},{216,216,224},{244,242,236},{30,77,68},
  {58,125,99},{109,179,136},{168,220,168},{38,48,63},{64,84,107},
  {106,134,160},{157,182,201},{91,42,77},{148,64,110},{207,111,150},
}
M.C = {
  slag_nero=1, slag_scuro=2, slag_caldo=3, terra=4, bronzo_s=5, bronzo=6,
  bronzo_c=7, oro=8, oro_p=9, brace_s=10, brace=11, fiamma=12, fiamma_c=13,
  bagliore=14, cen_nera=15, cen_scura=16, cenere=17, cen_chiara=18, fumo=19,
  bianco=20, ver_s=21, ver=22, ver_c=23, patina=24, ard_s=25, ard=26,
  ard_c=27, ard_p=28, pru_s=29, pru=30, pru_c=31,
}

function M.rgba(idx)
  local p = M.PAL[idx]
  return app.pixelColor.rgba(p[1], p[2], p[3], 255)
end

function M.px(img, x, y, idx)
  if x >= 0 and y >= 0 and x < img.width and y < img.height then
    img:putPixel(x, y, M.rgba(idx))
  end
end

function M.fillRect(img, x, y, w, h, idx)
  for yy = y, y + h - 1 do
    for xx = x, x + w - 1 do M.px(img, xx, yy, idx) end
  end
end

-- Indice del colore di palette piu' vicino (pesi percettivi grossolani).
function M.nearest(r, g, b)
  local best, bd = 1, math.huge
  for i, p in ipairs(M.PAL) do
    local dr, dg, db = r - p[1], g - p[2], b - p[3]
    local d = dr*dr*3 + dg*dg*6 + db*db*2
    if d < bd then bd, best = d, i end
  end
  return best
end

-- "Trasparenza" in palette: fonde i pixel sotto verso il colore idx e
-- riaggancia al colore di palette piu' vicino (resta palette-pura).
function M.blendRect(img, x, y, w, h, idx, t)
  local p = M.PAL[idx]
  local P = app.pixelColor
  for yy = y, y + h - 1 do
    for xx = x, x + w - 1 do
      if xx >= 0 and yy >= 0 and xx < img.width and yy < img.height then
        local v = img:getPixel(xx, yy)
        local r = P.rgbaR(v)*(1 - t) + p[1]*t
        local g = P.rgbaG(v)*(1 - t) + p[2]*t
        local b = P.rgbaB(v)*(1 - t) + p[3]*t
        img:putPixel(xx, yy, M.rgba(M.nearest(r, g, b)))
      end
    end
  end
end

-- Variante smorzata di un colore (per celle minimappa non visitate, ecc.).
function M.dimIdx(idx, t)
  local p, q = M.PAL[idx], M.PAL[M.C.cen_nera]
  return M.nearest(p[1]*(1-t)+q[1]*t, p[2]*(1-t)+q[2]*t, p[3]*(1-t)+q[3]*t)
end

-- Renderer S1: griglia di materiali (stringhe), mappa char->indice palette,
-- outline nero automatico 1px attorno alla silhouette. scale intero.
function M.renderS1(img, grid, map, ox, oy, scale)
  scale = scale or 1
  local h = #grid
  local w = #grid[1]
  local function at(x, y)
    if x < 1 or y < 1 or x > w or y > h then return "." end
    return grid[y]:sub(x, x)
  end
  for y = 1, h do
    assert(#grid[y] == w, "riga " .. y .. " lunga " .. #grid[y] .. " != " .. w)
    for x = 1, w do
      local ch = at(x, y)
      local idx = nil
      if ch ~= "." then
        idx = map[ch]
        assert(idx, "materiale senza colore: " .. ch)
      else
        for dy = -1, 1 do
          for dx = -1, 1 do
            if not (dx == 0 and dy == 0) and at(x + dx, y + dy) ~= "." then
              idx = M.C.slag_nero
            end
          end
        end
      end
      if idx then
        for sy = 0, scale - 1 do
          for sx = 0, scale - 1 do
            M.px(img, ox + (x-1)*scale + sx, oy + (y-1)*scale + sy, idx)
          end
        end
      end
    end
  end
end

-- ------------------------------------------------------------------ frame 9
-- Pannello/slot S1: outline nero, bevel bronzo (luce alto/sx, ombra basso/dx),
-- interno scuro. o = { interior=idx, blend=t|nil, rivets=bool }
function M.frame9(img, x, y, w, h, o)
  o = o or {}
  local Cc = M.C
  M.fillRect(img, x, y, w, 1, Cc.slag_nero)
  M.fillRect(img, x, y + h - 1, w, 1, Cc.slag_nero)
  M.fillRect(img, x, y, 1, h, Cc.slag_nero)
  M.fillRect(img, x + w - 1, y, 1, h, Cc.slag_nero)
  M.fillRect(img, x + 1, y + 1, w - 2, 1, Cc.bronzo_c)
  M.fillRect(img, x + 1, y + 1, 1, h - 2, Cc.bronzo_c)
  M.fillRect(img, x + 1, y + h - 2, w - 2, 1, Cc.bronzo_s)
  M.fillRect(img, x + w - 2, y + 1, 1, h - 2, Cc.bronzo_s)
  M.fillRect(img, x + 2, y + 2, w - 4, 1, Cc.bronzo)
  M.fillRect(img, x + 2, y + h - 3, w - 4, 1, Cc.bronzo)
  M.fillRect(img, x + 2, y + 2, 1, h - 4, Cc.bronzo)
  M.fillRect(img, x + w - 3, y + 2, 1, h - 4, Cc.bronzo)
  local ix, iy, iw, ih = x + 3, y + 3, w - 6, h - 6
  if o.blend then
    M.blendRect(img, ix, iy, iw, ih, o.interior or Cc.cen_nera, o.blend)
  else
    M.fillRect(img, ix, iy, iw, ih, o.interior or Cc.cen_nera)
  end
  M.fillRect(img, ix, iy, iw, 1, Cc.slag_nero)
  M.fillRect(img, ix, iy, 1, ih, Cc.slag_nero)
  if o.rivets then
    M.fillRect(img, x + 2, y + 2, 2, 2, Cc.oro)
    M.fillRect(img, x + w - 4, y + 2, 2, 2, Cc.oro)
    M.fillRect(img, x + 2, y + h - 4, 2, 2, Cc.oro)
    M.fillRect(img, x + w - 4, y + h - 4, 2, 2, Cc.oro)
  end
end

-- ------------------------------------------------------------------- font
-- Font pixel 5px di altezza, larghezza variabile, solo maiuscole.
M.FONT = {
  A={"010","101","111","101","101"}, B={"110","101","110","101","110"},
  C={"011","100","100","100","011"}, D={"110","101","101","101","110"},
  E={"111","100","110","100","111"}, F={"111","100","110","100","100"},
  G={"011","100","101","101","011"}, H={"101","101","111","101","101"},
  I={"111","010","010","010","111"}, J={"001","001","001","101","010"},
  K={"101","110","100","110","101"}, L={"100","100","100","100","111"},
  M={"10001","11011","10101","10001","10001"},
  N={"1001","1101","1011","1001","1001"},
  O={"010","101","101","101","010"}, P={"110","101","110","100","100"},
  Q={"0110","1001","1001","1010","0101"},
  R={"110","101","110","110","101"}, S={"011","100","010","001","110"},
  T={"111","010","010","010","010"}, U={"101","101","101","101","011"},
  V={"101","101","101","101","010"},
  W={"10001","10001","10101","10101","01010"},
  X={"101","101","010","101","101"}, Y={"101","101","010","010","010"},
  Z={"111","001","010","100","111"},
  ["0"]={"111","101","101","101","111"}, ["1"]={"010","110","010","010","111"},
  ["2"]={"111","001","111","100","111"}, ["3"]={"111","001","111","001","111"},
  ["4"]={"101","101","111","001","001"}, ["5"]={"111","100","111","001","111"},
  ["6"]={"111","100","111","101","111"}, ["7"]={"111","001","010","010","010"},
  ["8"]={"111","101","111","101","111"}, ["9"]={"111","101","111","001","111"},
  [":"]={"0","1","0","1","0"}, ["/"]={"001","001","010","100","100"},
  ["-"]={"000","000","111","000","000"}, ["."]={"0","0","0","0","1"},
  ["["]={"11","10","10","10","11"}, ["]"]={"11","01","01","01","11"},
  [">"]={"100","010","001","010","100"}, ["+"]={"000","010","111","010","000"},
  ["?"]={"110","001","010","000","010"}, ["!"]={"1","1","1","0","1"},
  [","]={"0","0","0","1","1"}, ["'"]={"1","1","0","0","0"},
  ["%"]={"101","001","010","100","101"}, ["="]={"000","111","000","111","000"},
}

function M.textW(s, scale)
  scale = scale or 1
  s = string.upper(s)
  local wsum = 0
  for i = 1, #s do
    local ch = s:sub(i, i)
    if ch == " " then wsum = wsum + 3
    else
      local g = M.FONT[ch] or M.FONT["?"]
      wsum = wsum + #g[1] + 1
    end
  end
  return (wsum > 0 and wsum - 1 or 0)*scale
end

function M.text(img, s, x, y, idx, scale)
  scale = scale or 1
  s = string.upper(s)
  local cx = x
  for i = 1, #s do
    local ch = s:sub(i, i)
    if ch == " " then
      cx = cx + 3*scale
    else
      local g = M.FONT[ch] or M.FONT["?"]
      local gw = #g[1]
      for gy = 1, 5 do
        for gx = 1, gw do
          if g[gy]:sub(gx, gx) == "1" then
            M.fillRect(img, cx + (gx-1)*scale, y + (gy-1)*scale, scale, scale, idx)
          end
        end
      end
      cx = cx + (gw + 1)*scale
    end
  end
  return cx - x
end

-- Testo con contorno nero (per elementi che galleggiano sul gioco).
function M.textOutlined(img, s, x, y, idx, scale)
  for _, d in ipairs({{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}}) do
    M.text(img, s, x + d[1], y + d[2], M.C.slag_nero, scale)
  end
  return M.text(img, s, x, y, idx, scale)
end

-- ------------------------------------------------------------------ griglie
local function D(n) return string.rep(".", n) end
local function R(ch, n) return string.rep(ch, n) end

M.grids = {}
M.maps = {}

M.grids.goblin24 = {
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
M.maps.goblin = {
  s=M.C.ver, S=M.C.ver_s, L=M.C.ver, e=M.C.bagliore, p=M.C.brace_s,
  c=M.C.terra, C=M.C.slag_caldo, t=M.C.bianco,
}

M.grids.goblin16 = {
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

M.grids.potion24 = {
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
  D(5).."g"..R("m",12).."g"..D(5),
  D(5).."g".."ll".."b"..R("l",9).."g"..D(5),
  D(5).."g"..R("l",7).."b"..R("l",4).."g"..D(5),
  D(5).."g"..R("l",10).."mm".."g"..D(5),
  D(6).."g"..R("l",8).."mm".."g"..D(6),
  D(7).."g"..R("m",8).."g"..D(7),
  D(8)..R("G",8)..D(8),
  D(24), D(24), D(24), D(24), D(24), D(24),
}
M.maps.potion = {
  k=M.C.bronzo, K=M.C.bronzo_s, g=M.C.ard_c, a=M.C.ard_p, h=M.C.ard_c,
  G=M.C.ard, m=M.C.brace, l=M.C.fiamma, b=M.C.bagliore,
}

-- Il personaggio: la Fonditrice, tuffatrice di fucina (elmo con visiera d'oro).
M.grids.hero24 = {
  D(24), D(24),
  D(8)..R("H",8)..D(8),
  D(6)..R("H",12)..D(6),
  D(5)..R("H",14)..D(5),
  D(5).."HH"..R("V",10).."HH"..D(5),
  D(5)..R("H",14)..D(5),
  D(6)..R("H",12)..D(6),
  D(4).."PPP"..R("A",10).."PPP"..D(4),
  D(4).."PP"..R("A",12).."PP"..D(4),
  D(4).."PP".."AAAAA".."VV".."AAAAA".."PP"..D(4),
  D(4).."PP"..R("A",11).."B".."PP"..D(4),
  D(5).."mmmmmm".."VV".."mmmmmm"..D(5),
  D(6)..R("A",11).."B"..D(6),
  D(6).."AAAAA"..D(2).."AAAAA"..D(6),
  D(6).."AAAAB"..D(2).."BAAAA"..D(6),
  D(7).."AAAA"..D(2).."AAAA"..D(7),
  D(7).."AAAB"..D(2).."BAAA"..D(7),
  D(6).."mmmmm"..D(2).."mmmmm"..D(6),
  D(6).."mmmmm"..D(2).."mmmmm"..D(6),
  D(24), D(24), D(24), D(24),
}
M.maps.hero = {
  H=M.C.bronzo, V=M.C.oro, A=M.C.ard, B=M.C.ard_s, P=M.C.bronzo_s,
  m=M.C.bronzo_c,
}

-- Cuore HUD 12x12 (base, temporaneo, vuoto; mezza vita via grids.heart12half).
M.grids.heart12 = {
  D(12), D(12),
  D(1).."xrrr"..D(2).."rrrr"..D(1),
  D(1).."xw"..R("r",6).."RR"..D(1),
  D(1)..R("r",8).."RR"..D(1),
  D(1)..R("r",8).."RR"..D(1),
  D(2)..R("r",6).."RR"..D(2),
  D(3)..R("r",4).."RR"..D(3),
  D(4).."rr".."RR"..D(4),
  D(5).."rR"..D(5),
  D(12), D(12),
}
M.grids.heart12half = {}
for i, row in ipairs(M.grids.heart12) do
  M.grids.heart12half[i] = row:sub(1, 6) .. D(6)
end
M.maps.heart = { r=M.C.brace, R=M.C.brace_s, x=M.C.fiamma, w=M.C.bagliore }
M.maps.heartTemp = { r=M.C.oro, R=M.C.bronzo, x=M.C.oro_p, w=M.C.bianco }
M.maps.heartEmpty = { r=M.C.cen_nera, R=M.C.cen_nera, x=M.C.cen_scura, w=M.C.cen_nera }

-- Icone risorse 11x11 (DEC-013/072: Ingots, Blast Charges, Cast Keys, Flux).
M.grids.ingot11 = {
  D(11), D(11),
  D(3).."w"..R("o",4)..D(3),
  D(2)..R("o",7)..D(2),
  D(1)..R("o",9)..D(1),
  D(1)..R("O",9)..D(1),
  D(11), D(11), D(11), D(11), D(11),
}
M.maps.ingot = { o=M.C.oro, O=M.C.bronzo_c, w=M.C.oro_p }

M.grids.charge11 = {
  D(11),
  D(6).."z"..D(4),
  D(5).."c"..D(5),
  D(3)..R("b",5)..D(3),
  D(3)..R("b",5)..D(3),
  D(3)..R("B",5)..D(3),
  D(3)..R("b",5)..D(3),
  D(3)..R("b",5)..D(3),
  D(3)..R("d",5)..D(3),
  D(11), D(11),
}
M.maps.charge = {
  z=M.C.fiamma_c, c=M.C.terra, b=M.C.cen_scura, B=M.C.brace, d=M.C.cen_nera,
}

M.grids.key11 = {
  D(11), D(11),
  D(2).."kkkk"..D(5),
  D(2).."k..k".."kkkkk",
  D(2).."k..k"..D(2).."k.k",
  D(2).."kkkk"..D(2).."k"..D(2),
  D(11), D(11), D(11), D(11), D(11),
}
M.maps.key = { k=M.C.bronzo_c }

M.grids.flux11 = {
  D(11),
  D(5).."l"..D(5),
  D(4).."lbl"..D(4),
  D(4).."lll"..D(4),
  D(2).."u"..R("l",5).."u"..D(2),
  D(2)..R("u",7)..D(2),
  D(3)..R("u",5)..D(3),
  D(4)..R("u",3)..D(4),
  D(11), D(11), D(11),
}
M.maps.flux = { l=M.C.fiamma, b=M.C.bagliore, u=M.C.cen_scura }

-- Icona Innesto (spina organica) e oggetto attivo (campana del richiamo).
M.grids.spike11 = {
  D(11),
  D(6).."tt"..D(3),
  D(5).."ttt"..D(3),
  D(4).."ttt"..D(4),
  D(3).."ttt"..D(5),
  D(2).."ttt"..D(6),
  D(1).."ttt"..D(7),
  D(1).."RR"..D(8),
  D(11), D(11), D(11),
}
M.maps.spike = { t=M.C.ver_c, R=M.C.ver }

M.grids.bell11 = {
  D(11),
  D(5).."o"..D(5),
  D(4).."ooo"..D(4),
  D(3)..R("o",5)..D(3),
  D(3)..R("o",5)..D(3),
  D(2)..R("o",7)..D(2),
  D(2)..R("o",7)..D(2),
  D(1)..R("O",9)..D(1),
  D(5).."O"..D(5),
  D(11), D(11),
}
M.maps.bell = { o=M.C.bronzo_c, O=M.C.bronzo_s }

-- Goblin di riferimento a 32px (DEC-177: scala base 32px).
M.grids.goblin32 = {
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

-- La Fonditrice a 32px (personaggio giocante, elmo con visiera d'oro).
M.grids.hero32 = {
  D(32), D(32),
  D(10)..R("H",12)..D(10),
  D(8)..R("H",16)..D(8),
  D(7)..R("H",18)..D(7),
  D(7)..R("H",18)..D(7),
  D(7).."HH"..R("V",14).."HH"..D(7),
  D(7).."HH"..R("V",14).."HH"..D(7),
  D(7)..R("H",18)..D(7),
  D(8)..R("H",16)..D(8),
  D(9)..R("H",14)..D(9),
  D(5).."PPPP"..R("A",14).."PPPP"..D(5),
  D(5).."PPP"..R("A",16).."PPP"..D(5),
  D(5).."PP"..R("A",18).."PP"..D(5),
  D(5).."PP".."AAAAAAA".."VVVV".."AAAAAAA".."PP"..D(5),
  D(5).."PP".."AAAAAAA".."VVVV".."AAAAAAA".."PP"..D(5),
  D(5).."PP"..R("A",17).."B".."PP"..D(5),
  D(5).."PP"..R("A",17).."B".."PP"..D(5),
  D(6)..R("m",9).."VV"..R("m",9)..D(6),
  D(7)..R("A",17).."B"..D(7),
  D(7)..R("A",7)..D(4)..R("A",7)..D(7),
  D(7).."AAAAAAB"..D(4).."BAAAAAA"..D(7),
  D(7).."AAAAAAB"..D(4).."BAAAAAA"..D(7),
  D(8)..R("A",6)..D(4)..R("A",6)..D(8),
  D(8).."AAAAAB"..D(4).."BAAAAA"..D(8),
  D(7)..R("m",7)..D(4)..R("m",7)..D(7),
  D(7)..R("m",7)..D(4)..R("m",7)..D(7),
  D(32), D(32), D(32), D(32), D(32),
}

-- Pozione a 32px (pickup a terra; l'icona inventario resta sulla griglia HUD).
M.grids.potion32 = {
  D(32), D(32),
  D(13)..R("k",6)..D(13),
  D(13).."kkkKKK"..D(13),
  D(12).."g".."KKKKKK".."g"..D(12),
  D(12).."gh"..R("a",5).."g"..D(12),
  D(12).."gh"..R("a",5).."g"..D(12),
  D(12).."gh"..R("a",5).."g"..D(12),
  D(12).."gh"..R("a",5).."g"..D(12),
  D(10).."gh"..R("a",9).."g"..D(10),
  D(9).."gh"..R("a",11).."g"..D(9),
  D(8).."gh"..R("a",13).."g"..D(8),
  D(7).."gh"..R("a",15).."g"..D(7),
  D(7).."g"..R("m",16).."g"..D(7),
  D(7).."g".."ll".."b"..R("l",13).."g"..D(7),
  D(7).."g"..R("l",9).."b"..R("l",6).."g"..D(7),
  D(7).."g"..R("l",16).."g"..D(7),
  D(7).."g"..R("l",5).."b"..R("l",10).."g"..D(7),
  D(7).."g"..R("l",14).."mm".."g"..D(7),
  D(7).."g"..R("l",13).."mmm".."g"..D(7),
  D(8).."g"..R("l",12).."mm".."g"..D(8),
  D(9).."g"..R("m",12).."g"..D(9),
  D(10)..R("G",12)..D(10),
  D(32), D(32), D(32), D(32), D(32), D(32), D(32), D(32), D(32),
}

-- ------------------------------------------ costruttore di griglie mutabili
function M.blank(n)
  local g = {}
  for y = 1, n do
    g[y] = {}
    for x = 1, n do g[y][x] = "." end
  end
  g.n = n
  return g
end
function M.gset(g, x, y, ch)
  if x >= 1 and y >= 1 and x <= g.n and y <= g.n then g[y][x] = ch end
end
function M.gseg(g, x1, y1, x2, y2, ch)
  local steps = math.max(math.abs(x2 - x1), math.abs(y2 - y1))
  if steps == 0 then M.gset(g, x1, y1, ch) return end
  for i = 0, steps do
    M.gset(g, math.floor(x1 + (x2 - x1) * i / steps + 0.5),
           math.floor(y1 + (y2 - y1) * i / steps + 0.5), ch)
  end
end
function M.gellipse(g, cx, cy, rx, ry, ch)
  for y = cy - ry, cy + ry do
    for x = cx - rx, cx + rx do
      local dx, dy = (x - cx) / rx, (y - cy) / ry
      if dx * dx + dy * dy <= 1.0 then M.gset(g, x, y, ch) end
    end
  end
end
function M.grows(g)
  local rows = {}
  for y = 1, g.n do rows[y] = table.concat(g[y]) end
  return rows
end

-- Rombo pieno (marcatori minimappa, pip di carica).
function M.diamond(img, cx, cy, r, idx)
  for dy = -r, r do
    local half = r - math.abs(dy)
    for dx = -half, half do M.px(img, cx + dx, cy + dy, idx) end
  end
end

-- ------------------------------------------------ infrastruttura condivisa
function M.mkPalette()
  local pal = Palette(#M.PAL)
  for i, p in ipairs(M.PAL) do
    pal:setColor(i - 1, Color{ r = p[1], g = p[2], b = p[3] })
  end
  return pal
end

-- Foglio a contratto: striscia orizzontale, righe = animazioni, json accanto.
-- sheet = { id, dir, fw, fh, anchor={x,y}, rows={ {name,fps,loop,frames={{grid,map,ox?}}} } }
function M.buildSheet(root, sheet)
  local cols = 0
  for _, row in ipairs(sheet.rows) do
    if #row.frames > cols then cols = #row.frames end
  end
  local sw, sh = cols * sheet.fw, #sheet.rows * sheet.fh
  local img = Image(sw, sh, ColorMode.RGB)
  for ri, row in ipairs(sheet.rows) do
    for fi, fr in ipairs(row.frames) do
      M.renderS1(img, fr.grid, fr.map, (fi - 1) * sheet.fw + (fr.ox or 0),
                 (ri - 1) * sheet.fh, 1)
    end
  end
  local spr = Sprite(sw, sh, ColorMode.RGB)
  spr:setPalette(M.mkPalette())
  spr.layers[1].name = sheet.id
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveAs(root .. "/assets/art-src/" .. sheet.dir .. "/" .. sheet.id .. ".aseprite")
  spr:saveCopyAs(root .. "/assets/art/" .. sheet.dir .. "/" .. sheet.id .. ".png")
  local parts = {}
  for ri, row in ipairs(sheet.rows) do
    parts[#parts + 1] = '"' .. row.name .. '":{"row":' .. (ri - 1) ..
      ',"frames":' .. #row.frames .. ',"fps":' .. row.fps ..
      ',"loop":' .. tostring(row.loop) .. '}'
  end
  local f = io.open(root .. "/assets/art/" .. sheet.dir .. "/" .. sheet.id .. ".json", "w")
  f:write('{"frame_w":' .. sheet.fw .. ',"frame_h":' .. sheet.fh ..
          ',"anchor":[' .. sheet.anchor[1] .. ',' .. sheet.anchor[2] ..
          '],"anims":{' .. table.concat(parts, ",") .. "}}\n")
  f:close()
  print(sheet.id .. ": sheet " .. sw .. "x" .. sh .. " ok")
end

-- Frame singolo 64x64 trasparente centrato + caption (dataset LoRA, DEC-175).
function M.datasetFrame(root, fam, name, fr, fw, fh, caption)
  local spr = Sprite(64, 64, ColorMode.RGB)
  spr:setPalette(M.mkPalette())
  local img = Image(64, 64, ColorMode.RGB)
  M.renderS1(img, fr.grid, fr.map, math.floor((64 - fw) / 2),
             math.floor((64 - fh) / 2), 1)
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  local base_ = root .. "/dataset/worldsmelt-style/" .. fam .. "/" .. name
  spr:saveCopyAs(base_ .. ".png")
  local f = io.open(base_ .. ".txt", "w")
  f:write(caption .. "\n")
  f:close()
end

return M
