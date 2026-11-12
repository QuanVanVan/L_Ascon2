#!/bin/bash
# run_tests.sh -- bo kiem thu cua trailtool (chay tu bat ky dau):
#   1. bang DDT/LAT viet cung cua ban GOC == tinh lai tu S-box (analysis/check_tables.py)
#   2. cong thuc diem cot la can duoi hop le (analysis/check_score.py)
#   3. 4 ham tuyen tinh GOC: L.L^-1 = I, <Lx,y> = <x,L^T y> (tests/test_linear.cpp)
#   4. tang tong quat: ascon-generic == ham viet cung; SMRM C++ == Python tham chieu (test_generic_layer.cpp)
set -e
source "$(dirname "${BASH_SOURCE[0]}")/../scripts/env.sh"
cd "$TT_ROOT/tests"
"$PYTHON" "$TT_ROOT/analysis/check_tables.py"
"$PYTHON" "$TT_ROOT/analysis/check_score.py"
g++ -O2 -static test_linear.cpp ../upstream/reference/C++/src/asconParts.cpp -o test_linear.exe && ./test_linear.exe
"$PYTHON" make_vectors.py
# ma tran results/mat/*.bin (Ascon + 6 ung vien) cho layer "matrix:" -- sinh lai tu ma nguon
(cd "$PRJ_ROOT/smrm" && "$PYTHON" export_baselines.py > /dev/null && "$PYTHON" export_finalists.py > /dev/null)
g++ -O2 -static -std=c++17 test_generic_layer.cpp ../src/genericLayer.cpp ../src/asconParts.cpp -o test_generic_layer.exe && ./test_generic_layer.exe
rm -f test_linear.exe test_generic_layer.exe
echo "ALL TESTS PASSED"
