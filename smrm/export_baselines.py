"""Xuat ma tran cua cac tang tuyen tinh doi chung (Ascon, Hsilu, MRM Bang 2) de bn.c danh gia."""
import os
import sys
from layers import ascon_pL, HSILU, MRM12, matrix_columns, write_matrix, inverse_cols, density

OUT = os.path.join(os.path.dirname(__file__), "..", "results", "mat")
os.makedirs(OUT, exist_ok=True)

for name, f in [("ascon", ascon_pL), ("hsilu", HSILU), ("mrm12", MRM12)]:
    cols = matrix_columns(f)
    inv = inverse_cols(cols)
    write_matrix(cols, os.path.join(OUT, name + ".bin"))
    print(f"{name:6s} density(M)={density(cols):6d}  density(M^-1)={density(inv) if inv else 'SINGULAR'}")
