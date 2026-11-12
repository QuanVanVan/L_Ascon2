#!/bin/bash
# Chung minh toi uu (hoac bao can) so S-box hoat dong 3 vong bang Kissat cho Ascon va cac ung vien chung ket.
# U lay tu trail3 (a1<=3): mot vet voi U S-box DA TON TAI; SAT kiem tra K = U-1.
cd "$(dirname "$0")"
TO=${1:-2400}
python trail_sat.py prove ascon 3 lin 13 $TO
while IFS='|' read nm ops d ud ul; do
  python trail_sat.py prove "smrm:$ops|$d" 3 diff $ud $TO | sed "s/^RESULT-PROVE/RESULT-PROVE $nm/"
  python trail_sat.py prove "smrm:$ops|$d" 3 lin $ul $TO | sed "s/^RESULT-PROVE/RESULT-PROVE $nm/"
done <<LIST
SMRM-5|3,0,32;4,3,56;3,2,47;1,3,43;3,1,2|0,54,42,22,45|18|17
SMRM-6|3,0,32;0,1,38;1,2,1;4,0,13;2,4,8;4,3,22|0,49,4,36,5|29|32
SMRM-7|0,4,22;3,0,30;2,0,44;0,1,21;1,2,38;1,3,35;4,1,49|0,14,17,5,61|40|40
SMRM-8|2,3,9;3,4,37;4,2,53;2,0,32;0,2,23;2,1,44;1,2,56;2,0,47|0,48,61,38,11|53|53
LIST
