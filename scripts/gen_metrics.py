#!/usr/bin/env python3
"""Metriche di generazione (roadmap 16/07/2026, settimana 1).

Legge la cartella prodotta da scripts/gen-metrics.sh (manifest-<seed>.json +
corpus-<seed>.jsonl per ogni run vera) e misura le due cose che i test
funzionali non misurano:

  1. VALIDITA': quanto Lua e' valido al primo colpo, quanto dopo retry,
     quanto ripiega sulla mini-VM (dal corpus JSONL di gen_corpus.c);
  2. VARIETA': quanto i contenuti inventati si ripetono FRA run diverse
     (temi, colpi, nemici, boss, stanze, oggetti). La varieta' e' il valore
     centrale del gioco (vedi memoria "shot-types-ai-generated"): un numero
     che peggiora qui vale piu' di qualunque impressione a occhio.

Il report chiude con un campione d'inglese (DEC-052, generazione contenuti
inglese-first dal 18/07) da giudicare a occhio: la qualita' della lingua non
si riduce a un numero.
"""

import json
import sys
from collections import Counter
from pathlib import Path


def norm(s):
    return " ".join(str(s).lower().split())


# Stopword inglesi (DEC-052, generazione contenuti inglese-first dal 18/07):
# articoli, preposizioni e congiunzioni piu' comuni nei nomi generati (temi/
# colpi/nemici/boss/stanze/oggetti). Scartate insieme alle parole < 3
# caratteri prima di contare le parole-contenuto ricorrenti (vedi word_report
# sotto). Nome neutro apposta (non piu' ITALIAN_STOPWORDS): stessa lista di
# tools/melting-gen/gen_novelty.c:STOPWORDS -- le due liste vanno tenute
# sincronizzate a mano, non esiste un punto unico di verita' condiviso fra
# Python e C in questo repo (vedi il commento in gen_novelty.h).
STOPWORDS = {
    "the", "of", "in", "a", "an", "and", "or", "for", "to", "on", "at",
    "with", "from",
}


def word_report(per_run, cat):
    """Parole-contenuto per categoria: quante ricompaiono in run diverse.

    Perche' esiste, oltre alla misura "unici su totale" qui sopra: su una
    baseline abbiamo misurato 14/15 nomi unici in "temi" eppure il
    vocabolario di fondo era sempre lo stesso -- 'cathedral', 'cavern',
    'desert' spuntavano in ogni run sotto un aggettivo diverso
    ('frozen cathedral', 'sunken cathedral', ...). Il conteggio sui
    nomi INTERI non lo vede: sono stringhe diverse. Solo un occhio umano che
    legge il campione se n'e' accorto. Qui si scompone ogni nome in parole,
    si scartano le stopword inglesi e le parole troppo corte (< 3
    caratteri), e si conta quante parole-contenuto ricorrono in PIU' RUN
    DIVERSE (non solo ripetute piu' volte nella stessa run): e' quella
    ripetizione fra run, non l'unicita' dei nomi, il segnale che il modello
    sta convergendo su un vocabolario fisso.
    """
    run_word_sets = []
    word_total_occ = Counter()
    for pr in per_run:
        words_here = Counter()
        for name in pr[cat]:
            for w in name.split():
                if w in STOPWORDS or len(w) < 3:
                    continue
                words_here[w] += 1
        run_word_sets.append(set(words_here))
        word_total_occ.update(words_here)
    run_count = Counter()
    for s in run_word_sets:
        run_count.update(s)
    shared = {w: c for w, c in run_count.items() if c >= 2}
    top5 = sorted(shared.items(),
                  key=lambda kv: (-kv[1], -word_total_occ[kv[0]], kv[0]))[:5]
    return len(word_total_occ), len(shared), top5, word_total_occ


def load_runs(outdir):
    runs = []
    for mf in sorted(outdir.glob("manifest-*.json")):
        seed = mf.stem.split("-", 1)[1]
        data = json.loads(mf.read_text())
        corpus = []
        cf = outdir / f"corpus-{seed}.jsonl"
        if cf.exists():
            corpus = [json.loads(l) for l in cf.read_text().splitlines() if l.strip()]
        runs.append({"seed": seed, "manifest": data, "corpus": corpus})
    return runs


def lua_stats(corpus):
    """Per oggetto (theme+item), l'esito finale dei tentativi registrati."""
    items = {}
    for rec in corpus:
        if rec.get("kind") != "lua":
            continue
        key = (rec.get("theme", ""), rec.get("item", ""))
        items.setdefault(key, []).append(rec)
    stats = Counter()
    for _key, recs in items.items():
        recs.sort(key=lambda r: r.get("attempt", 0))
        outcomes = [r.get("outcome") for r in recs]
        if outcomes and outcomes[0] == "ok":
            stats["primo colpo"] += 1
        elif "ok" in outcomes:
            stats["dopo retry"] += 1
        elif "opted-out" in outcomes:
            stats["nessun comportamento"] += 1
        else:
            stats["ripiegato (mini-VM)"] += 1
    stats["oggetti"] = len(items)
    return stats


def category_values(manifest):
    """Le liste di nomi inventati, per categoria, da un manifest."""
    floors = manifest.get("floors", [])
    cats = {
        "temi": [f.get("theme", "") for f in floors],
        "colpi": [(f.get("shot") or {}).get("name", "") for f in floors],
        "nemici": [e.get("name", "") for f in floors for e in (f.get("enemies") or [])],
        "boss": [(f.get("bossType") or {}).get("name", "") for f in floors],
        "stanze": [(f.get("room") or {}).get("name", "") for f in floors],
        "oggetti": [i.get("name", "") for f in floors for i in (f.get("items") or [])],
    }
    return {k: [norm(v) for v in vals if v] for k, vals in cats.items()}


def main():
    outdir = Path(sys.argv[1] if len(sys.argv) > 1 else "logs/gen-metrics")
    runs = load_runs(outdir)
    if not runs:
        print(f"nessun manifest-*.json in {outdir}", file=sys.stderr)
        return 1

    print(f"== metriche su {len(runs)} run ({outdir}) ==\n")

    # -- fonte: una run finita in fallback misurerebbe il vocabolario fisso
    #    del procedurale, non il modello: va esclusa dalla varieta'.
    model_runs = []
    for r in runs:
        src = r["manifest"].get("source", "?")
        marker = "" if src.startswith("local:") else "   << NON dal modello: esclusa dalla varieta'"
        print(f"run seed {r['seed']}: source={src}{marker}")
        if src.startswith("local:"):
            model_runs.append(r)
    print()

    # -- 1. validita' Lua ----------------------------------------------------
    total = Counter()
    for r in runs:
        total.update(lua_stats(r["corpus"]))
    n = total.pop("oggetti", 0)
    print(f"-- validita' Lua ({n} oggetti totali) --")
    if n:
        for k in ("primo colpo", "dopo retry", "nessun comportamento", "ripiegato (mini-VM)"):
            v = total.get(k, 0)
            print(f"  {k:24s} {v:3d}  ({100.0*v/n:5.1f}%)")
    else:
        print("  nessun record 'lua' nel corpus (corpus spento o run senza fase Lua?)")
    print()

    # -- 2. varieta' inter-run ----------------------------------------------
    print("-- varieta' fra run (solo run dal modello) --")
    if len(model_runs) >= 2:
        per_run = [category_values(r["manifest"]) for r in model_runs]
        for cat in ("temi", "colpi", "nemici", "boss", "stanze", "oggetti"):
            allvals = [v for pr in per_run for v in pr[cat]]
            uniq = len(set(allvals))
            dup = len(allvals) - uniq
            # sovrapposizione media fra coppie di run (Jaccard sui nomi esatti)
            pairs, overlap = 0, 0.0
            for i in range(len(per_run)):
                for j in range(i + 1, len(per_run)):
                    a, b = set(per_run[i][cat]), set(per_run[j][cat])
                    if a or b:
                        overlap += len(a & b) / len(a | b)
                        pairs += 1
            jac = overlap / pairs if pairs else 0.0
            dupes = Counter(allvals)
            worst = ", ".join(f"'{k}'x{c}" for k, c in dupes.most_common(3) if c > 1)
            print(f"  {cat:9s} {uniq:3d} unici su {len(allvals):3d}  "
                  f"(duplicati {dup:2d}, jaccard medio fra run {jac:4.2f})"
                  f"{'  peggiori: ' + worst if worst else ''}")
        print()

        # -- 2b. parole ricorrenti fra run --------------------------------
        # (vedi word_report per il motivo: i nomi interi possono essere
        # tutti "unici" mentre il vocabolario di fondo converge lo stesso.)
        print("-- parole ricorrenti fra run (vocabolario, non nomi interi) --")
        for cat in ("temi", "colpi", "nemici", "boss", "stanze", "oggetti"):
            total_words, shared_n, top5, word_total_occ = word_report(per_run, cat)
            print(f"  {cat:9s} {total_words:3d} parole-contenuto totali, "
                  f"{shared_n:3d} condivise fra >=2 run")
            if top5:
                items = ", ".join(
                    f"'{w}' in {c}/{len(per_run)} run ({word_total_occ[w]} occ.)"
                    for w, c in top5)
                print(f"    piu' ricorrenti: {items}")
    else:
        print("  servono almeno 2 run dal modello per misurare la varieta'")
    print()

    # -- 3. campione d'inglese ------------------------------------------------
    print("-- campione d'inglese (giudica a occhio) --")
    for r in model_runs[:3]:
        floors = r["manifest"].get("floors", [])
        themes = " | ".join(f.get("theme", "?") for f in floors)
        print(f"  seed {r['seed']}: {themes}")
        if floors:
            names = [i.get("name", "?") for i in (floors[0].get("items") or [])]
            shot = (floors[0].get("shot") or {}).get("name", "?")
            print(f"    piano 1: colpo '{shot}', oggetti: {', '.join(names)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
