# Generador de informes para Windows

Adaptación para **Windows de 64 bits** del [generador de Jocarsa](https://github.com/jocarsa/generador).

## Descargar y usar

**[Descargar el ZIP para Windows](https://github.com/MTNRS/generador-windows/releases/latest/download/jocarsa-generador-windows-x64.zip)**

1. Descarga y extrae el ZIP completo.
2. Arrastra la carpeta de tu proyecto sobre **Generar informe.cmd**, o abre ese archivo y pega la ruta de la carpeta.
3. Recoge el informe Markdown (`.md`) en la subcarpeta **documentacion** de tu proyecto.

No hace falta instalar Python, SQLite, WSL ni Visual Studio para utilizarlo.
El `.exe` es un programa de consola: utiliza el archivo `.cmd` para el uso sencillo.

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
El ejecutable se genera en `dist`. Las pruebas se ejecutan con `py -3 test-windows.py`.

Consulta [las instrucciones de Windows](LEEME-Windows.md) para más detalles.
