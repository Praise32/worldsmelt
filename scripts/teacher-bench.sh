#!/usr/bin/env bash
# teacher-bench — orchestratore Stage A del benchmark teacher immagine
# (mandato 07-MANDATO-CLAUDE-CODEX.md del dossier ricerca agosto 2026,
# matrice in docs/ai-production/../03-MATRICE-BENCHMARK.md dello stesso
# dossier -- non canonico nel repo, solo la fonte del mandato). Chiama
# sd-cli di deps/stable-diffusion.cpp DIRETTAMENTE (non bin/melting-sprites,
# cablato su SD1.5+prompt/negative/quality-gate del gioco: qui servono
# checkpoint/LoRA/step/CFG arbitrari, uno per config), SEQUENZIALMENTE:
# stessa regola ferrea GPU di scripts/image-comparison.sh, un solo processo
# alla volta.
#
# Gli 8 soggetti canonici (+ negative per categoria) sono un CONTRATTO letto
# da docs/ai-production/dataset/teacher-bench-2026-08-prompts.json, versionato
# nel repo (senza prompt congelati il benchmark non e' riproducibile a
# distanza di mesi). Schema atteso (vedi quel file):
#   { "version": int, "seeds": [int, ...], "subjects": [ {id, category,
#     prompt, negative, silhouette_reference}, ... ],
#     "config_prompt_prefix": {"<CONFIG_ID>": "..."} }
# "seeds" e "config_prompt_prefix" sono opzionali (default sotto); ogni
# subject SENZA "negative" usa stringa vuota (non bloccante, ma va notato a
# revisione: la matrice vuole un negative per categoria).
#
# STAGE A E' SOLO TEXT-TO-IMAGE. La matrice di benchmark chiede per ogni
# prompt anche una "versione con silhouette originale": questo script NON la
# implementa e non ha alcun percorso img2img/ControlNet -- la variante e'
# DIFFERITA alle due configurazioni promosse dopo Stage A, insieme a 20/30
# step e al prompt fuori distribuzione. Il campo "silhouette_reference" dei
# soggetti resta quindi null e viene ignorato qui: e' il segnaposto di quel
# passaggio, non un campo dimenticato. Scegliere la base teacher prima di
# introdurre il condizionamento strutturale tiene una variabile alla volta.
#
# TRACK F ("caccia libera all'asset", righe F1..F9/S1..S4): secondo esperimento
# parallelo a Track P (A0..A4/T0), sempre text-only. Ogni modello gira alle
# SUE impostazioni native (trigger/peso/step/CFG/sampler del creatore) invece
# che a parita' di condizioni; il contratto e' un file DIVERSO,
# docs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json --
# stessi 8 soggetti e seed di Track P ma SENZA il blocco anti-outline/
# anti-dithering nei negative (in Track F l'outline e' ammesso e va giudicato
# dall'occhio) e con "judge_scale":64 in radice (il giudizio umano e' a 64x64
# nativo, non 32: alcuni candidati -8bitdiffuser64, la famiglia _s64- sono
# addestrati per quella scala). Le righe S1..S4 sono l'asse VELOCITA'
# few-step (LCM/Hyper-SD) a parita' di stile, non un secondo track a parte:
# usano anche loro il contratto trackF. Si lancia con:
#   scripts/teacher-bench.sh F1 F2 F3 F4 F5 F6 F7 F8 F9 S1 S2 S3 S4
#
# Uso:
#   scripts/teacher-bench.sh [CONFIG_ID ...]      (default: A0 A1 A2 T0)
#   scripts/teacher-bench.sh --list                elenca le config note ed esce
# Variabili:
#   PROMPTS_FILE   default docs/ai-production/dataset/teacher-bench-2026-08-prompts.json
#                  (contratto Track P; le righe F*/S* portano il proprio
#                  contratto nella tabella CONFIG_ROW, vedi sotto)
#   SD_CLI         default deps/stable-diffusion.cpp/build/bin/sd-cli
#   OUT_ROOT       default artifacts/image-model-research
#   WIDTH/HEIGHT   default 512 512 (cartella raw-512/, coerente col training a 512 di 01-VINCOLI)
#   DRY_RUN=1      stampa i comandi sd-cli pianificati (dopo i controlli di file mancanti/ledger)
#                  senza invocare GPU: usato dai test di questo script, utile anche a mano.
#
# Resume/incrementalita': un'immagine con raw-512/<config>/<subject>_<seed>.png
# gia' presente viene SALTATA (non rigenerata) -- e' quello che permette di
# aggiungere A3/A4 dopo senza ripetere A0/A1/A2/T0 gia' fatte. Il salto NON e'
# muto: se il manifest dell'immagine gia' presente dichiara prompt/negative/
# step/CFG/sampler diversi da quelli che si userebbero adesso, lo dice con un
# WARN -- e' il caso di un contratto di prompt cambiato dopo una corsa
# parziale, dove i vecchi PNG non sono confrontabili con i nuovi e vanno
# cancellati a mano prima di rilanciare. Un fallimento di UNA immagine
# (sd-cli non-zero o file di output assente/vuoto) NON ferma la suite
# (niente 'set -e'): viene registrato in failures/ e si continua.
set -uo pipefail
cd "$(dirname "$0")/.."

OUT_ROOT="${OUT_ROOT:-artifacts/image-model-research}"
RAW_DIR="$OUT_ROOT/raw-512"
MANIFEST_DIR="$OUT_ROOT/manifests"
FAILURES_DIR="$OUT_ROOT/failures"
LEDGER_FILE="$MANIFEST_DIR/model-ledger.json"
BENCH_MODEL_DIR="models/teacher-bench-2026-08"
LICENSE_SNAPSHOT_DIR="$BENCH_MODEL_DIR/license-snapshots"
LOG_DIR="logs/teacher-bench"   # log grezzi sd-cli per immagine: diagnostica, non output "obbligatorio"
                                # della matrice -- sotto logs/, gia' gitignored (vedi .gitignore).

PROMPTS_FILE="${PROMPTS_FILE:-docs/ai-production/dataset/teacher-bench-2026-08-prompts.json}"
SD_CLI="${SD_CLI:-deps/stable-diffusion.cpp/build/bin/sd-cli}"
WIDTH="${WIDTH:-512}"
HEIGHT="${HEIGHT:-512}"
DRY_RUN="${DRY_RUN:-0}"

DEFAULT_CONFIGS=(A0 A1 A2 T0)

if [ "${1:-}" = "--list" ]; then
  echo "config note: A0 A1 A2 A3 A4 T0 (Track P, default se nessun argomento: ${DEFAULT_CONFIGS[*]})"
  echo "config note: F1 F2 F3 F4 F5 F6 F7 F8 F9 S1 S2 S3 S4 (Track F, contratto trackF, mai nel default)"
  exit 0
fi

[ -f "$PROMPTS_FILE" ] || { echo "teacher-bench: prompt mancanti: $PROMPTS_FILE" >&2; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "teacher-bench: python3 richiesto" >&2; exit 1; }
if [ "$DRY_RUN" != "1" ]; then
  [ -x "$SD_CLI" ] || { echo "teacher-bench: sd-cli non trovato/eseguibile: $SD_CLI (compila deps/stable-diffusion.cpp)" >&2; exit 1; }
  # REGOLA FERREA GPU (CLAUDE.md/mandato): mai due processi GPU insieme.
  # SENZA -f apposta: pgrep -f confronta la riga di comando INTERA, e in
  # alcuni ambienti (harness che mostrano il comando eseguito per esteso in
  # ps, script che incollano questo stesso pattern in un log) il processo
  # che sta lanciando QUESTO controllo puo' auto-corrispondere -- verificato
  # in sessione: falso positivo riproducibile con -f, mai senza. Senza -f
  # pgrep confronta solo il nome del processo (comm, dal binario eseguito
  # davvero), che per sd-cli/melting-sprites e' esattamente il nome cercato.
  if pgrep 'sd-cli|melting-sprites' >/dev/null 2>&1; then
    echo "teacher-bench: un processo GPU (sd-cli/melting-sprites) e' gia' in esecuzione -- attendi e rilancia." >&2
    exit 1
  fi
fi

SD_CPP_COMMIT="unknown"
if [ -d deps/stable-diffusion.cpp/.git ]; then
  SD_CPP_COMMIT=$(git -C deps/stable-diffusion.cpp rev-parse --short=7 HEAD 2>/dev/null || echo unknown)
fi
TIME_BIN_AVAILABLE=""
command -v /usr/bin/time >/dev/null 2>&1 && TIME_BIN_AVAILABLE=1

mkdir -p "$RAW_DIR" "$MANIFEST_DIR" "$FAILURES_DIR" "$LOG_DIR"

# -- Tabella delle config Stage A + Track F -----------------------------------
# Campi (separatore \x1f, nessuno dei valori tecnici lo contiene -- le note
# leggibili stanno APPOSTA in CONFIG_NOTE sotto, stringa "normale": i
# commenti in italiano di questo repo sono pieni di apostrofi ("e'", "un'"),
# che dentro un $'...' ANSI-C andrebbero escappati uno per uno, fragile).
#   model_path \x1f lora_dir \x1f lora_name \x1f lora_weight \x1f steps \x1f
#   cfg_scale \x1f sampler \x1f scheduler \x1f default_trigger \x1f contract \x1f
#   extra_lora \x1f subjects_filter
# lora_name vuoto = nessuna LoRA. default_trigger = prefisso di prompt usato
# SOLO se il contratto (campo "contract" sotto) non definisce gia' un
# config_prompt_prefix per questa config (il JSON, contratto scritto da chi
# cura i contenuti, vince sempre quando presente).
# contract = path del contratto prompt/soggetti per QUESTA riga (item 2 Track
# F: righe diverse possono leggere contratti diversi nella stessa invocazione
# dello script). Vuoto = default $PROMPTS_FILE (il contratto P, retrocompatibile:
# le righe A0..A4/T0 storiche non lo valorizzano affatto e `read` assegna
# stringa vuota ai campi mancanti in coda, zero byte cambiati su quelle righe).
# extra_lora (opzionale) = "<dir>:<nome>:<peso>" di un SECONDO <lora:...> da
# comporre SOPRA la style LoRA (asse velocita' delle righe S*: una LoRA di
# accelerazione, LCM/Hyper-SD, che spesso vive in una cartella diversa dalla
# style LoRA -- vedi run_one, e' incorporata nel prompt con un path ASSOLUTO
# cosi' bypassa --lora-model-dir invece di richiederne un secondo, che sd-cli
# non supporta). Vuoto = nessuna seconda LoRA.
# subjects_filter (opzionale) = lista categorie separate da virgola (es.
# "weapon,item"): limita la riga ai soli soggetti di quelle categorie -- per
# LoRA addestrate su un sottoinsieme di oggetti (RPG Icons: solo item/arma).
# Vuoto = tutti i soggetti del contratto.
#
# Sampler/scheduler fissi (euler_a/karras) su A0-A4 per confronto onesto
# (matrice: "sampler deterministico documentato", MAI cambiato fra config
# senza che sia la variabile sotto esame); T0 usa lcm/lcm perche' e' la
# baseline di VELOCITA', non un teacher (mandato, "T0 ... non teacher").
#
# Track F (F1..F9/S1..S4) usa invece le impostazioni NATIVE del creatore per
# ogni modello (censimento agente ricerca agosto 2026): sampler diversi da
# riga a riga per costruzione, non e' un difetto di coerenza. Nota sui
# sampler "DPM++ 2M (SDE) Karras" dei creatori (8bitdiffuser, basepixel): il
# censimento di partenza presumeva sd.cpp senza l'equivalente esatto e
# chiedeva di documentare una deviazione -- VERIFICATO invece (sd-cli --help,
# --sampling-method) che questa build espone gia' "dpm++2m_sde" e "dpm++2m"
# come sampler REALI, piu' lo scheduler "karras": F1/F2/F8 usano quindi il
# nome del creatore 1:1, NESSUNA sostituzione necessaria. Verificato in
# sessione, non e' garantito restare vero su build future di sd.cpp -- se
# --sampling-method smette di elencare "dpm++2m_sde"/"dpm++2m", questa nota
# e' da riaprire insieme alle righe F1/F2/F8.
declare -A CONFIG_ROW
CONFIG_ROW[A0]=$'models/sd15-vanilla-pruned-emaonly.safetensors\x1f\x1f\x1f\x1f25\x1f7\x1feuler_a\x1fkarras\x1f'
CONFIG_ROW[A1]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1f\x1f\x1f\x1f25\x1f7\x1feuler_a\x1fkarras\x1f'
CONFIG_ROW[A2]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f0.6\x1f25\x1f7\x1feuler_a\x1fkarras\x1fbasepixel, '
CONFIG_ROW[A3]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1f8bitdiffuser64-v4-PX64NOCAP_epoch_10\x1f1.0\x1f25\x1f7\x1feuler_a\x1fkarras\x1fpixel_art, '
CONFIG_ROW[A4]=$'models/teacher-bench-2026-08/dreamshaperPixelart_v10.safetensors\x1f\x1f\x1f\x1f25\x1f7\x1feuler_a\x1fkarras\x1fpixel art, '
CONFIG_ROW[T0]=$'models/teacher-bench-2026-08/tokforge-dreamshaper-7-lcm-q4_0.gguf\x1f\x1f\x1f\x1f6\x1f1.5\x1flcm\x1flcm\x1f'
CONFIG_ROW[F1]=$'models/teacher-bench-2026-08/anyloraCheckpoint_bakedvaeBlessedFp16.safetensors\x1fmodels/teacher-bench-2026-08\x1f8bitdiffuser64-v4-PX64NOCAP_epoch_10\x1f1.0\x1f20\x1f7\x1fdpm++2m_sde\x1fkarras\x1fpixel_art, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[F2]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1f8bitdiffuser64-v4-PX64NOCAP_epoch_10\x1f1.0\x1f20\x1f7\x1fdpm++2m_sde\x1fkarras\x1fpixel_art, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[F3]=$'models/teacher-bench-2026-08/dreamshaperPixelart_v10.safetensors\x1f\x1f\x1f\x1f25\x1f7\x1feuler_a\x1fkarras\x1fpixel art, pixel art style, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[F4]=$'models/Public-Prompts-Pixel-Model.ckpt\x1f\x1f\x1f\x1f20\x1f10\x1feuler_a\x1fkarras\x1fpixelsprite, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[F5]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fmpixel-v3-pixel_f2\x1f1.0\x1f25\x1f7\x1feuler_a\x1fkarras\x1fpixel, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[F6]=$'models/teacher-bench-2026-08/pixelArtSpriteDiffusion_safetensors.safetensors\x1f\x1f\x1f\x1f25\x1f7\x1feuler_a\x1fkarras\x1fPixelartFSS \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[F7]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fPixhell_15\x1f1.0\x1f25\x1f7\x1feuler_a\x1fkarras\x1fpixelart, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[F8]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f1.0\x1f28\x1f7\x1fdpm++2m\x1fkarras\x1fbasepixel, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[F9]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1frpg-icons-lora\x1f1.0\x1f25\x1f7\x1feuler_a\x1fkarras\x1frpgicondiff, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1fweapon,item'
CONFIG_ROW[S1]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f0.6\x1f6\x1f1.5\x1flcm\x1flcm\x1fbasepixel, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1fmodels:lcm-lora-sdv1-5:1.0\x1f'
CONFIG_ROW[S2]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f0.6\x1f8\x1f5\x1feuler_a\x1fkarras\x1fbasepixel, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1fmodels/teacher-bench-2026-08:Hyper-SD15-8steps-CFG-lora:1.0\x1f'
CONFIG_ROW[S3]=$'models/teacher-bench-2026-08/anyloraCheckpoint_lcm.safetensors\x1fmodels/teacher-bench-2026-08\x1f8bitdiffuser64-v4-PX64NOCAP_epoch_10\x1f1.0\x1f6\x1f1.5\x1flcm\x1flcm\x1fpixel_art, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[S4]=$'models/teacher-bench-2026-08/DreamShaper8_LCM.safetensors\x1f\x1f\x1f\x1f8\x1f2\x1flcm\x1flcm\x1f\x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'

declare -A CONFIG_NOTE
CONFIG_NOTE[A0]="A0 baseline: SD1.5 vanilla, nessuna LoRA"
CONFIG_NOTE[A1]="A1: DreamShaper 8, nessuna LoRA (controllo base)"
CONFIG_NOTE[A2]="A2: DreamShaper 8 + Basepixel 0.6 (candidato principale)"
CONFIG_NOTE[A3]="A3: DreamShaper 8 + 8bitdiffuser64 (candidato griglia, Civitai). Peso LoRA 1.0 e trigger pixel_art (trainedWords Civitai versione 636318) sono un default di questo script: la matrice benchmark non specifica un peso, verificare prima di promuovere risultati"
CONFIG_NOTE[A4]="A4: DreamShaper PixelArt (Civitai), checkpoint sfidante. Prefisso 'pixel art' e' un default di questo script (trainedWords Civitai versione 142421), non un obbligo del contratto JSON"
CONFIG_NOTE[T0]="T0 smoke: baseline di VELOCITA (non teacher), CFG 1.5/6 step"
CONFIG_NOTE[F1]="F1 Track F: AnyLoRA (bakedVAE fp16) + 8bitdiffuser64 v4 1.0, impostazioni del creatore (Civitai 185743: scala LoRA 0.85-1.25 -> 1.0 come richiesto dal task, step 10-30/CFG 4-10 -> 20/7 in mezzo al range, default di questo script). Sampler creatore 'DPM++ 2M SDE Karras' VERIFICATO 1:1 su dpm++2m_sde+karras di sd-cli: nessuna deviazione (vedi nota sopra la tabella)"
CONFIG_NOTE[F2]="F2 Track F: DreamShaper 8 + 8bitdiffuser64 v4 1.0, STESSI settings di F1 (20 step/CFG 7/dpm++2m_sde+karras) a parita' di LoRA -- confronto di base AnyLoRA vs DreamShaper sotto lo stesso accessorio"
CONFIG_NOTE[F3]="F3 Track F: DreamShaper PixelArt (Civitai 129879) da solo, trigger 'pixel art, pixel art style, ' (trainedWords), 25 step/CFG 7 -- variante nativa di A4 (che in Track P usa solo 'pixel art, ')"
CONFIG_NOTE[F4]="F4 Track F: All-in-one Pixel Model (models/Public-Prompts-Pixel-Model.ckpt), trigger 'pixelsprite, ', 20 step/CFG 10 (creatore: CFG alto ~10). Checkpoint .ckpt, non .safetensors: se sd-cli non lo carica senza conversione e' un esito della fase GPU, non verificabile qui (regola ferrea niente GPU)"
CONFIG_NOTE[F5]="F5 Track F: DreamShaper 8 + M_Pixel v3 1.0, trigger 'pixel, ', 25 step/CFG 7 (non dichiarati dal creatore nel censimento, default di questo script)"
CONFIG_NOTE[F6]="F6 Track F: Pixel Art Sprite Diffusion, trigger 'PixelartFSS ', 25 step/CFG 7. Il modello genera FOGLI di camminata frontale, e' il suo scopo dichiarato: va giudicato per quello, non come sprite isolato singolo come gli altri candidati"
CONFIG_NOTE[F7]="F7 Track F: DreamShaper 8 + PIXHELL 1.0, trigger 'pixelart, ', 25 step/CFG 7 (non dichiarati dal creatore nel censimento, default di questo script)"
CONFIG_NOTE[F8]="F8 Track F: DreamShaper 8 + Basepixel 1.0 (vs 0.6 di A2) alle impostazioni del creatore: 28 step/CFG 7/'DPM++ 2M Karras'. Sampler creatore VERIFICATO 1:1 su dpm++2m+karras di sd-cli: nessuna deviazione (vedi nota sopra la tabella). Variante nativa di A2"
CONFIG_NOTE[F9]="F9 Track F: DreamShaper 8 + RPG Icons 1.0, trigger 'rpgicondiff, ', 25 step/CFG 7 (non dichiarati dal creatore, default di questo script). subjects_filter=weapon,item: la LoRA (Civitai) e' addestrata solo su oggetti/armi isolati, i soggetti character/enemy/boss sono fuori scopo e vengono saltati per questa riga"
CONFIG_NOTE[S1]="S1 asse velocita': DreamShaper 8 + Basepixel 0.6 + LCM-LoRA 1.0 sopra (extra_lora), sampler lcm, 6 step, CFG 1.5 (range creatore LCM-LoRA: 4-8 step/CFG 1-2). Peso 1.0 della LCM-LoRA e' un default di questo script, il creatore non ne dichiara uno"
# Scala di step (notte 06-07/08, mandato "diversifica i test"): le stesse due
# config S2/S3 a step diversi, per la curva qualita'-vs-step. Righe IDENTICHE
# alle madri salvo il campo step: la differenza misurata e' SOLO quella.
CONFIG_ROW[S2A]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f0.6\x1f4\x1f5\x1feuler_a\x1fkarras\x1fbasepixel, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1fmodels/teacher-bench-2026-08:Hyper-SD15-8steps-CFG-lora:1.0\x1f'
CONFIG_ROW[S2B]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f0.6\x1f6\x1f5\x1feuler_a\x1fkarras\x1fbasepixel, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1fmodels/teacher-bench-2026-08:Hyper-SD15-8steps-CFG-lora:1.0\x1f'
CONFIG_ROW[S3A]=$'models/teacher-bench-2026-08/anyloraCheckpoint_lcm.safetensors\x1fmodels/teacher-bench-2026-08\x1f8bitdiffuser64-v4-PX64NOCAP_epoch_10\x1f1.0\x1f4\x1f1.5\x1flcm\x1flcm\x1fpixel_art, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_ROW[S3B]=$'models/teacher-bench-2026-08/anyloraCheckpoint_lcm.safetensors\x1fmodels/teacher-bench-2026-08\x1f8bitdiffuser64-v4-PX64NOCAP_epoch_10\x1f1.0\x1f8\x1f1.5\x1flcm\x1flcm\x1fpixel_art, \x1fdocs/ai-production/dataset/teacher-bench-2026-08-prompts-trackF.json\x1f\x1f'
CONFIG_NOTE[S2A]="S2A curva step: identica a S2 ma 4 step (la Hyper-SD-LoRA e' tarata per 8: qui si misura il degrado sotto taratura)"
CONFIG_NOTE[S2B]="S2B curva step: identica a S2 ma 6 step"
CONFIG_NOTE[S3A]="S3A curva step: identica a S3 ma 4 step"
CONFIG_NOTE[S3B]="S3B curva step: identica a S3 ma 8 step (LCM oltre la taratura 6: si misura se migliora o satura)"
CONFIG_NOTE[S2]="S2 asse velocita': DreamShaper 8 + Basepixel 0.6 + Hyper-SD15 8-step-CFG-LoRA 1.0 sopra (extra_lora), sampler normale (euler_a/karras), 8 step, CFG 5 (range creatore: CFG 5-8). Peso 1.0 e' un default di questo script"
CONFIG_NOTE[S3]="S3 asse velocita': AnyLoRA-LCM (checkpoint gia' fuso LCM) + 8bitdiffuser64 1.0, sampler lcm, 6 step, CFG 1.5 -- nessuna extra_lora: l'accelerazione e' gia' nel checkpoint, non va sommata"
CONFIG_NOTE[S4]="S4 asse velocita': DreamShaper8_LCM da solo (nessuna LoRA pixel-art), sampler lcm, 8 step, CFG 2 -- primo candidato runtime della ricerca (vedi nota su questo path in MODEL_META_JSON), qui usato solo come riferimento di velocita' pura senza stile pixel-art dedicato"

# -- Metadati modelli/LoRA per il ledger (item 4 del task) -------------------
# UNICA fonte di verita' per licenza/provenienza: ensure_ledger_entries()
# aggiunge sha256/data/file_status calcolati dal file reale, il resto viene
# da qui. "unknown" e' onesto quando la provenienza non e' verificabile in
# automatico (06-LICENZE-E-RISCHI.md del dossier: una pagina "download"/
# "commercial use" non dimostra da sola diritto di training/redistribuzione).
MODEL_META_JSON=$(cat <<'JSON'
{
  "models/sd15-vanilla-pruned-emaonly.safetensors": {
    "name": "SD1.5 vanilla (v1-5-pruned-emaonly)",
    "type": "checkpoint",
    "source_url": "https://huggingface.co/stable-diffusion-v1-5/stable-diffusion-v1-5",
    "license": "CreativeML OpenRAIL-M",
    "training_allowed": "verified",
    "commercial_use": "verified",
    "redistribution": "verified (Attachment A si propaga se si ridistribuiscono i pesi; il gioco non li ridistribuisce mai, vedi docs/ai-production/licenze.md)",
    "components": [],
    "license_snapshot_url": "https://huggingface.co/stable-diffusion-v1-5/stable-diffusion-v1-5/raw/main/README.md",
    "license_snapshot_file": "sd15-vanilla-README.md",
    "notes": "gia' verificato in docs/ai-production/licenze.md e docs/plans/completed/model-comparison.md"
  },
  "models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors": {
    "name": "Lykon/DreamShaper (DreamShaper_8_pruned)",
    "type": "checkpoint",
    "source_url": "https://huggingface.co/Lykon/DreamShaper",
    "license": "other (tag scheda HF); nessun file LICENSE nel repo; la scheda rimanda a civitai.com/models/4384/dreamshaper per i termini completi",
    "training_allowed": "unknown",
    "commercial_use": "unknown",
    "redistribution": "unknown",
    "components": [],
    "license_snapshot_url": "https://huggingface.co/Lykon/DreamShaper/raw/main/README.md",
    "license_snapshot_file": "dreamshaper8-README.md",
    "notes": "DIVERGENZA dal dossier di ricerca (06-LICENZE-E-RISCHI.md), che classifica DreamShaper 8 fra i 'piu' chiari' con OpenRAIL-M 'dichiarata sul repository ufficiale': la scheda HF ha invece license:'other' e nessun file LICENSE nel repo (verificato 2026-08-06, vedi snapshot). Registrata qui per onesta', il dossier esterno non viene corretto da questo script."
  },
  "models/teacher-bench-2026-08/basepixel-20.safetensors": {
    "name": "skyatmoon/MyPixelArtLora (basepixel-20)",
    "type": "lora",
    "source_url": "https://huggingface.co/skyatmoon/MyPixelArtLora",
    "license": "non dichiarata (nessun campo license nella scheda HF, nessun file LICENSE nel repo)",
    "training_allowed": "unknown",
    "commercial_use": "unknown",
    "redistribution": "unknown",
    "components": [],
    "license_snapshot_url": "https://huggingface.co/skyatmoon/MyPixelArtLora/raw/main/README.md",
    "license_snapshot_file": "basepixel-20-README.md",
    "notes": "elencata fra i modelli 'da auditare prima del training o della distribuzione' in 06-LICENZE-E-RISCHI.md del dossier; nessuna licenza esplicita trovata da questo script"
  },
  "models/teacher-bench-2026-08/tokforge-dreamshaper-7-lcm-q4_0.gguf": {
    "name": "darkmaniac7/TokForge-DreamShaper-LCM-GGUF-q4 (dreamshaper-7-lcm-q4_0)",
    "type": "checkpoint",
    "source_url": "https://huggingface.co/darkmaniac7/TokForge-DreamShaper-LCM-GGUF-q4",
    "license": "CreativeML OpenRAIL-M (dichiarata nel front-matter della scheda HF del repack)",
    "training_allowed": "unknown -- e comunque VIETATO allenare su pesi GGUF/Q4 (regola ferrea, regole-agenti-ml.md)",
    "commercial_use": "verified (dichiarata dal repack, eredita dai componenti sotto)",
    "redistribution": "unknown",
    "components": ["Lykon/dreamshaper-7", "latent-consistency/lcm-lora-sdv1-5"],
    "license_snapshot_url": "https://huggingface.co/darkmaniac7/TokForge-DreamShaper-LCM-GGUF-q4/raw/main/README.md",
    "license_snapshot_file": "tokforge-dreamshaper-7-lcm-q4-README.md",
    "notes": "fusione GGUF di terzi (DreamShaper-7 + LCM-LoRA fuso nell'UNet, 'merge opacity' per 06-LICENZE-E-RISCHI.md); solo T0/baseline di velocita', mai base di training"
  },
  "models/teacher-bench-2026-08/8bitdiffuser64-v4-PX64NOCAP_epoch_10.safetensors": {
    "name": "8bitdiffuser 64x | a perfect pixel art model (Civitai, LoRA A3)",
    "type": "lora",
    "source_url": "https://civitai.com/models/185743",
    "license": "permessi Civitai (non SPDX): allowCommercialUse include 'Image' (immagini generate vendibili), allowDerivatives=true, allowNoCredit=true, allowDifferentLicense=true -- verificato via API pubblica 2026-08-06, vedi snapshot JSON",
    "training_allowed": "unknown (i permessi Civitai coprono output/redistribuzione del modello, non l'uso esplicito come base per fine-tuning/QLoRA)",
    "commercial_use": "verified (Civitai: 'Image' in allowCommercialUse)",
    "redistribution": "verified (allowDifferentLicense=true, indicativo non legale)",
    "components": [],
    "license_snapshot_url": "https://civitai.com/api/v1/models/185743",
    "license_snapshot_file": "8bitdiffuser64-civitai-model-185743.json",
    "notes": "A3: peso LoRA 1.0 in CONFIG_ROW e' un placeholder, la matrice benchmark non specifica un peso per questo candidato"
  },
  "models/teacher-bench-2026-08/DreamShaper8_LCM.safetensors": {
    "name": "Lykon/DreamShaper (DreamShaper8_LCM)",
    "type": "checkpoint",
    "source_url": "https://huggingface.co/Lykon/DreamShaper",
    "license": "da verificare nello snapshot: il dossier 06-LICENZE-E-RISCHI.md dichiara CreativeML OpenRAIL-M per DreamShaper 8 LCM, ma la scheda HF del repo Lykon/DreamShaper ha license:'other' (stessa divergenza gia' registrata per DreamShaper_8_pruned)",
    "training_allowed": "unknown",
    "commercial_use": "unknown",
    "redistribution": "unknown",
    "components": ["Lykon/dreamshaper-8", "LCM (distillazione fusa nel checkpoint)"],
    "license_snapshot_url": "https://huggingface.co/Lykon/DreamShaper/raw/main/README.md",
    "license_snapshot_file": "dreamshaper8-README.md",
    "notes": "NESSUNA config Stage A lo usa: e' il candidato runtime di Stage C (matrice benchmark, C2/C3/C4). Provenienza dedotta dalla convenzione di nome del repo HF Lykon/DreamShaper, lo stesso di DreamShaper_8_pruned gia' in ledger -- il log del download non e' stato conservato, quindi la fonte va riconfermata prima di qualunque uso oltre il benchmark"
  },
  "models/teacher-bench-2026-08/dreamshaperPixelart_v10.safetensors": {
    "name": "Dreamshaper PixelArt (Civitai, checkpoint A4)",
    "type": "checkpoint",
    "source_url": "https://civitai.com/models/129879",
    "license": "permessi Civitai (non SPDX): allowCommercialUse include 'Image' (immagini generate vendibili), allowDerivatives=true, allowNoCredit=true, allowDifferentLicense=true -- verificato via API pubblica 2026-08-06, vedi snapshot JSON",
    "training_allowed": "unknown (i permessi Civitai coprono output/redistribuzione del modello, non l'uso esplicito come base per fine-tuning/QLoRA)",
    "commercial_use": "verified (Civitai: 'Image' in allowCommercialUse)",
    "redistribution": "verified (allowDifferentLicense=true, indicativo non legale)",
    "components": [],
    "license_snapshot_url": "https://civitai.com/api/v1/models/129879",
    "license_snapshot_file": "dreamshaper-pixelart-v10-civitai-model-129879.json",
    "notes": "A4: checkpoint sfidante, 'da auditare' anche per 06-LICENZE-E-RISCHI.md del dossier"
  }
}
JSON
)

# -- Parsing argomenti --------------------------------------------------------
if [ $# -gt 0 ]; then
  REQUESTED_CONFIGS=("$@")
else
  REQUESTED_CONFIGS=("${DEFAULT_CONFIGS[@]}")
fi

# -- Soggetti e seed di UN contratto -------------------------------------------
# Item 2 Track F: righe diverse di CONFIG_ROW possono portare contratti
# diversi (P per A0..A4/T0, trackF per F*/S*) nella STESSA invocazione dello
# script (es. "teacher-bench.sh A0 F1"): niente piu' un caricamento globale
# unico, ma una funzione richiamabile per ogni contratto incontrato. Assegna
# alle globali SUBJ_FIELDS/SEEDS/PROMPTS_VERSION sovrascrivendole -- sicuro
# perche' l'uso e' sempre sequenziale e immediato (mai concorrente, mai letto
# da una chiamata precedente dopo che la successiva e' partita). NUL-
# delimitato (mapfile -d ''): i prompt possono contenere qualunque carattere
# di testo (virgole, apici) senza rischiare collisioni di delimitatore, a
# differenza di un formato TSV/CSV fatto a mano.
load_prompts_contract() {
  local contract_path="$1"
  [ -f "$contract_path" ] || { echo "teacher-bench: contratto mancante: $contract_path" >&2; return 1; }

  mapfile -d '' SUBJ_FIELDS < <(python3 - "$contract_path" <<'PY'
import json, sys
data = json.load(open(sys.argv[1]))
for s in data.get("subjects", []):
    for key in ("id", "category", "prompt", "negative"):
        sys.stdout.write(str(s.get(key, "")))
        sys.stdout.write("\0")
PY
)
  if [ "${#SUBJ_FIELDS[@]}" -eq 0 ] || [ $(( ${#SUBJ_FIELDS[@]} % 4 )) -ne 0 ]; then
    echo "teacher-bench: $contract_path non contiene 'subjects' validi" >&2
    return 1
  fi

  mapfile -d '' SEED_FIELDS < <(python3 - "$contract_path" <<'PY'
import json, sys
data = json.load(open(sys.argv[1]))
seeds = data.get("seeds") or [4242, 90210]
for s in seeds:
    sys.stdout.write(str(s))
    sys.stdout.write("\0")
PY
)
  SEEDS=("${SEED_FIELDS[@]}")
  [ "${#SEEDS[@]}" -gt 0 ] || { echo "teacher-bench: nessun seed in $contract_path (JSON o default)" >&2; return 1; }

  # version E judge_scale nella stessa lettura. judge_scale serve QUI solo
  # alla riga di provenienza di generate_config(): chi lo consuma davvero e'
  # teacher_bench_post.py, che rilegge il contratto dal path scritto nel
  # manifest. Contratto che non lo dichiara (quello P) -> stringa vuota, e la
  # riga di log lo dice come "32 (default)", che e' il default di quello
  # script -- mai un numero inventato qui.
  IFS=$'\x1f' read -r PROMPTS_VERSION JUDGE_SCALE < <(python3 -c "
import json, sys
d = json.load(open(sys.argv[1]))
sys.stdout.write(str(d.get('version', '')) + '\x1f' + str(d.get('judge_scale', '')) + '\n')
" "$contract_path")
  return 0
}

# Caricamento di default (contratto P, $PROMPTS_FILE) come sola VALIDAZIONE
# d'ingresso: un $PROMPTS_FILE illeggibile ferma la corsa qui invece che a
# meta' della prima riga che lo usa. Non alimenta piu' il banner (vedi in
# fondo), che di questo contratto non dichiara piu' niente.
# generate_config() richiama load_prompts_contract() DI NUOVO per ogni riga
# col contratto proprio di quella riga (vedi CONFIG_ROW), sovrascrivendo
# queste stesse globali subito prima di usarle -- ridondante per le righe
# A0..A4/T0 (stesso file, stesso risultato) ma e' quello che rende Track F
# possibile senza duplicare la logica di parsing.
load_prompts_contract "$PROMPTS_FILE" || exit 1

# -- Ledger modelli + snapshot licenze (item 4) -------------------------------
# Idempotente: gira una volta per invocazione. Copre TRE insiemi di path, non
# solo quelli della corsa in corso:
#   1. modello/LoRA delle REQUESTED_CONFIGS (segnati come usati da Stage A);
#   2. ogni peso presente su disco in models/teacher-bench-2026-08/ anche se
#      nessuna config lo tocca (DreamShaper8_LCM, ControlNet, Hyper-SD...):
#      "mai usare asset senza ledger" (07-MANDATO) vale dal momento in cui il
#      file entra nella macchina, non da quando lo si genera con;
#   3. i path descritti in MODEL_META_JSON ma non ancora scaricati, che
#      restano nel ledger come pending-download.
# Un peso scoperto sul disco e SENZA metadati curati entra comunque, con
# licenza "unknown" e una nota esplicita: un buco dichiarato e' auditabile,
# un file assente dal ledger no.
ledger_field() {
  python3 -c "
import json, sys
try:
    d = json.load(open(sys.argv[1])).get('entries', {}).get(sys.argv[2]) or {}
except FileNotFoundError:
    d = {}
v = d.get(sys.argv[3])
print('' if v is None else v, end='')
" "$LEDGER_FILE" "$1" "$2"
}

ensure_ledger_entries() {
  mkdir -p "$MANIFEST_DIR" "$LICENSE_SNAPSHOT_DIR"

  declare -A seen=()
  local used_pairs=() paths=()
  local cfgid model lora_dir lora_name lora_weight steps cfg_scale sampler scheduler \
        default_trigger contract extra_lora subjects_filter lora_path
  local ex_dir ex_name ex_weight ex_path
  for cfgid in "${REQUESTED_CONFIGS[@]}"; do
    [ -n "${CONFIG_ROW[$cfgid]+x}" ] || continue   # config sconosciuta: segnalata dopo, non qui
    IFS=$'\x1f' read -r model lora_dir lora_name lora_weight steps cfg_scale sampler scheduler \
      default_trigger contract extra_lora subjects_filter <<< "${CONFIG_ROW[$cfgid]}"
    used_pairs+=("$model	$cfgid")
    if [ -z "${seen[$model]:-}" ]; then seen[$model]=1; paths+=("$model"); fi
    if [ -n "$lora_name" ]; then
      lora_path="$lora_dir/$lora_name.safetensors"
      used_pairs+=("$lora_path	$cfgid")
      if [ -z "${seen[$lora_path]:-}" ]; then seen[$lora_path]=1; paths+=("$lora_path"); fi
    fi
    # extra_lora (righe S*, asse velocita'): stesso obbligo di ledger della
    # style LoRA, "mai usare asset senza ledger" non fa eccezioni per la
    # SECONDA LoRA solo perche' e' opzionale.
    if [ -n "$extra_lora" ]; then
      IFS=':' read -r ex_dir ex_name ex_weight <<< "$extra_lora"
      if [ -n "$ex_dir" ] && [ -n "$ex_name" ]; then
        ex_path="$ex_dir/$ex_name.safetensors"
        used_pairs+=("$ex_path	$cfgid")
        if [ -z "${seen[$ex_path]:-}" ]; then seen[$ex_path]=1; paths+=("$ex_path"); fi
      fi
    fi
  done

  # (2) pesi presenti sul disco ma non richiesti da nessuna config di questa corsa
  local f
  if [ -d "$BENCH_MODEL_DIR" ]; then
    while IFS= read -r f; do
      [ -n "$f" ] || continue
      if [ -z "${seen[$f]:-}" ]; then seen[$f]=1; paths+=("$f"); fi
    done < <(find "$BENCH_MODEL_DIR" -maxdepth 1 -type f \( -name '*.safetensors' -o -name '*.gguf' -o -name '*.ckpt' \) | LC_ALL=C sort)
  fi

  # (3) path descritti nei metadati ma non ancora scaricati (pending-download)
  while IFS= read -r f; do
    [ -n "$f" ] || continue
    if [ -z "${seen[$f]:-}" ]; then seen[$f]=1; paths+=("$f"); fi
  done < <(python3 -c "
import json, sys
for k in json.loads(sys.argv[1]):
    print(k)
" "$MODEL_META_JSON")

  local p snap_url snap_file
  for p in "${paths[@]}"; do
    IFS=$'\x1f' read -r snap_url snap_file < <(python3 -c "
import json, sys
d = json.loads(sys.argv[1]).get(sys.argv[2]) or {}
sys.stdout.write((d.get('license_snapshot_url') or '') + '\x1f' + (d.get('license_snapshot_file') or ''))
" "$MODEL_META_JSON" "$p")
    if [ -n "$snap_url" ] && [ -n "$snap_file" ] && [ ! -f "$LICENSE_SNAPSHOT_DIR/$snap_file" ]; then
      echo "teacher-bench: scarico snapshot licenza -> $snap_file"
      curl -sL --max-time 20 -o "$LICENSE_SNAPSHOT_DIR/$snap_file" "$snap_url" \
        || echo "teacher-bench: WARN snapshot licenza fallito per $p ($snap_url)" >&2
    fi
  done

  MB_MODEL_META_JSON="$MODEL_META_JSON" \
  MB_PATHS="$(printf '%s\n' "${paths[@]}")" \
  MB_USED_PAIRS="$(printf '%s\n' ${used_pairs[@]+"${used_pairs[@]}"})" \
  MB_LEDGER_FILE="$LEDGER_FILE" \
  MB_SD_COMMIT="$SD_CPP_COMMIT" \
  python3 <<'PY'
import hashlib
import json
import os
from pathlib import Path
from datetime import datetime, timezone

meta = json.loads(os.environ["MB_MODEL_META_JSON"])
paths = [p for p in os.environ["MB_PATHS"].splitlines() if p]
ledger_path = Path(os.environ["MB_LEDGER_FILE"])

# "quale config Stage A usa questo peso" si ACCUMULA fra le corse: lanciare
# solo A3 domani non deve far risultare "non usato" il checkpoint di A1.
used_by = {}
for line in os.environ.get("MB_USED_PAIRS", "").splitlines():
    if "\t" not in line:
        continue
    path, cfgid = line.split("\t", 1)
    used_by.setdefault(path, set()).add(cfgid)

# nota di default per un peso trovato sul disco senza metadati curati: la
# voce esiste comunque (regola "mai usare asset senza ledger"), ma dichiara
# di non sapere nulla invece di inventare una provenienza dal nome del file.
# La nota cita il path REALE della voce e non una cartella fissa: da Track F
# il ledger copre anche pesi fuori da models/teacher-bench-2026-08/ (es.
# models/Public-Prompts-Pixel-Model.ckpt, models/lcm-lora-sdv1-5.safetensors),
# e un campo di ledger che dichiara la cartella sbagliata e' falso quanto una
# licenza inventata.
def uncurated_note(path):
    return (
        f"peso presente sul disco ({path}) ma senza metadati curati in "
        "scripts/teacher-bench.sh: provenienza, licenza e diritti NON verificati da questo "
        "script (il log del download non e' conservato). Solo benchmark interno finche' "
        "qualcuno non lo audita a mano -- vedi 06-LICENZE-E-RISCHI.md del dossier ricerca."
    )

if ledger_path.exists():
    ledger = json.loads(ledger_path.read_text())
else:
    ledger = {
        "version": 1,
        "note": ("Stage A teacher-bench 2026-08 -- ogni voce e' benchmark-only finche' "
                 "una decisione dedicata non promuove la base a training/runtime "
                 "(06-LICENZE-E-RISCHI.md e 07-MANDATO-CLAUDE-CODEX.md del dossier ricerca)."),
        "entries": {},
    }
ledger.setdefault("entries", {})


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


for p in paths:
    m = meta.get(p, {})
    curated = p in meta
    exists = os.path.isfile(p)
    entry = ledger["entries"].get(p, {})
    entry.update({
        "name": m.get("name", os.path.basename(p)),
        "type": m.get("type", "unknown"),
        "source_url": m.get("source_url", ""),
        "license": m.get("license", "unknown"),
        "training_allowed": m.get("training_allowed", "unknown"),
        "commercial_use": m.get("commercial_use", "unknown"),
        "redistribution": m.get("redistribution", "unknown"),
        "components": m.get("components", []),
        "decision": "benchmark-only",
        "notes": m.get("notes", "") if curated else uncurated_note(p),
        "license_snapshot_file": m.get("license_snapshot_file"),
        "used_by_configs": sorted(set(entry.get("used_by_configs") or []) | used_by.get(p, set())),
    })
    if exists:
        mtime = os.path.getmtime(p)
        # ricalcola sha256 solo se il file e' cambiato dall'ultima corsa (mtime
        # diverso) o se non era ancora presente: un checkpoint da 2 GB non va
        # rihashato ad ogni singola invocazione dello script.
        if entry.get("file_hash") and entry.get("_stat_mtime") == mtime:
            pass
        else:
            entry["file_hash"] = sha256_of(p)
            entry["_stat_mtime"] = mtime
            entry["download_date"] = datetime.fromtimestamp(mtime, tz=timezone.utc).strftime("%Y-%m-%d")
        entry["file_status"] = "present"
    else:
        entry.setdefault("file_hash", None)
        entry.setdefault("download_date", None)
        entry["file_status"] = "pending-download"
    ledger["entries"][p] = entry

ledger["updated_at"] = datetime.now(tz=timezone.utc).isoformat()
ledger["stable_diffusion_cpp_commit"] = os.environ.get("MB_SD_COMMIT", "")
ledger_path.parent.mkdir(parents=True, exist_ok=True)
ledger_path.write_text(json.dumps(ledger, indent=2, ensure_ascii=False, sort_keys=True) + "\n")
print(f"teacher-bench: ledger aggiornato ({len(paths)} voci controllate) -> {ledger_path}")
PY
}

# -- Generazione di una singola immagine --------------------------------------
run_one() {
  local cfgid="$1" sid="$2" category="$3" sprompt="$4" snegative="$5" seed="$6" \
        model="$7" model_sha="$8" model_license="$9"
  shift 9
  local lora_path="$1" lora_sha="$2" lora_license="$3" lora_name="$4" lora_weight="$5" prefix="$6" \
        steps="$7" cfg_scale="$8" sampler="$9"
  shift 9
  local scheduler="$1" contract_path="$2" \
        extra_lora_rel="$3" extra_lora_weight="$4" extra_lora_sha="$5" extra_lora_license="$6"

  local raw_dir="$RAW_DIR/$cfgid"
  local raw_path="$raw_dir/${sid}_${seed}.png"
  local manifest_path="$MANIFEST_DIR/${cfgid}_${sid}_${seed}.json"

  local full_prompt="${prefix}${sprompt}"
  local extra_args=()
  if [ -n "$lora_name" ]; then
    full_prompt="${full_prompt}<lora:${lora_name}:${lora_weight}>"
    extra_args+=(--lora-model-dir "$(dirname "$lora_path")")
  fi
  # extra_lora (righe S*): SECONDA LoRA sopra la style LoRA, per l'asse
  # velocita'. Path ASSOLUTO nel tag <lora:...> invece di un secondo
  # --lora-model-dir (sd-cli ne accetta uno solo, examples/common/common.cpp
  # SDGenerationParams::extract_and_remove_lora): quando raw_path e'
  # assoluto (is_absolute_path) sd-cli lo apre diretto, ignorando
  # lora-model-dir -- verificato leggendo quella funzione, non solo dedotto
  # dall'--help. $PWD e' la radice del repo per tutta la vita dello script
  # (il primo comando e' "cd $(dirname "$0")/.." e non c'e' nessun altro cd),
  # quindi "$PWD/$extra_lora_rel" e' sempre l'assoluto corretto.
  if [ -n "$extra_lora_rel" ]; then
    full_prompt="${full_prompt}<lora:${PWD}/${extra_lora_rel}:${extra_lora_weight}>"
    # extract_and_remove_lora di sd.cpp esce SUBITO se lora_model_dir e'
    # vuoto: senza --lora-model-dir nessun tag <lora:...> viene estratto,
    # nemmeno uno con path assoluto, e il tag resterebbe LETTERALE dentro il
    # prompt -- in silenzio, senza applicare nulla. Oggi --lora-model-dir
    # arriva solo dalla style LoRA sopra; nessuna riga attuale ha extra_lora
    # senza style LoRA (S1/S2 hanno entrambe basepixel), ma niente lo
    # impedirebbe a una riga futura.
    if [ -z "$lora_name" ]; then
      extra_args+=(--lora-model-dir "$(dirname "$extra_lora_rel")")
    fi
  fi

  if [ -f "$raw_path" ]; then
    # Il resume salta il file gia' presente, ma un PNG generato con un
    # contratto di prompt PRECEDENTE non e' confrontabile con i nuovi: se il
    # manifest dice altro da quello che si userebbe adesso, lo si dice ad
    # alta voce invece di far convivere in silenzio due esperimenti diversi
    # nella stessa cartella.
    local drift=""
    if [ -f "$manifest_path" ]; then
      drift=$(MB_MANIFEST="$manifest_path" MB_PROMPT="$full_prompt" MB_NEG="$snegative" \
              MB_STEPS="$steps" MB_CFG="$cfg_scale" MB_SAMPLER="$sampler" MB_SCHED="$scheduler" \
              python3 <<'PY'
import json, os
try:
    m = json.load(open(os.environ["MB_MANIFEST"]))
except Exception:
    raise SystemExit(0)
diffs = []
def cmp(label, old, new):
    if old is None:
        return
    if str(old) != str(new):
        diffs.append(label)
cmp("prompt", m.get("prompt_full"), os.environ["MB_PROMPT"])
cmp("negative", m.get("negative_prompt"), os.environ["MB_NEG"])
cmp("steps", m.get("steps"), int(os.environ["MB_STEPS"]))
cmp("cfg", m.get("cfg_scale"), float(os.environ["MB_CFG"]))
cmp("sampler", m.get("sampling_method"), os.environ["MB_SAMPLER"])
cmp("scheduler", m.get("scheduler"), os.environ["MB_SCHED"])
print(", ".join(diffs), end="")
PY
)
    fi
    if [ -n "$drift" ]; then
      echo "teacher-bench:   WARN [$cfgid] $sid seed=$seed presente ma generata con ALTRO ($drift): cancella $raw_path per rigenerarla" >&2
    fi
    echo "teacher-bench:   [$cfgid] $sid seed=$seed gia' presente, salto (resume)"
    return 0
  fi
  mkdir -p "$raw_dir"

  if [ "$DRY_RUN" = "1" ]; then
    printf 'DRY-RUN [%s] %s seed=%s :: %s -m %s -p %q -n %q -W %s -H %s --steps %s --cfg-scale %s --sampling-method %s --scheduler %s --seed %s %s -o %s\n' \
      "$cfgid" "$sid" "$seed" "$SD_CLI" "$model" "$full_prompt" "$snegative" \
      "$WIDTH" "$HEIGHT" "$steps" "$cfg_scale" "$sampler" "$scheduler" "$seed" "${extra_args[*]:-}" "$raw_path"
    return 0
  fi

  local tmp_out="${raw_path}.tmp.png"
  local sdlog="$LOG_DIR/${cfgid}_${sid}_${seed}.log"
  local timelog="$LOG_DIR/${cfgid}_${sid}_${seed}.time.txt"

  echo "teacher-bench:   [$cfgid] $sid seed=$seed ..."
  local t0 t1 latency_ms rc
  # %s%N (secondi+nanosecondi, sempre 9 cifre fisse su GNU date E su uutils
  # coreutils) invece di %s%3N: il troncamento GNU a 3 cifre (%3N) NON e'
  # onorato da uutils coreutils (restituisce i nanosecondi INTERI ignorando
  # la larghezza) -- verificato in sessione, differenza riproducibile fra le
  # due implementazioni. Dividendo sempre per 1e6 alla fine il calcolo resta
  # corretto su entrambe.
  t0=$(date +%s%N)
  if [ -n "$TIME_BIN_AVAILABLE" ]; then
    /usr/bin/time -v -o "$timelog" "$SD_CLI" -m "$model" -p "$full_prompt" -n "$snegative" \
      -W "$WIDTH" -H "$HEIGHT" --steps "$steps" --cfg-scale "$cfg_scale" \
      --sampling-method "$sampler" --scheduler "$scheduler" --seed "$seed" \
      "${extra_args[@]}" -o "$tmp_out" -v > "$sdlog" 2>&1
    rc=$?
  else
    "$SD_CLI" -m "$model" -p "$full_prompt" -n "$snegative" \
      -W "$WIDTH" -H "$HEIGHT" --steps "$steps" --cfg-scale "$cfg_scale" \
      --sampling-method "$sampler" --scheduler "$scheduler" --seed "$seed" \
      "${extra_args[@]}" -o "$tmp_out" -v > "$sdlog" 2>&1
    rc=$?
  fi
  t1=$(date +%s%N)
  latency_ms=$(( (t1 - t0) / 1000000 ))

  local gen_ok=0 raw_rel=""
  if [ "$rc" -eq 0 ] && [ -s "$tmp_out" ]; then
    mv "$tmp_out" "$raw_path"
    gen_ok=1
    raw_rel="raw-512/${cfgid}/${sid}_${seed}.png"
    echo "teacher-bench:     ok (${latency_ms} ms)"
  else
    rm -f "$tmp_out"
    echo "teacher-bench:     FALLITO (rc=$rc, ${latency_ms} ms) -- vedi $sdlog" >&2
    mkdir -p "$FAILURES_DIR"
    {
      echo "config=$cfgid subject=$sid seed=$seed rc=$rc latency_ms=$latency_ms"
      echo "--- ultime righe log sd-cli ---"
      tail -n 20 "$sdlog" 2>/dev/null
    } > "$FAILURES_DIR/${cfgid}_${sid}_${seed}.txt"
  fi

  local rss_kb="" vram_note=""
  if [ -f "$timelog" ]; then
    rss_kb=$(grep -oE 'Maximum resident set size \(kbytes\): [0-9]+' "$timelog" 2>/dev/null | grep -oE '[0-9]+$' || true)
  fi
  # euristica best-effort: sd-cli -v logga a volte dimensioni di buffer
  # compute/param in MB/GB (vedi docs/ai-production/experiments/image-comparison-gen2-2026-07-23.md,
  # es. "mmdit compute 660 MB") -- non garantito su ogni build/modello, per
  # questo resta un "note" testuale e non un numero strutturato.
  vram_note=$(grep -oiE '[a-z_]*(vram|compute buffer|params? buffer)[a-z_ :]*[0-9]+(\.[0-9]+)? ?(MB|GB)' "$sdlog" 2>/dev/null | head -3 | tr '\n' ';' || true)

  MB_CONFIG="$cfgid" MB_SUBJECT="$sid" MB_CATEGORY="$category" MB_SEED_VAL="$seed" \
  MB_MODEL_PATH="$model" MB_MODEL_SHA="$model_sha" MB_LICENSE_MODEL="$model_license" \
  MB_LORA_PATH="$lora_path" MB_LORA_SHA="$lora_sha" MB_LORA_WEIGHT="$lora_weight" \
  MB_LORA_TRIGGER="$prefix" MB_LICENSE_LORA="$lora_license" \
  MB_EXTRA_LORA_PATH="$extra_lora_rel" MB_EXTRA_LORA_SHA="$extra_lora_sha" \
  MB_EXTRA_LORA_WEIGHT="$extra_lora_weight" MB_LICENSE_EXTRA_LORA="$extra_lora_license" \
  MB_PROMPT_FULL="$full_prompt" MB_NEGATIVE="$snegative" \
  MB_STEPS="$steps" MB_CFG_SCALE="$cfg_scale" MB_SAMPLER="$sampler" MB_SCHEDULER="$scheduler" \
  MB_WIDTH="$WIDTH" MB_HEIGHT="$HEIGHT" MB_SD_COMMIT="$SD_CPP_COMMIT" \
  MB_PROMPTS_VERSION="$PROMPTS_VERSION" MB_CONTRACT_PATH="$contract_path" \
  MB_LATENCY_MS="$latency_ms" MB_RSS_KB="$rss_kb" MB_VRAM_NOTE="$vram_note" \
  MB_GEN_OK="$gen_ok" MB_RAW_REL="$raw_rel" MB_MANIFEST_PATH="$manifest_path" \
  python3 <<'PY'
import json
import os
from datetime import datetime, timezone


def envf(key, cast=None):
    v = os.environ.get(key, "")
    if v == "":
        return None
    return cast(v) if cast else v


record = {
    "config_id": os.environ["MB_CONFIG"],
    "subject_id": os.environ["MB_SUBJECT"],
    "category": os.environ["MB_CATEGORY"],
    "seed": int(os.environ["MB_SEED_VAL"]),
    "model": {
        "path": os.environ["MB_MODEL_PATH"],
        "sha256": envf("MB_MODEL_SHA"),
        "license": os.environ.get("MB_LICENSE_MODEL", ""),
    },
    "lora": None,
    # Stage A e' text-only per decisione (vedi testata dello script): il campo
    # e' esplicito nel manifest cosi' chi legge un artefatto fra sei mesi non
    # deve dedurre da un'assenza se la silhouette guidata fosse in gioco.
    "generation_mode": "text-only",
    "prompts_contract_version": os.environ.get("MB_PROMPTS_VERSION") or None,
    # item 3 Track F: quale contratto ha generato QUESTA immagine (P o
    # trackF) -- teacher_bench_post.py legge "judge_scale" da questo path per
    # scrivere la colonna omonima in metrics.csv, e teacher_bench_review.py
    # lo usa per raggruppare le card per track.
    "prompts_contract_path": os.environ.get("MB_CONTRACT_PATH") or None,
    "prompt_full": os.environ["MB_PROMPT_FULL"],
    "negative_prompt": os.environ.get("MB_NEGATIVE", ""),
    "steps": int(os.environ["MB_STEPS"]),
    "cfg_scale": float(os.environ["MB_CFG_SCALE"]),
    "sampling_method": os.environ["MB_SAMPLER"],
    "scheduler": os.environ["MB_SCHEDULER"],
    "width": int(os.environ["MB_WIDTH"]),
    "height": int(os.environ["MB_HEIGHT"]),
    "sd_cpp_commit": os.environ.get("MB_SD_COMMIT", ""),
    "latency_ms": envf("MB_LATENCY_MS", int),
    "rss_kb": envf("MB_RSS_KB", int),
    "vram_note": envf("MB_VRAM_NOTE"),
    "generation_ok": os.environ["MB_GEN_OK"] == "1",
    "generated_at": datetime.now(tz=timezone.utc).isoformat(),
    "raw_image": os.environ.get("MB_RAW_REL") or None,
    "postproc": None,   # riempito da scripts/teacher_bench_post.py in un secondo momento
}
lora_path = os.environ.get("MB_LORA_PATH", "")
if lora_path:
    record["lora"] = {
        "path": lora_path,
        "sha256": envf("MB_LORA_SHA"),
        "weight": float(os.environ.get("MB_LORA_WEIGHT", "0") or 0),
        "trigger": os.environ.get("MB_LORA_TRIGGER") or None,
        "license": os.environ.get("MB_LICENSE_LORA", ""),
    }

# extra_lora (righe S*, asse velocita'): SECONDA LoRA sopra la style LoRA,
# stessa forma di "lora" cosi' chi legge il manifest non deve indovinare uno
# schema diverso per capire cosa e' stato sommato.
record["extra_lora"] = None
extra_lora_path = os.environ.get("MB_EXTRA_LORA_PATH", "")
if extra_lora_path:
    record["extra_lora"] = {
        "path": extra_lora_path,
        "sha256": envf("MB_EXTRA_LORA_SHA"),
        "weight": float(os.environ.get("MB_EXTRA_LORA_WEIGHT", "0") or 0),
        "license": os.environ.get("MB_LICENSE_EXTRA_LORA", ""),
    }

out_path = os.environ["MB_MANIFEST_PATH"]
os.makedirs(os.path.dirname(out_path), exist_ok=True)
with open(out_path, "w") as f:
    json.dump(record, f, indent=2, ensure_ascii=False)
    f.write("\n")
PY
}

# -- Generazione di una config intera (8 soggetti x N seed, meno un eventuale
# subjects_filter) --------------------------------------------------------
generate_config() {
  local cfgid="$1"
  if [ -z "${CONFIG_ROW[$cfgid]+x}" ]; then
    echo "teacher-bench: config sconosciuta: $cfgid (note: --list per l'elenco)" >&2
    return 1
  fi
  local model lora_dir lora_name lora_weight steps cfg_scale sampler scheduler default_trigger \
        contract extra_lora subjects_filter
  IFS=$'\x1f' read -r model lora_dir lora_name lora_weight steps cfg_scale sampler scheduler \
    default_trigger contract extra_lora subjects_filter <<< "${CONFIG_ROW[$cfgid]}"
  echo "== teacher-bench: $cfgid -- ${CONFIG_NOTE[$cfgid]:-} =="

  # contratto DI QUESTA RIGA (item 2 Track F): vuoto = $PROMPTS_FILE (il
  # contratto P) -- le righe A0..A4/T0 storiche non valorizzano "contract" e
  # finiscono sempre qui, stesso file di sempre. Ricarica SUBJ_FIELDS/SEEDS/
  # PROMPTS_VERSION per QUESTO contratto: ridondante ma innocuo per le righe P
  # (stesso file gia' caricato in cima allo script), necessario per le righe
  # F*/S* che leggono un contratto diverso.
  local contract_path="${contract:-$PROMPTS_FILE}"
  if ! load_prompts_contract "$contract_path"; then
    echo "teacher-bench: [$cfgid] salto l'intera config (contratto non caricabile)" >&2
    mkdir -p "$FAILURES_DIR"
    printf 'config=%s reason=contract_missing path=%s\n' "$cfgid" "$contract_path" > "$FAILURES_DIR/${cfgid}-contract-missing.txt"
    return 0
  fi

  if [ ! -f "$model" ]; then
    echo "teacher-bench: [$cfgid] modello mancante ($model) -- salto l'intera config" >&2
    mkdir -p "$FAILURES_DIR"
    printf 'config=%s reason=model_missing path=%s\n' "$cfgid" "$model" > "$FAILURES_DIR/${cfgid}-model-missing.txt"
    return 0
  fi

  local lora_path=""
  if [ -n "$lora_name" ]; then
    lora_path="$lora_dir/$lora_name.safetensors"
    if [ ! -f "$lora_path" ]; then
      echo "teacher-bench: [$cfgid] LoRA mancante ($lora_path) -- salto l'intera config" >&2
      mkdir -p "$FAILURES_DIR"
      printf 'config=%s reason=lora_missing path=%s\n' "$cfgid" "$lora_path" > "$FAILURES_DIR/${cfgid}-lora-missing.txt"
      return 0
    fi
  fi

  # extra_lora (righe S*, asse velocita'): stesso controllo di esistenza
  # della style LoRA. Formato "dir:nome:peso" (":" invece di "\x1f": e' un
  # SOTTO-campo dentro un solo campo \x1f di CONFIG_ROW, vedi commento sopra
  # la tabella). Peso mancante nel campo -> 1.0 di default (nessuna riga
  # attuale lo lascia vuoto, ma un default esplicito e' meglio di un errore
  # muto se una futura riga lo fa).
  local extra_lora_rel="" extra_lora_weight="" extra_lora_sha="" extra_lora_license=""
  if [ -n "$extra_lora" ]; then
    local ex_dir ex_name ex_w
    IFS=':' read -r ex_dir ex_name ex_w <<< "$extra_lora"
    if [ -z "$ex_dir" ] || [ -z "$ex_name" ]; then
      echo "teacher-bench: [$cfgid] extra_lora malformato ('$extra_lora', attesa 'dir:nome:peso') -- salto l'intera config" >&2
      mkdir -p "$FAILURES_DIR"
      printf 'config=%s reason=extra_lora_malformed value=%s\n' "$cfgid" "$extra_lora" > "$FAILURES_DIR/${cfgid}-extra-lora-malformed.txt"
      return 0
    fi
    extra_lora_rel="$ex_dir/$ex_name.safetensors"
    if [ ! -f "$extra_lora_rel" ]; then
      echo "teacher-bench: [$cfgid] extra LoRA mancante ($extra_lora_rel) -- salto l'intera config" >&2
      mkdir -p "$FAILURES_DIR"
      printf 'config=%s reason=extra_lora_missing path=%s\n' "$cfgid" "$extra_lora_rel" > "$FAILURES_DIR/${cfgid}-extra-lora-missing.txt"
      return 0
    fi
    extra_lora_weight="${ex_w:-1.0}"
    extra_lora_sha=$(ledger_field "$extra_lora_rel" file_hash)
    extra_lora_license=$(ledger_field "$extra_lora_rel" license)
  fi

  # prefisso di prompt: il config_prompt_prefix del contratto DI QUESTA RIGA
  # vince sempre quando presente; altrimenti il default_trigger di CONFIG_ROW.
  local prefix
  prefix=$(python3 -c "
import json, sys
d = json.load(open(sys.argv[1]))
p = (d.get('config_prompt_prefix') or {}).get(sys.argv[2])
print(p if p is not None else sys.argv[3], end='')
" "$contract_path" "$cfgid" "$default_trigger")

  local model_sha model_license lora_sha lora_license
  model_sha=$(ledger_field "$model" file_hash)
  model_license=$(ledger_field "$model" license)
  lora_sha="" lora_license=""
  if [ -n "$lora_path" ]; then
    lora_sha=$(ledger_field "$lora_path" file_hash)
    lora_license=$(ledger_field "$lora_path" license)
  fi

  # subjects_filter (opzionale, righe F9...): limita il giro ai soli soggetti
  # delle categorie elencate (LoRA addestrate su un sottoinsieme, es. RPG
  # Icons su weapon/item -- generare anche i soggetti character/enemy/boss
  # sarebbe fuori scopo per quella LoRA). Assente = tutti i soggetti, come
  # prima di questo campo.
  local -A filter_set=()
  if [ -n "$subjects_filter" ]; then
    local filter_arr=() cat
    IFS=',' read -ra filter_arr <<< "$subjects_filter"
    for cat in "${filter_arr[@]}"; do filter_set["$cat"]=1; done
  fi

  # Provenienza dichiarata PER RIGA, non nel banner d'avvio: ogni riga puo'
  # leggere un contratto diverso (P o trackF) con versione, judge_scale e
  # numero di soggetti propri. Un'unica dichiarazione in testa alla corsa
  # varrebbe solo per il contratto di default e attribuirebbe alle righe F*/S*
  # un contratto che non le ha generate -- in un benchmark la provenienza
  # sbagliata vale meno di nessuna provenienza. Il conteggio e' quello EFFETTIVO
  # (dopo subjects_filter), non gli 8 soggetti del contratto.
  local n_eff=0 j
  for ((j = 1; j < ${#SUBJ_FIELDS[@]}; j += 4)); do
    if [ -n "$subjects_filter" ] && [ -z "${filter_set[${SUBJ_FIELDS[j]}]:-}" ]; then continue; fi
    n_eff=$((n_eff + 1))
  done
  local judge_note="${JUDGE_SCALE:-32 (default)}"
  echo "teacher-bench:   contratto $contract_path (v${PROMPTS_VERSION:-?}, judge_scale $judge_note): $n_eff soggetti x ${#SEEDS[@]} seed${subjects_filter:+, subjects_filter=$subjects_filter}"

  local i sid category sprompt snegative seed
  for ((i = 0; i < ${#SUBJ_FIELDS[@]}; i += 4)); do
    sid="${SUBJ_FIELDS[i]}"; category="${SUBJ_FIELDS[i+1]}"; sprompt="${SUBJ_FIELDS[i+2]}"; snegative="${SUBJ_FIELDS[i+3]}"
    if [ -n "$subjects_filter" ] && [ -z "${filter_set[$category]:-}" ]; then
      continue
    fi
    for seed in "${SEEDS[@]}"; do
      run_one "$cfgid" "$sid" "$category" "$sprompt" "$snegative" "$seed" \
        "$model" "$model_sha" "$model_license" \
        "$lora_path" "$lora_sha" "$lora_license" "$lora_name" "$lora_weight" "$prefix" \
        "$steps" "$cfg_scale" "$sampler" "$scheduler" "$contract_path" \
        "$extra_lora_rel" "$extra_lora_weight" "$extra_lora_sha" "$extra_lora_license"
    done
  done
}

# Il banner NON dichiara piu' soggetti/seed/versione del contratto di default:
# da quando ogni riga porta il proprio contratto (Track F), quei numeri
# descrivono solo $PROMPTS_FILE e stamparli in testa a una corsa di sole righe
# F*/S* significherebbe attribuire all'esperimento un contratto che non ha
# generato nulla. La dichiarazione vera e' per riga, in generate_config().
echo "== teacher-bench: config richieste: ${REQUESTED_CONFIGS[*]} (text-only; contratto, soggetti e seed dichiarati per riga qui sotto) =="
ensure_ledger_entries
for cfgid in "${REQUESTED_CONFIGS[@]}"; do
  generate_config "$cfgid"
done
echo "== teacher-bench: fatto. raw in $RAW_DIR, manifest in $MANIFEST_DIR, fallimenti (se presenti) in $FAILURES_DIR =="
