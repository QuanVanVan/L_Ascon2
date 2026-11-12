#!/bin/bash
# build.sh -- bien dich cac chuong trinh C cua smrm/ (dat .exe canh ma nguon, cac script Python goi
# chung theo duong dan tuong doi). Trinh bien dich: MSYS2 UCRT64 gcc 13, lien ket tinh.
set -e
cd "$(dirname "${BASH_SOURCE[0]}")"
export PATH="/c/msys64/ucrt64/bin:$PATH"
mkdir -p tmp; export TMP="$(cygpath -w "$PWD/tmp")" TEMP="$(cygpath -w "$PWD/tmp")"
F="-O3 -march=native -fopenmp -static"
# smrm_gen.h (ma chuyen biet cho perm_bench2.c) SINH TU finalists.txt, khong luu trong kho ma
python gen_smrm_c.py ../results/02_finalists/finalists.txt
for p in bn trail3 search_smrm opt_smrm opt_trail perm_bench perm_bench2; do
  gcc $F $p.c -o $p.exe && echo "built $p.exe"
done
