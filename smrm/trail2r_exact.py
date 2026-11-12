"""
trail2r_exact.py -- trong so NHO NHAT CHINH XAC cua vet vi sai / tuyen tinh 2 vong (S-box Ascon, tang tuyen tinh bat ky).

Lap luan (vet 2 vong = S -> L -> S, dau vao vong 1 va dau ra vong 2 tu do):
  * moi S-box hoat dong co trong so >= 2 (DDT max 8/32; |corr| max 1/2), trong so nguyen;
  * so S-box hoat dong >= B (so nhanh chinh xac, bn.exe)  =>  w >= 2B;
  * w <= 2B+1 chi xay ra khi tong so S-box hoat dong = B. Voi B = 5, cac cap (a_vao, a_ra) co tong 5 deu co
    a_vao <= 2 hoac a_ra <= 2 -> liet ke HET: mau <= 2 cot o dau vao (qua L) va <= 2 cot o dau ra (qua L^-1).
    Xoay dong thoi 5 tu giao hoan voi L (Ascon, SMRM) -> co dinh cot dau tien = 0.
  * Vi sai: x = hieu ra S vong 1, L(x) = hieu vao S vong 2. Trong so vong 1 cua cot b = min_a w(a->b) (DDT theo cot),
    vong 2 cua cot a = min_b w(a->b) (DDT theo hang). Tuyen tinh: v = mat na vao S vong 2, u = L^T v = mat na ra S vong 1.
Chay: python smrm/trail2r_exact.py   (Ascon phai ra 8 / 8 -- doi chung)
"""
import math
from sbox import ddt, lat
from layers import SMRM, ascon_pL, W, NW

D, LT = ddt(), lat()
dw = lambda c: -math.log2(c / 32)
lw = lambda c: -2 * math.log2(abs(c) / 16)
D_FW = {a: min(dw(D[a][b]) for b in range(32) if D[a][b]) for a in range(1, 32)}    # cot vao a -> ra tot nhat
D_BW = {b: min(dw(D[a][b]) for a in range(32) if D[a][b]) for b in range(1, 32)}    # cot ra b <- vao tot nhat
L_IN = {a: min(lw(LT[a][b]) for b in range(1, 32) if LT[a][b]) for a in range(1, 32)}  # mat na vao a
L_OUT = {b: min(lw(LT[a][b]) for a in range(1, 32) if LT[a][b]) for b in range(1, 32)}  # mat na ra b


def cols(st):
    return [sum(((st[i] >> c) & 1) << i for i in range(NW)) for c in range(W)
            if any((st[i] >> c) & 1 for i in range(NW))]


def place(vp):
    st = [0] * NW
    for v, c in vp:
        for i in range(NW):
            if (v >> i) & 1:
                st[i] |= 1 << c
    return st


def transpose(f):
    img = [f(place([(1 << w_, b)])) for w_ in range(NW) for b in range(W)]

    def T(st):
        out = [0] * NW
        for w_ in range(NW):
            for b in range(W):
                par = sum(bin(y & s).count("1") for y, s in zip(img[w_ * W + b], st)) & 1
                out[w_] |= par << b
        return out
    return T


def patterns():
    yield from ([(v, 0)] for v in range(1, 32))
    yield from ([(v, 0), (u, c)] for c in range(1, W) for v in range(1, 32) for u in range(1, 32))


def best2r(fwd, inv, w_first, w_second):
    """min trong so tren cac cap (s, fwd(s)) voi s hoac fwd(s) co <= 2 cot; tra {tong S-box: (w, a1, a2)}."""
    best = {}
    for p in patterns():
        s = place(p)
        for first, second in ((s, fwd(s)), (inv(s), s)):
            a, b = cols(first), cols(second)
            w = sum(w_first[c] for c in a) + sum(w_second[c] for c in b)
            k = len(a) + len(b)
            if k not in best or w < best[k][0]:
                best[k] = (w, len(a), len(b))
    return best


def report(name, f, finv):
    out = []
    for kind, fwd, inv, w1, w2 in (("vi sai", f, finv, D_BW, D_FW),
                                    ("tuyen tinh", transpose(f), transpose(finv), L_OUT, L_IN)):
        best = best2r(fwd, inv, w1, w2)
        B = min(best)
        w, a1, a2 = best[B]
        exact = w <= 2 * B + 1          # moi vet khac co >= B+1 S-box -> w >= 2B+2
        out.append(f"{name:8s} {kind:10s}: B = {B}, cuc tieu tren vet {B} S-box = {w:g} (a1={a1}, a2={a2})"
                   f"  =>  w_2R {'= ' + format(w, 'g') + ' CHINH XAC' if exact else '>= ' + str(2 * B + 2) + ' (chua chot)'}")
    return out


if __name__ == "__main__":
    L6 = SMRM([(1, 4, 45), (4, 3, 31), (3, 2, 23), (2, 0, 15), (0, 1, 22), (1, 4, 21)], [0, 39, 2, 16, 7], "L_Ascon2")
    print("S-box Ascon: min w vi sai =", min(D_FW.values()), ", min w tuyen tinh =", min(L_IN.values()))

    # nghich dao p_L Ascon bang khu Gauss tren 320 bit
    def gf2_inverse(f):
        n = NW * W
        rows = []
        for w_ in range(NW):
            for b in range(W):
                y = f(place([(1 << w_, b)]))
                v = 0
                for i in range(NW):
                    v |= y[i] << (i * W)
                rows.append(v)                         # cot j cua ma tran = anh cua e_j
        # giai: tim ma tran nghich dao qua [M | I] theo cot -> lam tren chuyen vi
        aug = [(rows[j], 1 << j) for j in range(n)]
        for bit in range(n):
            piv = next(k for k in range(bit, n) if (aug[k][0] >> bit) & 1)
            aug[bit], aug[piv] = aug[piv], aug[bit]
            for k in range(n):
                if k != bit and (aug[k][0] >> bit) & 1:
                    aug[k] = (aug[k][0] ^ aug[bit][0], aug[k][1] ^ aug[bit][1])
        # aug[bit] = (e_bit, to hop cac cot cho ra e_bit) => M^-1 e_bit = to hop do
        def inv(st):
            v = 0
            for i in range(NW):
                v |= st[i] << (i * W)
            r = 0
            for bit in range(n):
                if (v >> bit) & 1:
                    r ^= aug[bit][1]
            return [(r >> (i * W)) & ((1 << W) - 1) for i in range(NW)]
        return inv

    lines = report("Ascon", ascon_pL, gf2_inverse(ascon_pL)) + report("L_Ascon2", L6, L6.inverse)
    for s in lines:
        print(s)
