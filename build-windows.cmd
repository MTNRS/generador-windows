@echo off
setlocal
cd /d "%~dp0"
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL exit /b 1
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist build mkdir build
if not exist dist mkdir dist
cl /nologo /O2 /MT /utf-8 /std:c11 /W3 /D_CRT_SECURE_NO_WARNINGS /Ivendor\sqlite /c jocarsa-documentacion.c /Fobuild\generador.obj
if errorlevel 1 exit /b 1
cl /nologo /O2 /MT /utf-8 /D_CRT_SECURE_NO_WARNINGS /DSQLITE_THREADSAFE=0 /c vendor\sqlite\sqlite3.c /Fobuild\sqlite3.obj
if errorlevel 1 exit /b 1
link /nologo build\generador.obj build\sqlite3.obj advapi32.lib /OUT:dist\jocarsa-documentacion.exe
if errorlevel 1 exit /b 1
"%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /platform:x64 /optimize+ /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /out:dist\Generador-Windows.exe GeneradorWindows.cs
if errorlevel 1 exit /b 1
copy /y "Generar informe.cmd" dist\ >nul
copy /y LEEME-Windows.md dist\ >nul
exit /b %errorlevel%
