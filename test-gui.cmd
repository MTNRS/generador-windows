@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build
"%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:exe /platform:x64 /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:dist\Generador-Windows.exe /out:dist\test-gui.exe test-gui.cs
if errorlevel 1 exit /b 1
dist\test-gui.exe
exit /b %errorlevel%
