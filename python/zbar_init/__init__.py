#!/usr/bin/env python
# _*_coding:utf-8_*_
"""
@Time   :  2021/2/5 19:14
@Author :  wqh
@Email  :  597935261@qq.com
"""
import os
import sys
from ctypes import CDLL

SYS_MAP = {"msys": ["msys-zbar", ".dll"],
           "win32": ["libzbar", ".dll"],
           "linux": ["libzbar", ".so"],
           "gnu": ["libzbar", ".so"]}
SYSTEM_KEY = [x for x in SYS_MAP.keys() if x.startswith(sys.platform)]
SYSTEM_KEY = SYSTEM_KEY[0] if SYSTEM_KEY else ""
LIB_PATTERN = SYS_MAP.get(SYSTEM_KEY, [])
print("system platform is {}".format(SYSTEM_KEY)
      if SYSTEM_KEY else "No valid zbar lib in this whl for {}".format(sys.platform))


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


CUR_DIR = os.path.realpath(os.path.dirname(__file__))
ZBAR_LIB_PATH = get_lib_path(CUR_DIR, LIB_PATTERN)
try:
    import zbar
except ImportError as err:
    print("warning : {}".format(err))
    print("warning : Fail to get so or dll in the default path")
    print("try to load {} ".format(ZBAR_LIB_PATH))
    CDLL(ZBAR_LIB_PATH)
    import zbar
