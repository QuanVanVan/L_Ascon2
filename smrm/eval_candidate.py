"""Danh gia day du mot ung vien SMRM: ma tran, kiem tra L^bT = L, so nhanh chinh xac (bn.exe), mat do.
Su dung: python eval_candidate.py "<ops>" "<d>" h [name]
"""
import os
import subprocess
import sys

from layers import (SMRM, N, W, NW, matrix_columns, inverse_cols, transpose_cols, write_matrix, density,
                    int_to_state, state_to_int, rotl)

HERE = os.path.dirname(os.path.abspath(__file__))


def parse(ops_s, d_s):
    ops = [tuple(map(int, o.split(","))) for o in ops_s.split(";")]
    d = list(map(int, d_s.split(",")))
    return SMRM(ops, d)


def block_transpose_cols(cols):
    """M^bT: khoi (i,j) <- khoi (j,i) (khong dao X). Tinh qua dinh nghia: M^bT = P M^T P,
    P dao nguoc vi tri bit p -> -p mod 64 trong moi tu."""
    def P(v):
        s = int_to_state(v)
        out = []
        for x in s:
            y = 0
            for p in range(W):
                if (x >> p) & 1:
                    y |= 1 << ((-p) % W)
            out.append(y)
        return state_to_int(out)
    T = transpose_cols(cols)
    # (P T P) e_k = P T e_{P(k)}
    res = []
    for k in range(N):
        w, p = divmod(k, W)
        kk = w * W + ((-p) % W)
        res.append(P(T[kk]))
    return res


def main():
    L = parse(sys.argv[1], sys.argv[2])
    h = sys.argv[3] if len(sys.argv) > 3 else "3"
    name = sys.argv[4] if len(sys.argv) > 4 else "cand"
    cols = matrix_columns(L)
    inv = inverse_cols(cols)
    # kiem tra nghich dao bang chuong trinh
    inv_prog = matrix_columns(L.inverse)
    assert inv == inv_prog, "nghich dao Gauss khac nghich dao bang chuong trinh!"
    sym = block_transpose_cols(cols) == cols
    print(f"{name}: k={len(L.ops)} XOR/bit={2*len(L.ops)/NW:.1f}  L^bT == L: {sym}  "
          f"density(L)={density(cols)} density(L^-1)={density(inv)}")
    os.makedirs(os.path.join(HERE, "..", "results", "mat"), exist_ok=True)
    path = os.path.join(HERE, "..", "results", "mat", name + ".bin")
    write_matrix(cols, path)
    out = subprocess.run([os.path.join(HERE, "bn.exe"), path, h], capture_output=True, text=True).stdout
    print(out, end="")


if __name__ == "__main__":
    main()
