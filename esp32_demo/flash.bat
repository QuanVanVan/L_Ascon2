@echo off
rem flash.bat -- build + nap firmware Ascon-SMRM len ESP32 (esp32dev) roi mo Serial Monitor.
rem   flash.bat          tu tim cong COM
rem   flash.bat COM5     chi dinh cong
rem Dau ra Serial duoc ghi them vao tep platformio-device-monitor-*.log trong thu muc nay (bo loc log2file).
rem Thoat Serial Monitor: Ctrl+C.
setlocal
cd /d "%~dp0"
set "PIO=%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
if not exist "%PIO%" set "PIO=pio"
"%PIO%" --version >nul 2>&1 || (echo Chua cai PlatformIO: xem HUONG_DAN_WINDOWS11.md & exit /b 1)

if "%~1"=="" (
    "%PIO%" run -e esp32dev -t upload || exit /b 1
    "%PIO%" device monitor -e esp32dev
) else (
    "%PIO%" run -e esp32dev -t upload --upload-port %1 || exit /b 1
    "%PIO%" device monitor -e esp32dev --port %1
)
