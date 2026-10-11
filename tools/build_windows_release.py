#!/usr/bin/env python3
"""Build Windows release archives from the matching MSYS2 MinGW environment."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def native_path(path):
    return Path(subprocess.check_output(
        ['cygpath', '-m', str(path)], text=True).strip())


def run(*command, **kwargs):
    return subprocess.run([str(arg) for arg in command], check=True, **kwargs)


def main():
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=root / 'build')
    args = parser.parse_args()
    arch = os.environ.get('ARCH')
    targets = {'x86_64': ('MINGW64', 'x64-mingw-static'),
               'i686': ('MINGW32', 'x86-mingw-static')}
    if arch not in targets:
        parser.error('Set ARCH to x86_64 or i686')
    msystem, triplet = targets[arch]
    if os.environ.get('MSYSTEM') != msystem:
        parser.error('ARCH does not match the active MSYS2 environment')
    if os.environ.get('VCPKG_DEFAULT_TRIPLET', triplet) != triplet:
        parser.error('VCPKG_DEFAULT_TRIPLET does not match ARCH')
    mingw_prefix = native_path(os.environ['MINGW_PREFIX'])
    vcpkg_root = native_path(os.environ.get('VCPKG_ROOT', root / 'build/vcpkg'))
    dependency_prefix = vcpkg_root / 'installed' / triplet
    gm = dependency_prefix / 'tools/graphicsmagick/bin/gm.exe'
    if not gm.is_file() or not (dependency_prefix / 'lib/libiconv.a').is_file():
        parser.error(f'Install graphicsmagick and libiconv with vcpkg for {triplet} first')

    env = os.environ.copy()
    env.update(CC='gcc', CXX='g++', CPPFLAGS='-D__USE_MINGW_ANSI_STDIO=1')
    # Meson launches native compilers without MSYS2's path conversion.
    prefix = dependency_prefix.as_posix()
    for name, flag in (('CFLAGS', f'-I"{prefix}/include"'),
                       ('CXXFLAGS', f'-I"{prefix}/include"'),
                       ('LDFLAGS', f'-L"{prefix}/lib"')):
        env[name] = f'{flag} {env.get(name, "")}'.strip()
    # Resolve iconv and codecs from vcpkg instead of MSYS2's shared packages.
    env['LIBRARY_PATH'] = f'{prefix}/lib'
    pkg_config_paths = os.pathsep.join(
        (f'{prefix}/lib/pkgconfig', f'{prefix}/share/pkgconfig'))
    env['PKG_CONFIG_PATH'] = env['PKG_CONFIG_LIBDIR'] = pkg_config_paths
    build_root = args.build_dir.resolve()
    packages = build_root / 'packages'
    packages.mkdir(parents=True, exist_ok=True)

    for video in ('VfW', 'DShow'):
        builddir = build_root / f'windows-vcpkg-{arch}-{video}'
        setup_options = (['--reconfigure']
                         if (builddir / 'meson-private/coredata.dat').is_file() else [])
        run('meson', 'setup', *setup_options, builddir, root,
            '--prefix=C:/', '--buildtype=release',
            '-Dlibrary_type=both', '-Dstatic_executables=true',
            '-Dvideo=true', f'-Ddirectshow={str(video == "DShow").lower()}',
            '-Dgtk=no', '-Dgir=false', '-Dpython=no', '-Dqt=false', '-Djava=disabled',
            '-Ddoc=false', '-Ddbus=disabled', '-Dnls=disabled', '-Dx11=disabled',
            '-Dxshm=disabled', '-Dxv=disabled', '-Dimagemagick=enabled',
            '-Dgraphicsmagick=enabled', '-Djpeg=enabled', '-Dpthread=true',
            '-Dbuild_tests=false', env=env, cwd=root)
        run('meson', 'compile', '-C', builddir, env=env, cwd=root)

        # A fresh stage prevents files from older builds entering the archive.
        with tempfile.TemporaryDirectory(prefix=f'stage-{arch}-{video}-',
                                         dir=build_root) as temporary:
            stage = Path(temporary)
            run('meson', 'install', '-C', builddir, '--destdir', stage,
                env=env, cwd=root)
            licenses = stage / 'share/licenses'
            shutil.copytree(mingw_prefix / 'share/licenses', licenses, dirs_exist_ok=True)
            for copyright in (dependency_prefix / 'share').glob('*/copyright'):
                destination = licenses / 'vcpkg' / copyright.parent.name
                destination.mkdir(parents=True, exist_ok=True)
                shutil.copy2(copyright, destination)
            formats = stage / 'share/zbar/image-formats.txt'
            formats.parent.mkdir(parents=True, exist_ok=True)
            formats.write_text(
                'GraphicsMagick format inventory. Some listed formats require external '
                'delegates not included in this package. See README-windows.md for '
                'supported formats.\n\n', encoding='utf-8')
            with formats.open('ab') as output:
                run(gm, 'convert', '-list', 'format', stdout=output, env=env)
            # Keep developer libraries and their dependencies in the same archive.
            run(sys.executable, root / 'tools/stage_windows.py', stage,
                '--runtime-bin', mingw_prefix / 'bin', env=env)
            sources = stage / 'sources'
            sources.mkdir(exist_ok=True)
            commands = {
                'msys2-packages.txt': ['pacman', '-Q'],
                'vcpkg-packages.txt': [vcpkg_root / 'vcpkg.exe', 'list'],
                'vcpkg-revision.txt': ['git', '-C', vcpkg_root, 'rev-parse', 'HEAD'],
                'zbar.tar.gz': ['git', 'archive', '--format=tar.gz', '--prefix=zbar/', 'HEAD'],
            }
            for name, command in commands.items():
                with (sources / name).open('wb') as output:
                    run(*command, stdout=output, env=env, cwd=root)
            # Replace the ZIP so entries from older packages cannot survive.
            archive = Path(shutil.make_archive(str(stage), 'zip', root_dir=stage))
            archive.replace(packages / f'zbar-win_{arch}-{video}.zip')


if __name__ == '__main__':
    main()
