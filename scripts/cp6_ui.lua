-- CP6 — asset HUD reali per il layout V3 approvato al CP2 (stile S1):
--   assets/art/ui/icons.png/.json      icone risorse + cuori + slot (16px)
--   assets/art/ui/panel-9patch.png     cornice pannelli con rivetti (24px, slice 6)
--   assets/art/ui/slot-9patch.png      cornice slot senza rivetti (24px, slice 4)
--   assets/art/ui/font-5px.png/.json   font pixel 5px, larghezza variabile
-- Il contratto delle estensioni UI (slice, font map) e' documentato in
-- docs/ai-production/08-PIPELINE-SPRITE-ANIMAZIONI.md.
-- Riproducibile: aseprite -b --script scripts/cp6_ui.lua

local ROOT = "/home/meri/progetti/melting-run-gpu"
local W = dofile(ROOT .. "/scripts/lib/wsprite.lua")
local C = W.C

local function center16(grid)
  -- centra una griglia 11/12px in un frame 16 (offset gestito dal renderer)
  return grid
end

-- ------------------------------------------------------------- icone 16px
local function pad16(grid, off)
  local out = {}
  local n = #grid
  local pre = string.rep(".", off)
  local post = string.rep(".", 16 - n - off)
  for _ = 1, off do out[#out + 1] = string.rep(".", 16) end
  for _, row in ipairs(grid) do out[#out + 1] = pre .. row .. post end
  while #out < 16 do out[#out + 1] = string.rep(".", 16) end
  return out
end
local ICONS = {
  { "ingot", pad16(W.grids.ingot11, 2), W.maps.ingot },
  { "charge", pad16(W.grids.charge11, 2), W.maps.charge },
  { "key", pad16(W.grids.key11, 2), W.maps.key },
  { "flux", pad16(W.grids.flux11, 2), W.maps.flux },
  { "heart", pad16(W.grids.heart12, 2), W.maps.heart },
  { "heart_temp", pad16(W.grids.heart12, 2), W.maps.heartTemp },
  { "heart_empty", pad16(W.grids.heart12, 2), W.maps.heartEmpty },
  { "heart_half", pad16(W.grids.heart12half, 2), W.maps.heart },
  { "graft", pad16(W.grids.spike11, 2), W.maps.spike },
  { "active", pad16(W.grids.bell11, 2), W.maps.bell },
}
local iconSheet = { id = "icons", dir = "ui", fw = 16, fh = 16, anchor = { 8, 8 }, rows = {} }
for _, ic in ipairs(ICONS) do
  iconSheet.rows[#iconSheet.rows + 1] = {
    name = ic[1], fps = 1, loop = true,
    frames = { { grid = ic[2], map = ic[3] } },
  }
end
W.buildSheet(ROOT, iconSheet)

-- --------------------------------------------------------------- 9-patch
local function ninePatch(path, rivets, slice)
  local img = Image(24, 24, ColorMode.RGB)
  W.frame9(img, 0, 0, 24, 24, { rivets = rivets })
  local spr = Sprite(24, 24, ColorMode.RGB)
  spr:setPalette(W.mkPalette())
  local cel = spr.cels[1]
  if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
  cel.image = img
  cel.position = Point(0, 0)
  spr:saveCopyAs(ROOT .. "/assets/art/ui/" .. path .. ".png")
  spr:saveAs(ROOT .. "/assets/art-src/ui/" .. path .. ".aseprite")
  local f = io.open(ROOT .. "/assets/art/ui/" .. path .. ".json", "w")
  f:write('{"frame_w":24,"frame_h":24,"anchor":[0,0],"slice":[' .. slice ..
          ',' .. slice .. ',' .. slice .. ',' .. slice ..
          '],"anims":{"idle":{"row":0,"frames":1,"fps":1,"loop":true}}}\n')
  f:close()
  print(path .. " ok")
end
ninePatch("panel-9patch", true, 6)
ninePatch("slot-9patch", false, 4)

-- ------------------------------------------------------------------ font
local ORDER = {}
for ch in string.gmatch("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", ".") do
  ORDER[#ORDER + 1] = ch
end
for _, ch in ipairs({ ":", "/", "-", ".", "[", "]", ">", "+", "?", "!", ",", "'", "%", "=" }) do
  ORDER[#ORDER + 1] = ch
end
local totalW = 0
for _, ch in ipairs(ORDER) do totalW = totalW + #W.FONT[ch][1] + 1 end
local img = Image(totalW, 7, ColorMode.RGB)
local meta = {}
local x = 0
for _, ch in ipairs(ORDER) do
  local g = W.FONT[ch]
  local gw = #g[1]
  for gy = 1, 5 do
    for gx = 1, gw do
      if g[gy]:sub(gx, gx) == "1" then
        W.px(img, x + gx - 1, gy, C.bianco)
      end
    end
  end
  meta[#meta + 1] = '"' .. (ch == '"' and '\\"' or ch) .. '":{"x":' .. x ..
                    ',"w":' .. gw .. '}'
  x = x + gw + 1
end
local spr = Sprite(totalW, 7, ColorMode.RGB)
spr:setPalette(W.mkPalette())
local cel = spr.cels[1]
if cel == nil then cel = spr:newCel(spr.layers[1], 1) end
cel.image = img
cel.position = Point(0, 0)
spr:saveCopyAs(ROOT .. "/assets/art/ui/font-5px.png")
spr:saveAs(ROOT .. "/assets/art-src/ui/font-5px.aseprite")
local f = io.open(ROOT .. "/assets/art/ui/font-5px.json", "w")
f:write('{"glyph_h":5,"baseline_y":1,"space_w":3,"letter_spacing":1,"glyphs":{' ..
        table.concat(meta, ",") .. "}}\n")
f:close()
print("font-5px ok (" .. totalW .. "x7)")
print("CP6 OK")
