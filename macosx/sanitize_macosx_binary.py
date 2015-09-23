#!/usr/bin/env python

import sys
import re
import os
import os.path
import subprocess
import shutil


if len(sys.argv) < 2:
    print "Usage: %s <name-of-exe>\n"%(sys.argv[0])
    sys.exit(1)

thisexename = sys.argv[1]


class MyStore: pass
e = MyStore()
e.otool = os.environ.get('OTOOL', 'otool')
e.install_name_tool = os.environ.get('INSTALL_NAME_TOOL', 'install_name_tool')

system_paths = [
    '/usr/lib',
    '/lib'
    ]


def fix_exe_object_lib_refs(exename):
    print "Fixing", exename, "..."
    output = subprocess.check_output([e.otool, "-L", exename])
    lines = output.split('\n')[1:] # all lines except first
    for l in lines:
        if not l:
            continue
        m = re.match(r'^\s*(?P<libname>\S+)\s.*$', l)
        if not m:
            print "Warning: Can't parse line: %r"%(l)
            continue
        libname = m.group('libname')
        if any([libname.startswith(x) for x in system_paths]):
            print "%s is in a system path, skipping..."%(libname)
            continue

        if libname.startswith('@'):
            print "%s is already a local reference, skipping..."%(libname)
            continue

        # copy the library next to the executable and rename link
        if (not os.path.exists(libname)):
            print "Warning: Can't find library %s, skipping..."%(libname)
            continue
        
        baselibname = os.path.basename(libname)
        locallibpath = os.path.join(os.path.dirname(exename), baselibname)
        shutil.copyfile(libname, locallibpath)
        subprocess.check_call([e.install_name_tool,
                               "-change",
                               libname,
                               "@executable_path/"+baselibname,
                               exename])
        subprocess.check_call([e.install_name_tool,
                               "-id",
                               "@executable_path/"+baselibname,
                               locallibpath])

        # and recursively fix the library itself, too.
        fix_exe_object_lib_refs(locallibpath)


fix_exe_object_lib_refs(thisexename)
