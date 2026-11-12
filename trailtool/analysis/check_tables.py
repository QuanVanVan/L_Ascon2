# Kiem tra doc lap cac bang viet cung trong trailExtension.cpp va asconColScore.cpp cua AsconTrailTool
import re, math
import os as _os
_TT = _os.path.dirname(_os.path.dirname(_os.path.abspath(__file__)))   # .../DATN/trailtool
_PRJ = _os.path.dirname(_TT)                                                  # .../DATN
UP = _os.path.join(_TT, 'upstream', 'reference', 'C++', 'src')   # ma nguon GOC (khong sua)
SRC = open(_os.path.join(UP, 'trailExtension.cpp'), encoding='utf-8-sig').read()
S = [0x4,0xb,0x1f,0x14,0x1a,0x15,0x9,0x2,0x1b,0x5,0x8,0x12,0x1d,0x3,0x6,0x1c,
     0x1e,0x13,0x7,0xe,0x0,0xd,0x11,0x18,0x10,0xc,0x1,0x19,0x16,0xa,0xf,0x17]
# S-box tu bitslice (spec) de doi chieu bang
def sbox_bitslice(v):
    x = [(v >> (4 - i)) & 1 for i in range(5)]  # x0 = MSB
    x[0] ^= x[4]; x[4] ^= x[3]; x[2] ^= x[1]
    t = [(x[i] ^ 1) & x[(i + 1) % 5] for i in range(5)]
    for i in range(5): x[i] ^= t[(i + 1) % 5]
    x[1] ^= x[0]; x[0] ^= x[4]; x[3] ^= x[2]; x[2] ^= 1
    return sum(x[i] << (4 - i) for i in range(5))
assert [sbox_bitslice(v) for v in range(32)] == S, "S-box mismatch"
par = lambda x: bin(x).count('1') & 1
DDT = [[0]*32 for _ in range(32)]
for x in range(32):
    for d in range(32): DDT[d][S[x] ^ S[x ^ d]] += 1
LAT = [[sum(1 if par(u & x) == par(v & S[x]) else -1 for x in range(32)) for v in range(32)] for u in range(32)]
def block(name, which):
    # lay khoi #if DTS==1 (which=0) hoac #else (which=1) trong Propagation::Propagation
    body = SRC[SRC.index('Propagation::Propagation()'):]
    body = body[:body.index('#endif')]
    parts = body.split('#else')
    part = parts[which]
    m = re.search(name + r'\s*=\s*\{(.*?)\};', part, re.S)
    return m.group(1)
def ints(s): return [int(t, 0) for t in re.findall(r'0x[0-9A-Fa-f]+|\d+', re.sub(r'//[^\n]*', '', s))]
def lists(s):
    s = re.sub(r'//[^\n]*', '', s)
    return [ints(g) for g in re.findall(r'\{([^{}]*)\}', s)]
ok = True
def chk(cond, msg):
    global ok
    if not cond: ok = False; print('FAIL', msg)
for which, kind in ((0, 'DTS'), (1, 'LTS')):
    W = ints(block('weightListPerInput', which))
    D = lists(block('directOutputListPerInput', which))
    R = lists(block('reverseOutputListPerInput', which))
    RW = lists(block('weightReverseOutputListPerInput', which))
    MR = ints(block('minRevWeightListPerOutput', which))
    AF = re.findall(r'AffineSpaceOfColumns\((0x[0-9A-Fa-f]+|\d+),\s*\{([^}]*)\}\)', block('affinePerInput', which))
    for b in range(1, 32):
        if kind == 'DTS':
            comp = sorted(a for a in range(32) if DDT[b][a]); w = int(math.log2(len(comp)))
            chk(all(DDT[b][a] == 32 >> w for a in comp), f'{kind} DDT not flat b={b}')
        else:   # b = mascara a la salida de S; compatibles = mascaras de entrada u con LAT[u][b] != 0
            comp = sorted(u for u in range(32) if LAT[u][b]); w = 2 * (5 - int(math.log2(abs(LAT[comp[0]][b]))))
            chk(all(abs(LAT[u][b]) == abs(LAT[comp[0]][b]) for u in comp), f'{kind} LAT not flat b={b}')
            chk(2 ** w == 2 ** (2*5) // LAT[comp[0]][b] ** 2, 'lin weight')
        chk(W[b] == w, f'{kind} weight b={b}: table {W[b]} vs {w}')
        chk(sorted(D[b]) == comp, f'{kind} direct list b={b}')
        off = int(AF[b][0], 0); gens = ints(AF[b][1]) if AF[b][1].strip() else []
        span = {off}
        for g in gens: span |= {s ^ g for s in span}
        chk(sorted(span) == comp and len(gens) == w if kind == 'DTS' else sorted(span) == comp, f'{kind} affine b={b}')
    for a in range(1, 32):
        if kind == 'DTS': rev = [b for b in range(32) if DDT[b][a]]
        else: rev = [v for v in range(32) if LAT[a][v]]
        rev = [b for b in rev if b]
        chk(sorted(R[a]) == sorted(rev), f'{kind} reverse list a={a}')
        chk(RW[a] == [W[b] for b in R[a]] and RW[a] == sorted(RW[a]), f'{kind} reverse weights a={a}')
        chk(MR[a] == min(W[b] for b in rev), f'{kind} minrev a={a}')
    print(kind, 'checked')
# bang trong asconColScore.cpp
CS = open(_os.path.join(UP, 'asconColScore.cpp')).read()
mr = re.findall(r'minRevWeightListPerOutput\[32\] = \{([^}]*)\}', CS)
wl = re.findall(r'weightListPerInput\[32\] = \{([^}]*)\}', CS)
for which, kind in ((0, 'DTS'), (1, 'LTS')):
    chk(ints(mr[which]) == ints(block('minRevWeightListPerOutput', which)), f'colscore minrev {kind}')
    chk(ints(wl[which]) == ints(block('weightListPerInput', which)), f'colscore weight {kind}')
print('ALL TABLES OK' if ok else 'TABLE ERRORS FOUND')
