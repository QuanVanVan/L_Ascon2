# Kiem tra: diem so cot cua asconColScore.cpp co la CAN DUOI hop le cua trong so cuoi cung khong,
# khi cac hang >= y (hoac > y) cua b con co the thay doi (them bit vao a o cac hang sau).
import itertools
import os as _o
exec(open(_o.path.join(_o.path.dirname(_o.path.abspath(__file__)), 'check_tables.py')).read().split('# bang trong')[0].split('ok = True')[0])
W_D = [0]+[int(math.log2(sum(1 for a in range(32) if DDT[b][a]))) for b in range(1,32)]
W_L = [0]+[2*(5-int(math.log2(abs(next(LAT[u][b] for u in range(32) if LAT[u][b]))))) for b in range(1,32)]
bit = lambda v,k: (v >> (4-k)) & 1      # row k, row0 = MSB
def bt2(v):  x=[bit(v,k) for k in range(5)]; return ((1-x[0])&x[4]) ^ ((1-x[0])&x[3]) ^ (x[0]&x[2]) ^ (x[0]&x[1])
def bt3(v):  x=[bit(v,k) for k in range(5)]; return (x[1]&(1-x[2])&x[3]) ^ ((1-x[0])&x[1]&(1-x[3])&x[4])
def bt2L(v):
    x=[bit(v,k) for k in range(5)]
    return (x[1]&x[3]) ^ ((1-x[2])&x[3]&x[4]) ^ ((1-x[0])&x[1]&x[4]) ^ ((1-x[0])&x[1]&x[2]) ^ (x[0]&(1-x[1])&x[2]) ^ ((1-x[0])&x[2]&(1-x[3])&x[4])
scoreD = lambda v: 2 + bt2(v) + bt3(v)
scoreL = lambda v: 2 + bt2L(v)
for name, W, sc in (('DTS', W_D, scoreD), ('LTS', W_L, scoreL)):
    exact = all(sc(v) == W[v] for v in range(1,32))
    lb = all(sc(v) <= W[v] for v in range(1,32))
    print(name, 'score==weight on full columns:', exact, '| score<=weight:', lb)
    # free = tap hop hang co the con thay doi; gia tri hien tai o hang tu do = 0 (nhu ma nguon dat)
    bad = []
    for free in range(32):
        for v in range(1,32):
            if v & free: continue                       # hang tu do hien bang 0
            comps = [v | f for f in range(32) if (f & ~free) == 0]
            m = min(W[c] for c in comps if c)           # cot van hoat dong vi v != 0 o hang co dinh
            if sc(v) > m: bad.append((free, v, sc(v), m))
    # truong hop su dung trong cong cu: free = cac hang > y (checkScore) hoac {y khong on dinh} U {> y}
    rel = [b for b in bad if any(b[0] == sum(1<<(4-k) for k in range(y+1,5)) or b[0] == sum(1<<(4-k) for k in range(y,5)) for y in range(5))]
    print(name, 'counterexamples (free rows = suffix of rows):', len(rel), rel[:6])
