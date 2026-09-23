# Generador de informes para Windows x64

Compilado a partir de https://github.com/jocarsa/generador, commit
`936c11c236bece7ad0ce872698061e4fe2025183`.

## Uso sencillo

Extrae el ZIP completo y abre **Generador-Windows.exe**.

1. Pulsa **Examinar?** y selecciona la carpeta de tu proyecto.
2. Pulsa **Generar informe**.
3. Cuando termine, pulsa **Abrir carpeta del informe**.

El informe `.md` se guarda en la subcarpeta `documentacion` del proyecto.
Esa subcarpeta se excluye de los informes, seg?n la configuraci?n original.
Mant?n `Generador-Windows.exe` y `jocarsa-documentacion.exe` en la misma carpeta.
La ventana utiliza .NET Framework 4.5 o posterior, incluido en Windows 10 y 11.
Tambi?n puedes seguir arrastrando carpetas sobre **Generar informe.cmd**.

## Uso desde PowerShell

Abre una terminal en la carpeta del ejecutable:

```powershell
.\jocarsa-documentacion.exe "C:\ruta\Mi proyecto" "C:\ruta\Informes"
.\jocarsa-documentacion.exe --verify "C:\ruta\Informes\Mi proyecto_FECHA.md"
```

El motor `jocarsa-documentacion.exe` es de consola. Para utilizar la ventana,
abre `Generador-Windows.exe`. No requiere Python, SQLite, WSL ni Visual Studio para usarlo.
El motor nativo solo depende de bibliotecas incluidas en Windows. Compilación de 64 bits.

Se mantienen el contenido del informe, las extensiones y carpetas excluidas,
la inspección del esquema SQLite y el cálculo HMAC del código original.
La adaptación usa rutas Unicode nativas de Windows y registra Windows en los
metadatos. Omite UID y versión del kernel Unix. No recorre enlaces ni junctions.

## Recompilar

En el código fuente, ejecuta `build-windows.cmd` con Visual Studio 2022 y sus
herramientas de desarrollo C++ instaladas. El resultado aparece en `dist`.
SQLite 3.53.4 está incluido en `vendor/sqlite` con URL y hash de procedencia.
Se enlazan estáticamente SQLite y el runtime de C (`/MT`).

Para ejecutar las pruebas de integración, con Python 3:

```powershell
py -3 test-windows.py
```

Comprueban generación, rutas con espacios y Unicode, destinos absolutos y
relativos, exclusiones, esquemas SQLite, HMAC con una implementación independiente,
rechazo de documentos alterados y errores de entrada.
