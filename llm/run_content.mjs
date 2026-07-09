import syncFs from "node:fs";
import fs from "node:fs/promises";
import path from "node:path";

function loadLocalEnv() {
  for (const fileName of [".env.local", ".env"]) {
    const filePath = path.resolve(fileName);
    if (!syncFs.existsSync(filePath)) continue;
    const text = syncFs.readFileSync(filePath, "utf8");
    for (const rawLine of text.split(/\r?\n/)) {
      const line = rawLine.trim();
      if (!line || line.startsWith("#")) continue;
      const eq = line.indexOf("=");
      if (eq <= 0) continue;
      const key = line.slice(0, eq).trim();
      const allowed = [
        "OPENAI_API_KEY",
        "OPENAI_MODEL",
        "OPENAI_REASONING_EFFORT",
        "OPENAI_IMAGE_MODEL",
        "OPENAI_IMAGE_QUALITY"
      ];
      if (!allowed.includes(key) || process.env[key]) continue;
      let value = line.slice(eq + 1).trim();
      if ((value.startsWith('"') && value.endsWith('"')) || (value.startsWith("'") && value.endsWith("'"))) {
        value = value.slice(1, -1);
      }
      process.env[key] = value;
    }
  }
}

loadLocalEnv();

export const DEFAULT_MODEL = process.env.OPENAI_MODEL || "gpt-5.5";
export const DEFAULT_REASONING_EFFORT = normalizeReasoningEffort(process.env.OPENAI_REASONING_EFFORT);
export const DEFAULT_IMAGE_MODEL = process.env.OPENAI_IMAGE_MODEL || "gpt-image-2";
export const DEFAULT_IMAGE_QUALITY = process.env.OPENAI_IMAGE_QUALITY || "medium";
export const GENERATED_DIR = path.resolve("generated");
export const RUN_JSON = path.join(GENERATED_DIR, "current_run.json");
export const RUN_MANIFEST = path.join(GENERATED_DIR, "current_run.txt");
export const RUN_ATLAS_PNG = path.join(GENERATED_DIR, "current_atlas.png");
export const RUN_ATLAS_BMP = path.join(GENERATED_DIR, "current_atlas.bmp");
export const RUN_ATLAS_JSON = path.join(GENERATED_DIR, "current_atlas.json");

const HEX = /^#[0-9a-fA-F]{6}$/;
const SLOTS = ["hat", "eyes", "hand", "back", "body", "aura"];
const TRAITS = ["bounce", "homing", "explode", "split", "pierce", "rapid", "giant", "slow", "vamp"];
const SCRIPT_TRAITS = [...TRAITS, "none"];
const SCRIPT_TRIGGERS = ["on_fire", "on_hit"];
const SCRIPT_OPS = ["burst", "projectile", "area", "heal"];

const TRAIT_SCRIPT_RULES = {
  bounce: { trigger: "on_fire", op: "burst", a: 2, b: 0.25, trait: "bounce" },
  homing: { trigger: "on_hit", op: "projectile", a: 2, b: 260, trait: "homing" },
  explode: { trigger: "on_hit", op: "area", a: 58, b: 0.48, trait: "explode" },
  split: { trigger: "on_fire", op: "burst", a: 3, b: 0.36, trait: "split" },
  pierce: { trigger: "on_hit", op: "projectile", a: 1, b: 420, trait: "pierce" },
  rapid: { trigger: "on_fire", op: "burst", a: 2, b: 0.16, trait: "rapid" },
  giant: { trigger: "on_hit", op: "area", a: 44, b: 0.34, trait: "giant" },
  slow: { trigger: "on_hit", op: "area", a: 54, b: 0.22, trait: "slow" },
  vamp: { trigger: "on_hit", op: "heal", a: 18, b: 1, trait: "vamp" }
};

const SCRIPT_BOUNDS = {
  burst: { a: [1, 6], b: [0.05, 1.2] },
  projectile: { a: [1, 6], b: [120, 720] },
  area: { a: [18, 96], b: [0.05, 1.15] },
  heal: { a: [0, 60], b: [1, 2] }
};

const OP_TRAITS = {
  burst: ["split", "bounce", "rapid", "homing", "pierce"],
  projectile: ["homing", "pierce", "bounce", "rapid"],
  area: ["explode", "slow", "giant"],
  heal: ["vamp"]
};

const SCRIPT_TRAIT_PRIORITY = ["split", "bounce", "rapid", "homing", "pierce", "explode", "slow", "giant", "vamp"];

export const RUN_SCHEMA = {
  type: "object",
  additionalProperties: false,
  required: ["floors"],
  properties: {
    floors: {
      type: "array",
      minItems: 5,
      maxItems: 5,
      items: {
        type: "object",
        additionalProperties: false,
        required: [
          "theme", "style", "boss", "bg", "floor", "wall", "accent",
          "accent2", "enemy", "bossColor", "items"
        ],
        properties: {
          theme: { type: "string", minLength: 3, maxLength: 48 },
          style: { type: "string", minLength: 3, maxLength: 36 },
          boss: { type: "string", minLength: 3, maxLength: 48 },
          bg: { type: "string", pattern: "^#[0-9a-fA-F]{6}$" },
          floor: { type: "string", pattern: "^#[0-9a-fA-F]{6}$" },
          wall: { type: "string", pattern: "^#[0-9a-fA-F]{6}$" },
          accent: { type: "string", pattern: "^#[0-9a-fA-F]{6}$" },
          accent2: { type: "string", pattern: "^#[0-9a-fA-F]{6}$" },
          enemy: { type: "string", pattern: "^#[0-9a-fA-F]{6}$" },
          bossColor: { type: "string", pattern: "^#[0-9a-fA-F]{6}$" },
          items: {
            type: "array",
            minItems: 3,
            maxItems: 3,
            items: {
              type: "object",
              additionalProperties: false,
              required: ["name", "slot", "traits", "color", "script"],
              properties: {
                name: { type: "string", minLength: 3, maxLength: 40 },
                slot: { type: "string", enum: SLOTS },
                traits: {
                  type: "array",
                  minItems: 1,
                  maxItems: 2,
                  items: { type: "string", enum: TRAITS }
                },
                color: { type: "string", pattern: "^#[0-9a-fA-F]{6}$" },
                script: {
                  type: "array",
                  minItems: 1,
                  maxItems: 3,
                  items: {
                    type: "object",
                    additionalProperties: false,
                    required: ["trigger", "op", "a", "b", "trait"],
                    properties: {
                      trigger: { type: "string", enum: SCRIPT_TRIGGERS },
                      op: { type: "string", enum: SCRIPT_OPS },
                      a: { type: "number" },
                      b: { type: "number" },
                      trait: { type: "string", enum: SCRIPT_TRAITS }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
};

function makeRng(seed) {
  let state = seed >>> 0 || 0xA341316C;
  return {
    next() {
      state ^= state << 13;
      state ^= state >>> 17;
      state ^= state << 5;
      state >>>= 0;
      return state || 0xA341316C;
    },
    range(min, max) {
      return min + (this.next() % (max - min + 1));
    }
  };
}

function hex(n) {
  return n.toString(16).padStart(2, "0");
}

function hsvToHex(h, s, v) {
  const c = v * s;
  const x = c * (1 - Math.abs((h / 60) % 2 - 1));
  const m = v - c;
  let r = 0, g = 0, b = 0;
  if (h < 60) [r, g, b] = [c, x, 0];
  else if (h < 120) [r, g, b] = [x, c, 0];
  else if (h < 180) [r, g, b] = [0, c, x];
  else if (h < 240) [r, g, b] = [0, x, c];
  else if (h < 300) [r, g, b] = [x, 0, c];
  else [r, g, b] = [c, 0, x];
  return `#${hex(Math.round((r + m) * 255))}${hex(Math.round((g + m) * 255))}${hex(Math.round((b + m) * 255))}`;
}

function fallbackScript(trait, rng) {
  if (trait === "bounce") return [{ trigger: "on_fire", op: "burst", a: 2, b: 0.25, trait: "bounce" }];
  if (trait === "homing") return [{ trigger: "on_hit", op: "projectile", a: 2, b: 260, trait: "homing" }];
  if (trait === "explode") return [{ trigger: "on_hit", op: "area", a: 58, b: 0.48, trait: "explode" }];
  if (trait === "split") return [{ trigger: "on_fire", op: "burst", a: 3, b: 0.36, trait: "split" }];
  if (trait === "pierce") return [{ trigger: "on_hit", op: "projectile", a: 1, b: 420, trait: "pierce" }];
  if (trait === "rapid") return [{ trigger: "on_fire", op: "burst", a: 2, b: 0.16, trait: "rapid" }];
  if (trait === "giant") return [{ trigger: "on_hit", op: "area", a: 44, b: 0.34, trait: "giant" }];
  if (trait === "slow") return [{ trigger: "on_hit", op: "area", a: 54, b: 0.22, trait: "slow" }];
  if (trait === "vamp") return [{ trigger: "on_hit", op: "heal", a: 18, b: 1, trait: "vamp" }];
  return [{ trigger: rng.range(0, 1) ? "on_hit" : "on_fire", op: "projectile", a: 1, b: 300, trait: "none" }];
}

export function fallbackRun(seed = Date.now()) {
  const rng = makeRng(Number(seed) || Date.now());
  const themeWords = ["Cantina", "Biblioteca", "Acquario", "Fucina", "Cattedrale", "Laboratorio", "Teatro"];
  const weirdWords = ["Neon", "Muffita", "Lunare", "Radioattiva", "di Zucchero", "Elettrica", "di Carta"];
  const styles = ["pixel semplice", "toon scuro", "arcade secco", "inchiostro piatto", "low-fi fantasy"];
  const itemNames = ["Corona", "Occhiali", "Guanto", "Mantello", "Medaglia", "Cappello", "Aureola", "Spada"];

  const floors = [];
  for (let i = 0; i < 5; i++) {
    const h = rng.range(0, 359);
    const items = [];
    for (let j = 0; j < 3; j++) {
      const trait = TRAITS[rng.range(0, TRAITS.length - 1)];
      items.push({
        name: `${itemNames[rng.range(0, itemNames.length - 1)]} ${trait}`,
        slot: SLOTS[rng.range(0, SLOTS.length - 1)],
        traits: [trait],
        color: hsvToHex((h + 80 + j * 53) % 360, 0.75, 0.92),
        script: fallbackScript(trait, rng)
      });
    }
    floors.push({
      theme: `${themeWords[rng.range(0, themeWords.length - 1)]} ${weirdWords[rng.range(0, weirdWords.length - 1)]}`,
      style: styles[rng.range(0, styles.length - 1)],
      boss: i === 4 ? "Ultimo Custode" : `Custode ${i + 1}`,
      bg: hsvToHex(h, 0.32, 0.12),
      floor: hsvToHex((h + 20) % 360, 0.38, 0.22),
      wall: hsvToHex((h + 52) % 360, 0.55, 0.45),
      accent: hsvToHex((h + 100) % 360, 0.62, 0.86),
      accent2: hsvToHex((h + 172) % 360, 0.70, 0.94),
      enemy: hsvToHex((h + 235) % 360, 0.58, 0.82),
      bossColor: hsvToHex((h + 300) % 360, 0.75, 0.88),
      items
    });
  }
  return { source: "fallback", seed: Number(seed) || Date.now(), floors };
}

function assertString(value, fallback) {
  return typeof value === "string" && value.trim() ? value.trim().slice(0, 64) : fallback;
}

function assertColor(value, fallback) {
  return typeof value === "string" && HEX.test(value) ? value : fallback;
}

function clampNumber(value, fallback, min, max) {
  const n = Number(value);
  if (!Number.isFinite(n)) return fallback;
  return Math.max(min, Math.min(max, n));
}

function normalizeReasoningEffort(value) {
  return ["low", "medium", "high", "xhigh"].includes(value) ? value : "medium";
}

function uniqueTraits(traits) {
  const found = [];
  for (const trait of Array.isArray(traits) ? traits : []) {
    if (TRAITS.includes(trait) && !found.includes(trait)) found.push(trait);
  }
  return found.sort((a, b) => SCRIPT_TRAIT_PRIORITY.indexOf(a) - SCRIPT_TRAIT_PRIORITY.indexOf(b));
}

function pickScriptTrait(op, traits, fallbackTrait) {
  const compatible = OP_TRAITS[op] || [];
  if (compatible.includes(fallbackTrait) && traits.includes(fallbackTrait)) return fallbackTrait;
  return traits.find((trait) => compatible.includes(trait)) || "none";
}

function preferredOpForTraits(traits) {
  const trait = uniqueTraits(traits)[0];
  return TRAIT_SCRIPT_RULES[trait]?.op || "projectile";
}

function normalizeScriptOp(op, traits) {
  const rawTrigger = SCRIPT_TRIGGERS.includes(op?.trigger) ? op.trigger : "on_hit";
  let kind = SCRIPT_OPS.includes(op?.op) ? op.op : "projectile";

  if (rawTrigger === "on_fire") kind = "burst";
  else if (kind === "burst") kind = "projectile";
  if (!traits.some((trait) => OP_TRAITS[kind]?.includes(trait))) kind = preferredOpForTraits(traits);

  const trait = pickScriptTrait(kind, traits, op?.trait);
  const rule = TRAIT_SCRIPT_RULES[trait] || { trigger: kind === "burst" ? "on_fire" : "on_hit", op: kind, a: 1, b: kind === "projectile" ? 280 : 0.35, trait };
  const bounds = SCRIPT_BOUNDS[kind];
  let a = clampNumber(op?.a, rule.a, bounds.a[0], bounds.a[1]);
  let b = clampNumber(op?.b, rule.b, bounds.b[0], bounds.b[1]);
  if (kind === "burst" || kind === "projectile" || kind === "heal") a = Math.round(a);
  if (kind === "heal") b = Math.round(b);
  return {
    trigger: kind === "burst" ? "on_fire" : "on_hit",
    op: kind,
    a,
    b,
    trait
  };
}

function fallbackScriptsForTraits(traits, fallback) {
  const ops = [];
  for (const trait of uniqueTraits(traits)) {
    if (TRAIT_SCRIPT_RULES[trait]) ops.push({ ...TRAIT_SCRIPT_RULES[trait] });
    if (ops.length >= 3) break;
  }
  return ops.length ? ops : fallback;
}

function normalizeScript(script, fallback, traits) {
  const itemTraits = uniqueTraits(traits);
  const rawOps = Array.isArray(script) ? script : [];
  const ops = rawOps.slice(0, 3).map((op) => normalizeScriptOp(op, itemTraits));
  const scriptedTraits = new Set(ops.map((op) => op.trait).filter((trait) => trait !== "none"));
  for (const trait of itemTraits) {
    if (ops.length >= 3) break;
    if (!scriptedTraits.has(trait) && TRAIT_SCRIPT_RULES[trait]) {
      ops.push({ ...TRAIT_SCRIPT_RULES[trait] });
      scriptedTraits.add(trait);
    }
  }
  return ops.length ? ops : fallbackScriptsForTraits(itemTraits, fallback);
}

export function normalizeRun(raw, seed) {
  const fallback = fallbackRun(seed);
  const floors = Array.isArray(raw?.floors) ? raw.floors : [];
  const normalized = { source: raw?.source || "openai", seed: Number(seed) || Date.now(), floors: [] };

  for (let i = 0; i < 5; i++) {
    const floor = floors[i] || {};
    const fb = fallback.floors[i];
    const items = Array.isArray(floor.items) ? floor.items : [];
    normalized.floors.push({
      theme: assertString(floor.theme, fb.theme),
      style: assertString(floor.style, fb.style),
      boss: assertString(floor.boss, fb.boss),
      bg: assertColor(floor.bg, fb.bg),
      floor: assertColor(floor.floor, fb.floor),
      wall: assertColor(floor.wall, fb.wall),
      accent: assertColor(floor.accent, fb.accent),
      accent2: assertColor(floor.accent2, fb.accent2),
      enemy: assertColor(floor.enemy, fb.enemy),
      bossColor: assertColor(floor.bossColor, fb.bossColor),
      items: [0, 1, 2].map((idx) => {
        const item = items[idx] || {};
        const fbi = fb.items[idx];
        const traits = Array.isArray(item.traits)
          ? item.traits.filter((trait) => TRAITS.includes(trait)).slice(0, 2)
          : [];
        const itemTraits = traits.length ? traits : fbi.traits;
        return {
          name: assertString(item.name, fbi.name),
          slot: SLOTS.includes(item.slot) ? item.slot : fbi.slot,
          traits: itemTraits,
          color: assertColor(item.color, fbi.color),
          script: normalizeScript(item.script, fbi.script, itemTraits)
        };
      })
    });
  }
  return normalized;
}

function extractOutputText(response) {
  if (typeof response.output_text === "string") return response.output_text;
  for (const item of response.output || []) {
    for (const part of item.content || []) {
      if (part.type === "output_text" && typeof part.text === "string") return part.text;
    }
  }
  return "";
}

export async function requestOpenAIRun({
  seed,
  model = DEFAULT_MODEL,
  reasoningEffort = DEFAULT_REASONING_EFFORT,
  apiKey = process.env.OPENAI_API_KEY
}) {
  if (!apiKey) throw new Error("OPENAI_API_KEY non impostata");

  const body = {
    model,
    instructions:
      "Crea contenuti originali per un piccolo action roguelite top-down. " +
      "Non copiare IP esistenti. Usa solo i valori ammessi dallo schema. " +
      "Gli script sono sandboxati e devono usare solo queste coppie valide: " +
      "on_fire puo' usare solo burst; on_hit puo' usare projectile, area o heal. " +
      "Parametri: burst a=count b=spreadRadians; projectile a=count b=speed; " +
      "area a=radius b=damageScale; heal a=chancePercent b=amount. " +
      "Ogni script deve rafforzare almeno uno dei tratti dell'oggetto.",
    input:
      `Seed run: ${seed}. Genera 5 piani, 3 oggetti per piano, temi e boss brevi. ` +
      `Ogni oggetto deve avere 1-3 script utili, diversi e coerenti con i suoi trait. ` +
      `Evita script puramente cosmetici: ogni oggetto deve creare una piccola sinergia giocabile.`,
    reasoning: { effort: normalizeReasoningEffort(reasoningEffort) },
    text: {
      verbosity: "low",
      format: {
        type: "json_schema",
        name: "melting_dynamic_run",
        strict: true,
        schema: RUN_SCHEMA
      }
    },
    max_output_tokens: 4200,
    store: false
  };

  const res = await fetch("https://api.openai.com/v1/responses", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Authorization: `Bearer ${apiKey}`
    },
    body: JSON.stringify(body)
  });

  const data = await res.json().catch(() => ({}));
  if (!res.ok) {
    const message = data?.error?.message || `${res.status} ${res.statusText}`;
    throw new Error(message);
  }

  const outputText = extractOutputText(data);
  if (!outputText) throw new Error("Risposta OpenAI senza output_text");
  return normalizeRun(JSON.parse(outputText), seed);
}

function scriptToManifest(script) {
  return script.map((op) => `${op.trigger}:${op.op},${op.a},${op.b},${op.trait}`).join("|");
}

export function runToManifest(run) {
  const atlasPath = run.atlas?.path || "generated/current_atlas.bmp";
  const lines = [
    "# Generated by llm/generate_run.mjs",
    `source=${run.source || "unknown"}`,
    `seed=${run.seed || ""}`,
    `atlas.path=${atlasPath}`
  ];
  run.floors.forEach((floor, index) => {
    const n = index + 1;
    lines.push(`floor${n}.theme=${floor.theme}`);
    lines.push(`floor${n}.style=${floor.style}`);
    lines.push(`floor${n}.boss=${floor.boss}`);
    lines.push(`floor${n}.bg=${floor.bg}`);
    lines.push(`floor${n}.floor=${floor.floor}`);
    lines.push(`floor${n}.wall=${floor.wall}`);
    lines.push(`floor${n}.accent=${floor.accent}`);
    lines.push(`floor${n}.accent2=${floor.accent2}`);
    lines.push(`floor${n}.enemy=${floor.enemy}`);
    lines.push(`floor${n}.bossColor=${floor.bossColor}`);
    floor.items.forEach((item, itemIndex) => {
      const m = itemIndex + 1;
      lines.push(`floor${n}.item${m}.name=${item.name}`);
      lines.push(`floor${n}.item${m}.slot=${item.slot}`);
      lines.push(`floor${n}.item${m}.traits=${item.traits.join(",")}`);
      lines.push(`floor${n}.item${m}.color=${item.color}`);
      lines.push(`floor${n}.item${m}.script=${scriptToManifest(item.script)}`);
    });
  });
  return `${lines.join("\n")}\n`;
}

function atlasMetadata(atlasPath, referencePath = "") {
  return {
    path: atlasPath,
    referencePath,
    width: 1024,
    height: 1024,
    cellSize: 128,
    columns: 8,
    rows: 8,
    sprites: {
      player: [0, 0],
      enemy_chaser: [1, 0],
      enemy_shooter: [2, 0],
      enemy_tank: [3, 0],
      boss: [4, 0],
      item: [5, 0],
      heart: [6, 0],
      coin: [7, 0],
      bomb: [0, 1],
      key: [1, 1],
      exit: [2, 1],
      shot: [3, 1]
    }
  };
}

function rgb(hexColor) {
  const value = HEX.test(hexColor) ? hexColor : "#ffffff";
  return [
    Number.parseInt(value.slice(1, 3), 16),
    Number.parseInt(value.slice(3, 5), 16),
    Number.parseInt(value.slice(5, 7), 16)
  ];
}

function putPixel(pixels, width, x, y, color) {
  if (x < 0 || y < 0 || x >= width || y >= width) return;
  const idx = (y * width + x) * 4;
  pixels[idx + 0] = color[2];
  pixels[idx + 1] = color[1];
  pixels[idx + 2] = color[0];
  pixels[idx + 3] = 255;
}

function clampByte(value) {
  return Math.max(0, Math.min(255, Math.round(value)));
}

function mixColor(a, b, t) {
  return [
    clampByte(a[0] + (b[0] - a[0]) * t),
    clampByte(a[1] + (b[1] - a[1]) * t),
    clampByte(a[2] + (b[2] - a[2]) * t)
  ];
}

function shadeColor(color, factor) {
  return color.map((value) => clampByte(value * factor));
}

function fillRect(pixels, width, x, y, w, h, color) {
  for (let yy = y; yy < y + h; yy++) {
    for (let xx = x; xx < x + w; xx++) putPixel(pixels, width, xx, yy, color);
  }
}

function fillCircle(pixels, width, cx, cy, radius, color) {
  const r2 = radius * radius;
  for (let y = cy - radius; y <= cy + radius; y++) {
    for (let x = cx - radius; x <= cx + radius; x++) {
      const dx = x - cx;
      const dy = y - cy;
      if (dx * dx + dy * dy <= r2) putPixel(pixels, width, x, y, color);
    }
  }
}

function drawLine(pixels, width, x0, y0, x1, y1, thickness, color) {
  const steps = Math.max(Math.abs(x1 - x0), Math.abs(y1 - y0), 1);
  for (let i = 0; i <= steps; i++) {
    const t = i / steps;
    fillCircle(
      pixels,
      width,
      Math.round(x0 + (x1 - x0) * t),
      Math.round(y0 + (y1 - y0) * t),
      thickness,
      color
    );
  }
}

function fillDiamond(pixels, width, cx, cy, radius, color) {
  for (let y = -radius; y <= radius; y++) {
    const span = radius - Math.abs(y);
    fillRect(pixels, width, cx - span, cy + y, span * 2 + 1, 1, color);
  }
}

function drawPlayableCell(pixels, width, cell, run) {
  const size = 128;
  const x = (cell % 8) * size;
  const y = Math.floor(cell / 8) * size;
  const cx = x + 64;
  const cy = y + 64;
  const floor = run.floors[cell % run.floors.length] || run.floors[0];
  const accent = rgb(floor.accent);
  const accent2 = rgb(floor.accent2);
  const enemy = rgb(floor.enemy);
  const boss = rgb(floor.bossColor);
  const wall = rgb(floor.wall);
  const dark = [7, 9, 12];
  const line = shadeColor(wall, 1.35);

  if (cell === 0) {
    fillCircle(pixels, width, cx, cy - 28, 14, accent2);
    fillRect(pixels, width, cx - 5, cy - 14, 10, 42, accent2);
    drawLine(pixels, width, cx - 25, cy - 2, cx + 25, cy - 2, 3, accent2);
    drawLine(pixels, width, cx - 5, cy + 27, cx - 20, cy + 54, 4, accent2);
    drawLine(pixels, width, cx + 5, cy + 27, cx + 20, cy + 54, 4, accent2);
    fillCircle(pixels, width, cx - 5, cy - 30, 3, dark);
    fillCircle(pixels, width, cx + 6, cy - 30, 3, dark);
  } else if (cell >= 1 && cell <= 4) {
    const body = cell === 4 ? boss : enemy;
    const r = cell === 4 ? 42 : cell === 3 ? 32 : 27;
    fillCircle(pixels, width, cx, cy, r, body);
    if (cell === 2) {
      fillRect(pixels, width, cx + 10, cy - 7, 38, 14, body);
      fillCircle(pixels, width, cx + 47, cy, 8, accent);
    }
    if (cell === 3) {
      fillRect(pixels, width, cx - 34, cy + 21, 68, 12, shadeColor(body, 0.7));
      fillRect(pixels, width, cx - 38, cy - 5, 12, 26, shadeColor(body, 0.8));
      fillRect(pixels, width, cx + 26, cy - 5, 12, 26, shadeColor(body, 0.8));
    }
    if (cell === 4) {
      for (let i = 0; i < 6; i++) {
        const a = (Math.PI * 2 * i) / 6;
        drawLine(pixels, width, cx, cy, cx + Math.round(Math.cos(a) * 55), cy + Math.round(Math.sin(a) * 55), 5, shadeColor(body, 0.8));
      }
      fillCircle(pixels, width, cx, cy, 17, accent);
    }
    fillCircle(pixels, width, cx - 12, cy - 8, 5, dark);
    fillCircle(pixels, width, cx + 12, cy - 8, 5, dark);
  } else if (cell === 5) {
    fillDiamond(pixels, width, cx, cy, 36, accent);
    fillDiamond(pixels, width, cx, cy, 21, accent2);
    fillCircle(pixels, width, cx, cy, 8, dark);
  } else if (cell === 6) {
    fillCircle(pixels, width, cx - 14, cy - 8, 19, boss);
    fillCircle(pixels, width, cx + 14, cy - 8, 19, boss);
    fillDiamond(pixels, width, cx, cy + 14, 31, boss);
    fillCircle(pixels, width, cx, cy + 1, 10, accent2);
  } else if (cell === 7) {
    fillCircle(pixels, width, cx, cy, 32, accent);
    fillCircle(pixels, width, cx, cy, 22, accent2);
    fillRect(pixels, width, cx - 5, cy - 23, 10, 46, accent);
  } else if (cell === 8) {
    fillCircle(pixels, width, cx, cy + 8, 32, shadeColor(wall, 1.55));
    drawLine(pixels, width, cx + 13, cy - 20, cx + 28, cy - 43, 3, accent);
    fillCircle(pixels, width, cx + 30, cy - 46, 5, boss);
  } else if (cell === 9) {
    fillCircle(pixels, width, cx - 23, cy, 17, accent2);
    fillCircle(pixels, width, cx - 23, cy, 8, dark);
    fillRect(pixels, width, cx - 7, cy - 4, 50, 8, accent2);
    fillRect(pixels, width, cx + 24, cy + 4, 8, 14, accent2);
    fillRect(pixels, width, cx + 38, cy + 4, 8, 22, accent2);
  } else if (cell === 10) {
    fillCircle(pixels, width, cx, cy, 43, accent);
    fillCircle(pixels, width, cx, cy, 32, dark);
    fillCircle(pixels, width, cx, cy, 24, accent2);
    fillCircle(pixels, width, cx, cy, 15, dark);
  } else if (cell === 11) {
    drawLine(pixels, width, cx - 34, cy + 8, cx + 29, cy - 9, 5, accent);
    fillCircle(pixels, width, cx + 35, cy - 11, 12, accent2);
    fillCircle(pixels, width, cx + 47, cy - 14, 5, boss);
  } else {
    const variant = cell % 8;
    if (variant < 2) {
      fillRect(pixels, width, cx - 28, cy + 16, 56, 14, wall);
      drawLine(pixels, width, cx, cy + 16, cx, cy - 28, 4, accent2);
      fillCircle(pixels, width, cx - 14, cy - 15, 12, enemy);
      fillCircle(pixels, width, cx + 17, cy - 22, 13, accent);
    } else if (variant < 4) {
      fillDiamond(pixels, width, cx, cy, 33, line);
      fillDiamond(pixels, width, cx, cy, 23, wall);
      fillCircle(pixels, width, cx, cy, 7, accent2);
    } else if (variant < 6) {
      fillRect(pixels, width, cx - 31, cy - 22, 62, 44, wall);
      fillRect(pixels, width, cx - 22, cy - 13, 44, 26, shadeColor(wall, 1.45));
      drawLine(pixels, width, cx - 26, cy + 28, cx + 26, cy + 28, 3, accent);
    } else {
      fillCircle(pixels, width, cx - 17, cy + 12, 20, line);
      fillCircle(pixels, width, cx + 18, cy + 5, 27, wall);
      fillCircle(pixels, width, cx + 23, cy - 2, 7, accent2);
    }
  }
}

export async function writePlayableAtlas(run) {
  await fs.mkdir(GENERATED_DIR, { recursive: true });
  const width = 1024;
  const pixels = Buffer.alloc(width * width * 4);
  for (let cell = 0; cell < 64; cell++) drawPlayableCell(pixels, width, cell, run);

  const header = Buffer.alloc(54);
  const fileSize = 54 + pixels.length;
  header.write("BM", 0, "ascii");
  header.writeUInt32LE(fileSize, 2);
  header.writeUInt32LE(54, 10);
  header.writeUInt32LE(40, 14);
  header.writeInt32LE(width, 18);
  header.writeInt32LE(-width, 22);
  header.writeUInt16LE(1, 26);
  header.writeUInt16LE(32, 28);
  header.writeUInt32LE(0, 30);
  header.writeUInt32LE(pixels.length, 34);
  await fs.writeFile(RUN_ATLAS_BMP, Buffer.concat([header, pixels]));
  return "generated/current_atlas.bmp";
}

function buildAtlasPrompt(run) {
  const f = run.floors[0];
  return [
    "Create one technical 1024x1024 pixel art sprite sheet for a top-down roguelite game.",
    "The sheet must be an exact invisible 8 columns x 8 rows grid. Each cell is exactly 128x128 pixels.",
    "Each sprite must be fully contained inside its own cell, centered inside the middle 88x88 pixels.",
    "Leave at least 18 pixels of empty dark padding between every sprite and every cell edge.",
    "Do not merge cells. Do not overlap neighboring cells. Do not crop any sprite.",
    "Do not add labels, text, numbers, visible grid lines, borders, UI, watermarks, captions, shadows crossing cells, or background scenes.",
    "Use crisp pixel art, front/top-down readable silhouettes, simple shapes, and high contrast.",
    "Use one flat nearly-black background color (#050708) across the whole sheet.",
    `Theme: ${f.theme}. Style: ${f.style}. Palette: ${f.accent}, ${f.accent2}, ${f.enemy}, ${f.bossColor}.`,
    "Use this exact cell order, indexed left to right then top to bottom:",
    "0 player stickman hero, 1 small chaser enemy, 2 shooter enemy with cannon, 3 tank enemy, 4 boss monster,",
    "5 item relic, 6 heart pickup, 7 coin pickup, 8 bomb pickup, 9 key pickup, 10 glowing exit portal, 11 player projectile.",
    "Cells 12 through 63 can be decorative props, but each must still be a single isolated centered sprite."
  ].join(" ");
}

async function requestImageAtlas({ run, apiKey = process.env.OPENAI_API_KEY, model = DEFAULT_IMAGE_MODEL, quality = DEFAULT_IMAGE_QUALITY }) {
  if (!apiKey) throw new Error("OPENAI_API_KEY non impostata");
  await fs.mkdir(GENERATED_DIR, { recursive: true });
  const body = {
    model,
    prompt: buildAtlasPrompt(run),
    size: "1024x1024",
    quality,
    output_format: "png",
    n: 1
  };
  const res = await fetch("https://api.openai.com/v1/images/generations", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Authorization: `Bearer ${apiKey}`
    },
    body: JSON.stringify(body)
  });
  const data = await res.json().catch(() => ({}));
  if (!res.ok) {
    const message = data?.error?.message || `${res.status} ${res.statusText}`;
    throw new Error(message);
  }
  const imageBase64 = data?.data?.[0]?.b64_json;
  if (!imageBase64) throw new Error("Image API senza b64_json");
  await fs.writeFile(RUN_ATLAS_PNG, Buffer.from(imageBase64, "base64"));
  return "generated/current_atlas.png";
}

export async function writeRunFiles(run) {
  await fs.mkdir(GENERATED_DIR, { recursive: true });
  await fs.writeFile(RUN_JSON, `${JSON.stringify(run, null, 2)}\n`, "utf8");
  await fs.writeFile(RUN_MANIFEST, runToManifest(run), "utf8");
  await fs.writeFile(
    RUN_ATLAS_JSON,
    `${JSON.stringify(atlasMetadata(run.atlas?.path || "generated/current_atlas.bmp", run.atlas?.referencePath || ""), null, 2)}\n`,
    "utf8"
  );
}

export async function generateRun({
  seed = Date.now(),
  forceFallback = false,
  model = DEFAULT_MODEL,
  reasoningEffort = DEFAULT_REASONING_EFFORT,
  generateImage = false,
  imageModel = DEFAULT_IMAGE_MODEL,
  imageQuality = DEFAULT_IMAGE_QUALITY,
  useImageAtlas = true
} = {}) {
  let run;
  if (forceFallback) {
    run = fallbackRun(seed);
  } else {
    try {
      run = await requestOpenAIRun({ seed, model, reasoningEffort });
    } catch (error) {
      run = fallbackRun(seed);
      run.source = `fallback:${error.message}`;
    }
  }

  let atlasPath = await writePlayableAtlas(run);
  let referencePath = "";
  if (generateImage && !run.source.startsWith("fallback:") && run.source !== "fallback") {
    try {
      referencePath = await requestImageAtlas({ run, model: imageModel, quality: imageQuality });
      if (useImageAtlas) atlasPath = referencePath;
    } catch (error) {
      run.source = `${run.source}:image_fallback:${error.message}`;
    }
  }

  run.atlas = {
    path: atlasPath,
    referencePath,
    cellSize: 128,
    columns: 8,
    sprites: {
      player: 0,
      enemy_chaser: 1,
      enemy_shooter: 2,
      enemy_tank: 3,
      boss: 4,
      item: 5,
      heart: 6,
      coin: 7,
      bomb: 8,
      key: 9,
      exit: 10,
      shot: 11
    }
  };
  await writeRunFiles(run);
  return run;
}
