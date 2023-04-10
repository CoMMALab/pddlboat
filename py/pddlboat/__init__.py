from os.path import dirname, abspath

def dll_loader(lib, path):
    from platform import system
    from os.path import isfile
    import ctypes
    from ctypes.util import find_library

    sys = system()
    if sys == 'Windows':
        ext = '.dll'
    elif sys == 'Darwin':
        ext = '.dylib'
    else: # Linux, other UNIX systems
        ext = '.so'
    fname = path + '/lib' + lib + ext
    if not isfile(fname):
        fname = find_library(lib)
    ctypes.CDLL(fname, ctypes.RTLD_GLOBAL)

dll_loader('pddlboat-cpp', dirname(abspath(__file__)))
from .pddlboat import *
