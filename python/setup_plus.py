#!/usr/bin/env python
# _*_coding:utf-8_*_
"""
@Time   :  2021/2/5 19:14
@Author :  wqh
@Email  :  597935261@qq.com
"""
import os

import sys

from setuptools import Extension
from setuptools import find_packages
from setuptools import setup

CUR_DIR = os.path.realpath(os.path.dirname(__file__))
zbar_init_path = os.path.join(CUR_DIR, "zbar_init")
SYS_MAP = {"msys": ["msys-zbar", ".dll"],
           "win32": ["libzbar", ".dll"],
           "linux": ["libzbar", ".so"],
           "gnu": ["libzbar", ".so"]}
SYSTEM_KEY = [x for x in SYS_MAP.keys() if x.startswith(sys.platform)]
SYSTEM_KEY = SYSTEM_KEY[0] if SYSTEM_KEY else ""
LIB_PATTERN = SYS_MAP.get(SYSTEM_KEY, [])


def get_lib_path(lib_dir, lib_pattern):
    """

    :param lib_dir:
    :param lib_pattern: [prefix,postfix]
    :return:
    """
    path = ""
    if os.path.isdir(lib_dir) and lib_pattern:
        file_list = os.listdir(lib_dir)
        start, end = lib_pattern
        for f in file_list:
            if f.startswith(start) and f.endswith(end):
                path = os.path.join(lib_dir, f)
    return path


LIB_PATH = get_lib_path(zbar_init_path, LIB_PATTERN)
WITH_LIB = bool(LIB_PATH)
print("#####")
print("setup zbar {} zbar lib".format("with" if WITH_LIB else "without"))
print("#####")
setup(
    name='zbar',
    version='0.23.1',
    author='Jeff Brown',
    author_email='spadix@users.sourceforge.net',
    url='https://github.com/mchehab/zbar',
    description='read barcodes from images or video',
    license='LGPL',
    long_description=open('README').read(),
    classifiers=[
        'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Environment :: Console',
        'Environment :: X11 Applications',
        'Environment :: Win32 (MS Windows)',
        'Operating System :: POSIX',
        'Operating System :: Unix',
        'Operating System :: Microsoft :: Windows',
        'Topic :: Communications',
        'Topic :: Multimedia :: Graphics',
        'Topic :: Software Development :: Libraries',
    ],
    packages=find_packages() if WITH_LIB else [],
    package_data={"zbar_init": [os.path.basename(LIB_PATH)]} if WITH_LIB else {},
    ext_modules=[
        Extension('zbar', [
            'zbarmodule.c',
            'enum.c',
            'exception.c',
            'symbol.c',
            'symbolset.c',
            'symboliter.c',
            'image.c',
            'processor.c',
            'imagescanner.c',
            'decoder.c',
            'scanner.c',
        ],
                  libraries=['zbar'],
                  include_dirs=['../include']
                  ),
    ],
)
