#!/bin/bash
# run_all.sh [TANG ...]  -- bien dich + chay toan bo kiem thu; log: results/run_all.log
#   Khong doi so: Ascon goc + L_Ascon2.   "all": Ascon goc + ca 6 ung vien trong results/02_finalists/finalists.txt.
# Moi buoc in PASS/FAIL; ma thoat != 0 neu co buoc nao that bai.
cd "$(dirname "${BASH_SOURCE[0]}")"
export PATH="/c/msys64/ucrt64/bin:$PATH"
PY=python.exe
LOG=results/run_all.log
mkdir -p results
layers=("$@"); [ ${#layers[@]} -eq 0 ] && layers=(L_Ascon2)
[ "${layers[0]}" = all ] && layers=($(cut -d'|' -f1 ../results/02_finalists/finalists.txt))
fail=0
step() { echo; echo "=== $*"; }
{
  echo "run_all.sh  $(date)   tang: ${layers[*]}"
  step "Bien dich"; bash build.sh || fail=1

  step "1. Ascon goc, Python: 1089 vector chinh thuc"; $PY python/test_kat.py || fail=1
  step "2. Ascon goc, C: 1089 vector chinh thuc"; ./c/ascon_tool.exe kat kat/LWC_AEAD_KAT_128_128.txt ascon || fail=1
  ./c/ascon_tool.exe gen results/c_ascon_KAT_regen.txt ascon > /dev/null
  if cmp -s results/c_ascon_KAT_regen.txt kat/LWC_AEAD_KAT_128_128.txt; then echo "C sinh lai KAT Ascon: TRUNG TUNG BYTE tep chinh thuc"
  else echo "C sinh lai KAT Ascon: KHAC tep chinh thuc -- FAIL"; fail=1; fi

  for L in "${layers[@]}"; do
    step "3. $L, Python: tang tuyen tinh + AEAD (giai ma dung, tu choi sua doi)"; $PY python/test_smrm_aead.py "$L" || fail=1
    step "4. $L, C: sinh KAT, doi chieu voi Python, kiem tra lai"
    ./c/ascon_tool.exe gen "results/c_${L}_KAT.txt" "$L"
    if cmp -s "results/c_${L}_KAT.txt" "results/py_${L}_KAT.txt"; then echo "KAT C == KAT Python (trung tung byte, 1089 vector)"
    else echo "KAT C != KAT Python -- FAIL"; fail=1; fi
    ./c/ascon_tool.exe kat "results/py_${L}_KAT.txt" "$L" || fail=1
    step "5. $L, demo"; ./c/ascon_tool.exe demo "$L" || fail=1
  done
  echo; [ $fail -eq 0 ] && echo "TONG KET: TAT CA PASS" || echo "TONG KET: CO BUOC FAIL"
} 2>&1 | tee "$LOG"
exit ${PIPESTATUS[0]}
