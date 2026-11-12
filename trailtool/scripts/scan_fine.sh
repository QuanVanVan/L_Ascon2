#!/bin/bash
# scan_fine.sh <NAME> <DTS_T0> <LTS_T0> [step=2] [budget_seconds=172800] [outroot]
#   Quet tiep 3 vong cho MOT ung vien (ten trong results/02_finalists/finalists.txt) voi buoc nguong nho
#   (mac dinh 2) va ngan sach lon, de tim toi thieu CHINH XAC thay vi chi can duoi.
#   DTS_T0 / LTS_T0: nguong dau tien can quet (= can duoi da chung minh hien co); "-" de bo qua mot che do.
#   Moi lan chon che do co lan quet truoc RE hon (thoi gian lan quet day du gan nhat nho hon) => hai can duoi
#   cung tang. Mot che do dung khi (a) tim thay loi (toi thieu chinh xac) hoac (b) het ngan sach (rc=124).
#   Ket qua ghi them vao <outroot>/SUMMARY.txt cung dinh dang voi run_all_3r.sh; chay lai se dung lai cac
#   nguong da xong (resume).
# Vi du: bash scan_fine.sh L_Ascon2 42 46
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
name=$1; T_d=$2; T_l=$3; step=${4:-2}; budget=${5:-172800}
root=${6:-$PRJ_ROOT/results/06_trail3r/run_3r}
summary=$root/SUMMARY.txt
spec="file:$FINALISTS_W#$name"
declare -A T=([DTS]=$T_d [LTS]=$T_l) last=([DTS]=0 [LTS]=0) done=([DTS]=0 [LTS]=0)
[ "$T_d" = - ] && done[DTS]=1; [ "$T_l" = - ] && done[LTS]=1
while [ ${done[DTS]} -eq 0 ] || [ ${done[LTS]} -eq 0 ]; do
  if [ ${done[DTS]} -eq 1 ]; then mode=LTS
  elif [ ${done[LTS]} -eq 1 ]; then mode=DTS
  elif [ ${last[DTS]} -le ${last[LTS]} ]; then mode=DTS; else mode=LTS; fi
  t=${T[$mode]}; d=$root/$name/$mode/T$t
  prev=$(grep -F "$name $mode T3<=$t rc=" "$summary" 2>/dev/null | tail -1)
  case "$prev" in
    "") ;;
    *"rc=124"*)  # het ngan sach: chi coi la xong neu ngan sach cu >= ngan sach hien tai
      pel=$(echo "$prev" | sed -n 's/.*elapsed=\([0-9]*\)s.*/\1/p')
      [ "${pel:-0}" -ge "$budget" ] && { done[$mode]=1; continue; } ;;
    *"min3R=none"*) T[$mode]=$((t + step)); continue ;;
    *"RESULT"*) done[$mode]=1; continue ;;
  esac
  start=$(date +%s)
  timeout $budget bash "$TT_SCRIPTS/scan3r.sh" "$d" "$spec" $mode $t generic > /dev/null 2>&1
  rc=$?
  [ $rc -eq 124 ] && taskkill //F //IM t_dts.exe > /dev/null 2>&1; [ $rc -eq 124 ] && taskkill //F //IM t_lts.exe > /dev/null 2>&1
  el=$(( $(date +%s) - start ))
  res=$(grep RESULT "$d/scan_${mode}_T$t.log" 2>/dev/null)
  echo "$name $mode T3<=$t rc=$rc elapsed=${el}s :: $res" | tee -a "$summary"
  last[$mode]=$el
  if [ $rc -ne 0 ] || [ -z "$res" ]; then done[$mode]=1; continue; fi
  minw=$(echo "$res" | sed -n 's/.*min3R=\([0-9a-z]*\).*/\1/p')
  if [ "$minw" != "none" ]; then done[$mode]=1; else T[$mode]=$((t + step)); fi
done
