#!/bin/bash
# make_package.sh -- dong goi thu muc gui sang may Windows 11 co cam ESP32: dist/esp32_demo_L_Ascon2_win11.zip
# Chi gom firmware + giao dien PC + tai lieu (khong gom host/, gen_layers.py, .pio/ -- chi dung tren may phat trien).
# Chay build_host.sh truoc (sinh layers_gen.h, pc/layers_spec.py va kiem thu).
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
STAGE="$(mktemp -d)"
P="$STAGE/esp32_demo"
mkdir -p "$P/ascon_esp32" "$P/pc" "$P/prebuilt" "$HERE/dist"
cd "$HERE"
cp platformio.ini flash.bat HUONG_DAN_WINDOWS11.md GIAO_THUC.md README.md "$P/"
cp ascon_esp32/ascon_esp32.ino ascon_esp32/platform_esp32.cpp ascon_esp32/demo_core.c ascon_esp32/demo_core.h \
   ascon_esp32/layers_gen.h ascon_esp32/led_signal.h "$P/ascon_esp32/"
cp prebuilt/esp32dev_L_Ascon2_merged.bin "$P/prebuilt/"     # anh nap tai 0x0, tao bang esptool merge-bin (README.md)
cp pc/pc_peer_gui.py pc/smrm_net.py pc/ascon_aead.py pc/layers_spec.py pc/run_gui.bat "$P/pc/"
cmp pc/ascon_aead.py ../ascon_aead_demo/python/ascon_aead.py      # ban sao phai trung ban da kiem chung KAT
rm -f dist/esp32_demo_L_Ascon2_win11.zip
(cd "$STAGE" && powershell.exe -NoProfile -Command \
    "Compress-Archive -Path esp32_demo -DestinationPath '$(cygpath -w "$HERE/dist/esp32_demo_L_Ascon2_win11.zip")'")
rm -rf "$STAGE"
ls -la dist/esp32_demo_L_Ascon2_win11.zip
