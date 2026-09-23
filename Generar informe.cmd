@echo off
setlocal
chcp 65001 >nul
set "proyecto=%~1"
if defined proyecto goto generar
echo Arrastra la carpeta de tu proyecto sobre este archivo para generar su informe.
echo Tambien puedes escribir o pegar aqui la ruta de la carpeta.
set /p "proyecto=Carpeta del proyecto: "
set "proyecto=%proyecto:"=%"
if not defined proyecto exit /b 1
:generar
"%~dp0jocarsa-documentacion.exe" "%proyecto%" "%proyecto%\documentacion"
if errorlevel 1 goto error
echo.
echo Informe guardado en la subcarpeta documentacion del proyecto.
pause
exit /b 0
:error
echo.
echo No se pudo generar el informe. Revisa el mensaje anterior.
pause
exit /b 1
