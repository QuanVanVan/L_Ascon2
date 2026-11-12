"""
trail_sat.py -- Mo hinh SAT tim SO S-BOX HOAT DONG TOI THIEU cua vet vi sai / tuyen tinh R vong
cho ham vong  R(x) = L(S(x))  tren trang thai 5 x 64 (S-box Ascon ap len 64 cot).

Mo hinh (chung minh tinh dung/day du: docs/methods/README_methods.md, muc "trail_sat"):
  Bien: a_r[k], b_r[k]  (k = w*64 + p): hieu (mat na) o dau vao / dau ra S-layer vong r.
  * Vi sai :  a_{r+1} = L . b_r                 (moi bit = XOR cac bit tren hang tuong ung cua L)
  * Tuyen tinh: b_r = L^T . a_{r+1}             (mat na di nguoc qua tang tuyen tinh)
  * S-box (che do 'exact'): voi moi cot, cam moi cap (din, dout) co DDT = 0 (hoac LAT = 0)
                            -> mo hinh CHINH XAC tap vet kha thi o muc cot.
           (che do 'relaxed'): chi rang buoc  din != 0  <=>  dout != 0.
  * act_r[j] <=> cot j cua a_r khac 0.  Rang buoc  sum act <= K  bang bo dem tuan tu (Sinz 2005).
  * Pha doi xung: act_0[0] = 1 (hop le vi L va S-layer giao hoan voi phep xoay dong thoi cac tu).
  Bai toan SAT <=> ton tai vet R vong khong tam thuong co <= K S-box hoat dong.
  Gia tri toi thieu = K nho nhat cho SAT; moi K nho hon da duoc chung minh UNSAT.

Su dung: python trail_sat.py <layer> <R> <diff|lin> [exact|relaxed] [Kstart] [timeout_s]
"""
import os
import subprocess
import sys
import time

from layers import N, W, NW, matrix_columns, columns_to_rows
from sbox import ddt, lat

Z3 = "z3"


class CNF:
    def __init__(self):
        self.nv = 0
        self.cl = []

    def var(self):
        self.nv += 1
        return self.nv

    def add(self, c):
        self.cl.append(c)

    def xor_eq(self, lits, parity=0):
        """XOR(lits) = parity. Tach thanh cac khoi <= 4 bien bang bien phu."""
        lits = list(lits)
        while len(lits) > 4:
            t = self.var()
            self._xor_small(lits[:3] + [t], 0)       # t = l1 ^ l2 ^ l3
            lits = [t] + lits[3:]
        self._xor_small(lits, parity)

    def _xor_small(self, lits, parity):
        n = len(lits)
        if n == 0:
            if parity:
                self.add([])
            return
        for m in range(1 << n):
            # cam gan gia tri co parity sai: gan lits[i] = bit i cua m
            if bin(m).count("1") % 2 != parity:
                self.add([(-lits[i] if (m >> i) & 1 else lits[i]) for i in range(n)])

    def at_most(self, xs, K):
        """Bo dem tuan tu Sinz: sum xs <= K."""
        n = len(xs)
        if K >= n:
            return
        if K == 0:
            for x in xs:
                self.add([-x])
            return
        s = [[self.var() for _ in range(K)] for _ in range(n)]
        self.add([-xs[0], s[0][0]])
        for j in range(1, K):
            self.add([-s[0][j]])
        for i in range(1, n):
            self.add([-xs[i], s[i][0]])
            self.add([-s[i - 1][0], s[i][0]])
            for j in range(1, K):
                self.add([-xs[i], -s[i - 1][j - 1], s[i][j]])
                self.add([-s[i - 1][j], s[i][j]])
            self.add([-xs[i], -s[i - 1][K - 1]])

    def write(self, path):
        with open(path, "w") as fh:
            fh.write(f"p cnf {self.nv} {len(self.cl)}\n")
            for c in self.cl:
                fh.write(" ".join(map(str, c)) + " 0\n")


def build(cols, R, mode, sbox_model, K):
    cnf = CNF()
    a = [[cnf.var() for _ in range(N)] for _ in range(R)]
    b = [[cnf.var() for _ in range(N)] for _ in range(R)]
    act = [[cnf.var() for _ in range(W)] for _ in range(R)]
    table = ddt() if mode == "diff" else lat()
    ok = [[table[x][y] != 0 for y in range(32)] for x in range(32)]
    for r in range(R):
        for j in range(W):
            ins = [a[r][w * W + j] for w in range(NW)]
            outs = [b[r][w * W + j] for w in range(NW)]
            # act <=> OR(ins)
            cnf.add([-act[r][j]] + ins)
            for v in ins:
                cnf.add([-v, act[r][j]])
            if sbox_model == "relaxed":
                cnf.add([-act[r][j]] + outs)
                for v in outs:
                    cnf.add([-v, act[r][j]])
            else:
                # gia tri 5 bit: bit (4 - w) <-> tu w
                for x in range(32):
                    for y in range(32):
                        if ok[x][y]:
                            continue
                        c = []
                        for w in range(NW):
                            c.append(-ins[w] if (x >> (4 - w)) & 1 else ins[w])
                            c.append(-outs[w] if (y >> (4 - w)) & 1 else outs[w])
                        cnf.add(c)
    rows = columns_to_rows(cols)
    for r in range(R - 1):
        for q in range(N):
            if mode == "diff":
                src = rows[q]                      # a_{r+1}[q] = XOR_{p in row q} b_r[p]
                lits = [b[r][p] for p in range(N) if (src >> p) & 1] + [a[r + 1][q]]
            else:
                src = cols[q]                      # b_r[q] = XOR_{p in col q of L} a_{r+1}[p]
                lits = [a[r + 1][p] for p in range(N) if (src >> p) & 1] + [b[r][q]]
            cnf.xor_eq(lits, 0)
    cnf.add([act[0][0]])
    flat = [v for r in range(R) for v in act[r]]
    cnf.at_most(flat, K)
    return cnf, act


KISSAT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "kissat.exe")


def solve(cnf, path, timeout):
    """Giai bang Kissat 4.0.2 (neu co), nguoc lai Z3. Tra ve (sat|unsat|timeout, model, thoi_gian)."""
    cnf.write(path)
    t0 = time.time()
    use_kissat = os.path.exists(KISSAT)
    cmd = [KISSAT, "-q", path] if use_kissat else [Z3, "-dimacs", f"-T:{timeout}", path]
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout).stdout
    except subprocess.TimeoutExpired:
        return "timeout", None, time.time() - t0
    dt = time.time() - t0
    lines = out.strip().splitlines()
    status = next((l for l in lines if l.startswith("s ") or l.strip() in ("sat", "unsat")), "")
    if "UNSAT" in status or status.strip() == "unsat":
        return "unsat", None, dt
    if "SATISFIABLE" in status or status.strip() == "sat":
        model = set()
        for line in lines:
            if line.startswith("v") or not use_kissat:
                for tok in line.replace("v", " ").split():
                    try:
                        v = int(tok)
                    except ValueError:
                        continue
                    if v > 0:
                        model.add(v)
        return "sat", model, dt
    return "unknown", None, dt


def min_active(cols, R, mode, sbox_model="exact", kstart=1, timeout=3600, tag="x", log=print):
    tmp = os.path.join(os.environ.get("TRAIL_TMP", "."), f"_{tag}_{R}_{mode}.cnf")
    K = kstart
    proven = None          # can duoi DA CHUNG MINH (K+1 khi K bao UNSAT)
    while True:
        cnf, act = build(cols, R, mode, sbox_model, K)
        st, model, dt = solve(cnf, tmp, timeout)
        log(f"  [{tag}] R={R} {mode} {sbox_model} K<={K}: {st} ({dt:.1f}s, {cnf.nv} vars, {len(cnf.cl)} clauses)")
        if st == "sat":
            pattern = [sum(1 for v in act[r] if v in model) for r in range(R)]
            return K, pattern
        if st != "unsat":
            lb = f"proven lower bound {proven}" if proven else "no lower bound proven in this run"
            return None, f"stopped at K={K} ({st}); {lb}"
        proven = K + 1
        K += 1


def prove_min(cols, R, mode, U, sbox_model="exact", timeout=3600, tag="x", log=print):
    """U = so S-box cua mot vet DA BIET ton tai (vd tu trail3). Chung minh toi uu bang cach kiem tra
    K = U-1: UNSAT => min = U (chinh xac); SAT => co vet nhe hon, ha U va lap lai."""
    tmp = os.path.join(os.environ.get("TRAIL_TMP", "."), f"_{tag}_{R}_{mode}_p.cnf")
    while True:
        K = U - 1
        cnf, act = build(cols, R, mode, sbox_model, K)
        st, model, dt = solve(cnf, tmp, timeout)
        log(f"  [{tag}] R={R} {mode} {sbox_model} K<={K}: {st} ({dt:.1f}s)")
        if st == "unsat":
            return U, "EXACT"
        if st == "sat":
            U = sum(1 for r in range(R) for v in act[r] if v in model)
            continue
        return U, f"upper bound only (K={K} {st})"


def get_layer(name):
    import layers
    if name == "ascon":
        return matrix_columns(layers.ascon_pL)
    if name == "hsilu":
        return matrix_columns(layers.HSILU)
    if name == "mrm12":
        return matrix_columns(layers.MRM12)
    if name.startswith("smrm:"):          # smrm:i,j,r;i,j,r;...|d0,d1,d2,d3,d4
        ops_s, d_s = name[5:].split("|")
        ops = [tuple(map(int, o.split(","))) for o in ops_s.split(";")]
        d = list(map(int, d_s.split(",")))
        return matrix_columns(layers.SMRM(ops, d))
    raise ValueError(name)


if __name__ == "__main__":
    if sys.argv[1] == "prove":          # python trail_sat.py prove <layer> <R> <mode> <U> [timeout]
        layer, R, mode, U = sys.argv[2], int(sys.argv[3]), sys.argv[4], int(sys.argv[5])
        to = int(sys.argv[6]) if len(sys.argv) > 6 else 3600
        cols = get_layer(layer)
        val, st = prove_min(cols, R, mode, U, "exact", to, tag=layer.split(":")[0].split("|")[0][:12])
        print(f"RESULT-PROVE layer={layer[:70]} R={R} mode={mode} min_active={val} {st}")
        sys.exit(0)
    layer, R, mode = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    model = sys.argv[4] if len(sys.argv) > 4 else "exact"
    k0 = int(sys.argv[5]) if len(sys.argv) > 5 else 1
    to = int(sys.argv[6]) if len(sys.argv) > 6 else 3600
    cols = get_layer(layer)
    K, pat = min_active(cols, R, mode, model, k0, to, tag=layer.split(":")[0])
    print(f"RESULT layer={layer} R={R} mode={mode} model={model} min_active={K} pattern={pat}")
