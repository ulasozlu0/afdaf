@echo off
openfiles >nul 2>nul
if %errorlevel% neq 0 (
    echo Not running as admin. Relaunching...
    powershell -Command "Start-Process cmd -ArgumentList '/c ""%~s0""' -Verb runAs"
    exit /b
)

:: === SAVE CURRENT DATE/TIME ===
for /f "tokens=1-3 delims=/.- " %%a in ("%date%") do (
    set old_date=%%a-%%b-%%c
)
set old_time=%time%

echo Current date: %date% %time%

:: === SET DATE TO 01-01-2017 ===
date 01-01-2017
time 00:00:00

echo Changed date: %date% %time%
echo.

pushd "%cd%"
cd /d "%~dp0"

set "file_path=ctx.dll"
echo Using: "%file_path%"
echo.

:: === WINDOWS SIGNS TOOL PATH ===
set "signtool=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe"

:: === SIGN (NO TIMESTAMP) ===
"%signtool%" sign ^
/f "%cd%\certificates\Password-ndasec-2016.pfx" ^
/p "ndasec-2016" ^
/fd sha256 ^
"%file_path%"

echo.
echo Done signing: "%file_path%"

:: === RESTORE DATE/TIME ===
echo Restoring original time...
date %old_date%
time %old_time%

echo Restored: %date% %time%
pause