#!/usr/bin/env python3
"""gen_equip_overlays — overlay sprite per i 6 slot visivi degli oggetti
equipaggiati (DEC-049), WP-ASSET-1.

Perche' questo script e non aseprite-mcp: 15 sprite piccoli (2-3 varianti per
slot) sono piu' veloci, deterministici e revisionabili come coordinate intere
in un file di testo che come una sequenza di tool call interattivi -- stessa
scelta gia' fatta da scripts/charrig.py per il rig del personaggio (DEC-205
in poi). Le due pipeline restano SEPARATE (charrig assembla un personaggio
intero da parti riusabili su una griglia 32x32 multi-cella; qui ogni sprite
e' un singolo fotogramma statico e autonomo) ma condividono lo stesso
principio: forme scritte come segmenti/pixel su coordinate intere, mai
disegnate a mano pixel-per-pixel in un editor.

REGOLA DI DESIGN (mandato WP-ASSET-1, non DEC-199 in senso stretto ma la
stessa cautela): ogni overlay resta NEUTRO (famiglie slag-* e cenere-* della
palette Fucina, mai le famiglie sature brace/verderame/ardesia/prugna) --
l'oggetto non ricolora mai lo sprite intero (il tinting "sporca", vedi
DEC-199 sul personaggio generato, stesso principio applicato qui). Il colore
VERO dell'oggetto (Item.color) si applica SOLO all'"accento" -- un blocco di
pixel nativi dedicato per sprite (1x1 per cinque slot su sei, 5x5 per
'aura', vedi la chiave 'accent_size' e la sezione aura sotto), alla
posizione 'accent' -- disegnato dal motore (DrawItemLayer, src/render/
item_layers.c) SOPRA lo sprite neutro, mai qui: questo script non conosce
mai un colore-oggetto, solo la base.

Ogni slot condivide (frame_w, frame_h, anchor, accent) fra le sue varianti:
item_layers.c usa UNA sola formula di offset per slot, non una per variante,
quindi le varianti DEVONO combaciare sulla stessa griglia -- ANCHOR e' il
punto che il motore appoggia sull'aggancio del personaggio (PlayerAnchors),
ACCENT e' l'angolo alto-sinistra NATIVO del blocco dove finira' il colore
dell'oggetto (giro 2, verdetto D1: DrawItemLayer/DrawEquipAccent non centra
piu' l'accento sull'ancora, lo piazza dove ArtDrawFrame piazzerebbe quel
preciso texel -- stesso sistema di coordinate "angolo alto-sinistra" dello
sprite). Per i cinque slot con accento 1x1 l'angolo alto-sinistra COINCIDE
col pixel stesso (nessuna differenza pratica); per 'aura' (blocco 5x5, vedi
la sezione aura) l'angolo e' quello del quadrato. In ogni caso l'accento deve
cadere DENTRO la sagoma gia' dipinta (mai un texel fuori sagoma) tranne
'aura', il cui centro resta cavo apposta -- vedi il commento li'.

Uso: python3 scripts/gen_equip_overlays.py
Scrive assets/art/equip/<slot>_<n>.png e .json (n da 1). Esce !=0 se un
qualunque controllo fallisce (palette fuori famiglia neutra, sprite vuoto,
contrasto insufficiente sul floor test chiaro/scuro -- DEC-205).
"""
import json
import math
import os
import sys

from PIL import Image

# Sottoinsieme NEUTRO della palette Fucina (assets/art-src/palette/
# worldsmelt-fucina.gpl): solo le famiglie "Bronzo/metallo" scure (slag-*,
# terra-bruciata, bronzo-scuro -- i toni piu' cupi e meno saturi della
# rampa) e "Cenere/neutri" per intero. Le lettere ripetono quelle di
# charrig.py dove le due palette si sovrappongono (N/S/C/T/b/n/s/e), estese
# con 'h'/'w' per i due toni cenere chiari che charrig non usa.
PAL = {
    'N': (20, 16, 14),      # slag-nero
    'S': (36, 26, 22),      # slag-scuro
    'C': (58, 38, 32),      # slag-caldo
    'T': (85, 53, 42),      # terra-bruciata
    'b': (122, 74, 43),     # bronzo-scuro
    'n': (43, 43, 49),      # cenere-nera
    's': (74, 74, 85),      # cenere-scura
    'e': (115, 115, 130),   # cenere
    'h': (167, 167, 181),   # cenere-chiara
    'w': (216, 216, 224),   # fumo
}

OUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'assets', 'art', 'equip')


def make_image(w, h, segments, pixels=()):
    """segments: lista di (y, x0, x1_esclusivo, lettera) -- una banda
    orizzontale per riga, cosi' come le stese di colore reali (masse
    raggruppate, DEC-205, mai dithering pixel-per-pixel). 'pixels': override
    puntuali (x, y, lettera) applicati DOPO i segmenti, per gli highlight/
    ombre d'angolo troppo piccoli per essere una banda intera."""
    img = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    for y, x0, x1, letter in segments:
        for x in range(x0, x1):
            img.putpixel((x, y), PAL[letter] + (255,))
    for x, y, letter in pixels:
        img.putpixel((x, y), PAL[letter] + (255,))
    return img


def build_ring(w, h, cx, cy, r_in, r_out, tone_fn, notch_angles=()):
    """Anello PIENO (giro 2, verdetto D6: "piu' massa", non piu' 2-8 punti
    isolati come nel giro 1): ogni pixel il cui centro cade nella corona
    [r_in, r_out] da (cx,cy) viene dipinto, mai lasciato vuoto a caso --
    resta comunque una MASSA raggruppata (DEC-205, mai dithering), non un
    riempimento uniforme senza piano di luce: 'tone_fn(dx,dy)' sceglie la
    lettera di palette dal vettore centro->pixel, cosi' la variante puo'
    marcare un piano di luce (alto-sinistra piu' chiaro) restando un'unica
    banda continua. 'notch_angles' apre piccole incisioni (variante 'rune')
    senza smontare l'anello: la porzione tolta e' minima, l'anello resta
    riconoscibile come tale."""
    img = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    for y in range(h):
        for x in range(w):
            dx, dy = x + 0.5 - cx, y + 0.5 - cy
            d = math.hypot(dx, dy)
            if not (r_in <= d <= r_out):
                continue
            skip = False
            if notch_angles:
                angle = math.atan2(dy, dx)
                for na in notch_angles:
                    diff = (angle - na + math.pi) % (2*math.pi) - math.pi
                    if abs(diff) < 0.22:
                        skip = True
                        break
            if skip:
                continue
            img.putpixel((x, y), PAL[tone_fn(dx, dy)] + (255,))
    return img


# ------------------------------------------------------------------
# Definizioni per slot: (frame_w, frame_h, anchor, accent, varianti[])
# 'accent' e' il pixel NATIVO (prima della scala di disegno) dove
# DrawItemLayer sovrappone Item.color. Ogni variante e' (nome, segments,
# pixels).
# ------------------------------------------------------------------

SLOTS = {}

SLOTS['hat'] = dict(
    # Ancora (7,7), non (7,11): giro 2, nota del verdetto -- con l'ancora
    # sul bordo bassissimo del canvas (riga 11, l'ultima) l'orlo della falda
    # finiva a filo dell'ancora invece che 8 unita' SOTTO come nella vecchia
    # forma geometrica (DrawRectangleRounded, brim da y a y+8): la pila
    # sedeva ~13px troppo in alto sulla testa della fonditrice. Spostando
    # l'ancora 4 righe piu' in su (riga7) e con EQUIP_HAT_SCALE=2.0, l'orlo
    # (riga 11) occupa [pos.y+8, pos.y+10): 2px sotto la vecchia falda, scarto
    # irrilevante a occhio -- vedi il conto su EQUIP_HAT_SCALE in item_layers.c.
    w=14, h=12, anchor=(7, 7),
    # Accento in riga 10 (l'orlo, non la cima riga2): giro 2, verdetto D3.
    # Con l'ancora sopra e il passo di impilamento a 8 unita' contro
    # un'altezza sprite di 24 (12 righe * scala 2), ogni cappello copre quasi
    # per intero quello sotto TRANNE le ultime tre righe (9-11, l'orlo): e'
    # l'unica banda che resta visibile su ENTRAMBI i cappelli di una coppia
    # consecutiva, quindi e' l'unico punto dove un accento sopravvive per
    # OGNI cappello della pila, non solo per quello in cima.
    accent=(7, 10),
    variants=[
        ('1', [  # berretto, grigio cenere freddo
            (1, 6, 8, 'h'), (2, 5, 9, 'h'), (3, 4, 10, 'h'), (4, 4, 10, 'h'),
            (5, 4, 11, 'e'), (6, 4, 11, 'e'), (7, 3, 11, 'e'), (8, 3, 11, 'e'),
            (9, 3, 12, 's'), (10, 2, 13, 'h'), (11, 2, 13, 'S'),
        ], []),
        ('2', [  # tricorno, cuoio caldo, falda spostata (asimmetria voluta)
            (1, 7, 9, 'b'), (2, 6, 10, 'b'), (3, 6, 10, 'b'),
            (4, 5, 11, 'T'), (5, 5, 11, 'T'), (6, 4, 11, 'T'), (7, 4, 12, 'T'),
            (8, 3, 12, 'C'), (9, 3, 13, 'C'), (10, 1, 13, 'T'), (11, 2, 14, 'N'),
        ], []),
        ('3', [  # falda larga, pallido, tesa asimmetrica (un lato si abbassa)
            (2, 6, 8, 'w'), (3, 5, 9, 'w'), (4, 5, 9, 'w'),
            (5, 4, 10, 'h'), (6, 4, 10, 'h'), (7, 3, 10, 'h'),
            (8, 2, 12, 'e'), (9, 0, 14, 'e'),
            (10, 0, 9, 'h'), (10, 9, 14, 's'), (11, 9, 14, 's'),
        ], []),
    ],
)

SLOTS['eyes'] = dict(
    # Ancora (6,2), non (6,3): le lenti occupano le righe 1-3 (3 righe), il
    # centro verticale vero e' la riga 2, non la 3 (il loro bordo BASSO) --
    # con l'ancora sulla riga2 lo sprite si impila centrato sul punto dove
    # stavano i vecchi cerchi geometrici (DrawCircleV centrato su anchors.
    # eyes.y), non 2 unita' piu' in basso.
    w=12, h=6, anchor=(6, 2), accent=(9, 3),
    variants=[
        ('1', [  # due lenti tonde + ponte
            (1, 1, 4, 'e'), (2, 1, 4, 'e'), (3, 1, 4, 's'),
            (1, 8, 11, 'e'), (2, 8, 11, 'e'), (3, 8, 11, 's'),
            (2, 4, 8, 'S'),
        ], [(1, 1, 'h'), (3, 3, 'N'), (8, 1, 'h'), (10, 3, 'N')]),
        ('2', [  # monocolo + catenella tratteggiata
            (1, 7, 10, 'e'), (2, 7, 10, 'e'), (3, 7, 10, 's'),
        ], [(7, 1, 'h'), (9, 3, 'N'), (0, 2, 'S'), (2, 2, 'S'), (4, 2, 'S'), (6, 2, 'S')]),
        ('3', [  # visiera angolare
            (1, 1, 11, 'h'), (2, 0, 12, 'S'), (3, 1, 11, 's'),
        ], []),
    ],
)

SLOTS['hand'] = dict(
    w=10, h=14, anchor=(5, 13), accent=(5, 2),
    variants=[
        ('1', [  # gemma su asta
            (1, 4, 6, 'h'), (2, 3, 7, 'h'), (3, 2, 8, 'e'), (4, 3, 7, 's'),
            (5, 4, 6, 's'), (6, 4, 6, 's'), (7, 4, 6, 's'),
            (8, 4, 6, 'S'), (9, 4, 6, 'S'), (10, 4, 6, 'S'),
            (11, 4, 6, 'N'), (12, 4, 6, 'N'), (13, 4, 6, 'N'),
        ], []),
        ('2', [  # uncino
            (1, 5, 7, 'h'), (2, 5, 8, 'e'), (3, 6, 8, 'e'), (4, 5, 7, 's'),
            (5, 4, 6, 's'), (6, 4, 6, 's'),
            (7, 4, 6, 'S'), (8, 4, 6, 'S'), (9, 4, 6, 'S'),
            (10, 4, 6, 'N'), (11, 4, 6, 'N'), (12, 4, 6, 'N'), (13, 4, 6, 'N'),
        ], []),
        ('3', [  # vessillo/pennone
            (1, 5, 6, 'h'), (2, 5, 7, 'h'), (3, 5, 8, 'e'), (4, 5, 7, 's'), (5, 5, 6, 's'),
            (6, 4, 6, 's'), (7, 4, 6, 's'),
            (8, 4, 6, 'S'), (9, 4, 6, 'S'), (10, 4, 6, 'S'),
            (11, 4, 6, 'N'), (12, 4, 6, 'N'), (13, 4, 6, 'N'),
        ], []),
    ],
)

# Il mantello condivide la STESSA sagoma (taper) fra le due varianti -- solo
# palette e "craft pass" (strappi, bisaccia) cambiano, cosi' l'accento resta
# valido per entrambe senza una seconda formula in item_layers.c.
_BACK_TAPER = [
    (2, 6, 10), (3, 6, 10), (4, 5, 11), (5, 5, 11), (6, 4, 11), (7, 4, 12),
    (8, 3, 12), (9, 3, 12), (10, 3, 13), (11, 2, 13), (12, 2, 13), (13, 2, 14),
    (14, 1, 14), (15, 1, 14), (16, 1, 15), (17, 1, 15), (18, 0, 15),
]

SLOTS['back'] = dict(
    w=16, h=20, anchor=(8, 2), accent=(8, 3),
    variants=[
        ('1', (
            [(y, x0, x1, 'h' if y <= 8 else ('e' if y <= 14 else 's')) for (y, x0, x1) in _BACK_TAPER]
            + [(19, 2, 4, 's'), (19, 11, 13, 's')]
        ), []),
        ('2', (
            # stessa sagoma, palette calda, due strappi nell'orlo (righe 17/18)
            # e una bisaccia (blocco extra a sinistra) -- il "craft pass" che
            # rompe il bordo dritto lungo di DEC-205.
            [(y, x0, x1, 'b' if y <= 8 else ('T' if y <= 14 else ('C' if y <= 18 else 'N')))
             for (y, x0, x1) in _BACK_TAPER if not (y == 17 and x0 <= 7 < x1) and not (y == 18 and x0 <= 5 < x1)]
            + [(17, 1, 7, 'C'), (17, 9, 14, 'C'), (18, 1, 5, 'S'), (18, 6, 14, 'S')]
            + [(19, 2, 4, 'N'), (19, 11, 13, 'N')]
            + [(11, 0, 2, 'S'), (12, 0, 2, 'S'), (13, 0, 2, 'S')]
        ), []),
    ],
)

SLOTS['body'] = dict(
    w=10, h=10, anchor=(5, 5), accent=(5, 5),
    variants=[
        ('1', [  # placca
            (2, 3, 7, 'h'), (3, 2, 8, 'h'), (4, 2, 8, 'e'), (5, 2, 8, 'e'),
            (6, 2, 8, 's'), (7, 2, 8, 's'), (8, 3, 7, 'S'),
        ], []),
        ('2', [  # fascia avvolta, calda
            (2, 1, 4, 'b'), (3, 2, 5, 'b'), (4, 3, 6, 'T'), (5, 4, 7, 'T'),
            (6, 5, 8, 'C'), (7, 6, 9, 'C'), (8, 7, 9, 'S'),
        ], []),
    ],
)

# Giro 2, verdetto D6: il giro 1 disegnava l'aura come 4-8 pixel isolati (una
# "spolverata" che sullo screenshot si leggeva come 2-3 puntini sparsi, molto
# meno leggibile del vecchio disco pieno DrawCircleV). Ora e' un anello VERO
# (build_ring sopra), diametro esterno ~12.6px nativi -- dentro la finestra
# 10-14px del verdetto -- spessore ~1.8-2px, sempre neutro; il centro cavo
# ospita l'accento (Item.color, disegnato dal motore), che qui e' un blocco
# 5x5 e non 1x1 come gli altri slot -- CRESCIUTO dal 2x2 del giro 1 (il cavo
# e' piu' largo, l'anello e' piu' spesso: un accento piccolo ci nuoterebbe
# dentro): e' l'unico slot il cui SCOPO primario (strato 3, "colore
# funzionale", visual-language.md) e' segnalare colore, e resta -- come gia'
# al giro 1, sia con size 2 che ora con size 5 -- il piu' grande del set.
_AURA_R_IN, _AURA_R_OUT = 4.5, 6.3


def _aura_lit_top_left(dx, dy):
    """Piano di luce unico dall'alto-sinistra (DEC-205): il quadrante
    alto-sinistra dell'anello e' un tono piu' chiaro, il resto piu' scuro --
    UNA sola soglia, mai un gradiente per-pixel (niente dithering)."""
    return 'h' if (dx + dy) < 0 else 's'


def _aura_dull(dx, dy):
    """Variante 'rune': si distingue da 'scintilla' per palette (parte da un
    grigio medio 'e', non dal chiaro 'h') e per le incisioni (notch_angles
    sotto), non per la massa (che resta quasi identica). Il tono scuro e'
    'n', non 's' come nel primo tentativo: 's' (lum~78) era troppo vicino
    alla luminanza media del tileset scuro (~47) e il floor test lo boccia
    -- 'n' (lum~45) accoppiato a 'e' (lum~120) tiene un salto di contrasto
    sufficiente su ENTRAMBI i pavimenti (vedi floor_test_ok)."""
    return 'e' if (dx + dy) < 0 else 'n'


SLOTS['aura'] = dict(
    w=14, h=14, anchor=(7, 7),
    # Accento 5x5, angolo alto-sinistra (4,4): il quadrato spazia da (4,4) a
    # (9,9), il suo angolo piu' lontano dal centro (7,7) e' a distanza
    # sqrt(3^2+3^2)=4.24, sotto R_IN=4.5 -- resta DENTRO il cavo dell'anello
    # con margine, mai a cavallo della corona dipinta.
    accent=(4, 4), accent_size=5,
    kind='ring',
    variants=[
        ('1', dict(tone_fn=_aura_lit_top_left)),                                            # scintilla: anello pieno, piano di luce
        ('2', dict(tone_fn=_aura_dull, notch_angles=[0.0, math.pi/2, math.pi, -math.pi/2])),  # runa: anello con 4 piccole incisioni cardinali
    ],
)


# Giro 2, verdetto D5: le scale devono restare a MEZZI PASSI (0.5, come
# ArtScaleForWidth impone al motore, art_draw.c) -- 1.8/1.4 del giro 1 non lo
# erano, ed erano anche SBAGLIATE rispetto a quelle vere usate da DrawItemLayer
# (item_layers.c): il floor test testava un ingrandimento diverso da quello
# che il giocatore vede davvero. Questa tabella DEVE restare uguale, slot per
# slot, a EQUIP_HAT_SCALE/EQUIP_EYES_SCALE/EQUIP_HAND_SCALE/EQUIP_BACK_SCALE/
# EQUIP_BODY_SCALE/EQUIP_AURA_SCALE in src/render/item_layers.c -- nessuno dei
# due file include l'altro (Python e C), quindi la sincronia e' manuale: chi
# cambia una scala la cambia in ENTRAMBI i posti.
ENGINE_SCALE = {
    'hat': 2.0, 'eyes': 2.0, 'hand': 2.0, 'back': 2.0, 'body': 1.5, 'aura': 1.5,
}


def floor_test_ok(icon, scale):
    """Contrasto minimo sul pavimento chiaro E scuro (DEC-205): compone
    l'icona ingrandita ESATTAMENTE come la disegna il motore in gioco (vedi
    ENGINE_SCALE sopra, che deve combaciare con le costanti EQUIP_*_SCALE di
    item_layers.c) sopra un ritaglio vero di un tileset chiaro e uno scuro, e
    misura la luminanza al bordo della sagoma contro il tile intorno. Stessa
    tecnica/soglie di scripts/charrig.py (validate, "floor test"). Qui il
    test e' piu' permissivo (soglie dimezzate): questi overlay sono accenti
    di 8-20px nativi, non un personaggio 32x32 a schermo intero, e un'icona
    cosi' piccola ha fisiologicamente meno bordo su cui misurare."""
    big_w = max(1, round(icon.width*scale))
    big_h = max(1, round(icon.height*scale))
    big = icon.resize((big_w, big_h), Image.NEAREST)
    ok = True
    for tile_path, name in (
        ('assets/art/tiles/lunar-forge.png', 'scuro'),
        ('assets/art/tiles/cathedral-of-sugar.png', 'chiaro'),
    ):
        full_path = os.path.join(os.path.dirname(__file__), '..', tile_path)
        if not os.path.exists(full_path):
            continue   # checkout senza il pacchetto tileset: non blocca la generazione
        # Ritaglio a (32,32): l'angolo (0,0) del PNG e' spesso una cella di
        # bordo/giunzione dell'atlas (piatta, a volte un tono che per puro
        # caso coincide con uno dei nostri), non il "pavimento" vero -- stessa
        # scelta di offset di scripts/charrig.py (validate, floor test).
        tile = Image.open(full_path).convert('RGBA').crop((32, 32, 32 + big.width, 32 + big.height))
        comp = tile.copy()
        comp.alpha_composite(big)
        cp, tp, ip = comp.load(), tile.load(), big.load()
        deltas = []
        for y in range(big.height):
            for x in range(big.width):
                if ip[x, y][3] == 0:
                    continue
                for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                    if 0 <= nx < big.width and 0 <= ny < big.height and ip[nx, ny][3] == 0:
                        l1 = sum(cp[x, y][:3]) / 3
                        l2 = sum(tp[nx, ny][:3]) / 3
                        deltas.append(abs(l1 - l2))
                        break
        if not deltas:
            continue
        deltas.sort()
        med = deltas[len(deltas) // 2]
        if med < 10:
            print(f'  floor test {name}: mediana bordo {med:.1f} (< 10, contrasto debole)')
            ok = False
    return ok


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    errors = []
    fucina_rgb = set(PAL.values())

    for slot, spec in SLOTS.items():
        w, h = spec['w'], spec['h']
        ax, ay = spec['anchor']
        cx, cy = spec['accent']
        accent_size = spec.get('accent_size', 1)
        kind = spec.get('kind', 'segments')
        engine_scale = ENGINE_SCALE[slot]

        for variant in spec['variants']:
            name = variant[0]
            if kind == 'ring':
                img = build_ring(w, h, ax, ay, _AURA_R_IN, _AURA_R_OUT, **variant[1])
            else:
                _, segments, pixels = variant
                img = make_image(w, h, segments, pixels)

            # controllo 1: nessun pixel fuori dal sottoinsieme neutro dichiarato
            px = img.load()
            painted = [(x, y) for y in range(h) for x in range(w) if px[x, y][3] > 0]
            if not painted:
                errors.append(f'{slot}_{name}: sprite vuoto')
                continue
            for x, y in painted:
                if px[x, y][:3] not in fucina_rgb:
                    errors.append(f'{slot}_{name}: colore fuori palette neutra a ({x},{y})')

            # controllo 2 (giro 2, D1): l'INTERO blocco d'accento -- angolo
            # alto-sinistra (cx,cy), lato accent_size -- deve stare dentro il
            # canvas, non solo il suo angolo: da quando DrawEquipAccent non
            # centra piu' (D1), un blocco che sborda dal canvas sborderebbe
            # anche a schermo.
            if not (0 <= cx and cx + accent_size <= w and 0 <= cy and cy + accent_size <= h):
                errors.append(f'{slot}_{name}: accent ({cx},{cy}) lato {accent_size} fuori canvas {w}x{h}')

            out_png = os.path.join(OUT_DIR, f'{slot}_{name}.png')
            out_json = os.path.join(OUT_DIR, f'{slot}_{name}.json')
            img.save(out_png)
            manifest = {
                'frame_w': w, 'frame_h': h, 'anchor': [ax, ay],
                'anims': {'idle': {'row': 0, 'frames': 1, 'fps': 1, 'loop': True}},
            }
            with open(out_json, 'w') as f:
                json.dump(manifest, f, separators=(',', ':'))

            if not floor_test_ok(img, engine_scale):
                errors.append(f'{slot}_{name}: floor test insufficiente')

    if errors:
        print('BOCCIA:')
        for e in errors:
            print(' -', e)
        sys.exit(1)
    total = sum(len(spec['variants']) for spec in SLOTS.values())
    print(f'OK: {total} overlay generati in {OUT_DIR}')


if __name__ == '__main__':
    main()
