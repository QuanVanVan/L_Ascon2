#!/bin/bash
# run_all_3r.sh [outroot] [budget_seconds]
#   Mac dinh: outroot = DATN/results/06_trail3r/run_3r, budget = 2400 s cho MOT lan quet (mot nguong T3).
# Quet 3 vong cho Ascon va 6 ung vien SMRM (results/02_finalists/finalists.txt), tang dan T3 cho den khi
#   (a) tim thay loi (=> trong so toi thieu CHINH XAC), hoac
#   (b) mot lan quet vuot ngan sach (bi dung bang timeout, rc=124) => chi giu can duoi cua lan quet
#       day du cuoi cung.
# CHAY TIEP (resume): ket qua da co trong <outroot>/SUMMARY.txt duoc dung lai, khong chay lai.
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
root=${1:-$PRJ_ROOT/results/06_trail3r/run_3r}; budget=${2:-2400}
mkdir -p "$root"
summary=$root/SUMMARY.txt
for name in ascon $(cut -d'|' -f1 "$FINALISTS"); do
  if [ $name = ascon ]; then spec=ascon; gen=tree; else spec="file:$FINALISTS_W#$name"; gen=generic; fi
  for mode in DTS LTS; do
    if [ $mode = DTS ]; then Ts="30 35 40 45 50 55 60 65 70 75 80"; else Ts="24 28 32 36 40 44 48 52 56 60 64 68 72"; fi
    for T in $Ts; do
      d=$root/$name/$mode/T$T
      # --- chay tiep (resume): dung lai ket qua da co trong SUMMARY.txt ---
      prev=$(grep -F "$name $mode T3<=$T rc=" "$summary" 2>/dev/null | tail -1)
      if [ -n "$prev" ]; then
        case "$prev" in
          *"rc=124"*) break ;;                                   # lan truoc da het ngan sach
          *"min3R=none"*) continue ;;                            # da quet day du, rong
          *"RESULT"*) break ;;                                   # da tim thay toi thieu chinh xac
        esac
      fi
      start=$(date +%s)
      timeout $budget bash "$TT_SCRIPTS/scan3r.sh" "$d" "$spec" $mode $T $gen > /dev/null 2>&1
      rc=$?
      [ $rc -eq 124 ] && taskkill //F //IM t_dts.exe > /dev/null 2>&1; [ $rc -eq 124 ] && taskkill //F //IM t_lts.exe > /dev/null 2>&1
      el=$(( $(date +%s) - start ))
      res=$(grep RESULT "$d/scan_${mode}_T$T.log" 2>/dev/null)
      echo "$name $mode T3<=$T rc=$rc elapsed=${el}s :: $res" | tee -a "$summary"
      if [ $rc -ne 0 ] || [ -z "$res" ]; then break; fi
      minw=$(echo "$res" | sed -n 's/.*min3R=\([0-9a-z]*\).*/\1/p')
      if [ "$minw" != "none" ]; then break; fi      # tim thay => toi thieu chinh xac
    done
  done
done
