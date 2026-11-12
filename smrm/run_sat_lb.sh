#!/bin/bash
# Can duoi CHUNG MINH DUOC (tang dan K) cho so S-box hoat dong 3 vong: moi K bao UNSAT la mot can duoi.
# Dung: run_sat_lb.sh <ten> <ops> <d> <mode> <Kstart> <timeout_moi_K>
cd "$(dirname "$0")"
mkdir -p "_tmp_$1_$4"; TRAIL_TMP="_tmp_$1_$4" PYTHONUNBUFFERED=1 python trail_sat.py "smrm:$2|$3" 3 $4 exact $5 $6 | sed -u "s/^/[$1 $4] /"
