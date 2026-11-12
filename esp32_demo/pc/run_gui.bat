@echo off
rem run_gui.bat -- mo giao dien PC cho demo Ascon-SMRM (can Python 3.10+ tu python.org, co san tkinter)
cd /d "%~dp0"
where py >nul 2>&1
if %errorlevel%==0 (py -3 pc_peer_gui.py) else (python pc_peer_gui.py)
if errorlevel 1 pause
