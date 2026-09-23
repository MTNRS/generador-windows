# Generador de informes para Windows

Adaptación para **Windows de 64 bits** del [generador de Jocarsa](https://github.com/jocarsa/generador).

## Descargar y usar

**[Descargar el ZIP para Windows](https://github.com/MTNRS/generador-windows/releases/latest/download/jocarsa-generador-windows-x64.zip)**

1. Descarga y extrae el ZIP completo.
2. Abre **Generador-Windows.exe**.
3. Pulsa **Examinar?** y selecciona la carpeta de tu proyecto.
4. Pulsa **Generar informe** y despu?s **Abrir carpeta del informe**.

El informe Markdown (`.md`) se guarda en la subcarpeta **documentacion** del proyecto.

No hace falta instalar Python, SQLite, WSL ni Visual Studio para utilizarlo.
La ventana utiliza .NET Framework 4.5 o posterior, incluido en Windows 10 y 11.
Mant?n los dos ejecutables del ZIP en la misma carpeta. El lanzador `.cmd` y el
motor de consola siguen disponibles.

## Desde PowerShell

```powershell
.\jocarsa-documentacion.exe "C:\ruta\Mi proyecto" "C:\ruta\Informes"
.\jocarsa-documentacion.exe --verify "C:\ruta\Informes\Mi proyecto_FECHA.md"
```

## Código y compilación

El programa original es obra de **Jocarsa**. Esta adaptación parte del commit
`936c11c236bece7ad0ce872698061e4fe2025183` y añade compatibilidad nativa con Windows,
rutas Unicode y SQLite integrado en el ejecutable.

Ejecuta `build-windows.cmd` con las herramientas C++ de Visual Studio 2022 instaladas.
Los ejecutables se generan en `dist`. Las pruebas se ejecutan con `py -3 test-windows.py`.
Ejecuta `test-gui.cmd` para comprobar también la ventana y la generación desde sus botones.

Consulta [las instrucciones de Windows](LEEME-Windows.md) para más detalles.
