import { generateRun } from "./run_content.mjs";

function argValue(name, fallback) {
  const prefix = `--${name}=`;
  const found = process.argv.find((arg) => arg.startsWith(prefix));
  return found ? found.slice(prefix.length) : fallback;
}

const seed = Number(argValue("seed", Date.now()));
const model = argValue("model", process.env.OPENAI_MODEL || undefined);
const reasoningEffort = argValue("reasoning", process.env.OPENAI_REASONING_EFFORT || undefined);
const imageModel = argValue("image-model", process.env.OPENAI_IMAGE_MODEL || undefined);
const imageQuality = argValue("quality", process.env.OPENAI_IMAGE_QUALITY || undefined);
const forceFallback = process.argv.includes("--fallback");
const generateImage = process.argv.includes("--image");
const forceLocalAtlas = process.argv.includes("--local-atlas");
const useImageAtlas = process.argv.includes("--use-image-atlas") || !forceLocalAtlas;

const run = await generateRun({
  seed,
  model,
  reasoningEffort,
  forceFallback,
  generateImage,
  imageModel,
  imageQuality,
  useImageAtlas
});

console.log(`Run generata: source=${run.source} seed=${run.seed}`);
console.log(`Atlas: ${run.atlas?.path || "nessuno"}`);
if (run.atlas?.referencePath) console.log(`Atlas riferimento IA: ${run.atlas.referencePath}`);
for (let i = 0; i < run.floors.length; i++) {
  const floor = run.floors[i];
  console.log(`- Piano ${i + 1}: ${floor.theme} / boss ${floor.boss}`);
  for (const item of floor.items) {
    const scripts = item.script.map((op) => `${op.trigger}:${op.op}`).join(", ");
    console.log(`  * ${item.name}: ${item.traits.join("+")} / ${scripts}`);
  }
}
