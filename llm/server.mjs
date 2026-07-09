import http from "node:http";
import { generateRun, RUN_JSON, RUN_MANIFEST } from "./run_content.mjs";

const PORT = Number(process.env.LLM_PORT || 8787);

function sendJson(res, status, body) {
  res.writeHead(status, { "Content-Type": "application/json; charset=utf-8" });
  res.end(`${JSON.stringify(body, null, 2)}\n`);
}

const server = http.createServer(async (req, res) => {
  const url = new URL(req.url, `http://127.0.0.1:${PORT}`);

  if (url.pathname === "/health") {
    sendJson(res, 200, { ok: true, service: "melting-run-llm", hasApiKey: Boolean(process.env.OPENAI_API_KEY) });
    return;
  }

  if (url.pathname === "/generate") {
    const seed = Number(url.searchParams.get("seed") || Date.now());
    const fallback = url.searchParams.get("fallback") === "1";
    const image = url.searchParams.get("image") === "1";
    const run = await generateRun({ seed, forceFallback: fallback, generateImage: image });
    sendJson(res, 200, { ok: true, source: run.source, seed: run.seed, json: RUN_JSON, manifest: RUN_MANIFEST, atlas: run.atlas?.path });
    return;
  }

  sendJson(res, 404, { ok: false, error: "Endpoint non trovato. Usa /health o /generate?seed=123." });
});

server.listen(PORT, "127.0.0.1", () => {
  console.log(`LLM sidecar attivo su http://127.0.0.1:${PORT}`);
  console.log("Endpoint utili: /health, /generate?seed=123");
});
