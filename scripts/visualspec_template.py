#!/usr/bin/env python3
"""visualspec_template — costruttore DETERMINISTICO di prompt SD dal
VisualSpec (mandato R2/R3 06/08, confronto architetture "Gemma->VisualSpec
JSON->template" vs "Gemma->prompt libero"). Nessuna chiamata a modelli qui
dentro: e' il lato "template", il contrario del free_prompt che Gemma scrive
gia' finito -- questo file trasforma lo stesso identico contenuto (spec) in
un prompt SD sempre uguale a parita' di input, cosi' scripts/runtime-bench.sh
puo' isolare "quanto vale lo spec strutturato" da "quanto vale il prompt
libero" tenendo fisso tutto il resto (config, seed, trigger).

Contratto VisualSpec (congelato dall'orchestratore, generated/visualspecs/
batch.json, scritto da tools/melting-gen --visualspecs -- vedi il commento in
cima a scripts/runtime-bench.sh per il contesto completo):
  spec = {"category": "<=uguale a domain>", "subtype": "1-4 parole",
          "body_plan": "1-4 parole", "materials": ["2-4 voci"],
          "distinctive_feature": "frase breve", "size_class": "small|medium|large"}
domain = "character" | "enemy" | "weapon" | "item" | "boss_part"

L'altro campo del contratto, "free_prompt", NON passa da qui: e' gia' un
prompt SD COMPLETO scritto da Gemma (vista + soggetto + resa + soggetto
singolo + sfondo grigio piatto -- lo impone ValidateFreePrompt in
tools/melting-gen/gen_visualspec.c) e l'harness lo manda a sd-cli tale e
quale. E' proprio il punto del confronto: i due bracci devono chiedere
all'immagine le STESSE cose, l'uno tramite questo template deterministico e
l'altro tramite la prosa del modello. Se solo il braccio "spec" chiedesse la
vista e lo sfondo, il postprocesso comune (flood-fill dello sfondo + rimappa
di palette) premierebbe quel braccio per costruzione.

Stile del prompt (item 1 del task R2/R3): vista top-down three-quarter,
soggetto singolo, sfondo grigio neutro piatto, masse piatte -- stesso
registro dei contratti teacher-bench (docs/ai-production/dataset/
teacher-bench-2026-08-prompts{,-trackF}.json), MA stile Track F: nessun
divieto di outline nei negative (qui l'outline e' ammesso, va giudicato
dall'occhio, esattamente come Track F -- vedi la nota "outline_is_allowed_here"
nel contratto trackF). Il trigger della config (es. "basepixel, ") NON vive
qui: lo aggiunge il chiamante (scripts/runtime-bench.sh/runtime_bench_fusion.sh),
stesso schema di CONFIG_ROW in scripts/teacher-bench.sh (prefix + prompt).

Uso da riga di comando (debug/test, non il percorso principale):
  python3 scripts/visualspec_template.py --spec-json '{"category":...}' [--domain X]
  python3 scripts/visualspec_template.py --batch generated/visualspecs/batch.json
    (stampa, per ogni richiesta, un record NUL-delimitato: id\\0domain\\0
    spec_json\\0spec_prompt\\0negative\\0free_prompt\\0 -- e' il formato che
    scripts/runtime-bench.sh legge con `mapfile -d ''`, stesso pattern di
    load_prompts_contract() in scripts/teacher-bench.sh)

Dipendenze: solo stdlib.
"""
import argparse
import json
import sys

# Negativo standard PER DOMINIO, in stile Track F (SENZA il blocco
# anti-outline/anti-dithering di Track P/DEC-205 -- qui l'outline e' una
# scelta stilistica come le altre, non un difetto da vietare a monte).
# character/enemy/weapon/item sono gli stessi 4 negative gia' congelati in
# docs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json (letti
# li' il 06/08, riportati qui perche' questo file non deve aprire quel JSON a
# ogni chiamata solo per un negative -- e' testo duplicato di PROPOSITO, non
# un contratto che vive in due posti: se trackF cambia un negative, questa
# copia non lo eredita automaticamente, e va bene cosi' (i due esperimenti
# sono paralleli, non lo stesso). boss_part NON ha un equivalente diretto in
# trackF (quel contratto ha "boss" intero, non un componente/parte staccata):
# adattato a mano sullo stesso registro di weapon/item (una parte isolata,
# non un personaggio, nessuna mano che la regge) piu' il nucleo di
# "boss_furnace_core" (faccia/testo/entita' multiple/sfondo scenico).
NEGATIVE_BY_DOMAIN = {
    "character": (
        "face, eyes, mouth, facial features, weapon, sword, staff, holding "
        "object, hands visible, multiple characters, multiple views, text, "
        "watermark, signature, blurry, photo, 3d render"
    ),
    "enemy": (
        "human face, weapon, text, watermark, signature, multiple creatures, "
        "multiple views, blurry, photo, 3d render, background scenery, "
        "extra limbs"
    ),
    "weapon": (
        "hand, hands, arm, character, person holding, face, text, watermark, "
        "signature, multiple weapons, multiple views, blurry, photo, 3d render"
    ),
    "item": (
        "hand, hands, character, face, weapon, text, watermark, signature, "
        "multiple items, multiple views, blurry, photo, 3d render, label, "
        "price tag"
    ),
    "boss_part": (
        "hand, hands, character, person holding, face, human face, text, "
        "watermark, signature, multiple parts, multiple entities, multiple "
        "views, blurry, photo, 3d render, background scenery"
    ),
}
# Dominio di fallback per un domain sconosciuto/malformato nel batch: "item"
# e' il piu' neutro (nessuna assunzione su arti/volto), non un valore
# scelto a caso -- meglio un negative onesto-generico che nessuno.
_FALLBACK_DOMAIN = "item"

DOMAINS = tuple(NEGATIVE_BY_DOMAIN.keys())

# Ingredienti FISSI del template (item 1 del task): vista, sfondo, luce e
# masse piatte non dipendono mai dallo spec -- sono la parte "deterministica"
# del confronto, uguali per ogni richiesta e ogni dominio. L'ordine conta per
# SD1.5 (i token piu' vicini all'inizio pesano di piu'): soggetto e materiali
# prima, vincoli di resa dopo, esattamente come nei contratti teacher-bench.
_VIEW_PREFIX = ("top-down three-quarter view", "2d game sprite")
_RENDER_SUFFIX = (
    "single subject",
    "bold flat color masses",
    "soft value contrast edges",
    "single light from upper left",
    "flat solid neutral gray background",
)


def negative_for_domain(domain):
    """Negative Track F-style per 'domain'; sconosciuto -> _FALLBACK_DOMAIN
    (mai un negative vuoto: un negative assente non e' "nessun vincolo", e'
    un bug silenzioso in un harness che deve restare confrontabile)."""
    return NEGATIVE_BY_DOMAIN.get(domain, NEGATIVE_BY_DOMAIN[_FALLBACK_DOMAIN])


def _spec_subject_clause(spec):
    """'<subtype> <category>' -- il nucleo del soggetto, stessa forma di
    "forge hammer weapon icon"/"molten ember blob enemy" dei contratti
    teacher-bench (subtype prima, categoria dopo)."""
    subtype = str(spec.get("subtype") or "").strip()
    category = str(spec.get("category") or "").strip()
    if subtype and category:
        return f"{subtype} {category}"
    return subtype or category or "subject"


def build_spec_prompt(spec):
    """spec (dict, vedi contratto in cima al file) -> prompt SD deterministico.
    Nessuna LoRA/trigger qui: quello lo aggiunge il chiamante (vedi testata
    del file). Campi mancanti/vuoti vengono saltati (mai una clausola vuota
    tipo ", , " nel prompt finale) -- un batch.json malformato produce un
    prompt piu' corto, non uno rotto: la validazione dello schema e'
    responsabilita' di chi scrive batch.json (melting-gen), non di questo
    template."""
    parts = list(_VIEW_PREFIX)
    parts.append(_spec_subject_clause(spec))

    body_plan = str(spec.get("body_plan") or "").strip()
    if body_plan:
        parts.append(body_plan)

    materials = spec.get("materials") or []
    materials_txt = ", ".join(str(m).strip() for m in materials if str(m).strip())
    if materials_txt:
        parts.append(materials_txt)

    distinctive = str(spec.get("distinctive_feature") or "").strip()
    if distinctive:
        parts.append(distinctive)

    size_class = str(spec.get("size_class") or "").strip()
    if size_class:
        parts.append(f"{size_class} size")

    parts.extend(_RENDER_SUFFIX)
    return ", ".join(parts)


def build_fusion_prompt(spec_a, spec_b):
    """Prompt di fusione per la tecnica (a) "spec-fusion" di
    scripts/runtime_bench_fusion.sh: "fusion of X and Y, <materiali di
    entrambi>, ...". Non e' build_spec_prompt(a) + build_spec_prompt(b)
    concatenati (produrrebbe DUE clausole "single subject" contraddittorie e
    due sfondi): il nucleo "fusion of X and Y" sostituisce il singolo
    _spec_subject_clause, il resto del template (materiali uniti, resa
    finale) resta identico a build_spec_prompt cosi' i tre metodi di
    fusione (a/b/c) restano confrontabili sullo stesso target testuale."""
    subject_a = _spec_subject_clause(spec_a)
    subject_b = _spec_subject_clause(spec_b)
    parts = list(_VIEW_PREFIX)
    parts.append(f"fusion of {subject_a} and {subject_b}")
    parts.append("hybrid single object combining both")

    materials = []
    for spec in (spec_a, spec_b):
        for m in spec.get("materials") or []:
            m = str(m).strip()
            if m and m not in materials:  # dedup preservando l'ordine (A poi B)
                materials.append(m)
    if materials:
        parts.append(", ".join(materials))

    for spec in (spec_a, spec_b):
        distinctive = str(spec.get("distinctive_feature") or "").strip()
        if distinctive:
            parts.append(distinctive)

    parts.extend(_RENDER_SUFFIX)
    return ", ".join(parts)


def negative_for_domains(domains):
    """Unione ordinata (senza duplicati) dei negative dei domini elencati --
    usata dalle fusioni cross-dominio (es. enemy+item di
    scripts/runtime_bench_fusion.sh): il negative deve proteggere da
    entrambi i soggetti, non solo dal primo."""
    seen = []
    for d in domains:
        for token in negative_for_domain(d).split(", "):
            if token not in seen:
                seen.append(token)
    return ", ".join(seen)


def _emit_batch(batch_path):
    """Stampa su stdout un record NUL-delimitato per richiesta del batch
    (vedi docstring del file). Usato SOLO da scripts/runtime-bench.sh via
    `mapfile -d ''`: un campo per richiesta, mai un array nidificato, per lo
    stesso motivo di load_prompts_contract() in scripts/teacher-bench.sh
    (un formato ad-hoc CSV/TSV fatto a mano sarebbe fragile su prompt che
    contengono virgole e apici)."""
    with open(batch_path) as f:
        data = json.load(f)
    for req in data.get("requests", []):
        rid = str(req.get("id", ""))
        domain = str(req.get("domain", ""))
        spec = req.get("spec") or {}
        free_prompt = str(req.get("free_prompt", ""))
        spec_prompt = build_spec_prompt(spec)
        negative = negative_for_domain(domain)
        for field in (rid, domain, json.dumps(spec, ensure_ascii=False), spec_prompt, negative, free_prompt):
            sys.stdout.write(field)
            sys.stdout.write("\0")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--spec-json", help="spec singolo (stringa JSON) -> stampa il prompt template su stdout")
    g.add_argument("--batch", help="path a un batch.json (contratto VisualSpec) -> emette record NUL-delimitati")
    ap.add_argument("--domain", default="", help="con --spec-json: dominio per il negative (default: spec['category'])")
    args = ap.parse_args()

    if args.batch:
        _emit_batch(args.batch)
        return 0

    spec = json.loads(args.spec_json)
    domain = args.domain or str(spec.get("category") or "")
    print(build_spec_prompt(spec))
    print(negative_for_domain(domain), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
