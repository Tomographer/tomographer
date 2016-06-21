#!/usr/bin/env python

# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
# 
# The MIT License (MIT)
# 
# Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe
# Faist
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from __future__ import print_function


import os
import os.path
import sys
import subprocess
import shutil

tomo_name = "tomographer"

if not os.path.exists(tomo_name):
    print("Error: there is no ", tomo_name, " directory. Are you sure "
          " you ran properly mk_mingw32_prepare.py, mk_ming32dist.bat and then only mk_mingw32_finish.py??\n")
    print("")
    sys.exit(2)

with open(os.path.join(tomo_name, "VERSION"), 'r') as f:
    gitversion = f.read().strip()

assert(len(gitversion) > 0)

tomo_name_w_ver = tomo_name+"-"+gitversion
install_name = tomo_name_w_ver+'-mingw32'
tomo_name_w_ver_a = {
    'zip': install_name + ".zip"
    }

print("Packaging Tomographer version", gitversion, "as", tomo_name_w_ver_a)

class MyStore: pass
e = MyStore()
e.git = os.environ.get('GIT', "git")
e.tar = os.environ.get('TAR', "tar")
e.zip = os.environ.get('ZIP', "zip")

def do_run(cmdargv, **opts):
    print("Running %r ..."%(cmdargv))
    subprocess.check_call(cmdargv, **opts)
    print("")

def do_rmtree(path):
    print("Removing %s ..."%(path))
    shutil.rmtree(path)

def do_copy_file(src, dst):
    print("Copying file from %s to %s ..."%(src, dst))
    shutil.copy2(src, dst)


# rename tomorun-INSTALL to the correct version
do_run(["mv",
        'tomographer-INSTALL',
        install_name],
       cwd=os.path.join(tomo_name, 'build'))

# copy in it all relevant libraries
for libfn in os.listdir('libs'):
    do_copy_file(os.path.join('libs', libfn), os.path.join(tomo_name, 'build', install_name, 'bin'))

do_run([e.zip, "-r", os.path.join('..', '..', tomo_name_w_ver_a['zip']), install_name],
       cwd=os.path.join(tomo_name, 'build'))

#do_rmtree(tomo_name) # -- leave commented for now, for debugging

print("Done.")
