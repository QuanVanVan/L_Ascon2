import sys
import os as _os
_PRJ = _os.path.dirname(_os.path.dirname(_os.path.dirname(_os.path.abspath(__file__))))
sys.path.insert(0, _os.path.join(_PRJ, 'smrm'))
from layers import SMRM, ascon_pL, NW, W, rotl
FIN = {}
for line in open(_os.path.join(_PRJ, 'results', '02_finalists', 'finalists.txt')):
    n, ops, d = line.strip().split('|')
    FIN[n] = SMRM([tuple(map(int, o.split(','))) for o in ops.split(';')], list(map(int, d.split(','))), n)
def supports(f):
    # sup[k][i] = set of offsets e: bit (i,0) -> bit (k,e)
    sup = [[set() for _ in range(NW)] for _ in range(NW)]
    for i in range(NW):
        s = [0]*NW; s[i] = 1
        y = f(s)
        for k in range(NW):
            for e in range(W):
                if (y[k] >> e) & 1: sup[k][i].add(e)
    return sup
def span(E):
    if not E: return 0
    E = sorted(E); best = 64
    for idx in range(len(E)):   # arc starting at E[idx]
        lo = E[idx]; hi = E[idx-1]
        best = min(best, (hi - lo) % 64)
    return best
def best_w(E):
    return min((span({(e*w) % 64 for e in E}), w) for w in range(1, 64, 2))
layers = {'ascon': ascon_pL}; layers.update(FIN)
for name, f in layers.items():
    sup = supports(f)
    tot = sum(len(sup[k][i]) for k in range(NW) for i in range(NW))
    E = set().union(*[sup[k][i] for k in range(NW) for i in range(NW)])
    rowsTouched = [sum(1 for k in range(NW) if sup[k][i]) for i in range(NW)]
    wts = [sum(len(sup[k][i]) for k in range(NW)) for i in range(NW)]
    print(f"{name:8s} nnz/col={wts} rowsTouched={rowsTouched} |E|={len(E)} bestspan(all)={best_w(E)}")
print()
def transpose_fn(f):
    # returns g with <f(x),y> = <x,g(y)> computed via matrix
    cols = []
    for i in range(NW):
        for p in range(W):
            s=[0]*NW; s[i]=1<<p; cols.append(f(s))
    def g(y):
        out=[0]*NW
        idx=0
        for i in range(NW):
            for p in range(W):
                c = cols[idx]; idx+=1
                par = 0
                for k in range(NW): par ^= bin(c[k] & y[k]).count('1') & 1
                if par: out[i] |= 1<<p
        return out
    return g
for name, f in layers.items():
    for tag, g in (('L', f), ('LT', transpose_fn(f))):
        sup = supports(g)
        R = [''.join(str(k) for k in range(NW) if sup[k][i]) for i in range(NW)]
        sp = []
        for i in range(NW):
            best = min(max(span({(e*w) % 64 for e in sup[k][i]}) for k in range(NW)) for w in range(1,64,2))
            sp.append(best)
        print(f"{name:8s} {tag:2s} touched rows per input row: {R}  per-row best max-span: {sp}")
