"""
milp_trail.py -- Mo hinh MILP (Mouha et al. 2011; Sun et al. 2014) cho so S-box hoat dong toi thieu
cua vet vi sai/tuyen tinh R vong, giai bang HiGHS. Khi dung theo gioi han thoi gian, HiGHS bao
 - "dual bound"  : CAN DUOI hop le cua so S-box hoat dong toi thieu,
 - "primal bound": so S-box cua mot vet thuc su (CAN TREN).
Hai gia tri trung nhau <=> toi uu da duoc chung minh.

Mo hinh (moi rang buoc deu la dieu kien CAN cua mot vet hop le -> gia tri toi uu cua MILP <= gia tri
that, nen dual bound la can duoi hop le):
  * bien nhi phan a_r[k], b_r[k] (dau vao / ra S-layer), A_r[j] (S-box j vong r hoat dong).
  * XOR tuyen tinh: sum_{p in S_q} b_r[p] + a_{r+1}[q] = 2 t_{r,q}, t nguyen  (vi sai)
                    sum_{p in col q} a_{r+1}[p] + b_r[q] = 2 t (tuyen tinh: b_r = L^T a_{r+1})
  * S-box: A >= moi bit vao; A <= tong bit vao; A >= moi bit ra; A <= tong bit ra (song anh);
           so nhanh bit cua S-box Ascon = 3:  sum vao + sum ra >= 3 A.
  * pha doi xung: A_0[0] = 1.
Muc tieu: min sum A.
Su dung: python milp_trail.py <layer> <R> <diff|lin> <time_limit_s>
"""
import os
import re
import subprocess
import sys

from layers import N, W, NW, columns_to_rows
from trail_sat import get_layer

HIGHS = "highs"


def write_lp(cols, R, mode, path, sbox_branch=3):
    rows = columns_to_rows(cols)
    cons = []
    binv, intv = [], []

    def a(r, k): return f"a{r}_{k}"
    def b(r, k): return f"b{r}_{k}"
    def A(r, j): return f"A{r}_{j}"
    for r in range(R):
        for k in range(N):
            binv += [a(r, k), b(r, k)]
        for j in range(W):
            binv.append(A(r, j))
            ins = [a(r, w * W + j) for w in range(NW)]
            outs = [b(r, w * W + j) for w in range(NW)]
            for v in ins + outs:
                cons.append(f"{A(r, j)} - {v} >= 0")
            cons.append(" + ".join(ins) + f" - {A(r, j)} >= 0")
            cons.append(" + ".join(outs) + f" - {A(r, j)} >= 0")
            cons.append(" + ".join(ins + outs) + f" - {sbox_branch} {A(r, j)} >= 0")
    ub = {}
    for r in range(R - 1):
        for q in range(N):
            if mode == "diff":
                lits = [b(r, p) for p in range(N) if (rows[q] >> p) & 1] + [a(r + 1, q)]
            else:
                lits = [a(r + 1, p) for p in range(N) if (cols[q] >> p) & 1] + [b(r, q)]
            t = f"t{r}_{q}"
            intv.append(t)
            ub[t] = len(lits) // 2
            cons.append(" + ".join(lits) + f" - 2 {t} = 0")
    with open(path, "w") as fh:
        fh.write("Minimize\n obj: " + " + ".join(A(r, j) for r in range(R) for j in range(W)) + "\n")
        fh.write("Subject To\n")
        for i, c in enumerate(cons):
            fh.write(f" c{i}: {c}\n")
        fh.write(f" sym: {A(0, 0)} = 1\n")
        fh.write("Bounds\n")
        for t in intv:
            fh.write(f" 0 <= {t} <= {ub[t]}\n")
        fh.write("Binary\n " + "\n ".join(binv) + "\n")
        fh.write("General\n " + "\n ".join(intv) + "\n")
        fh.write("End\n")


def run(layer, R, mode, tlim):
    cols = get_layer(layer)
    tag = layer.split(":")[0] if not layer.startswith("smrm:") else "smrm"
    path = os.path.join(os.environ.get("TRAIL_TMP", "."), f"_{tag}_{R}_{mode}.lp")
    write_lp(cols, R, mode, path)
    out = subprocess.run([HIGHS, "--model_file", path, "--time_limit", str(tlim)],
                         capture_output=True, text=True).stdout
    prim = re.search(r"Primal bound\s+([-\d.e+inf]+)", out)
    dual = re.search(r"Dual bound\s+([-\d.e+inf]+)", out)
    stat = re.search(r"Status\s+(.+)", out)
    return (stat.group(1).strip() if stat else "?",
            float(prim.group(1)) if prim else None,
            float(dual.group(1)) if dual else None, out)


if __name__ == "__main__":
    layer, R, mode, tlim = sys.argv[1], int(sys.argv[2]), sys.argv[3], int(sys.argv[4])
    st, p, d, out = run(layer, R, mode, tlim)
    print(f"RESULT layer={layer[:60]} R={R} mode={mode} status={st} upper(primal)={p} lower(dual)={d}")
