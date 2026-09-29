#!/usr/bin/env python3
"""Build the existing NSIS installer from a Meson installation."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', type=Path)
parser.add_argument('--runtime-prefix', type=Path, required=True)
parser.add_argument('--objdump', default='objdump')
parser.add_argument('--dlltool', default='dlltool')
args = parser.parse_args()
source = Path(__file__).resolve().parent.parent
build = args.build.resolve()
project = json.loads(subprocess.check_output(
    ['meson', 'introspect', '--projectinfo', str(build)], text=True))
subprocess.run(['meson', 'compile', '-C', str(build), 'html'], check=True)
with tempfile.TemporaryDirectory() as temporary:
    stage = Path(temporary)
    subprocess.run(['meson', 'install', '-C', str(build),
                    '--destdir', str(stage)], check=True)
    bin_dir, = stage.rglob('bin')
    root = bin_dir.parent
    subprocess.run(['python3', str(source / 'tools/stage_windows.py'),
                    str(stage), '--runtime-bin',
                    str(args.runtime_prefix / 'bin'),
                    '--objdump', args.objdump], check=True)
    dll = bin_dir / 'libzbar-0.dll'
    exports = subprocess.check_output([args.objdump, '-p', str(dll)], text=True)
    names = re.findall(r'^\s*\[\s*\d+\]\s+((?:zbar|_zbar.*_error)_\w+)\s*$',
                       exports, flags=re.MULTILINE)
    definition = root / 'lib/libzbar-0.def'
    definition.write_text('EXPORTS\n' + ''.join(name + '\n' for name in names))
    fmt = subprocess.check_output([args.objdump, '-f', str(dll)], text=True)
    machine = 'i386:x86-64' if 'pei-x86-64' in fmt else 'i386'
    subprocess.run([args.dlltool, '--machine', machine, '--dllname', dll.name,
                    '--def', str(definition), '--output-lib',
                    str(root / 'lib/libzbar-0.lib')], check=True)
    shutil.copytree(build / 'doc/html', root / 'share/doc/zbar/html')
    defines = ['-DVERSION=' + project['version']]
    for tool in ['zbarimg', 'zbarcam']:
        if (bin_dir / (tool + '.exe')).exists():
            defines.append('-DHAVE_' + tool.upper())
    subprocess.run(['makensis', '-NOCD', '-V2'] + defines +
                   [str(source / 'zbar.nsi')], cwd=root, check=True)
    installer = 'zbar-' + project['version'] + '-setup.exe'
    shutil.copy2(root / installer, build / installer)
