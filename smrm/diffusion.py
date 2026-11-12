"""
diffusion.py -- So vong khuech tan day du (full diffusion) cua R = L o S, chieu thuan va chieu nghich.
Ma tran phu thuoc Boolean D (320x320): D[i][j] = 1 neu bit ra i phu thuoc (thuc su) bit vao j.
  * S-layer: bit ra (w, p) phu thuoc bit vao (w', p) neu thanh phan Boolean thu w cua S-box phu thuoc
    bien w' (tinh tu bang chan tri: ton tai x voi S(x) != S(x ^ e_w') o bit w).
  * L: D_L = gia cua ma tran L (phu thuoc tuyen tinh = he so khac 0).
  * Mot vong: D_R = D_L o D_S (tich Boolean). r vong: D_R^r. Khuech tan day du khi moi phan tu = 1.
Ghi chu: tich Boolean cho can tren cua phu thuoc that (co the co trieu tieu), la thuoc do tieu chuan
(Ascon, Xoodoo dung cung dinh nghia).
Su dung: python diffusion.py
"""
import os
import sys

from layers import N, W, NW, matrix_columns, columns_to_rows, inverse_cols, ascon_pL, HSILU, SMRM
from sbox import SBOX


def sbox_dep(S):
    inv = [0] * 32
    for x in range(32):
        inv[S[x]] = x
    def dep(T):
        d = [[0] * 5 for _ in range(5)]          # d[out_w][in_w]
        for wi in range(5):
            for x in range(32):
                y = T[x] ^ T[x ^ (1 << (4 - wi))]
                for wo in range(5):
                    if (y >> (4 - wo)) & 1:
                        d[wo][wi] = 1
        return d
    return dep(S), dep(inv)


def rows_S(d):
    """Hang cua ma tran phu thuoc S-layer 320x320."""
    rows = [0] * N
    for wo in range(NW):
        for p in range(W):
            r = 0
            for wi in range(NW):
                if d[wo][wi]:
                    r |= 1 << (wi * W + p)
            rows[wo * W + p] = r
    return rows


def bool_mul(A, B):
    """(A o B)[i] = OR_{j in A[i]} B[j]  (hang dang bitmask)."""
    out = []
    for a in A:
        r = 0
        while a:
            lb = a & -a
            r |= B[lb.bit_length() - 1]
            a ^= lb
        out.append(r)
    return out


def full_rounds(Lrows, Srows, maxr=10, first="S"):
    """So vong toi thieu de D_R^r day du; vong = L sau S (thuan) hoac S^-1 sau L^-1 (nghich)."""
    full = (1 << N) - 1
    R1 = bool_mul(Lrows, Srows) if first == "S" else bool_mul(Srows, Lrows)
    D = R1
    for r in range(1, maxr + 1):
        if all(x == full for x in D):
            return r
        D = bool_mul(R1, D)
    return None


def evaluate(name, f):
    cols = matrix_columns(f)
    Lrows = columns_to_rows(cols)
    Lirows = columns_to_rows(inverse_cols(cols))
    dS, dSi = sbox_dep(SBOX)
    fwd = full_rounds(Lrows, rows_S(dS), first="S")          # R = L o S
    bwd = full_rounds(Lirows, rows_S(dSi), first="L")        # R^-1 = S^-1 o L^-1
    print(f"{name:10s} full diffusion: forward {fwd} rounds, backward {bwd} rounds")


if __name__ == "__main__":
    evaluate("Ascon", ascon_pL)
    evaluate("Hsilu-MRM", HSILU)
    for line in open(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "results", "02_finalists", "finalists.txt")):
        nm, o, d = line.strip().split("|")
        ops = [tuple(map(int, t.split(","))) for t in o.split(";")]
        evaluate(nm, SMRM(ops, list(map(int, d.split(",")))))
