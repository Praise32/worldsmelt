#!/usr/bin/env python3
"""Genera la dashboard HTML consultabile della knowledge base (docs/).

Uso:  python3 scripts/docs/build_dashboard.py [output.html]

Legge tutti i Markdown sotto docs/ (archivio incluso, marcato come storico),
converte il Markdown in HTML lato build (nessuna dipendenza, nessun CDN: la
pagina e' autosufficiente e pubblicabile come Artifact) e produce una singola
pagina con sidebar per dominio, ricerca sui metadati, chip di stato/autorita'
e navigazione interna fra documenti (i link relativi .md diventano route #).

Rigenerare a ogni cambiamento rilevante della doc; l'HTML non si committa.
"""

import html
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_knowledge_index import parse_front_matter  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"

SKIP_NAMES = {"LINK-REPORT.md", "STALE-DOCUMENTS.md"}
GENERATED_MARKER = "<!-- GENERATED"

DOMAIN_ORDER = ["design", "engineering", "ai-production", "plans", "references",
                "_meta", "(radice)", "archive"]
DOMAIN_LABEL = {
    "design": "Design (canonico)", "engineering": "Engineering",
    "ai-production": "AI production", "plans": "Piani", "references": "Riferimenti",
    "_meta": "Meta e standard", "(radice)": "docs/", "archive": "Archivio (storico)",
}


def inline_md(s):
    s = html.escape(s, quote=False)
    s = re.sub(r"`([^`]+)`", r"<code>\1</code>", s)
    s = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", s)
    s = re.sub(r"(?<!\*)\*([^*\n]+)\*(?!\*)", r"<em>\1</em>", s)
    s = re.sub(r"~~([^~]+)~~", r"<del>\1</del>", s)
    return s


def link_target(base, href, all_paths):
    href = href.split("#", 1)[0]
    if not href or href.startswith(("http://", "https://", "mailto:")):
        return None
    cand = (base / href).resolve()
    try:
        rel = cand.relative_to(ROOT)
    except ValueError:
        return None
    rel = str(rel)
    return rel if rel in all_paths else None


def md_to_html(text, doc_path, all_paths):
    base = (ROOT / doc_path).parent
    lines = text.splitlines()
    out, i = [], 0
    in_list = None  # 'ul' | 'ol'
    in_quote = False

    def close_list():
        nonlocal in_list
        if in_list:
            out.append(f"</{in_list}>")
            in_list = None

    def close_quote():
        nonlocal in_quote
        if in_quote:
            out.append("</blockquote>")
            in_quote = False

    def links(s):
        def repl(m):
            label, href = m.group(1), m.group(2)
            tgt = link_target(base, href, all_paths)
            if tgt:
                return f'<a href="#{tgt}" data-nav>{label}</a>'
            if href.startswith(("http://", "https://")):
                return f'<a href="{href}" target="_blank" rel="noopener">{label}</a>'
            return f"<span class=\"deadlink\" title=\"{html.escape(href)}\">{label}</span>"
        return re.sub(r"\[([^\]]*)\]\(([^)\s]+)\)", repl, s)

    while i < len(lines):
        line = lines[i]
        # fenced code
        if line.lstrip().startswith("```"):
            close_list(); close_quote()
            fence = []
            i += 1
            while i < len(lines) and not lines[i].lstrip().startswith("```"):
                fence.append(lines[i]); i += 1
            out.append("<pre><code>" + html.escape("\n".join(fence)) + "</code></pre>")
            i += 1
            continue
        # table
        if line.lstrip().startswith("|") and i + 1 < len(lines) and re.match(r"^\s*\|[\s:|-]+\|?\s*$", lines[i + 1]):
            close_list(); close_quote()
            hdr = [c.strip() for c in line.strip().strip("|").split("|")]
            rows, i2 = [], i + 2
            while i2 < len(lines) and lines[i2].lstrip().startswith("|"):
                rows.append([c.strip() for c in lines[i2].strip().strip("|").split("|")])
                i2 += 1
            t = ["<div class=\"tablewrap\"><table><thead><tr>"]
            t += [f"<th>{links(inline_md(c))}</th>" for c in hdr]
            t.append("</tr></thead><tbody>")
            for r in rows:
                t.append("<tr>" + "".join(f"<td>{links(inline_md(c))}</td>" for c in r) + "</tr>")
            t.append("</tbody></table></div>")
            out.append("".join(t))
            i = i2
            continue
        m = re.match(r"^(#{1,6})\s+(.*)$", line)
        if m:
            close_list(); close_quote()
            lvl = min(len(m.group(1)) + 1, 6)  # h1 riservato al titolo pagina
            out.append(f"<h{lvl}>{links(inline_md(m.group(2)))}</h{lvl}>")
            i += 1
            continue
        if re.match(r"^\s*(-{3,}|\*{3,})\s*$", line):
            close_list(); close_quote()
            out.append("<hr>")
            i += 1
            continue
        if line.lstrip().startswith(">"):
            close_list()
            if not in_quote:
                out.append("<blockquote>"); in_quote = True
            content = line.lstrip()[1:].lstrip()
            if content:
                out.append(f"<p>{links(inline_md(content))}</p>")
            i += 1
            continue
        m = re.match(r"^(\s*)([-*]|\d+\.)\s+(.*)$", line)
        if m:
            close_quote()
            kind = "ol" if m.group(2).rstrip(".").isdigit() else "ul"
            if in_list != kind:
                close_list()
                out.append(f"<{kind}>"); in_list = kind
            body = m.group(3)
            # continuazioni indentate della stessa voce
            while i + 1 < len(lines) and re.match(r"^\s{2,}\S", lines[i + 1]) \
                    and not re.match(r"^\s*([-*]|\d+\.)\s+", lines[i + 1]) \
                    and not lines[i + 1].lstrip().startswith(("#", ">", "|", "```")):
                body += " " + lines[i + 1].strip(); i += 1
            out.append(f"<li>{links(inline_md(body))}</li>")
            i += 1
            continue
        if not line.strip():
            close_list(); close_quote()
            i += 1
            continue
        close_list(); close_quote()
        para = line.strip()
        while i + 1 < len(lines) and lines[i + 1].strip() \
                and not re.match(r"^(\s*([-*]|\d+\.)\s+|#{1,6}\s|\s*\||>|```|(-{3,})\s*$)", lines[i + 1]):
            para += " " + lines[i + 1].strip(); i += 1
        out.append(f"<p>{links(inline_md(para))}</p>")
        i += 1
    close_list(); close_quote()
    return "\n".join(out)


def collect():
    paths = sorted(p for p in DOCS.rglob("*.md") if p.name not in SKIP_NAMES)
    all_rel = {str(p.relative_to(ROOT)) for p in paths}
    docs = []
    for p in paths:
        rel = str(p.relative_to(ROOT))
        text = p.read_text(encoding="utf-8")
        if GENERATED_MARKER in text[:300] and p.name == "INDEX.md":
            continue
        fm, _ = parse_front_matter(text, rel)
        body = text
        if fm is not None and text.startswith("---\n"):
            parts = text.split("\n---\n", 1)
            if len(parts) == 2:
                body = parts[1]
        fm = fm or {}
        m = re.search(r"^#\s+(.+)$", body, re.M)
        title = fm.get("title") or (m.group(1).strip() if m else p.stem)
        seg = Path(rel).parts
        domain = seg[1] if len(seg) > 2 else ("(radice)" if len(seg) == 2 else "(radice)")
        if len(seg) == 2:
            domain = "(radice)"
        docs.append({
            "p": rel,
            "t": title,
            "d": domain,
            "sub": seg[2] if len(seg) > 3 else "",
            "s": fm.get("status", ""),
            "a": fm.get("authority", ""),
            "id": fm.get("id", ""),
            "rev": fm.get("last_reviewed", ""),
            "topics": fm.get("topics", []) if isinstance(fm.get("topics"), list) else [],
            "sum": fm.get("summary", ""),
            "h": md_to_html(body, rel, all_rel),
        })
    return docs


def build(docs, out_path):
    payload = json.dumps(docs, ensure_ascii=False, separators=(",", ":"))
    n_live = sum(1 for d in docs if d["d"] != "archive")
    groups_js = json.dumps(DOMAIN_ORDER, ensure_ascii=False)
    labels_js = json.dumps(DOMAIN_LABEL, ensure_ascii=False)
    page = """<title>Worldsmelt — Knowledge Base</title>
<style>
:root{
  --bg:#f2efe9; --panel:#e9e4da; --panel2:#dfd9cc; --ink:#26221c; --ink2:#6b6355;
  --line:#cfc7b8; --ember:#c26a1a; --ember2:#9c500e; --code:#e5dfd2;
  --ok:#5d7c4a; --info:#4a6d8c; --warn:#a3781f; --dead:#8c8478;
}
@media (prefers-color-scheme: dark){:root{
  --bg:#16130f; --panel:#201b15; --panel2:#2a241c; --ink:#e8e0d2; --ink2:#a89c88;
  --line:#3a3227; --ember:#e8862e; --ember2:#f2a45a; --code:#241f18;
  --ok:#8fac6f; --info:#7da3c4; --warn:#d9a441; --dead:#7a7264;
}}
:root[data-theme="dark"]{
  --bg:#16130f; --panel:#201b15; --panel2:#2a241c; --ink:#e8e0d2; --ink2:#a89c88;
  --line:#3a3227; --ember:#e8862e; --ember2:#f2a45a; --code:#241f18;
  --ok:#8fac6f; --info:#7da3c4; --warn:#d9a441; --dead:#7a7264;
}
:root[data-theme="light"]{
  --bg:#f2efe9; --panel:#e9e4da; --panel2:#dfd9cc; --ink:#26221c; --ink2:#6b6355;
  --line:#cfc7b8; --ember:#c26a1a; --ember2:#9c500e; --code:#e5dfd2;
  --ok:#5d7c4a; --info:#4a6d8c; --warn:#a3781f; --dead:#8c8478;
}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);
  font:15px/1.6 system-ui,-apple-system,"Segoe UI",sans-serif}
#app{display:flex;min-height:100vh}
#side{width:310px;min-width:310px;border-right:1px solid var(--line);
  background:var(--panel);padding:14px 0;position:sticky;top:0;height:100vh;
  overflow-y:auto}
#brand{padding:2px 16px 10px;border-bottom:1px solid var(--line)}
#brand .name{font-size:19px;font-weight:700;letter-spacing:.4px}
#brand .name b{color:var(--ember)}
#brand .sub{font-size:11.5px;color:var(--ink2);margin-top:2px}
#q{width:calc(100% - 32px);margin:12px 16px 6px;padding:7px 10px;
  border:1px solid var(--line);border-radius:6px;background:var(--bg);
  color:var(--ink);font:inherit;font-size:13.5px}
#q:focus{outline:2px solid var(--ember);outline-offset:-1px}
#nav details{border-bottom:1px solid var(--line)}
#nav summary{cursor:pointer;padding:8px 16px;font-size:11.5px;font-weight:700;
  letter-spacing:.09em;text-transform:uppercase;color:var(--ink2);list-style:none;
  display:flex;justify-content:space-between;align-items:center}
#nav summary::-webkit-details-marker{display:none}
#nav summary .n{font-weight:400;font-variant-numeric:tabular-nums}
#nav a{display:flex;gap:8px;align-items:baseline;padding:4px 16px 4px 22px;
  color:var(--ink);text-decoration:none;font-size:13.5px;line-height:1.35}
#nav a:hover{background:var(--panel2)}
#nav a.on{background:var(--panel2);box-shadow:inset 3px 0 0 var(--ember)}
#nav .subh{padding:6px 16px 2px 22px;font-size:10.5px;letter-spacing:.08em;
  text-transform:uppercase;color:var(--ink2)}
.dot{width:7px;min-width:7px;height:7px;border-radius:50%;display:inline-block;
  background:var(--dead);transform:translateY(-1px)}
.dot.approved,.dot.implemented{background:var(--ok)}
.dot.draft,.dot.proposed,.dot.experimental{background:var(--warn)}
.dot.superseded,.dot.deprecated,.dot.archived{background:var(--dead)}
#main{flex:1;min-width:0;padding:30px 44px 80px}
#crumb{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:12px;
  color:var(--ink2);margin-bottom:6px;word-break:break-all}
#main h1{font-size:27px;line-height:1.2;margin:.1em 0 .35em;text-wrap:balance}
.chips{display:flex;flex-wrap:wrap;gap:6px;margin:0 0 14px}
.chip{font-size:11px;letter-spacing:.05em;text-transform:uppercase;
  padding:2px 9px;border-radius:99px;border:1px solid var(--line);color:var(--ink2)}
.chip.s-approved,.chip.s-implemented{color:var(--ok);border-color:var(--ok)}
.chip.s-draft,.chip.s-proposed,.chip.s-experimental{color:var(--warn);border-color:var(--warn)}
.chip.canonical{color:var(--ember);border-color:var(--ember);font-weight:600}
.summary{background:var(--panel);border:1px solid var(--line);border-left:3px solid var(--ember);
  border-radius:6px;padding:10px 14px;color:var(--ink2);font-size:13.5px;margin-bottom:22px}
#body{max-width:78ch}
#body h2{font-size:21px;margin:1.4em 0 .5em;border-bottom:1px solid var(--line);
  padding-bottom:.25em;text-wrap:balance}
#body h3{font-size:17px;margin:1.3em 0 .4em}
#body h4,#body h5,#body h6{font-size:14.5px;margin:1.2em 0 .3em}
#body a{color:var(--ember2);text-decoration:none;border-bottom:1px solid transparent}
#body a:hover{border-bottom-color:var(--ember2)}
#body code{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:.88em;
  background:var(--code);padding:1px 5px;border-radius:4px}
#body pre{background:var(--code);border:1px solid var(--line);border-radius:8px;
  padding:12px 14px;overflow-x:auto}
#body pre code{background:none;padding:0}
#body blockquote{margin:1em 0;padding:2px 16px;border-left:3px solid var(--ember);
  background:var(--panel);border-radius:0 6px 6px 0;color:var(--ink2)}
#body .tablewrap{overflow-x:auto;margin:1em 0}
#body table{border-collapse:collapse;font-size:13.5px;min-width:50%}
#body th,#body td{border:1px solid var(--line);padding:6px 10px;text-align:left;
  vertical-align:top}
#body th{background:var(--panel);font-size:11.5px;letter-spacing:.05em;
  text-transform:uppercase;color:var(--ink2)}
#body hr{border:0;border-top:1px solid var(--line);margin:1.6em 0}
#body del{color:var(--ink2)}
.deadlink{border-bottom:1px dotted var(--dead);color:var(--ink2)}
#home .tiles{display:grid;grid-template-columns:repeat(auto-fill,minmax(210px,1fr));
  gap:12px;margin:18px 0 26px}
.tile{background:var(--panel);border:1px solid var(--line);border-radius:8px;
  padding:12px 14px;cursor:pointer}
.tile:hover{border-color:var(--ember)}
.tile .k{font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--ink2)}
.tile .v{font-size:15px;font-weight:600;margin-top:3px}
#results a{display:block;padding:7px 10px;border-bottom:1px solid var(--line);
  color:var(--ink);text-decoration:none}
#results a:hover{background:var(--panel2)}
#results .p{font-family:ui-monospace,monospace;font-size:11px;color:var(--ink2)}
#themeBtn{position:fixed;right:16px;top:12px;background:var(--panel);
  border:1px solid var(--line);border-radius:6px;color:var(--ink2);
  padding:4px 10px;cursor:pointer;font:inherit;font-size:12px}
@media (max-width:860px){#app{flex-direction:column}
  #side{width:100%;min-width:0;height:auto;position:static;max-height:45vh}
  #main{padding:20px 16px 60px}}
@media (prefers-reduced-motion: no-preference){
  #body{animation:fadein .18s ease}
  @keyframes fadein{from{opacity:.4}to{opacity:1}}}
</style>
<div id="app">
 <nav id="side">
  <div id="brand">
    <div class="name">WORLD<b>SMELT</b></div>
    <div class="sub">Knowledge base — __NLIVE__ documenti vivi + archivio</div>
  </div>
  <input id="q" type="search" placeholder="Cerca titolo, percorso, topic…" autocomplete="off">
  <div id="results" hidden></div>
  <div id="nav"></div>
 </nav>
 <main id="main"></main>
</div>
<button id="themeBtn" type="button">tema</button>
<script>
const DOCS=__PAYLOAD__;
const ORDER=__ORDER__, LABEL=__LABELS__;
const byPath=new Map(DOCS.map(d=>[d.p,d]));
const nav=document.getElementById('nav'), main=document.getElementById('main');
const q=document.getElementById('q'), results=document.getElementById('results');

function esc(s){const e=document.createElement('span');e.textContent=s;return e.innerHTML}
function buildNav(){
  let h='';
  for(const g of ORDER){
    const items=DOCS.filter(d=>d.d===g);
    if(!items.length)continue;
    const open=(g==='design')?' open':'';
    h+=`<details${open}><summary>${esc(LABEL[g]||g)} <span class="n">${items.length}</span></summary>`;
    const subs=[...new Set(items.map(d=>d.sub))];
    for(const s of subs){
      if(s)h+=`<div class="subh">${esc(s)}/</div>`;
      for(const d of items.filter(x=>x.sub===s))
        h+=`<a href="#${esc(d.p)}" data-p="${esc(d.p)}"><span class="dot ${esc(d.s)}"></span><span>${esc(d.t)}</span></a>`;
    }
    h+='</details>';
  }
  nav.innerHTML=h;
}
function chips(d){
  let c='';
  if(d.s)c+=`<span class="chip s-${esc(d.s)}">${esc(d.s)}</span>`;
  if(d.a)c+=`<span class="chip ${d.a==='canonical'?'canonical':''}">${esc(d.a)}</span>`;
  if(d.id)c+=`<span class="chip">${esc(d.id)}</span>`;
  if(d.rev)c+=`<span class="chip">rev ${esc(d.rev)}</span>`;
  return c;
}
function show(p){
  const d=byPath.get(p);
  if(!d){home();return}
  main.innerHTML=`<div id="crumb">${esc(d.p)}</div><h1>${esc(d.t)}</h1>
    <div class="chips">${chips(d)}</div>
    ${d.sum?`<div class="summary">${esc(d.sum)}</div>`:''}
    <div id="body">${d.h}</div>`;
  for(const a of nav.querySelectorAll('a'))a.classList.toggle('on',a.dataset.p===p);
  const cur=nav.querySelector(`a[data-p="${CSS.escape(p)}"]`);
  if(cur){cur.closest('details').open=true}
  main.scrollIntoView({block:'start'});
}
function home(){
  const kd=DOCS.filter(d=>d.d==='design'&&!d.p.includes('archive')).length;
  const canon=DOCS.filter(d=>d.a==='canonical').length;
  const quick=[['Percorso di lettura del design','docs/design/README.md'],
    ['Decision log (DEC-001..)','docs/design/governance/decision-log.md'],
    ['Open questions','docs/design/governance/open-questions.md'],
    ['Architettura verificata','docs/engineering/architecture.md'],
    ['Difetti noti','docs/engineering/known-issues.md'],
    ['Standard documentale','docs/_meta/DOCUMENT-STANDARDS.md'],
    ['AI production','docs/ai-production/README.md'],
    ['Audit documentale 22/07','docs/_meta/DOCUMENT-AUDIT.md']];
  main.innerHTML=`<div id="home"><h1>Il crogiolo della conoscenza</h1>
   <p style="color:var(--ink2);max-width:70ch">Tutta la documentazione di Worldsmelt,
   per dominio: il design canonico (con le decisioni DEC), lo stato tecnico verificato,
   la produzione IA, i piani, i riferimenti e l'archivio storico. Usa la ricerca o la
   colonna a sinistra; i link fra documenti navigano dentro la dashboard.</p>
   <div class="tiles">${quick.map(([t,p])=>`<div class="tile" onclick="location.hash='${p}'">
     <div class="k">${byPath.get(p)?esc(byPath.get(p).d):''}</div><div class="v">${esc(t)}</div></div>`).join('')}
   </div>
   <p style="color:var(--ink2);font-size:13px">${DOCS.length} documenti totali ·
   ${canon} canonici · ${kd} nel dominio design.</p></div>`;
  for(const a of nav.querySelectorAll('a'))a.classList.remove('on');
}
function route(){const p=decodeURIComponent(location.hash.slice(1));p?show(p):home()}
window.addEventListener('hashchange',route);
q.addEventListener('input',()=>{
  const s=q.value.trim().toLowerCase();
  if(s.length<2){results.hidden=true;nav.hidden=false;return}
  const hit=DOCS.filter(d=>(d.t+' '+d.p+' '+d.sum+' '+(d.topics||[]).join(' ')).toLowerCase().includes(s)).slice(0,40);
  results.innerHTML=hit.map(d=>`<a href="#${esc(d.p)}"><span class="dot ${esc(d.s)}"></span> ${esc(d.t)}<div class="p">${esc(d.p)}</div></a>`).join('')||'<a>Nessun risultato nei metadati</a>';
  results.hidden=false;nav.hidden=true;
});
document.getElementById('themeBtn').addEventListener('click',()=>{
  const r=document.documentElement;
  const cur=r.dataset.theme||(matchMedia('(prefers-color-scheme: dark)').matches?'dark':'light');
  r.dataset.theme=cur==='dark'?'light':'dark';
});
buildNav();route();
</script>"""
    page = page.replace("__NLIVE__", str(n_live))
    page = page.replace("__ORDER__", groups_js).replace("__LABELS__", labels_js)
    page = page.replace("__PAYLOAD__", payload)
    Path(out_path).write_text(page, encoding="utf-8")
    print(f"dashboard: {out_path} ({len(page)//1024} KB, {len(docs)} documenti)")


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else str(ROOT / "logs" / "dashboard.html")
    build(collect(), out)
