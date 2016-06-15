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

if (len(sys.argv) != 2):
    print("Usage: %s <version>\n(where <version> is a git version tag)\n"%(sys.argv[0]))
    sys.exit(1)

gitversion = sys.argv[1]
tomo_name_w_ver = "tomographer-"+gitversion
tomo_name_w_ver_a = {
    'tar.gz': tomo_name_w_ver + ".tar.gz",
    'tar.bz2': tomo_name_w_ver + ".tar.bz2",
    'zip': tomo_name_w_ver + ".zip"
    }

if os.path.exists(tomo_name_w_ver) or any((os.path.exists(n) for n in tomo_name_w_ver_a.values())):
    print("Error: some files with conflicting names exist, aborting.\n" +
          "Please remove the following files if you wish to proceed:\n" +
          "\t"+tomo_name_w_ver)
    for fn in tomo_name_w_ver_a.values():
        print("\t"+fn)
    print("")
    sys.exit(2)

print("Packaging Tomographer version", gitversion, "as", tomo_name_w_ver)

tomographer_url = "https://github.com/Tomographer/tomographer.git"

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

# git clone
do_run([e.git, "clone", tomographer_url, tomo_name_w_ver])
# git checkout <CORRECT-VERSION>
do_run([e.git, "checkout", gitversion], cwd=tomo_name_w_ver)
# create VERSION file with version number
with open(os.path.join(tomo_name_w_ver, 'VERSION'), 'w') as f:
    do_run([e.git, "describe", "--tags"], stdout=f, cwd=tomo_name_w_ver)
# remove .git files etc.
do_rmtree(os.path.join(tomo_name_w_ver, '.git'))

do_run([e.tar, "cvfz", tomo_name_w_ver_a['tar.gz'], tomo_name_w_ver])
do_run([e.tar, "cvfj", tomo_name_w_ver_a['tar.bz2'], tomo_name_w_ver])
do_run([e.zip, "-r", tomo_name_w_ver_a['zip'], tomo_name_w_ver])

do_rmtree(tomo_name_w_ver)

print("Done.")
