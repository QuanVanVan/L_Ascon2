"""
rank_candidates.py -- Xep hang ung vien SMRM theo vet 3 vong (trail3.exe, mo hinh S-box chinh xac).
Doc cac dong "BEST k=.. m=.. ops=.. d=.." tu file ket qua cua opt_smrm, xuat ma tran, chay trail3
(a1 <= h) cho vi sai va tuyen tinh, in bang xep hang theo min(trong so vi sai, trong so tuyen tinh).
Su dung: python rank_candidates.py <file_opt> <h> [ascon|chi]
"""
import os
import re
import subprocess
import sys

from layers import SMRM, matrix_columns, write_matrix

HERE = os.path.dirname(os.path.abspath(__file__))
MAT = os.path.join(HERE, "..", "results", "mat")
os.makedirs(MAT, exist_ok=True)


def trail3(path, mode, h, sb):
    out = subprocess.run([os.path.join(HERE, "trail3.exe"), path, mode, str(h), sb],
                         capture_output=True, text=True).stdout
    act = int(re.search(r"active-sboxes: min over trails with a1<=\d+ = (\d+)", out).group(1))
    wt = int(re.search(r"weight\s+: min over trails with a1<=\d+ = (\d+)", out).group(1))
    return act, wt


def main():
    fn, h = sys.argv[1], int(sys.argv[2])
    sb = sys.argv[3] if len(sys.argv) > 3 else "ascon"
    rows = []
    for line in open(fn):
        m = re.match(r"BEST k=(\d+) chain=(\d+) m=(\d+) .*ops=(\S+) d=(\S+)", line)
        if not m:
            continue
        k, ch, bm, ops_s, d_s = int(m[1]), int(m[2]), int(m[3]), m[4], m[5]
        ops = [tuple(map(int, o.split(","))) for o in ops_s.split(";")]
        d = list(map(int, d_s.split(",")))
        name = f"smrm_k{k}_c{ch}_{os.path.basename(fn).split('.')[0]}"
        path = os.path.join(MAT, name + ".bin")
        write_matrix(matrix_columns(SMRM(ops, d)), path)
        da, dw = trail3(path, "diff", h, sb)
        la, lw = trail3(path, "lin", h, sb)
        rows.append((min(dw, lw), k, ch, bm, da, dw, la, lw, ops_s, d_s))
        print(f"k={k} chain={ch} B<={bm}  diff: {da} act / w {dw}   lin: {la} act / w {lw}", flush=True)
    rows.sort(reverse=True)
    print("\nRANKING (by min(diff weight, lin weight), a1<=%d, sbox=%s)" % (h, sb))
    for r in rows:
        print(f"  minW={r[0]:3d} k={r[1]} chain={r[2]} B={r[3]} diff={r[4]}/{r[5]} lin={r[6]}/{r[7]}  ops={r[8]} d={r[9]}")


if __name__ == "__main__":
    main()
