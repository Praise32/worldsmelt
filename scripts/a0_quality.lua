-- A0 — prova di qualità «S1+» (richiesta del proprietario, 28/07):
-- outline nero + shading a 3 toni con luce alto-sinistra, proporzioni chibi,
-- niente blocchi piatti, dettaglio interno, ombra a terra (del motore, qui
-- simulata), oggetti attivi/passivi levitanti sul piedistallo.
-- Produce: fonditrice rifatta, goblin rifatto, oggetto «Ember Husk» sul
-- piedistallo, e il foglio di confronto PRIMA/DOPO.
-- Riproducibile: aseprite -b --script scripts/a0_quality.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local blank, set, seg, ell, rows = W.blank, W.gset, W.gseg, W.gellipse, W.grows

-- ------------------------------------------------- la Fonditrice, rifatta
-- Chibi: elmo tondo ~metà altezza, visiera d'oro grande con OCCHI, corpo
-- piccolo tondo, guanti e stivali. Luce alto-sinistra via shade3 + accenti.
local function fonditriceNew()
  local g = blank(32)
  -- corpo (prima, la testa lo copre)
  ell(g, 16, 23, 7, 5, "A")
  -- guanti
  ell(g, 8, 24, 2, 2, "m")
  ell(g, 24, 24, 2, 2, "m")
  -- stivali
  ell(g, 12, 29, 2, 1, "m")
  ell(g, 20, 29, 2, 1, "m")
  set(g, 11, 30, "m"); set(g, 12, 30, "m"); set(g, 20, 30, "m"); set(g, 21, 30, "m")
  -- elmo tondo grande
  ell(g, 16, 11, 10, 9, "H")
  -- cresta
  set(g, 15, 1, "H"); set(g, 16, 1, "H"); set(g, 17, 1, "H")
  -- visiera d'oro grande, angoli smussati (bordo d'elmo pieno sotto)
  for y = 9, 15 do
    for x = 10, 23 do set(g, x, y, "V") end
  end
  set(g, 10, 9, "H"); set(g, 23, 9, "H"); set(g, 10, 15, "H"); set(g, 23, 15, "H")
  -- shading direzionale
  W.shade3(g, "A", "c", "B")
  W.shade3(g, "H", "i", "j")
  W.shade3(g, "V", "w", "u")
  -- occhi DENTRO la visiera (il carattere del personaggio)
  for _, ex in ipairs({ 12, 19 }) do
    for dy = 0, 2 do
      set(g, ex, 11 + dy, "p"); set(g, ex + 1, 11 + dy, "p")
    end
    set(g, ex, 11, "E")               -- luce negli occhi
  end
  -- accenti manuali: patch di luce sull'elmo, rivetti, glint visiera
  set(g, 10, 4, "i"); set(g, 11, 4, "i"); set(g, 12, 3, "i"); set(g, 13, 3, "i")
  set(g, 11, 5, "i"); set(g, 12, 4, "i")
  set(g, 7, 12, "r"); set(g, 25, 12, "r")
  set(g, 21, 10, "w"); set(g, 22, 10, "w")
  -- cintura con fibbia
  for x = 11, 21 do set(g, x, 26, "b") end
  set(g, 15, 26, "k"); set(g, 16, 26, "k"); set(g, 17, 26, "k")
  return rows(g)
end
local fondMap = {
  A=C.ard, c=C.ard_c, B=C.ard_s,
  H=C.bronzo, i=C.bronzo_c, j=C.bronzo_s,
  V=C.oro, w=C.oro_p, u=C.bronzo_c,
  m=C.bronzo_s, b=C.bronzo, k=C.oro, r=C.oro,
  p=C.slag_scuro, E=C.bianco,
}

-- ---------------------------------------------------- il goblin, rifatto
-- Geometria storica (testone, orecchie a pipistrello) + shading S1+ e volto
-- nuovo: occhi neri grandi con luce, sopracciglia, bocca larga con zanne.
local function goblinNew()
  local g = blank(32)
  local old = W.grids.goblin32
  for y = 1, 32 do
    for x = 1, 32 do
      local ch = old[y]:sub(x, x)
      if ch == "L" or ch == "S" or ch == "e" or ch == "p" or ch == "t" then
        ch = "s"
      elseif ch == "C" then
        ch = "c"
      end
      if ch ~= "." then set(g, x, y, ch) end
    end
  end
  W.shade3(g, "s", "L", "S")
  W.shade3(g, "c", "e", "d")
  -- occhi neri 4x4 smussati con luce
  for _, ex in ipairs({ 9, 20 }) do
    for dy = 0, 3 do
      for dx = 0, 3 do set(g, ex + dx, 11 + dy, "O") end
    end
    set(g, ex, 11, "s"); set(g, ex + 3, 11, "s")
    set(g, ex, 14, "s"); set(g, ex + 3, 14, "s")
    set(g, ex + 1, 12, "E")
  end
  -- sopracciglia arrabbiate (interno piu' basso)
  set(g, 9, 10, "S"); set(g, 10, 10, "S"); set(g, 11, 10, "S"); set(g, 12, 11, "S")
  set(g, 23, 10, "S"); set(g, 22, 10, "S"); set(g, 21, 10, "S"); set(g, 20, 11, "S")
  -- naso
  set(g, 15, 15, "S"); set(g, 16, 15, "S")
  -- bocca larga con tre zanne
  for x = 9, 22 do set(g, x, 17, "M") end
  for x = 10, 21 do set(g, x, 18, "M") end
  set(g, 11, 18, "t"); set(g, 11, 19, "t")
  set(g, 15, 18, "t")
  set(g, 20, 18, "t"); set(g, 20, 19, "t")
  -- artigli e verruche
  set(g, 7, 26, "t"); set(g, 26, 26, "t")
  set(g, 7, 8, "z"); set(g, 25, 7, "z")
  return rows(g)
end
local gobMap = {
  s=C.ver, L=C.ver_c, S=C.ver_s,
  c=C.terra, e=C.bronzo_s, d=C.slag_caldo,
  E=C.bianco, O=C.slag_nero, M=C.slag_scuro, t=C.bianco, z=C.patina,
}

-- --------------------------------- oggetto: Ember Husk (guscio di brace)
local function emberHusk()
  local g = blank(24)
  ell(g, 12, 13, 8, 7, "h")
  -- apertura in cima
  set(g, 11, 6, "l"); set(g, 12, 6, "l"); set(g, 13, 6, "l")
  W.shade3(g, "h", "i", "j")
  -- fessure incandescenti
  for _, sx in ipairs({ 9, 12, 15 }) do
    for y = 10, 16 do set(g, sx, y, "l") end
    set(g, sx, 13, "b")
  end
  set(g, 12, 11, "b")
  return rows(g)
end
local huskMap = { h=C.terra, i=C.bronzo_s, j=C.slag_caldo, l=C.fiamma, b=C.bagliore }

-- --------------------------------------------------------- composizioni
local function mkSprite(w, h)
  local spr = Sprite(w, h, ColorMode.RGB)
  spr:setPalette(W.mkPalette())
  local img = Image(w, h, ColorMode.RGB)
  return spr, img
end
local function commit(spr, img, path)
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveCopyAs(path)
end

-- ombra a terra "del motore": ellisse scura fusa col fondo
local function groundShadow(img, cx, cy, rx, ry)
  for dy = -ry, ry do
    for dx = -rx, rx do
      if (dx*dx)/(rx*rx+0.01) + (dy*dy)/(ry*ry+0.01) <= 1.0 then
        W.px(img, cx + dx, cy + dy, C.cen_nera)
      end
    end
  end
end

local fnew = fonditriceNew()
local gnew = goblinNew()
local husk = emberHusk()

-- foglio confronto PRIMA/DOPO su fondo stanza
local CWc, CHc = 240, 84
local spr1, img1 = mkSprite(CWc, CHc)
W.fillRect(img1, 0, 0, CWc, CHc, C.ard_s)
W.fillRect(img1, 0, 66, CWc, 18, C.ard_s)
W.text(img1, "PRIMA", 12, 4, C.cen_chiara, 1)
W.text(img1, "DOPO", 58, 4, C.oro_p, 1)
W.text(img1, "PRIMA", 116, 4, C.cen_chiara, 1)
W.text(img1, "DOPO", 162, 4, C.oro_p, 1)
groundShadow(img1, 26, 62, 10, 2)
W.renderS1(img1, W.grids.hero32, W.maps.hero, 10, 30, 1)
groundShadow(img1, 72, 62, 10, 2)
W.renderS1(img1, fnew, fondMap, 56, 30, 1)
groundShadow(img1, 130, 62, 10, 2)
W.renderS1(img1, W.grids.goblin32, W.maps.goblin, 114, 30, 1)
groundShadow(img1, 176, 62, 10, 2)
W.renderS1(img1, gnew, gobMap, 160, 30, 1)
commit(spr1, img1, ROOT .. "/assets/art-src/preview-a0/confronto.png")

-- scena piedistallo: oggetto levitante con ombra e scintille (2 frame bob)
local PW, PH = 64, 80
for f = 1, 2 do
  local spr2, img2 = mkSprite(PW, PH)
  W.fillRect(img2, 0, 0, PW, PH, C.slag_scuro)
  W.fillRect(img2, 0, 64, PW, 16, C.slag_caldo)
  -- piedistallo (griglia CP5 senza outline gia' inclusa nel renderer)
  -- roccia-piedistallo bombata come nei riferimenti
  local ped = {}
  do
    local g = blank(32)
    ell(g, 16, 13, 14, 8, "q")
    for x = 6, 26 do set(g, x, 20, "Q") end
    W.shade3(g, "q", "r", "Q")
    seg(g, 9, 12, 13, 15, "Q"); seg(g, 19, 9, 22, 13, "Q")
    set(g, 12, 8, "r"); set(g, 13, 8, "r")
    ped = rows(g)
  end
  local pedMap = { q=C.cen_scura, r=C.cenere, Q=C.cen_nera }
  W.renderS1(img2, ped, pedMap, 16, 44, 1)
  -- ombra dell'oggetto sulla roccia
  groundShadow(img2, 32, 51, 7 - f, 1)
  -- oggetto levitante appena sopra la roccia (bob di 1px)
  W.renderS1(img2, husk, huskMap, 20, 24 + f, 1)
  -- scintille
  if f == 2 then
    W.px(img2, 16, 22, C.bagliore); W.px(img2, 47, 30, C.bagliore)
    W.px(img2, 40, 14, C.oro_p)
  else
    W.px(img2, 44, 20, C.bagliore); W.px(img2, 18, 34, C.oro_p)
  end
  commit(spr2, img2, ROOT .. "/assets/art-src/preview-a0/piedistallo-" .. f .. ".png")
end

print("A0 OK")
