#!/bin/bash
# build.sh -- bien dich hai chuong trinh chinh vao trailtool/bin/:
#   t_dts.exe : vet vi sai      t_lts.exe : vet tuyen tinh
# va chay tu kiem tra tang tuyen tinh (checklayer) cho Ascon va 6 ung vien SMRM.
set -e
source "$(dirname "${BASH_SOURCE[0]}")/scripts/env.sh"
mkdir -p "$TT_BIN"
bash "$TT_SCRIPTS/build_variant.sh" "$TT_BIN/t_dts.exe" 1 40 11
bash "$TT_SCRIPTS/build_variant.sh" "$TT_BIN/t_lts.exe" 0 40 11
echo "built: $TT_BIN/t_dts.exe $TT_BIN/t_lts.exe"
for L in ascon ascon-generic $(cut -d'|' -f1 "$FINALISTS"); do
  # moi ten khac ascon / ascon-generic la ung vien trong finalists.txt (vd. SMRM-5, L_Ascon2)
  spec=$L; case $L in ascon|ascon-generic) ;; *) spec="file:$FINALISTS_W#$L";; esac
  for e in t_dts t_lts; do
    printf "%-14s %s: " "$L" "$e"; "$TT_BIN/$e.exe" "--layer=$spec" checklayer | grep -E "checklayer|error"
  done
done
