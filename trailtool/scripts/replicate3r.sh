#!/bin/bash
# replicate3r.sh <workdir> [extra g++ flags]
# Tai lap cong thuc "HOW TO REPLICATE THE SEARCH FOR 3 ROUNDS" cua README upstream cho Ascon
# (tuyen tinh T = 28, vi sai T = 40) bang cay duyet goc ("generate"), da song song hoa.
#   -DLEGACY_THRESHOLDS : giu nguong cu (loi #6 trong CHANGES.md) de doi chieu tung byte voi ban goc.
set -e
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
w=$1; shift; mkdir -p "$w/bin"; cd "$w"
EXTRA=("$@")
B() { bash "$TT_SCRIPTS/build_variant.sh" bin/t_$1_$2_$3.exe $1 $2 $3 "${EXTRA[@]}"; }
B 0 26 21; B 0 28 12; B 1 39 21; B 1 40 12
for cfg in "0 26 21 forward 28" "0 28 12 backward 28" "1 39 21 forward 40" "1 40 12 backward 40"; do
  set -- $cfg
  echo "=== DTS=$1 limit=$2 ww=$3 ; extendWW $4 $5"
  TIMEFORMAT="generate wall: %Rs"; time ./bin/t_$1_$2_$3.exe generate | grep -v "currentDate\|finished comp"
  TIMEFORMAT="extend wall: %Rs"; time ./bin/t_$1_$2_$3.exe extendWW $4 $5
done
wc -l *TrailCores* | grep -v total
