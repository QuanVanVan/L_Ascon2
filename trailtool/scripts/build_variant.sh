#!/bin/bash
# build_variant.sh <outexe> <DTS> <LIMIT_WEIGHT> <WEIGHTED_WEIGHTS> [extra g++ flags]
#   DTS=1 vet vi sai, DTS=0 vet tuyen tinh. LIMIT_WEIGHT/WEIGHTED_WEIGHTS chi dung cho lenh "generate"
#   (cay duyet goc cua bai bao, chi cho Ascon); "generic2r", "extend*" nhan nguong tu dong lenh.
# Lien ket TINH la bat buoc tren may nay: neu khong, chuong trinh nap libstdc++-6.dll cua Git-for-Windows
# (phien ban GCC khac) va bi segfault (loi ABI).
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
out=$1; dts=$2; lw=$3; ww=$4; shift 4
g++ -O3 -std=c++17 -Wall -Wextra -Wno-write-strings -Wno-deprecated-declarations -static -fopenmp \
    -DDTS=$dts -DLIMIT_WEIGHT=$lw -DWEIGHTED_WEIGHTS=$ww "$@" "$TT_SRC"/*.cpp -o "$out"
