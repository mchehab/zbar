#!/usr/bin/env python3
"""Compile a Java archive and combine JNI headers in the build directory."""
from pathlib import Path
import subprocess
import sys
import tempfile

javac, jar, archive, header, *sources = sys.argv[1:]
archive = Path(archive).resolve()
header = Path(header).resolve()
with tempfile.TemporaryDirectory(dir=archive.parent) as temporary:
    root = Path(temporary)
    classes = root / 'classes'
    headers = root / 'headers'
    classes.mkdir()
    headers.mkdir()
    subprocess.run([javac, '-d', str(classes), '-h', str(headers),
                    *sources], check=True)
    header.write_text(''.join(p.read_text() for p in sorted(headers.glob('*.h'))))
    subprocess.run([jar, 'cf', str(archive), '-C', str(classes), '.'], check=True)
