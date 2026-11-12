"""
verify_trail.py -- Tai dung va KIEM TRA DOC LAP mot vet 3 vong do trail3.exe bao cao (a1 <= 2),
bang Python thuan (khong dung lai ma C):
  * cho tap cot vong giua va gia tri X1 (5a bit, bit 4 cua moi nhom = tu 0), duyet moi Y1 tuong thich,
  * dung lai toan bo vet (X0, Y0, X1, Y1, X2, Y2) voi lua chon toi uu o hai vong ngoai,
  * kiem tra tung buoc: quan he tuyen tinh qua L (hoac L^T), tung S-box co DDT/LAT != 0,
  * cong trong so tu bang DDT/LAT tinh truc tiep.
Su dung: python verify_trail.py <layer> <diff|lin> <ascon|chi> <c0,c1,..> <X1hex>
"""
import math
import sys

from layers import N, W, NW, inverse_cols, transpose_cols, apply_cols, int_to_state, state_to_int
from trail_sat import get_layer
from sbox import SBOX


def chi_table():
    S = []
    for v in range(32):
        x = [(v >> (4 - w)) & 1 for w in range(5)]
        y = [x[j] ^ ((1 - x[(j + 1) % 5]) & x[(j + 2) % 5]) for j in range(5)]
        S.append(sum(y[w] << (4 - w) for w in range(5)))
    return S


def weight_table(S, mode):
    T = [[None] * 32 for _ in range(32)]
    for a in range(32):
        for b in range(32):
            if mode == "diff":
                c = sum(1 for x in range(32) if S[x] ^ S[x ^ a] == b)
                T[a][b] = None if c == 0 else 5 - math.log2(c)
            else:
                c = sum(1 if (bin(a & x).count("1") + bin(b & S[x]).count("1")) % 2 == 0 else -1 for x in range(32))
                T[a][b] = None if c == 0 else -2 * math.log2(abs(c) / 32)
    return T


def columns(v):
    s = int_to_state(v)
    out = {}
    for p in range(W):
        val = sum(((s[w] >> p) & 1) << (4 - w) for w in range(NW))
        if val:
            out[p] = val
    return out


def from_columns(cd):
    s = [0] * NW
    for p, val in cd.items():
        for w in range(NW):
            if (val >> (4 - w)) & 1:
                s[w] |= 1 << p
    return state_to_int(s)


def main():
    layer, mode, sb, cs, x1 = sys.argv[1], sys.argv[2], sys.argv[3], list(map(int, sys.argv[4].split(","))), int(sys.argv[5], 16)
    S = chi_table() if sb == "chi" else SBOX
    T = weight_table(S, mode)
    L = get_layer(layer)
    Li = inverse_cols(L)
    LT, LiT = transpose_cols(L), transpose_cols(Li)
    back, fwd = (Li, L) if mode == "diff" else (LT, LiT)
    a = len(cs)
    X1 = {cs[t]: (x1 >> (5 * t)) & 31 for t in range(a)}
    assert all(X1.values()), "X1 phai khac 0 tren moi cot"
    vX1 = from_columns(X1)
    Y0 = columns(apply_cols(back, vX1))               # vi sai: Y0 = L^-1 X1 ; tuyen tinh: v0 = L^T u1
    w0 = 0.0
    X0 = {}
    for p, y in Y0.items():                           # chon dau vao tot nhat cho tung S-box vong 0
        best = min((T[x][y], x) for x in range(32) if T[x][y] is not None)
        w0 += best[0]; X0[p] = best[1]
    best_total = None
    import itertools
    choices = [[(y, T[X1[c]][y]) for y in range(32) if T[X1[c]][y] is not None] for c in cs]
    for combo in itertools.product(*choices):
        Y1 = {cs[t]: combo[t][0] for t in range(a)}
        w1 = sum(c[1] for c in combo)
        X2 = columns(apply_cols(fwd, from_columns(Y1)))   # vi sai: X2 = L Y1 ; tuyen tinh: u2 = L^-T v1
        w2 = sum(min(T[x][y] for y in range(32) if T[x][y] is not None) for x in X2.values())
        tot = w0 + w1 + w2
        if best_total is None or tot < best_total[0]:
            best_total = (tot, Y1, X2, w1, w2)
    tot, Y1, X2, w1, w2 = best_total
    # kiem tra lai quan he tuyen tinh mot cach doc lap
    if mode == "diff":
        assert apply_cols(L, from_columns(Y0)) == vX1
        assert apply_cols(L, from_columns(Y1)) == from_columns(X2)
    else:
        assert apply_cols(LT, vX1) == from_columns(Y0)          # v0 = L^T u1
        assert apply_cols(LT, from_columns(X2)) == from_columns(Y1)   # v1 = L^T u2
    print(f"layer={layer[:40]} mode={mode} sbox={sb}")
    print(f"  round0: {len(Y0)} S-box, weight {w0:g}")
    print(f"  round1: {a} S-box, weight {w1:g}")
    print(f"  round2: {len(X2)} S-box, weight {w2:g}")
    print(f"  TOTAL : {len(Y0) + a + len(X2)} active S-boxes, weight {tot:g}  (relations verified)")


if __name__ == "__main__":
    main()
