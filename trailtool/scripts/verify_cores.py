# verify_cores.py <DTS|LTS> <layer: ascon | smrm-name> <finalists.txt> <file> [maxlines]
#   finalists.txt: DATN/results/02_finalists/finalists.txt
# Kiem tra DOC LAP (khong dung ma C++) moi loi vet trong tep dinh dang AsconTrailTool:
#   * a_i = lambda^{-1}(b_i) voi lambda = p_L (vi sai) hoac p_L^T (tuyen tinh), tinh bang smrm/layers.py
#   * b_i -> a_{i+1} tuong thich qua S-box tung cot (DDT / LAT tinh lai tu bang S-box)
#   * trong so: w(b_i) va wrev(a_1) tinh lai tu DDT/LAT; tong khop voi tep
# In ra: so loi, so loi sai, trong so nho nhat, so S-box hoat dong theo vong cua loi nhe nhat.
import sys, math
import os as _os
_TT = _os.path.dirname(_os.path.dirname(_os.path.abspath(__file__)))   # .../DATN/trailtool
_PRJ = _os.path.dirname(_TT)                                                  # .../DATN
sys.path.insert(0, _os.path.join(_PRJ, 'smrm'))
from layers import SMRM, ascon_pL, matrix_columns, transpose_cols, inverse_cols, apply_cols, state_to_int, int_to_state

S = [0x4,0xb,0x1f,0x14,0x1a,0x15,0x9,0x2,0x1b,0x5,0x8,0x12,0x1d,0x3,0x6,0x1c,
     0x1e,0x13,0x7,0xe,0x0,0xd,0x11,0x18,0x10,0xc,0x1,0x19,0x16,0xa,0xf,0x17]
par = lambda x: bin(x).count('1') & 1
DDT = [[0]*32 for _ in range(32)]
for x in range(32):
    for d in range(32): DDT[d][S[x] ^ S[x ^ d]] += 1
LAT = [[sum(1 if par(u & x) == par(v & S[x]) else -1 for x in range(32)) for v in range(32)] for u in range(32)]

mode, layer, layfile, fn = sys.argv[1:5]
maxlines = int(sys.argv[5]) if len(sys.argv) > 5 else 10**9
if layer == 'ascon':
    f = ascon_pL
else:
    spec = None
    for line in open(layfile):
        n, ops, d = line.strip().split('|')
        if n == layer: spec = (ops, d)
    ops, d = spec
    f = SMRM([tuple(map(int, o.split(','))) for o in ops.split(';')], list(map(int, d.split(','))), layer)
cols = matrix_columns(f)
lam = cols if mode == 'DTS' else transpose_cols(cols)
laminv = inverse_cols(lam)

def colv(st, c):                    # MSB = row 0 (convention of the tool)
    return sum(((st[k] >> c) & 1) << (4 - k) for k in range(5))
if mode == 'DTS':
    w_in = [0] + [int(math.log2(sum(1 for a in range(32) if DDT[b][a]))) for b in range(1, 32)]
    compat = lambda b, a: DDT[b][a] != 0                  # b input diff, a output diff
    revlist = lambda a: [b for b in range(1, 32) if DDT[b][a]]
else:
    w_in = [0] + [2 * (5 - int(math.log2(abs(next(LAT[u][b] for u in range(32) if LAT[u][b]))))) for b in range(1, 32)]
    compat = lambda b, a: LAT[a][b] != 0                  # b output mask, a input mask
    revlist = lambda a: [v for v in range(1, 32) if LAT[a][v]]
minrev = [0] + [min(w_in[b] for b in revlist(a)) for a in range(1, 32)]

n = bad = 0; best = None
for line in open(fn):
    t = line.split()
    if not t: continue
    n += 1
    if n > maxlines: break
    nr = int(t[0]); W, w0 = int(t[1]), int(t[2]); ws = list(map(int, t[3:3 + nr - 1]))
    vals = [int(v, 16) for v in t[3 + nr - 1:]]
    bs = [vals[5 * i:5 * i + 5] for i in range(nr - 1)]
    a_s = [int_to_state(apply_cols(laminv, state_to_int(b))) for b in bs]
    ok = True
    # weights
    cw0 = sum(minrev[colv(a_s[0], c)] for c in range(64) if colv(a_s[0], c))
    cws = [sum(w_in[colv(b, c)] for c in range(64) if colv(b, c)) for b in bs]
    if cw0 != w0 or cws != ws or W != cw0 + sum(cws): ok = False
    # compatibility b_i -> a_{i+1}
    for i in range(len(bs) - 1):
        for c in range(64):
            bv, av = colv(bs[i], c), colv(a_s[i + 1], c)
            if (bv == 0) != (av == 0) or (bv and not compat(bv, av)): ok = False
    if not ok: bad += 1
    act = [sum(1 for c in range(64) if colv(a_s[0], c))] + [sum(1 for c in range(64) if colv(b, c)) for b in bs]
    if best is None or W < best[0]: best = (W, [w0] + ws, act)
print(f"{fn}: {min(n, maxlines)} cores checked, {bad} INVALID" + (f"; lightest weight {best[0]} profile {best[1]} active S-boxes per round {best[2]}" if best else ""))
sys.exit(1 if bad else 0)
