-- CP2 — bozze GUI/HUD di Worldsmelt su mock 960x640 (DEC-174), stile S1
-- (DEC-176) a scala base 32px (rettifica del proprietario), palette Fucina
-- (DEC-173). Tre varianti di layout + BuildScreen con fascia fusione.
-- Riproducibile: aseprite -b --script scripts/cp2_hud_mocks.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local OUT = ROOT .. "/assets/art-src/ui-mocks/"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C
local CW, CH = 960, 640

-- ------------------------------------------------------------- game view
-- Stanza 1x1 a inquadratura intera (DEC-170), tema freddo cosi' l'HUD caldo
-- stacca: pavimento ardesia, pareti terra bruciata, porte da 100px (DOOR_HALF).
local RX, RY, RW, RH = 64, 84, 832, 488

local function paintGameView(img)
  W.fillRect(img, 0, 0, CW, CH, C.slag_nero)
  local wt = 16
  W.fillRect(img, RX-wt-1, RY-wt-1, RW+2*wt+2, RH+2*wt+2, C.slag_nero)
  W.fillRect(img, RX-wt, RY-wt, RW+2*wt, RH+2*wt, C.terra)
  W.fillRect(img, RX-wt, RY-wt, RW+2*wt, 2, C.bronzo_s)
  W.fillRect(img, RX-1, RY-1, RW+2, RH+2, C.slag_nero)
  W.fillRect(img, RX, RY, RW, RH, C.ard_s)
  for gy = RY+32, RY+RH-1, 64 do
    for gx = RX+32, RX+RW-1, 64 do
      W.px(img, gx, gy, C.ard); W.px(img, gx+1, gy, C.ard)
    end
  end
  -- porta alta aperta
  local dx = RX + RW/2 - 50
  W.fillRect(img, dx-4, RY-wt-1, 108, wt+1, C.slag_nero)
  W.fillRect(img, dx, RY-wt-1, 100, wt, C.cen_nera)
  W.fillRect(img, dx-4, RY-wt-1, 4, wt+1, C.bronzo_c)
  W.fillRect(img, dx+100, RY-wt-1, 4, wt+1, C.bronzo_c)
  -- porta destra chiusa
  local dy = RY + RH/2 - 50
  W.fillRect(img, RX+RW-1, dy-4, wt+2, 108, C.slag_nero)
  W.fillRect(img, RX+RW, dy, wt, 100, C.bronzo_s)
  W.fillRect(img, RX+RW+2, dy+6, wt-4, 88, C.bronzo)
  W.fillRect(img, RX+RW+5, dy+44, 5, 12, C.brace)
  -- ostacoli
  local function pillar(px_, py_)
    W.fillRect(img, px_-1, py_-1, 42, 42, C.slag_nero)
    W.fillRect(img, px_, py_, 40, 40, C.cen_scura)
    W.fillRect(img, px_, py_, 40, 4, C.cenere)
  end
  pillar(310, 300); pillar(620, 390)
  -- colpi in volo
  local function shot(sx, sy)
    W.fillRect(img, sx-3, sy-3, 6, 6, C.slag_nero)
    W.fillRect(img, sx-2, sy-2, 4, 4, C.bagliore)
  end
  shot(540, 320); shot(596, 308); shot(650, 298)
  -- entita' a scala reale (32px)
  W.renderS1(img, W.grids.hero32, W.maps.hero, 420, 310, 1)
  W.renderS1(img, W.grids.goblin32, W.maps.goblin, 680, 260, 1)
  W.renderS1(img, W.grids.goblin32, W.maps.goblin, 240, 420, 1)
  W.renderS1(img, W.grids.potion32, W.maps.potion, 720, 460, 1)
  W.renderS1(img, W.grids.ingot11, W.maps.ingot, 350, 500, 2)
end

-- ------------------------------------------------------------ elementi HUD
local function elHearts(img, x, y)
  local hx = x
  for i = 1, 4 do
    if i == 3 then
      W.renderS1(img, W.grids.heart12, W.maps.heartEmpty, hx, y, 1)
      W.renderS1(img, W.grids.heart12half, W.maps.heart, hx, y, 1)
    elseif i == 4 then
      W.renderS1(img, W.grids.heart12, W.maps.heartEmpty, hx, y, 1)
    else
      W.renderS1(img, W.grids.heart12, W.maps.heart, hx, y, 1)
    end
    hx = hx + 13
  end
  hx = hx + 4
  for _ = 1, 2 do
    W.renderS1(img, W.grids.heart12, W.maps.heartTemp, hx, y, 1)
    hx = hx + 13
  end
  return hx - x
end

local function elResources(img, x, y, outlined)
  local defs = {
    { W.grids.ingot11, W.maps.ingot, "47", C.oro_p, false },
    { W.grids.charge11, W.maps.charge, "3", C.fumo, false },
    { W.grids.key11, W.maps.key, "2", C.fumo, false },
    { W.grids.flux11, W.maps.flux, "1", C.bagliore, true },
  }
  local cx = x
  for _, d in ipairs(defs) do
    local gx = cx
    W.renderS1(img, d[1], d[2], cx, y, 1)
    cx = cx + 13
    if outlined then W.textOutlined(img, d[3], cx, y + 2, d[4], 2)
    else W.text(img, d[3], cx, y + 2, d[4], 2) end
    cx = cx + W.textW(d[3], 2)
    -- Flux evidenziato quando basta per una fusione (hud.md)
    if d[5] then
      local bw = cx - gx + 4
      W.fillRect(img, gx-2, y-2, bw, 1, C.bagliore)
      W.fillRect(img, gx-2, y+13, bw, 1, C.bagliore)
      W.fillRect(img, gx-2, y-2, 1, 16, C.bagliore)
      W.fillRect(img, gx+bw-3, y-2, 1, 16, C.bagliore)
    end
    cx = cx + 10
  end
  return cx - x
end

local function elSlots(img, x, y)
  -- slot attivo: campana + pip di carica; slot Innesto: spina
  W.frame9(img, x, y, 30, 30, {})
  W.renderS1(img, W.grids.bell11, W.maps.bell, x + 4, y + 3, 2)
  W.fillRect(img, x + 3, y + 23, 24, 4, C.cen_nera)
  W.diamond(img, x + 9, y + 24, 2, C.bagliore)
  W.diamond(img, x + 15, y + 24, 2, C.bagliore)
  W.diamond(img, x + 21, y + 24, 2, C.cen_scura)
  W.text(img, "[E]", x + 8, y + 33, C.cen_chiara, 1)
  local gx = x + 40
  W.frame9(img, gx, y, 30, 30, {})
  W.renderS1(img, W.grids.spike11, W.maps.spike, gx + 4, y + 3, 2)
  W.text(img, "[G]", gx + 8, y + 33, C.cen_chiara, 1)
  return 70
end

local function elTimer(img, cx, y, boxed)
  local s = "07:42"
  local w = W.textW(s, 2)
  if boxed then
    W.frame9(img, cx - w/2 - 9, y, w + 18, 21, { blend = 0.78 })
    W.text(img, s, cx - w/2, y + 6, C.bianco, 2)
  else
    W.textOutlined(img, s, cx - w/2, y + 6, C.bianco, 2)
  end
end

-- Minimappa 5x5 (GRID_SIZE) a celle fuse multi-taglia (DEC-170).
local MMROOMS = {
  { cells={{2,2}}, kind="normal", visited=true, current=true },
  { cells={{1,1},{2,1}}, kind="normal", visited=true },
  { cells={{3,1}}, kind="treasure", visited=true },
  { cells={{3,2}}, kind="normal", visited=true },
  { cells={{0,2},{0,3},{1,3}}, kind="normal", visited=true },
  { cells={{3,3},{4,3},{3,4},{4,4}}, kind="normal", visited=false },
  { cells={{4,1}}, kind="shop", visited=false },
  { cells={{2,0}}, kind="boss", visited=false },
  { cells={{1,2}}, kind="normal", visited=true },
  { cells={{2,3}}, kind="normal", visited=false },
}
local KINDCOL = { normal=C.ard, treasure=C.oro, shop=C.ver, boss=C.brace }

local function elMinimap(img, x, y, cell, gap)
  for _, room in ipairs(MMROOMS) do
    local col = KINDCOL[room.kind]
    if not room.visited then col = W.dimIdx(col, 0.62) end
    local function has(cx_, cy_)
      for _, c2 in ipairs(room.cells) do
        if c2[1] == cx_ and c2[2] == cy_ then return true end
      end
      return false
    end
    for _, c2 in ipairs(room.cells) do
      local px_ = x + c2[1]*(cell+gap)
      local py_ = y + c2[2]*(cell+gap)
      W.fillRect(img, px_, py_, cell, cell, col)
      if has(c2[1]+1, c2[2]) then W.fillRect(img, px_+cell, py_, gap, cell, col) end
      if has(c2[1], c2[2]+1) then W.fillRect(img, px_, py_+cell, cell, gap, col) end
      if has(c2[1]+1, c2[2]) and has(c2[1], c2[2]+1) and has(c2[1]+1, c2[2]+1) then
        W.fillRect(img, px_+cell, py_+cell, gap, gap, col)
      end
    end
    local border = room.current and C.bianco or C.slag_nero
    for _, c2 in ipairs(room.cells) do
      local px_ = x + c2[1]*(cell+gap)
      local py_ = y + c2[2]*(cell+gap)
      local ex = has(c2[1]+1, c2[2]) and gap or 0
      local ey = has(c2[1], c2[2]+1) and gap or 0
      if not has(c2[1], c2[2]-1) then W.fillRect(img, px_, py_, cell+ex, 1, border) end
      if not has(c2[1], c2[2]+1) then W.fillRect(img, px_, py_+cell-1, cell+ex, 1, border) end
      if not has(c2[1]-1, c2[2]) then W.fillRect(img, px_, py_, 1, cell+ey, border) end
      if not has(c2[1]+1, c2[2]) then W.fillRect(img, px_+cell-1, py_, 1, cell+ey, border) end
    end
    if room.visited and room.kind ~= "normal" then
      local px_ = x + room.cells[1][1]*(cell+gap)
      local py_ = y + room.cells[1][2]*(cell+gap)
      W.diamond(img, px_ + math.floor(cell/2), py_ + math.floor(cell/2), 2, C.bianco)
    end
  end
  return 5*cell + 4*gap
end

local function elDiscoveryCard(img, cx, y)
  local cw, ch = 250, 48
  local x = math.floor(cx - cw/2)
  W.frame9(img, x, y, cw, ch, { blend = 0.8, rivets = true })
  W.frame9(img, x + 5, y + 5, 38, 38, {})
  W.renderS1(img, W.grids.goblin16, W.maps.goblin, x + 8, y + 8, 2)
  W.text(img, "GOBLIN DI SLAG", x + 50, y + 10, C.oro_p, 2)
  W.text(img, "RUBA LINGOTTI QUANDO TI COLPISCE", x + 50, y + 28, C.fumo, 1)
  W.text(img, "NUOVO!", x + cw - W.textW("NUOVO!", 1) - 7, y + 6, C.bagliore, 1)
end

local function elGen(img, x, y)
  W.renderS1(img, W.grids.flux11, W.maps.flux, x, y, 1)
  W.textOutlined(img, "FORGIA", x + 15, y, C.cen_chiara, 1)
  W.fillRect(img, x + 15, y + 8, 42, 6, C.slag_nero)
  W.fillRect(img, x + 16, y + 9, 40, 4, C.cen_scura)
  W.fillRect(img, x + 16, y + 9, 26, 4, C.bagliore)
end

-- --------------------------------------------------------------- varianti
local function hudV1(img)
  paintGameView(img)
  -- vitali alto-sinistra
  local px_, py_, pw, ph = 8, 8, 210, 104
  W.frame9(img, px_, py_, pw, ph, { blend = 0.78, rivets = true })
  W.text(img, "LA FONDITRICE", px_+10, py_+8, C.ard_p, 1)
  elHearts(img, px_+10, py_+17)
  elResources(img, px_+10, py_+34, false)
  elSlots(img, px_+10, py_+56)
  -- stato run alto-destra
  local rw_, rh_ = 128, 150
  local rpx, rpy = CW - rw_ - 8, 8
  W.frame9(img, rpx, rpy, rw_, rh_, { blend = 0.78, rivets = true })
  W.text(img, "CROGIOLO", rpx+10, rpy+8, C.oro_p, 1)
  W.text(img, "DELLE SPORE", rpx+10, rpy+15, C.oro_p, 1)
  W.text(img, "PIANO 2/5 - TESORO", rpx+10, rpy+24, C.bianco, 1)
  W.text(img, "FONTE: LLM", rpx+10, rpy+32, C.cen_chiara, 1)
  elMinimap(img, rpx+18, rpy+44, 16, 3)
  -- timer, card, forgia, hint
  elTimer(img, CW/2, 8, true)
  elDiscoveryCard(img, CW/2, CH - 56)
  elGen(img, CW - 70, CH - 20)
  W.textOutlined(img, "[TAB] BUILD", 10, CH - 12, C.cen_chiara, 1)
end

local function hudV2(img)
  paintGameView(img)
  -- barra unica in alto
  W.frame9(img, 0, 0, CW, 46, { blend = 0.82 })
  W.text(img, "LA FONDITRICE", 12, 6, C.ard_p, 1)
  elHearts(img, 12, 16)
  elResources(img, 140, 17, false)
  elTimer(img, CW/2 + 60, 12, false)
  W.text(img, "PIANO 2/5 - TESORO", 640, 12, C.bianco, 1)
  W.text(img, "CROGIOLO DELLE SPORE", 640, 24, C.oro_p, 1)
  W.text(img, "FONTE: LLM", 800, 12, C.cen_chiara, 1)
  -- minimappa sotto la barra, a destra
  local side = 5*14 + 4*3
  W.frame9(img, CW - side - 24, 52, side + 16, side + 16, { blend = 0.78 })
  elMinimap(img, CW - side - 16, 60, 14, 3)
  -- slot in basso-sinistra
  W.frame9(img, 8, CH - 66, 96, 58, { blend = 0.78 })
  elSlots(img, 18, CH - 56)
  elDiscoveryCard(img, CW/2, CH - 56)
  elGen(img, CW - 70, CH - 20)
  W.textOutlined(img, "[TAB] BUILD", 118, CH - 24, C.cen_chiara, 1)
end

local function hudV3(img)
  paintGameView(img)
  -- niente pannelli: elementi flottanti con contorno
  W.textOutlined(img, "LA FONDITRICE", 10, 8, C.ard_p, 1)
  elHearts(img, 10, 17)
  elResources(img, 10, 34, true)
  elSlots(img, 10, CH - 76)
  elTimer(img, CW/2, 8, false)
  local flw = W.textW("PIANO 2/5 - TESORO", 2)
  W.textOutlined(img, "PIANO 2/5 - TESORO", CW - flw - 10, 8, C.bianco, 2)
  local wlw = W.textW("CROGIOLO DELLE SPORE", 2)
  W.textOutlined(img, "CROGIOLO DELLE SPORE", CW - wlw - 10, 22, C.oro_p, 2)
  elMinimap(img, CW - (5*14+4*3) - 10, 40, 14, 3)
  elDiscoveryCard(img, CW/2, CH - 56)
  elGen(img, CW - 70, CH - 20)
  W.textOutlined(img, "[TAB] BUILD", 90, CH - 50, C.cen_chiara, 2)
end

-- -------------------------------------------------------------- BuildScreen
-- Correzioni del proprietario (CP2): font x2 per tutto il contenuto e ogni
-- area dentro una cornice esplicita con la sua etichetta.
local function sectionFrame(img, x, y, w, h, title)
  W.frame9(img, x, y, w, h, {})
  local tw = W.textW(title, 2)
  W.fillRect(img, x + 10, y - 5, tw + 12, 14, C.slag_scuro)
  W.fillRect(img, x + 10, y - 5, tw + 12, 1, C.bronzo_s)
  W.fillRect(img, x + 10, y + 8, tw + 12, 1, C.bronzo_s)
  W.fillRect(img, x + 10, y - 5, 1, 14, C.bronzo_s)
  W.fillRect(img, x + 10 + tw + 11, y - 5, 1, 14, C.bronzo_s)
  W.text(img, title, x + 16, y - 3, C.oro_p, 2)
end

local function buildScreen(img)
  paintGameView(img)
  W.blendRect(img, 0, 0, CW, CH, C.slag_nero, 0.55)
  local px_, py_, pw, ph = 40, 14, 880, 612
  W.frame9(img, px_, py_, pw, ph, { rivets = true })
  W.text(img, "BUILD E SINERGIE", px_+16, py_+10, C.oro_p, 3)
  W.text(img, "[BUILD]", px_+330, py_+14, C.bagliore, 2)
  W.text(img, "PROVE", px_+400, py_+14, C.cen_chiara, 2)
  local hint = "[TAB] CHIUDI"
  W.text(img, hint, px_+pw-W.textW(hint,2)-16, py_+14, C.cen_chiara, 2)
  W.fillRect(img, px_+12, py_+34, pw-24, 1, C.bronzo_s)

  -- area sinistra: oggetti
  local ox_, oy_ = px_ + 16, py_ + 56
  sectionFrame(img, ox_, oy_, 360, 320, "OGGETTI")
  local items = {
    { "MARTELLO DI SLAG", C.brace, false, true },
    { "LENTE DEL CROGIOLO", C.ard_c, false, false },
    { "CUORE DI BRACE", C.brace, false, true },
    { "STIVALI DI PATINA", C.ver_c, false, false },
    { "SANGUE DI FLUX", C.pru_c, true, false },
    { "CAMPANA RICHIAMO [E]", C.bronzo_c, false, false },
    { "INNESTO: SPINE [G]", C.ver, false, false },
  }
  local ly = oy_ + 18
  for _, it in ipairs(items) do
    if it[3] then
      W.fillRect(img, ox_+6, ly-4, 348, 30, C.slag_scuro)
      W.fillRect(img, ox_+6, ly-4, 348, 1, C.bagliore)
      W.fillRect(img, ox_+6, ly+25, 348, 1, C.bagliore)
      W.fillRect(img, ox_+6, ly-4, 1, 30, C.bagliore)
      W.fillRect(img, ox_+353, ly-4, 1, 30, C.bagliore)
      W.text(img, ">", ox_+11, ly+5, C.bagliore, 2)
    end
    W.frame9(img, ox_+24, ly-1, 24, 24, {})
    W.diamond(img, ox_+35, ly+10, 5, it[2])
    W.text(img, it[1], ox_+56, ly+6, it[3] and C.bianco or C.fumo, 2)
    if it[4] then W.diamond(img, ox_+340, ly+10, 4, C.fiamma) end
    ly = ly + 34
  end
  W.text(img, "ROMBO ROSSO = IN SINERGIA", ox_+8, oy_+320-14, C.cen_chiara, 1)

  -- area destra: dettaglio
  local dx = px_ + 400
  sectionFrame(img, dx, py_+56, 460, 130, "DETTAGLIO")
  W.frame9(img, dx+10, py_+70, 78, 78, { rivets = true })
  W.renderS1(img, W.grids.potion32, W.maps.potion, dx+17, py_+77, 2)
  W.text(img, "SANGUE DI FLUX", dx+100, py_+72, C.bianco, 2)
  W.text(img, "MOD - RARO", dx+100, py_+88, C.pru_c, 2)
  W.text(img, "OGNI FUSIONE RESTITUISCE", dx+100, py_+108, C.fumo, 2)
  W.text(img, "1 CUORE TEMPORANEO", dx+100, py_+122, C.fumo, 2)
  W.text(img, "PREZZO PAGATO: 18 LINGOTTI", dx+10, py_+160, C.cen_chiara, 2)

  -- statistiche
  sectionFrame(img, dx, py_+208, 460, 58, "STATISTICHE")
  W.text(img, "SALUTE 4+2  DANNO 7  RITMO 2.4", dx+12, py_+224, C.bianco, 2)
  W.text(img, "VELOCITA 86  FORTUNA +1", dx+12, py_+242, C.bianco, 2)

  -- sinergie implicite
  sectionFrame(img, dx, py_+288, 460, 62, "SINERGIE ATTIVE")
  W.diamond(img, dx+20, py_+316, 6, C.fiamma)
  W.text(img, "BRACIA RISONANTE", dx+36, py_+302, C.fiamma_c, 2)
  W.text(img, "MARTELLO + CUORE: I COLPI BRUCIANO", dx+36, py_+320, C.fumo, 2)
  W.text(img, "COMPONENTI SEGNATI IN LISTA COL ROMBO", dx+36, py_+336, C.cen_chiara, 1)

  -- effetti temporanei
  sectionFrame(img, dx, py_+372, 460, 40, "EFFETTI TEMPORANEI")
  W.text(img, "SCUDO DI PATINA - 24S", dx+12, py_+388, C.patina, 2)

  -- fascia fusione in basso, cornice dedicata
  local fx, fy = px_ + 16, py_ + ph - 172
  sectionFrame(img, fx, fy, pw - 32, 150, "FUSIONE")
  W.frame9(img, fx+14, fy+22, 54, 54, {})
  W.diamond(img, fx+41, fy+49, 11, C.brace)
  W.text(img, "MARTELLO", fx+8, fy+82, C.fumo, 2)
  W.text(img, "+", fx+78, fy+42, C.bianco, 3)
  W.frame9(img, fx+100, fy+22, 54, 54, {})
  W.diamond(img, fx+127, fy+49, 11, C.brace)
  W.text(img, "CUORE", fx+106, fy+82, C.fumo, 2)
  W.renderS1(img, W.grids.flux11, W.maps.flux, fx+168, fy+32, 3)
  W.text(img, "1 FLUX", fx+160, fy+82, C.bagliore, 2)
  W.text(img, "=", fx+216, fy+42, C.bianco, 3)
  W.frame9(img, fx+240, fy+22, 54, 54, { interior = C.slag_scuro })
  W.text(img, "?", fx+260, fy+35, C.bagliore, 5)
  W.text(img, "MARTELLO ARDENTE?", fx+228, fy+82, C.cen_chiara, 2)
  W.text(img, "[F] FONDI - PRONTA", fx+400, fy+30, C.bagliore, 3)
  W.text(img, "CONSUMA LE DUE SORGENTI E 1 FLUX", fx+400, fy+56, C.fumo, 2)
  W.text(img, "ESITO SUBITO, RIFINITURA IA IN SOTTOFONDO", fx+400, fy+72, C.cen_chiara, 2)
  W.text(img, "SE MANCA IL FLUX LA CONFERMA NON HA EFFETTO", fx+400, fy+94, C.cen_chiara, 1)
end

-- ------------------------------------------------------------------ output
local SHEETS = {
  { name = "CP2-V1-angoli", paint = hudV1 },
  { name = "CP2-V2-barra", paint = hudV2 },
  { name = "CP2-V3-minimal", paint = hudV3 },
  { name = "CP2-BuildScreen", paint = buildScreen },
}

for _, sh in ipairs(SHEETS) do
  local spr = Sprite(CW, CH, ColorMode.RGB)
  local pal = Palette(#W.PAL)
  for i, p in ipairs(W.PAL) do
    pal:setColor(i - 1, Color{ r = p[1], g = p[2], b = p[3] })
  end
  spr:setPalette(pal)
  spr.layers[1].name = "mock"
  local img = Image(CW, CH, ColorMode.RGB)
  sh.paint(img)
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveAs(OUT .. sh.name .. ".aseprite")
  print(sh.name .. " salvato")
end
print("CP2 OK")
