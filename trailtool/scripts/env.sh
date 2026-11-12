#!/bin/bash
# env.sh -- duong dan dung chung cho moi script cua trailtool (source tu cac script khac).
# Moi duong dan suy ra tu VI TRI cua thu muc nay, nen ca thu muc DATN co the doi cho tuy y.
#   TT_ROOT   = .../DATN/trailtool
#   PRJ_ROOT  = .../DATN
#   TT_BIN    = .../DATN/trailtool/bin           (t_dts.exe, t_lts.exe)
#   TT_SRC    = .../DATN/trailtool/src           (ma nguon da sua)
#   FINALISTS = .../DATN/results/02_finalists/finalists.txt (6 ung vien SMRM, dinh dang NAME|ops|d)
#   FINALISTS_W = cung tep, duong dan dang Windows (E:/...) de truyen cho chuong trinh .exe
TT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRJ_ROOT="$(cd "$TT_ROOT/.." && pwd)"
TT_BIN="$TT_ROOT/bin"
TT_SRC="$TT_ROOT/src"
TT_SCRIPTS="$TT_ROOT/scripts"
FINALISTS="$PRJ_ROOT/results/02_finalists/finalists.txt"
FINALISTS_W="$(cygpath -m "$FINALISTS")"
PYTHON="${PYTHON:-/c/msys64/ucrt64/bin/python.exe}"
# trinh bien dich: MSYS2 UCRT64 (gcc 13). Thu muc tam ghi duoc la bat buoc tren may nay.
export PATH="/c/msys64/ucrt64/bin:$PATH"
mkdir -p "$TT_ROOT/tmp"
export TMP="$(cygpath -w "$TT_ROOT/tmp")" TEMP="$(cygpath -w "$TT_ROOT/tmp")"
