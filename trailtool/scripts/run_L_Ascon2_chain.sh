#!/bin/bash
# run_L_Ascon2_chain.sh -- chuoi quet L_Ascon2 chay doc lap voi Claude Code (dang ky bang Windows Task Scheduler,
# tac vu "DATN_L_Ascon2_scan"). Thu tu theo yeu cau 2026-10-29: quet CHI vi sai (T3 = 44, 46, ...) den khi co toi
# thieu chinh xac hoac mot nguong vuot 48 h; sau do moi quay lai tuyen tinh tu T3 = 48.
# Chay lai an toan: scan_fine.sh bo qua cac nguong da co trong SUMMARY.txt.
D="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
LOG=$D/results/06_trail3r/run_3r/L_Ascon2/scan_fine.log
cd "$D"
echo "$(date) [task] bat dau: chi vi sai" >> "$LOG"
bash trailtool/scripts/scan_fine.sh L_Ascon2 42 - 2 172800 >> "$LOG" 2>&1
echo "$(date) [task] vi sai ket thuc -> tuyen tinh" >> "$LOG"
bash trailtool/scripts/scan_fine.sh L_Ascon2 - 46 2 172800 >> "$LOG" 2>&1
echo "$(date) [task] xong ca hai" >> "$LOG"
