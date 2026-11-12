#!/bin/bash
# build_host.sh -- sinh layers_gen.h + pc/layers_spec.py, bien dich demo_core.c tren PC, chay tu kiem tra va test giao thuc
# C <-> Python (phai TAT CA PASS ca hai truoc khi nap ESP32).
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
export PATH=/c/msys64/ucrt64/bin:$PATH
python "$HERE/../gen_layers.py"
gcc -O2 -std=c99 -Wall -Wextra -static -o "$HERE/host_test.exe" "$HERE/host_main.c" "$HERE/../ascon_esp32/demo_core.c"
"$HERE/host_test.exe"
python "$HERE/test_net.py"
gcc -O2 -std=c99 -Wall -Wextra -static -o "$HERE/test_led.exe" "$HERE/test_led.c"     # chuoi bao hieu 2 LED
"$HERE/test_led.exe"
