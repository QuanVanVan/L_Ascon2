@echo off
rem run_gui_local.bat -- mo GUI demo muc 3 tren may phat trien (Windows 10).
rem Ly do: lenh "py" tren may nay tro toi C:\Python314 da bi go; Python cua PlatformIO khong co tkinter.
rem Dung CPython 3.12 do uv cai (co tkinter 8.6).
cd /d "%~dp0"
"%APPDATA%\uv\python\cpython-3.12.16-windows-x86_64-none\python.exe" pc_peer_gui.py
if errorlevel 1 pause
