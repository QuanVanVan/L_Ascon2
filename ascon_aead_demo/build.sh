#!/bin/bash
# build.sh -- bien dich c/ascon_tool.exe (MSYS2 UCRT64 gcc, lien ket tinh nhu cac phan khac cua du an).
set -e
cd "$(dirname "${BASH_SOURCE[0]}")/c"
export PATH="/c/msys64/ucrt64/bin:$PATH"
gcc -O2 -std=c11 -Wall -Wextra -static ascon_aead.c ascon_tool.c -o ascon_tool.exe
echo "built c/ascon_tool.exe"
