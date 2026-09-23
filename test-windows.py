"""Prueba de integración del ejecutable nativo (Python 3, sin paquetes)."""
from pathlib import Path
import hashlib
import hmac
import re
import sqlite3
import subprocess
import tempfile

base = Path(__file__).resolve().parent
exe = base / 'dist/jocarsa-documentacion.exe'
out = base / 'test-output'
out.mkdir(exist_ok=True)
assert out.resolve().is_relative_to(base)

def run(*args, expected=0):
    p = subprocess.run([str(exe), *map(str, args)], capture_output=True)
    assert p.returncode == expected, (p.returncode, p.stdout, p.stderr)
    return p

with tempfile.TemporaryDirectory(prefix='integracion-', dir=out) as folder:
    folder = Path(folder)
    project = folder / 'Proyecto español 漢字'
    project.mkdir()
    (project / 'código.py').write_text('print("Hola, España")\n', encoding='utf-8')
    for name in ['node_modules', 'privado']:
        d = project / name
        d.mkdir()
        (d / 'oculto.py').write_text('NO_INCLUIR_ESTE_CONTENIDO')
    (project / 'privado/.jocarsa-documentacion-exclude').touch()
    with sqlite3.connect(project / 'datos ñ.sqlite') as db:
        db.execute('CREATE TABLE alumnos (id INTEGER PRIMARY KEY, nombre TEXT)')
        db.execute("INSERT INTO alumnos VALUES (1, 'NO_VOLCAR_REGISTROS')")
    db.close()
    dest = folder / 'informes con tildes á' / 'subcarpeta'
    run(project, dest)
    report = next(dest.glob('*.md'))
    data = report.read_bytes()
    text = data.decode('utf-8')
    for item in ['Hola, España', 'código.py', 'Proyecto español 漢字', 'alumnos', 'nombre', '**Sistema operativo:** Windows']:
        assert item in text, item
    assert 'NO_INCLUIR_ESTE_CONTENIDO' not in text
    assert 'NO_VOLCAR_REGISTROS' not in text
    run('--verify', report)
    source = (base / 'jocarsa-documentacion.c').read_text(encoding='utf-8')
    secret = re.search(r'SECRETO_JOCARSA\s*=\s*"([^"]*)"', source).group(1).encode()
    match = re.search(rb'HMAC-SHA-256 de autenticidad:\*\* `([a-f0-9]{64})`', data)
    unsigned = data[:match.start(1)] + b'0' * 64 + data[match.end(1):]
    assert hmac.new(secret, unsigned, hashlib.sha256).hexdigest().encode() == match.group(1)
    report.write_bytes(data + b'\nAlterado\n')
    run('--verify', report, expected=2)
    run(project / 'no-existe', dest, expected=1)
    run(project / 'código.py', dest, expected=1)
    run(project / 'privado', dest, expected=1)
    run(expected=1)
    relative_dest = folder.relative_to(base) / 'salida relativa'
    run(str(project) + '\\', relative_dest)
    run('--verify', next((base / relative_dest).glob('*.md')))
print('OK: rutas Unicode y espacios, destino absoluto/relativo, SQLite, exclusiones, HMAC independiente, manipulación y errores.')
