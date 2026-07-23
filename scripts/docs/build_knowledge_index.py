#!/usr/bin/env python3
"""Indice e verifica della knowledge base di Worldsmelt (docs/).

Tre modalita' (vedi docs/_meta/DOCUMENT-STANDARDS.md):

  --check   verifica vincolante, exit 1 su qualunque violazione (default)
  --index   rigenera KNOWLEDGE_MANIFEST.json, docs/INDEX.md e gli INDEX di dominio
  --audit   rigenera docs/_meta/LINK-REPORT.md e docs/_meta/STALE-DOCUMENTS.md

Solo stdlib. Il front matter e' YAML semplice (scalari, liste inline [a, b],
liste a blocchi "- voce", scalari ripiegati ">-"): il parser qui sotto copre
esattamente quel sottoinsieme e fallisce rumorosamente sul resto, cosi' un
front matter esotico non passa inosservato.
"""

import argparse
import datetime
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"

GENERATED_MARKER = "<!-- GENERATED"

# Domini vivi sottoposti a verifica. docs/archive/ e' escluso per contratto;
# tutto cio' che sta fuori da queste cartelle (durante la migrazione: i vecchi
# docs/*.md, docs/ai-production/dataset/) non e' ancora nel perimetro.
SCAN_DIRS = ["design", "engineering", "ai-production", "plans", "references", "_meta"]
SCAN_ROOT_FILES = ["README.md"]

# File di profilo a root: i loro puntatori *.md devono esistere.
POINTER_FILES = ["README.md", "AGENTS.md", "CLAUDE.md", "HANDOFF.md"]
POINTER_GLOBS = [".claude/agents/*.md"]

REQUIRED_FIELDS = ["id", "title", "domain", "status", "authority", "summary"]
DOMAINS = ["design", "engineering", "ai-production", "plans", "references", "archive", "meta"]
STATUSES = ["draft", "proposed", "experimental", "approved", "implemented",
            "superseded", "deprecated", "archived"]
AUTHORITIES = ["canonical", "supporting", "historical", "generated"]
STALE_DAYS = 60

INDEXED_DOMAIN_DIRS = ["design", "engineering", "ai-production"]


def fail(msg):
    print(f"ERRORE INTERNO: {msg}", file=sys.stderr)
    sys.exit(2)


def parse_front_matter(text, path):
    """Ritorna (dict, errori). Front matter assente -> (None, [])."""
    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        return None, []
    fm, errors = {}, []
    i, key = 1, None
    while i < len(lines):
        line = lines[i]
        if line.strip() == "---":
            return fm, errors
        if not line.strip() or line.lstrip().startswith("#"):
            i += 1
            continue
        m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):\s*(.*)$", line)
        if m:
            key, raw = m.group(1), m.group(2).strip()
            if raw in (">-", ">", "|", "|-"):
                folded = []
                i += 1
                while i < len(lines) and (lines[i].startswith("  ") or not lines[i].strip()):
                    if lines[i].strip() == "---":
                        break
                    folded.append(lines[i].strip())
                    i += 1
                fm[key] = " ".join(x for x in folded if x).strip()
                continue
            if raw == "":
                fm[key] = []  # attesa lista a blocchi
            elif raw.startswith("[") and raw.endswith("]"):
                inner = raw[1:-1].strip()
                fm[key] = [v.strip().strip("'\"") for v in inner.split(",") if v.strip()] if inner else []
            else:
                fm[key] = raw.strip("'\"").split("#")[0].strip() if " #" in raw else raw.strip("'\"")
        elif re.match(r"^\s+-\s+", line) and key is not None and isinstance(fm.get(key), list):
            fm[key].append(line.split("-", 1)[1].strip().strip("'\""))
        else:
            errors.append(f"{path}: riga di front matter non riconosciuta: {line!r}")
        i += 1
    errors.append(f"{path}: front matter aperto con '---' ma mai chiuso")
    return fm, errors


def is_generated(text):
    return GENERATED_MARKER in text[:300]


def live_documents():
    """Tutti i doc vivi nel perimetro: [(path, text, fm, fm_errors)]."""
    out = []
    paths = []
    for d in SCAN_DIRS:
        base = DOCS / d
        if base.is_dir():
            paths.extend(sorted(base.rglob("*.md")))
    for name in SCAN_ROOT_FILES:
        p = DOCS / name
        if p.is_file():
            paths.append(p)
    for p in paths:
        rel = p.relative_to(ROOT)
        if p.name == "CLAUDE.md":
            continue
        text = p.read_text(encoding="utf-8")
        if is_generated(text):
            continue
        fm, errors = parse_front_matter(text, rel)
        out.append((p, text, fm, errors))
    return out


LINK_RE = re.compile(r"\[[^\]]*\]\(([^)\s]+)\)")
BACKTICK_MD_RE = re.compile(r"`([A-Za-z0-9_./-]+\.md)`")


def iter_md_links(text):
    """Link markdown fuori dai blocchi di codice."""
    in_fence = False
    for line in text.splitlines():
        if line.lstrip().startswith("```"):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        for m in LINK_RE.finditer(line):
            yield m.group(1)


def check_link_target(base_dir, target, root_fallback=False):
    """True se il target relativo esiste (ignora http/mailto/ancore pure).

    root_fallback: nei file di profilo a root (CLAUDE.md, .claude/agents/*, ...)
    i percorsi in backtick sono spesso relativi alla radice del repo, non al
    file che li cita: si accetta anche quella risoluzione.
    """
    if target.startswith(("http://", "https://", "mailto:", "#")):
        return True
    path = target.split("#", 1)[0]
    if not path or "*" in path:
        return True
    try:
        from urllib.parse import unquote
        path = unquote(path)
    except Exception:
        pass
    if path.startswith("/"):
        return (ROOT / path.lstrip("/")).exists()
    if (base_dir / path).resolve().exists():
        return True
    return root_fallback and (ROOT / path).exists()


def collect(check_today=None):
    """Raccoglie documenti + violazioni. Ritorna (docs, errors, warnings)."""
    errors, warnings = [], []
    docs = []
    seen_ids = {}
    today = check_today or datetime.date.today()

    entries = live_documents()
    for p, text, fm, fm_errors in entries:
        rel = str(p.relative_to(ROOT))
        errors.extend(fm_errors)
        if fm is None:
            errors.append(f"{rel}: front matter mancante (doc vivo)")
            continue
        for f in REQUIRED_FIELDS:
            if f not in fm or fm[f] in ("", []):
                errors.append(f"{rel}: campo obbligatorio mancante nel front matter: {f}")
        doc_id = fm.get("id")
        if isinstance(doc_id, str) and doc_id:
            if doc_id in seen_ids:
                errors.append(f"{rel}: id duplicato '{doc_id}' (gia' in {seen_ids[doc_id]})")
            else:
                seen_ids[doc_id] = rel
        if fm.get("domain") not in DOMAINS:
            errors.append(f"{rel}: domain non valido: {fm.get('domain')!r}")
        if fm.get("status") not in STATUSES:
            errors.append(f"{rel}: status non valido: {fm.get('status')!r}")
        if fm.get("authority") not in AUTHORITIES:
            errors.append(f"{rel}: authority non valida: {fm.get('authority')!r}")
        if fm.get("authority") == "canonical" and not fm.get("owner"):
            errors.append(f"{rel}: canonical senza owner")
        if fm.get("status") in ("approved", "implemented") and not fm.get("last_verified_commit"):
            errors.append(f"{rel}: status {fm.get('status')} senza last_verified_commit")
        if fm.get("authority") == "canonical" and fm.get("status") in ("superseded", "deprecated", "archived"):
            errors.append(f"{rel}: conflitto di authority: status {fm['status']} ma authority canonical")
        for sf in fm.get("source_files", []) or []:
            if "*" in sf:
                continue
            if not (ROOT / sf).exists():
                errors.append(f"{rel}: source_file inesistente: {sf}")
        lr = fm.get("last_reviewed")
        if isinstance(lr, str) and re.match(r"^\d{4}-\d{2}-\d{2}$", lr):
            age = (today - datetime.date.fromisoformat(lr)).days
            if age > STALE_DAYS:
                warnings.append(f"{rel}: stale ({age} giorni da last_reviewed {lr})")
        elif lr:
            warnings.append(f"{rel}: last_reviewed non in formato YYYY-MM-DD: {lr!r}")
        docs.append({"path": rel, "abs": p, "text": text, "fm": fm})

    # link interni dei doc vivi
    for d in docs:
        for target in iter_md_links(d["text"]):
            if not check_link_target(d["abs"].parent, target):
                errors.append(f"{d['path']}: link rotto: {target}")

    # related/supersedes verso id esistenti
    all_ids = set(seen_ids)
    for d in docs:
        for field in ("related", "supersedes"):
            for ref in d["fm"].get(field, []) or []:
                if ref and ref not in all_ids:
                    errors.append(f"{d['path']}: {field} verso id inesistente: {ref}")

    # supersedes -> il doc sostituito non puo' restare canonical/vivo-approvato
    by_id = {d["fm"].get("id"): d for d in docs if d["fm"].get("id")}
    for d in docs:
        for ref in d["fm"].get("supersedes", []) or []:
            target = by_id.get(ref)
            if target and target["fm"].get("status") not in ("superseded", "deprecated", "archived"):
                errors.append(
                    f"{d['path']}: supersede '{ref}' ma {target['path']} ha ancora status "
                    f"{target['fm'].get('status')} (atteso superseded/deprecated/archived)")

    # puntatori nei file di profilo a root
    pointer_paths = [ROOT / n for n in POINTER_FILES]
    for g in POINTER_GLOBS:
        pointer_paths.extend(sorted(ROOT.glob(g)))
    for p in pointer_paths:
        if not p.is_file():
            continue
        text = p.read_text(encoding="utf-8")
        rel = str(p.relative_to(ROOT))
        refs = set(iter_md_links(text)) | set(BACKTICK_MD_RE.findall(text))
        for target in refs:
            if not check_link_target(p.parent, target, root_fallback=True):
                errors.append(f"{rel}: puntatore rotto: {target}")

    return docs, errors, warnings


def manifest_payload(docs):
    payload = {
        "note": "GENERATO da scripts/docs/build_knowledge_index.py -- non modificare a mano",
        "count": len(docs),
        "documents": [],
    }
    for d in sorted(docs, key=lambda x: x["path"]):
        fm = d["fm"]
        payload["documents"].append({
            "id": fm.get("id"),
            "path": d["path"],
            "title": fm.get("title"),
            "domain": fm.get("domain"),
            "status": fm.get("status"),
            "authority": fm.get("authority"),
            "owner": fm.get("owner"),
            "summary": fm.get("summary"),
            "topics": fm.get("topics", []),
            "related": fm.get("related", []),
            "supersedes": fm.get("supersedes", []),
            "source_files": fm.get("source_files", []),
            "last_reviewed": fm.get("last_reviewed"),
            "last_verified_commit": fm.get("last_verified_commit"),
        })
    return payload


def manifest_text(docs):
    return json.dumps(manifest_payload(docs), ensure_ascii=False, indent=2, sort_keys=False) + "\n"


def domain_of_dir(d):
    parts = Path(d["path"]).parts
    return parts[1] if len(parts) > 1 else ""


def render_domain_index(docs, domain_dir):
    rows = [d for d in docs
            if domain_of_dir(d) == domain_dir and d["fm"].get("authority") != "historical"]
    hist = [d for d in docs
            if domain_of_dir(d) == domain_dir and d["fm"].get("authority") == "historical"]
    lines = [GENERATED_MARKER + ": make docs-index -- non modificare a mano -->", ""]
    lines.append(f"# Indice `docs/{domain_dir}/`")
    lines.append("")
    by_sub = {}
    for d in rows:
        parts = Path(d["path"]).parts
        sub = parts[2] if len(parts) > 3 else "."
        by_sub.setdefault(sub, []).append(d)
    for sub in sorted(by_sub, key=lambda s: (s != ".", s)):
        if sub != ".":
            lines.append(f"## {sub}/")
            lines.append("")
        for d in sorted(by_sub[sub], key=lambda x: x["path"]):
            fm = d["fm"]
            relpath = str(Path(d["path"]).relative_to(Path("docs") / domain_dir))
            badge = f"{fm.get('status')}/{fm.get('authority')}"
            lines.append(f"- [{fm.get('title')}]({relpath}) — {fm.get('summary')} `[{badge}]`")
        lines.append("")
    if hist:
        lines.append(f"_{len(hist)} documenti historical esclusi dall'indice._")
        lines.append("")
    return "\n".join(lines)


def render_root_index(docs):
    lines = [GENERATED_MARKER + ": make docs-index -- non modificare a mano -->", ""]
    lines.append("# Indice della documentazione di Worldsmelt")
    lines.append("")
    lines.append("Rigenerato da `make docs-index`. Punto d'ingresso umano: [README.md](README.md).")
    lines.append("Standard e regole: [_meta/DOCUMENT-STANDARDS.md](_meta/DOCUMENT-STANDARDS.md).")
    lines.append("")
    counts = {}
    for d in docs:
        counts[domain_of_dir(d) or "docs/"] = counts.get(domain_of_dir(d) or "docs/", 0) + 1
    for domain_dir in INDEXED_DOMAIN_DIRS:
        n = counts.get(domain_dir, 0)
        lines.append(f"- [`{domain_dir}/`]({domain_dir}/INDEX.md) — {n} documenti")
    for extra in ("plans", "references", "_meta"):
        rows = [d for d in docs if domain_of_dir(d) == extra
                and d["fm"].get("authority") != "historical"]
        if not rows:
            continue
        lines.append(f"- `{extra}/`:")
        for d in sorted(rows, key=lambda x: x["path"]):
            relpath = str(Path(d["path"]).relative_to("docs"))
            lines.append(f"  - [{d['fm'].get('title')}]({relpath}) — {d['fm'].get('summary')}")
    lines.append("")
    lines.append("`archive/` è escluso dagli indici: contiene solo materiale storico.")
    lines.append("")
    return "\n".join(lines)


def do_index(docs):
    (DOCS / "_meta" / "KNOWLEDGE_MANIFEST.json").write_text(manifest_text(docs), encoding="utf-8")
    (DOCS / "INDEX.md").write_text(render_root_index(docs), encoding="utf-8")
    for domain_dir in INDEXED_DOMAIN_DIRS:
        base = DOCS / domain_dir
        if base.is_dir():
            (base / "INDEX.md").write_text(render_domain_index(docs, domain_dir), encoding="utf-8")
    print(f"docs-index: manifest + INDEX rigenerati ({len(docs)} documenti vivi)")


def do_audit(docs, errors, warnings):
    link_lines = [GENERATED_MARKER + ": make docs-audit -- non modificare a mano -->", "",
                  "# Link report", ""]
    broken = [e for e in errors if "link rotto" in e or "puntatore rotto" in e]
    link_lines.append(f"Link/puntatori rotti: **{len(broken)}**")
    link_lines.append("")
    link_lines.extend(f"- {e}" for e in broken)
    link_lines.append("")
    (DOCS / "_meta" / "LINK-REPORT.md").write_text("\n".join(link_lines), encoding="utf-8")

    stale_lines = [GENERATED_MARKER + ": make docs-audit -- non modificare a mano -->", "",
                   "# Documenti stale", "",
                   f"Soglia: {STALE_DAYS} giorni da `last_reviewed`.", ""]
    stale = [w for w in warnings if "stale" in w]
    stale_lines.append(f"Documenti stale: **{len(stale)}**")
    stale_lines.append("")
    stale_lines.extend(f"- {w}" for w in stale)
    stale_lines.append("")
    (DOCS / "_meta" / "STALE-DOCUMENTS.md").write_text("\n".join(stale_lines), encoding="utf-8")
    print(f"docs-audit: LINK-REPORT.md ({len(broken)} rotti) e STALE-DOCUMENTS.md ({len(stale)} stale)")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--index", action="store_true")
    mode.add_argument("--audit", action="store_true")
    ap.add_argument("--today", help="override della data (YYYY-MM-DD), per test")
    args = ap.parse_args()

    today = datetime.date.fromisoformat(args.today) if args.today else None
    docs, errors, warnings = collect(check_today=today)

    if args.index:
        do_index(docs)
        return
    if args.audit:
        do_audit(docs, errors, warnings)
        return

    # --check (default): manifest e indici devono essere gia' aggiornati
    manifest_file = DOCS / "_meta" / "KNOWLEDGE_MANIFEST.json"
    if not manifest_file.is_file():
        errors.append("docs/_meta/KNOWLEDGE_MANIFEST.json assente: eseguire make docs-index")
    elif manifest_file.read_text(encoding="utf-8") != manifest_text(docs):
        errors.append("manifest non aggiornato rispetto ai documenti: eseguire make docs-index")
    index_file = DOCS / "INDEX.md"
    if not index_file.is_file():
        errors.append("docs/INDEX.md assente: eseguire make docs-index")
    elif index_file.read_text(encoding="utf-8") != render_root_index(docs):
        errors.append("docs/INDEX.md non aggiornato: eseguire make docs-index")
    for domain_dir in INDEXED_DOMAIN_DIRS:
        f = DOCS / domain_dir / "INDEX.md"
        if (DOCS / domain_dir).is_dir():
            if not f.is_file():
                errors.append(f"docs/{domain_dir}/INDEX.md assente: eseguire make docs-index")
            elif f.read_text(encoding="utf-8") != render_domain_index(docs, domain_dir):
                errors.append(f"docs/{domain_dir}/INDEX.md non aggiornato: eseguire make docs-index")

    for w in warnings:
        print(f"AVVISO: {w}")
    if errors:
        for e in errors:
            print(f"ERRORE: {e}")
        print(f"docs-check: FALLITO ({len(errors)} errori, {len(warnings)} avvisi, {len(docs)} doc vivi)")
        sys.exit(1)
    print(f"docs-check: OK ({len(docs)} documenti vivi, {len(warnings)} avvisi)")


if __name__ == "__main__":
    main()
