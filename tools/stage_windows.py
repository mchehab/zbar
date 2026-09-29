#!/usr/bin/env python3
"""Include the DLL dependencies of a staged Windows installation."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('stage', type=Path)
parser.add_argument('--runtime-bin', type=Path, required=True)
parser.add_argument('--objdump', default='objdump')
args = parser.parse_args()
system_dlls = {
    'advapi32.dll', 'avicap32.dll', 'bcrypt.dll', 'comctl32.dll',
    'comdlg32.dll', 'crypt32.dll', 'd3d11.dll', 'dxgi.dll', 'gdi32.dll',
    'gdiplus.dll', 'imm32.dll', 'kernel32.dll', 'kernelbase.dll',
    'msvcrt.dll', 'netapi32.dll', 'ntdll.dll', 'ole32.dll', 'oleaut32.dll',
    'psapi.dll', 'rpcrt4.dll', 'secur32.dll', 'setupapi.dll', 'shell32.dll',
    'shlwapi.dll', 'user32.dll', 'ucrtbase.dll', 'version.dll', 'vfw32.dll',
    'winmm.dll', 'winspool.drv', 'ws2_32.dll', 'wldap32.dll',
}
# Meson may install under usr/bin, a configured prefix, or directly in bin.
bin_dirs = [p for p in args.stage.rglob('bin') if p.is_dir()]
if len(bin_dirs) != 1:
    parser.error('expected exactly one installed bin directory')
bin_dir = bin_dirs[0]
runtime = {p.name.lower(): p for p in args.runtime_bin.iterdir() if p.is_file()}
pending = list(bin_dir.glob('*.exe')) + list(bin_dir.glob('*.dll'))
visited = set()
while pending:
    binary = pending.pop()
    if binary.name.lower() in visited:
        continue
    visited.add(binary.name.lower())
    output = subprocess.check_output([args.objdump, '-p', str(binary)], text=True)
    for name in re.findall(r'DLL Name:\s*(\S+)', output):
        key = name.lower()
        if key in system_dlls or key.startswith(('api-ms-', 'ext-ms-')):
            continue
        destination = bin_dir / name
        if not destination.exists():
            source = runtime.get(key)
            if source is None:
                raise RuntimeError('Missing runtime DLL: ' + name)
            shutil.copy2(source, destination)
        pending.append(destination)
