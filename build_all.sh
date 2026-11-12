#!/bin/bash
# build_all.sh -- bien dich + sinh lai moi tep can thiet + chay toan bo kiem thu, tu MA NGUON trong kho nay.
# Thu tu co phu thuoc: ma tran (results/mat) can cho trailtool/tests; KAT (ascon_aead_demo/results) can cho esp32 gen_layers.py.
# Moi tep sinh ra deu nam trong .gitignore. Dung lai ngay khi mot buoc FAIL.
# Moi truong: Git Bash / MSYS2 tren Windows, gcc 13 + Python 3 cua MSYS2 UCRT64 (C:\msys64\ucrt64\bin).
set -eo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PATH="/c/msys64/ucrt64/bin:$PATH"
step() { echo; echo "######## $*"; }

step "1/6 smrm: bien dich cong cu tim kiem / danh gia (sinh smrm_gen.h)"
bash "$ROOT/smrm/build.sh"

step "2/6 smrm: ghi ma tran results/mat/*.bin (Ascon, Hsilu, MRM, 6 ung vien gom L_Ascon2)"
(cd "$ROOT/smrm" && python export_baselines.py && python export_finalists.py)

step "3/6 trailtool: bien dich t_dts.exe, t_lts.exe + checklayer"
bash "$ROOT/trailtool/build.sh"

step "4/6 trailtool: bo kiem thu"
bash "$ROOT/trailtool/tests/run_tests.sh" | tail -4

step "5/6 ascon_aead_demo: Ascon goc (1089 KAT chinh thuc) + 6 tang thay the, sinh KAT"
bash "$ROOT/ascon_aead_demo/run_all.sh" all > /dev/null 2>&1 || { tail -30 "$ROOT/ascon_aead_demo/results/run_all.log"; exit 1; }
tail -1 "$ROOT/ascon_aead_demo/results/run_all.log"

step "6/6 esp32_demo: sinh layers_gen.h + pc/, tu kiem tra firmware tren PC, giao thuc C<->Python, chuoi LED"
bash "$ROOT/esp32_demo/host/build_host.sh" | grep -E "PASS|FAIL|LOI:|CO [0-9]+ LOI|da ghi"

echo
echo "XONG: moi buoc PASS. Firmware ESP32: cd esp32_demo && pio run -e esp32dev (xem esp32_demo/README.md)."
