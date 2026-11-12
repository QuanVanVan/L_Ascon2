#!/bin/bash
# scan3r.sh <workdir> <layer-spec> <DTS|LTS> <T3> [gen: generic|tree]
# Quet DAY DU khong gian loi vet 3 vong co trong so <= T3 theo chien luoc cua [El Hirch et al., Sec. 6.1]:
#   (i)  w0 <  w2 : loi 2 vong (a1,b1) voi 2w0+w1 <= T3-1 (vi sai) / T3-2 (tuyen tinh, trong so chan),
#                   mo rong xuoi 1 vong toi T3 (giu w0 < w2);
#   (ii) w0 >= w2 : loi 2 vong (a2,b2) voi w0+2w1 <= T3, mo rong nguoc 1 vong toi T3 (giu w0 >= w2).
# Khong gian rong <=> moi vet 3 vong co trong so >= T3+1 (vi sai) / T3+2 (tuyen tinh).
# layer-spec: ascon | ascon-generic | smrm:<ops>|<d> | file:<duong dan Windows>#NAME
# gen=generic: bo sinh tong quat (generic2r); gen=tree: cay duyet goc cua bai bao (chi cho Ascon).
set -e
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
w=$1; layer=$2; mode=$3; T=$4; gen=${5:-generic}
exe="$TT_BIN/t_dts.exe"; [ "$mode" = LTS ] && exe="$TT_BIN/t_lts.exe"
dts=1; [ "$mode" = LTS ] && dts=0
mkdir -p "$w"; cd "$w"
if [ "$mode" = DTS ]; then T21=$((T-1)); else T21=$((T-2)); fi
P=$mode-2RTrailCoresUpToWeight
log=scan_${mode}_T$T.log
: > $log
t0=$(date +%s.%N)
if [ "$gen" = tree ]; then
  bash "$TT_SCRIPTS/build_variant.sh" gen21.exe $dts $T21 21 >> $log 2>&1
  bash "$TT_SCRIPTS/build_variant.sh" gen12.exe $dts $T 12 >> $log 2>&1
  ./gen21.exe generate >> $log; ./gen12.exe generate >> $log
  rm -f gen21.exe gen12.exe
else
  "$exe" "--layer=$layer" generic2r 21 $T21 >> $log
  "$exe" "--layer=$layer" generic2r 12 $T >> $log
fi
"$exe" "--layer=$layer" extendWW forward $T >> $log
"$exe" "--layer=$layer" extendWW backward $T >> $log
t1=$(date +%s.%N)
F=${P}${T21}_2w0+w1-forwUpTo$T; B=${P}${T}_w0+2w1-backUpTo$T
n2a=$(wc -l < ${P}${T21}_2w0+w1); n2b=$(wc -l < ${P}${T}_w0+2w1)
nf=$(wc -l < $F); nb=$(wc -l < $B)
minw=$(cat $F $B | awk 'NF{print $2}' | sort -n | head -1)
printf "RESULT layer=%s mode=%s T3<=%d  2R-cores(2w0+w1<=%d)=%d  2R-cores(w0+2w1<=%d)=%d  3R-cores: fwd=%d bwd=%d  min3R=%s  time=%.1fs\n" \
  "$layer" $mode $T $T21 $n2a $T $n2b $nf $nb "${minw:-none}" $(awk "BEGIN{print $t1 - $t0}") | tee -a $log
