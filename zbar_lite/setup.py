#!/usr/bin/env python
# _*_coding:utf-8_*_
"""
@Time   :  2021/2/7 18:51
@Author :  wqh
@Email  :  597935261@qq.com
"""
try:
    import setuptools
    from setuptools import setup, Extension
    from setuptools.command.install import install
    from setuptools.command.build_ext import build_ext


    class MyBuildExt(build_ext):
        """
        MyBuildExt
        """

        def build_extensions(self):
            """

            :return:
            """
            self.compiler.dll_libraries = []
            build_ext.build_extensions(self)


    setuptools_opts = dict(cmdclass={"install": install, "build_ext": MyBuildExt})

except ImportError as err:
    print(err)
    from distutils.core import setup
    from distutils.extension import Extension

    setuptools_opts = {}
import os
import ctypes
import ctypes.util

CUR_DIR = os.path.realpath(os.path.dirname(__file__))


def get_c_file(target_dir):
    """
    to find the .c file
    :param target_dir:
    :return:
    """
    return [os.path.join(target_dir, x) for x in os.listdir(target_dir) if x.endswith(".c")]


SRCS = get_c_file("src/zbar") + \
       get_c_file("src/zbar/decoder") + \
       get_c_file("src/zbar/video") + \
       get_c_file("src/zbar/qrcode") + \
       get_c_file("src/zbar/processor") + \
       get_c_file("src/zbar/window") + \
       get_c_file("zbar")

# pdf417 can not use now
SRCS.remove(os.path.join("src/zbar/decoder", "pdf417.c"))

INCLUDE = ['./src', './src/zbar', "./src/zbar/decoder", "./src/zbar/qrcode", "./zbar"]


def has_libc_iconv():
    """
    find iconv
    :return:
    """
    if os.name != 'posix':
        return False
    libc = ctypes.CDLL(ctypes.util.find_library('c'))
    return hasattr(libc, 'iconv')


# don't try to link to standalone iconv library if it's already in libc
# (iconv is in glibc, but on OS X one needs a stanalone libiconv)
LIBS = [] if has_libc_iconv() else ['iconv']
zbar = Extension('zbar', sources=SRCS, include_dirs=INCLUDE, libraries=LIBS)

setup(name='zbar-lite',
      version='0.23.1',
      description='zbar lite package only support scan image and recognize barcode',
      long_description=open(os.path.join(CUR_DIR, "README.md"), encoding="utf-8").read(),
      long_description_content_type="text/markdown",
      url='https://github.com/mchehab/zbar',
      author='Qinghua Wang',
      author_email='597935261@qq.com',
      ext_modules=[zbar],
      # packages=['zbar'],
      license='LGPLV2',
      **setuptools_opts)
