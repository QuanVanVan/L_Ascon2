"""export_finalists.py -- ghi ma tran 320x320 (results/mat/<TEN>.bin) cua moi ung vien trong
results/02_finalists/finalists.txt (vd. L_Ascon2.bin), dinh dang giong eval_candidate.py nhung KHONG chay bn.exe.
Dung cho: trailtool/tests (layer "matrix:"), smrm/trail3.exe, smrm/bn.exe.
Kiem tra truoc khi ghi: nghich dao Gauss == nghich dao bang chuong trinh, va L^bT == L (dinh ly SMRM).
Chay: python export_finalists.py [finalists.txt]
"""
import os
import sys

from eval_candidate import parse, block_transpose_cols
from layers import matrix_columns, inverse_cols, write_matrix

HERE = os.path.dirname(os.path.abspath(__file__))
FIN = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "..", "results", "02_finalists", "finalists.txt")
OUT = os.path.join(HERE, "..", "results", "mat")


def main():
    os.makedirs(OUT, exist_ok=True)
    for line in open(FIN):
        line = line.strip()
        if not line:
            continue
        name, ops, d = line.split("|")
        L = parse(ops, d)
        cols = matrix_columns(L)
        assert inverse_cols(cols) == matrix_columns(L.inverse), f"{name}: nghich dao sai"
        assert block_transpose_cols(cols) == cols, f"{name}: L^bT != L"
        write_matrix(cols, os.path.join(OUT, name + ".bin"))
        print(f"{name:9s} -> results/mat/{name}.bin  (L^bT == L, nghich dao kiem tra)")


if __name__ == "__main__":
    main()
