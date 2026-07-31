#!/usr/bin/env python3
"""charrig — rig deterministico per gli sheet dei personaggi (DEC-205..208, DEC-210).

Principio (dal proprietario, 31/07): lo spritesheet e' il formato di ASSEMBLAGGIO,
mai l'immagine da inventare frame per frame. Le parti si disegnano una volta sola
come griglie ASCII; ogni frame e' una lista (parte, dx, dy, mirror) da tabella;
un validatore impone gli invarianti:
  - piedi/terra a y=28 in ogni frame a terra;
  - stessa massa delle parti in ogni frame (le gambe non cambiano pixel);
  - luce sempre da schermo-sinistra, anche nelle viste specchiate
    (le parti di luce si piazzano in coordinate assolute, non specchiate);
  - un solo componente connesso (salvo braci dichiarate);
  - palette solo Fucina; contrasto minimo del bordo su pavimento chiaro E scuro.

Uso: python3 scripts/charrig.py fonditrice out.png   (exit!=0 se la validazione fallisce)
"""
import json, sys
from PIL import Image

PAL = {
    'N': (20,16,14), 'S': (36,26,22), 'C': (58,38,32), 'T': (85,53,42),
    'b': (122,74,43), 'B': (156,101,38), 'L': (201,138,46), 'O': (232,183,74),
    'P': (245,223,143), 'A': (255,196,107), 'F': (224,91,35), 'R': (177,58,30),
    'n': (43,43,49), 's': (74,74,85), 'e': (115,115,130),
}
FLASH = {'n':'s','s':'e','B':'L','b':'L','T':'b','C':'T','S':'C','O':'A','N':'S'}
CELL, COLS, ROWS, GROUND = 32, 4, 7, 28

def G(*rows):
    return [r for r in rows]

# ---------------- PARTI (disegnate una volta) ----------------
PARTS = {
 'helm_f': (10,3, G('...PPPPB....','..PPPPPBBB..','.PPPPPBBBBB.','.PPPPBBBBBB.',
                    '.PPPBBBBBBB.','..LBBBBBBBb.','bbbbbbbbbbbb')),
 'void_f': (12,10, G('NNNNNNNN','NNNNNNNN')),
 'neck_b': (12,10, G('bbbbbbbb','.BBBBBB.')),
 'torso_f':(10,12, G('.sssnnnnnnS.','sssnTTTTTCnS','sssnTTTTTCnS','sssnTTTTTCnS',
                     'sssnTTTTTCnS','SssnTTTTTCnS','sssnTTTTTCnS','.snnTTTTTCS.',
                     '.OOOOOAOOOO.')),
 'torso_b':(10,12, G('.sssnnnnnnn.','sssnnnnnnnnn','sssnnnnnnnnn','sssnnnnnnnnS',
                     'sssnnnnnnnnn','nssnnnnnnnnn','sssnnnnnnnnn','.nnnnnnnnnn.',
                     '.OOOOOOOOOO.')),
 'strap_b':(13,12, G('.......C','......C.','.....C..','....C...','...C....',
                     '..C.....','.C......','C.......')),
 'hem_f':  (13,21, G('TTTTTC','.STRS.')),
 'hem_b':  (14,21, G('nnnn','.n..')),
 'arm_l':  (9,13,  G('s','s','s','S')),
 'arm_r':  (22,13, G('S','S','S','S')),
 'leg':    (0,0,   G('sCS','sCS','sCS','sCS','SSS')),
 'leg_k':  (0,0,   G('sCS','sCS','sCS','SSS')),
 'boot':   (0,0,   G('bbbbb','SCCCS','SSSSS')),
 'helm_s': (11,3,  G('.BBBBBB...','BBBBBBBBB.','BBBBBBBBBB','BBBBBBBBBB',
                     'BBBBBBBBBB','BBBBBBBBBb')),
 'brim':   (10,9,  G('bbbbbbbbbbbb')),
 'void_s': (12,10, G('BBBBNNNNN','.BBBNNNN.')),
 'torso_s':(11,12, G('.nnnnnnnn.','nnnnnnnnn.','nnnnnnnnnn','.nnnnnnnnn',
                     'nnnnnnnnnn','nnnnnnnnn.','.nnnnnnnnn','.nnnnnnnn.')),
 'apron_s':(16,13, G('TT..','TTT.','TT..','TTTC','TT..','TT..')),
 'belt_s': (12,20, G('OOOOOAOO')),
 'hem_s':  (14,21, G('TTTT','..R.')),
 'arm_s':  (0,0,   G('S','S','C')),
 # luce: coordinate ASSOLUTE a schermo, mai specchiate
 'glow_helm_s': (11,3, G('.PPPP.....','PPPPP.....','PPPP......','PPP.......',
                         'PPP.......','.L........')),
 'glow_side':   (11,12,G('s.','ss','ss','.s','ss','s.','.s','s.')),
 # morte: masse fuse con bordi irregolari
 'mound_1':(9,18,  G('....nnnnnn....','..nnnnnnnnnn..','.nnnnnnnnnnnn.','nnnnnnnnnnnn..',
                     '.CCnnnnnnnCCC.','CCCCCCCCCCCC..','.SCCCCCCCCCCS.')),
 'mound_2':(9,21,  G('...nnnnnnn....','.nnnnnnnnnnnn.','..CCCCCCCCC...','.CCCCCCCCCCC..')),
 'pud_1':  (8,25,  G('.SSSSSSSSSSSS...','SSSSSSSSSSSSSSS.','.CCCCCCCCCCCCCC.','SSSSSSSSSSSSSS..')),
 'pud_2':  (7,25,  G('..SSSSSSSSSSSSS...','SSSSSSSSSSSSSSSSS.','.CCCCCCCCCCCCCCCC.','SSSSSSSSSSSSSSS...')),
 'helm_top':(11,0, G('..PPPPB...','.PPPPPBBB.','PPPPPBBBBB','PPPPBBBBBB')),
}

# ---------------- POSE (tabelle: parte, dx, dy) ----------------
def _fb(torso, hem, legL, legR, dy, extra=()):
    """vista frontale/posteriore: gambe inchiodate a terra, corpo che respira."""
    seq = []
    for leg, x in ((legL, 12), (legR, 17)):
        if leg == 'back':
            seq += [('leg_k', x, 21, 0), ('boot', x-1, 25, 0)]
        else:
            seq += [('leg', x, 21, 0), ('boot', x-1, 26, 0)]
    seq += [('helm_f', 0, dy, 0)]
    seq += list(extra)
    seq += [(torso, 0, dy, 0), (hem, 0, dy, 0), ('arm_l', 0, dy, 0), ('arm_r', 0, dy, 0)]
    return seq

def front(legL='std', legR='std', dy=0):
    return _fb('torso_f', 'hem_f', legL, legR, dy, extra=[('void_f', 0, dy, 0)])

def back(legL='std', legR='std', dy=0):
    return _fb('torso_b', 'hem_b', legL, legR, dy, extra=[('neck_b', 0, dy, 0)]) + [('strap_b', 0, dy, 0)]

def side(phase, mir):
    """profilo: geometria specchiabile, luce SEMPRE assoluta (mai specchiata)."""
    seq = []
    if phase in (1, 3):
        fx = 15 if phase == 1 else 14
        bx = 11 if phase == 1 else 12
        seq += [('leg', fx, 21, mir), ('boot', fx-1, 26, mir)]
        seq += [('leg_k', bx, 21, mir), ('boot', bx-1, 25, mir)]
        dy = 0
    else:
        seq += [('leg', 12, 21, mir), ('boot', 12, 26, mir)]
        dy = -1
    ax = 13 if phase == 3 else 15
    seq += [('helm_s', 0, dy, mir), ('brim', 0, dy, mir), ('void_s', 0, dy, mir),
            ('torso_s', 0, dy, mir), ('apron_s', 0, dy, mir), ('belt_s', 0, dy, mir),
            ('hem_s', 0, dy, mir),
            ('arm_s', ax, 14+dy, mir),
            ('glow_helm_s', 0, dy, 0), ('glow_side', 0, dy, 0)]   # luce: mai mirror
    return seq

def death(stage):
    if stage == 1:
        return front(dy=2)
    if stage == 2:
        return [('pud_1', 0, 0, 0), ('mound_1', 0, 0, 0), ('helm_f', 0, 9, 0), ('void_f', 0, 9, 0)]
    if stage == 3:
        return [('pud_2', 0, 0, 0), ('mound_2', 0, 0, 0), ('helm_f', 0, 13, 0)]
    return [('pud_2', 0, 0, 0), ('helm_top', 0, 21, 0)]

DEATH_EMBERS = {1: [], 2: [(9,26,'F'),(21,27,'A'),(14,24,'R')],
                3: [(8,26,'F'),(22,25,'A'),(13,27,'R'),(18,26,'F')],
                4: [(8,26,'F'),(21,26,'A'),(15,25,'R'),(24,27,'F')]}

FONDITRICE = {
    'walk_down':  [front('fwd','back'), front(dy=-1), front('back','fwd'), front(dy=-1)],
    'walk_up':    [back('fwd','back'),  back(dy=-1),  back('back','fwd'),  back(dy=-1)],
    'walk_right': [side(p, 0) for p in (1,2,3,4)],
    'walk_left':  [side(p, 1) for p in (1,2,3,4)],
    'idle':       [front(), front(dy=1) ],
    'hit':        ['FLASH:front'],
    'death':      [death(s) for s in (1,2,3,4)],
}
ROW_OF = {'walk_down':0,'walk_up':1,'walk_right':2,'walk_left':3,'idle':4,'hit':5,'death':6}
GLINT = {('idle',1): [(15,21,'A'),(16,21,'A')]}

# ---------------- assemblatore ----------------
def stamp(img, part, dx, dy, mir, cmap=None):
    x0, y0, rows = PARTS[part]
    w = max(len(r) for r in rows)
    for ry, row in enumerate(rows):
        for rx, ch in enumerate(row):
            if ch == '.':
                continue
            ch2 = cmap.get(ch, ch) if cmap else ch
            px = (CELL-1) - (x0+rx) if mir else (x0+rx)
            img.putpixel((px+dx if not mir else px-dx, y0+ry+dy), PAL[ch2]+(255,))

def build(char):
    sheet = Image.new('RGBA', (COLS*CELL, ROWS*CELL), (0,0,0,0))
    for anim, frames in char.items():
        row = ROW_OF[anim]
        for col, frame in enumerate(frames):
            cell = Image.new('RGBA', (CELL, CELL), (0,0,0,0))
            if frame == 'FLASH:front':
                for part, dx, dy, mir in front():
                    stamp(cell, part, dx+1, dy, mir, cmap=FLASH)
            else:
                for part, dx, dy, mir in frame:
                    stamp(cell, part, dx, dy, mir)
            for gx, gy, gc in GLINT.get((anim, col), []):
                cell.putpixel((gx, gy), PAL[gc]+(255,))
            if anim == 'death':
                for gx, gy, gc in DEATH_EMBERS[col+1]:
                    cell.putpixel((gx, gy), PAL[gc]+(255,))
            sheet.alpha_composite(cell, (col*CELL, row*CELL))
    return sheet

# ---------------- validatore ----------------
def frames_of(sheet):
    out = {}
    for anim, row in ROW_OF.items():
        n = {'idle':2,'hit':1}.get(anim, 4)
        for c in range(n):
            out[(anim,c)] = sheet.crop((c*CELL,row*CELL,(c+1)*CELL,(row+1)*CELL))
    return out

def validate(sheet):
    errs = []
    fucina = set(PAL.values())
    F = frames_of(sheet)
    row_ref = {}
    for (anim,c), im in sorted(F.items()):
        px = im.load()
        pts = [(x,y) for y in range(CELL) for x in range(CELL) if px[x,y][3]>0]
        if not pts:
            errs.append(f'{anim}[{c}]: vuoto'); continue
        for x,y in pts:
            if px[x,y][:3] not in fucina:
                errs.append(f'{anim}[{c}]: colore fuori Fucina {px[x,y][:3]} a {x},{y}'); break
        bottom = max(y for _,y in pts)
        if bottom != GROUND:
            errs.append(f'{anim}[{c}]: bottom={bottom} invece di {GROUND}')
        # connettivita' su tutti i pixel opachi (le braci sono appoggiate, non flottanti)
        core = pts
        seen, todo = {core[0]}, [core[0]]
        cs = set(core)
        while todo:
            x,y = todo.pop()
            for nx,ny in ((x+1,y),(x-1,y),(x,y+1),(x,y-1)):
                if (nx,ny) in cs and (nx,ny) not in seen:
                    seen.add((nx,ny)); todo.append((nx,ny))
        if len(seen) != len(core):
            errs.append(f'{anim}[{c}]: {len(core)-len(seen)} px staccati dal corpo')
        # massa costante DENTRO la stessa vista (gambe = stesse parti sempre)
        if anim.startswith('walk'):
            ref = row_ref.setdefault(anim, len(pts))
            if abs(len(pts)-ref) > ref*0.12:
                errs.append(f'{anim}[{c}]: massa {len(pts)} vs frame 0 della riga {ref} (>12%)')
        # luce da schermo-sinistra: centroide dei toni chiari a sinistra del centro-massa
        light = [ (x,y) for x,y in pts if px[x,y][:3] in (PAL['P'],PAL['s'],PAL['L']) ]
        if anim.startswith('walk') and light:
            lc = sum(x for x,_ in light)/len(light)
            mc = sum(x for x,_ in pts)/len(pts)
            if lc > mc + 0.5:
                errs.append(f'{anim}[{c}]: luce a destra (centroide {lc:.1f} vs massa {mc:.1f})')
    # floor test automatico su tile chiaro e scuro
    for tile_path, name in (('assets/art/tiles/lunar-forge.png','scuro'),
                            ('assets/art/tiles/cathedral-of-sugar.png','chiaro')):
        tile = Image.open(tile_path).convert('RGBA').crop((32,32,64,64))
        idle = F[('idle',0)]
        comp = Image.new('RGBA',(CELL,CELL)); comp.paste(tile,(0,0)); base=comp.copy()
        comp.alpha_composite(idle)
        cp, bp, ip = comp.load(), base.load(), idle.load()
        deltas = []
        for y in range(CELL):
            for x in range(CELL):
                if ip[x,y][3]==0: continue
                for nx,ny in ((x+1,y),(x-1,y),(x,y+1),(x,y-1)):
                    if 0<=nx<CELL and 0<=ny<CELL and ip[nx,ny][3]==0:
                        l1 = sum(cp[x,y][:3])/3; l2 = sum(bp[nx,ny][:3])/3
                        deltas.append(abs(l1-l2)); break
        deltas.sort()
        med = deltas[len(deltas)//2]
        low = sum(1 for d in deltas if d<15)/len(deltas)
        if med < 20 or low > 0.22:
            errs.append(f'floor test {name}: mediana bordo {med:.0f}, sotto-15 {low:.0%}')
    return errs

if __name__ == '__main__':
    out = sys.argv[2] if len(sys.argv)>2 else 'charrig-out.png'
    sheet = build(FONDITRICE)
    errs = validate(sheet)
    sheet.save(out)
    if errs:
        print('BOCCIA:'); [print(' -', e) for e in errs]; sys.exit(1)
    print(f'OK: sheet valido, salvato in {out}')
