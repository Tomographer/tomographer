# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
#
# The MIT License (MIT)
#
# Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
# Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from __future__ import print_function

import codecs
import os
import os.path
import sys
import subprocess
import re
import glob
import shutil



# include path is set in setup.py -- this is the tomographer.include sub-package ...
from include import ensure_str, BOOST_DEPS_COMPONENTS   # beurk!!


def get_version_info(version):

    # major/minor sections of version
    m = re.match(r'^v(?P<major>\d+)\.(?P<minor>\d+)', version)
    version_maj = int(m.group('major'))
    version_min = int(m.group('minor'))

    # Normalize version string for PIP/setuptools
    version_for_pip = version
    # remove initial 'v' in 'v3.1'
    if version_for_pip[0] == 'v':
        version_for_pip = version_for_pip[1:]
    # make PEP-440 compatible if it is a specific git-describe commit number
    m = re.match(r'^(?P<vtag>.*)-(?P<ncommits>\d+)-(?P<githash>g[a-fA-F0-9]+)$', version_for_pip)
    if m:
        version_for_pip = "{vtag}+git{ncommits}.{githash}".format(**m.groupdict())

    return (version_maj, version_min, version_for_pip)



def setup_sources(thisdir, vv):

    if not os.path.exists(os.path.join(thisdir, '..', 'tomographer', 'tomographer_version.h.in')):
        # we use ../tomographer/tomographer_version.h.in to see if we are being run in an
        # original Tomographer source checkout (not necessarily git, but full project
        # checked out as opposed to a python sdist pacakge)
        print("Running from Python packaged source")
        return False

    # We are being run from checked out Tomographer project sources.  Prepare the python
    # sources correctly.

    #
    # setup the VERSION file
    #
    version = None
    try:
        version = ensure_str(subprocess.check_output([vv.get('GIT'), 'describe', '--tags', 'HEAD'],
                                                     cwd=thisdir, stderr=subprocess.STDOUT)).strip()
    except Exception:
        pass
    if not version:
        if os.path.exists(os.path.join(thisdir, '..', 'VERSION')):
            with open(os.path.join(thisdir, '..', 'VERSION')) as f:
                version = ensure_str(f.read()).strip()
    if not version:
        raise RuntimeError("Cannot determine Tomographer version (no git info and no VERSION file)")

    # create VERSION file here -- for sdist
    with open(os.path.join(thisdir, 'VERSION'), 'w') as f:
        f.write(version + "\n")

    (version_maj, version_min, version_for_pip) = get_version_info(version)


    #
    # Include our source headers in package data, along with dependencies
    #


    def ignore_not_header(d, files):
        return [f for f in files
                if not os.path.isdir(os.path.join(thisdir,d,f)) and not f.endswith('.h') and not f.endswith('.hpp')]

    #
    # Copy all tomographer headers inside tomographer.include as package data
    #
    src_tomographer_include = os.path.join(thisdir, '..', 'tomographer')
    target_tomographer_include = os.path.join(thisdir, 'tomographer', 'include')
    if os.path.exists(os.path.join(target_tomographer_include, 'tomographer')):
        shutil.rmtree(os.path.join(target_tomographer_include, 'tomographer'))
    shutil.copytree(src_tomographer_include,
                    os.path.join(target_tomographer_include, 'tomographer'),
                    ignore=ignore_not_header)

    #
    # Copy all tomographerpy headers inside tomographer.include as package data
    #
    src_tomographerpy_include = os.path.join(thisdir, 'cxx', 'tomographerpy')
    target_tomographer_include_tomographerpy = os.path.join(thisdir, 'tomographer', 'include', 'tomographerpy')
    if os.path.exists(os.path.join(target_tomographer_include_tomographerpy)):
        shutil.rmtree(os.path.join(target_tomographer_include_tomographerpy))
    shutil.copytree(src_tomographerpy_include,
                    os.path.join(target_tomographer_include_tomographerpy),
                    ignore=ignore_not_header)

    #
    # Copy all dependency headers (boost, eigen)
    #
    target_tomographer_include_deps = os.path.join(target_tomographer_include, 'deps')
    if not os.path.exists(target_tomographer_include_deps):
        os.mkdir(target_tomographer_include_deps)

    if (not vv.get('Boost_INCLUDE_DIR') or
        os.path.realpath(vv.get('Boost_INCLUDE_DIR')).startswith(os.path.realpath(target_tomographer_include_deps))):
        raise ValueError("Invalid Boost_INCLUDE_DIR: "+repr(vv.get('Boost_INCLUDE_DIR')))

    target_tomographer_include_deps_boost = os.path.join(target_tomographer_include_deps, 'boost')
    if os.path.exists(target_tomographer_include_deps_boost):
        shutil.rmtree(target_tomographer_include_deps_boost)
    os.mkdir(target_tomographer_include_deps_boost)
    # Arrgh! "/usr/bin/bcp --boost=/usr/include ..." will copy all system files as well!!
    # solution: create a symlink to '/usr/include/boost' in a sandboxed directory...
    if not os.path.isdir(os.path.join(thisdir,'tmp')):
        os.mkdir(os.path.join(thisdir,'tmp'))
    if not os.path.isdir(os.path.join(thisdir,'tmp','boost_dir')):
        os.mkdir(os.path.join(thisdir,'tmp','boost_dir'))
    if not os.path.exists(os.path.join(thisdir,'tmp','boost_dir','boost')):
        os.symlink(os.path.join(vv.get('Boost_INCLUDE_DIR'), 'boost'), # source
                   os.path.join(thisdir,'tmp','boost_dir','boost')) # link_name

    subprocess.check_output([vv.get('BCP'), '--boost='+os.path.join(thisdir,'tmp','boost_dir')] +
                             BOOST_DEPS_COMPONENTS +
                             [target_tomographer_include_deps_boost ])

    if (not vv.get('EIGEN3_INCLUDE_DIR') or
        os.path.realpath(vv.get('EIGEN3_INCLUDE_DIR')).startswith(os.path.realpath(target_tomographer_include_deps))):
        raise ValueError("Invalid EIGEN3_INCLUDE_DIR: "+repr(vv.get('EIGEN3_INCLUDE_DIR')))

    target_tomographer_include_deps_eigen3 = os.path.join(target_tomographer_include_deps, 'eigen3')
    if os.path.exists(target_tomographer_include_deps_eigen3):
        shutil.rmtree(target_tomographer_include_deps_eigen3)
    shutil.copytree(vv.get('EIGEN3_INCLUDE_DIR'),
                    os.path.join(target_tomographer_include_deps_eigen3))
    
    #
    # create tomographer_version.h
    #
    tomographer_version_h_content = """\
/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef zzzTOMOGRAPHER_VERSION_H_
#define zzzTOMOGRAPHER_VERSION_H_

#define TOMOGRAPHER_VERSION "{version}"
#define TOMOGRAPHER_VERSION_MAJ {version_maj}
#define TOMOGRAPHER_VERSION_MIN {version_min}

#endif
""".format(version=version,version_maj=version_maj,version_min=version_min)
    with open(os.path.join(thisdir, 'tomographer', 'include', 'tomographer', 'tomographer_version.h'), 'w') as f:
        f.write(tomographer_version_h_content)

    #
    # include LICENSE file in this directory
    #
    shutil.copy2(os.path.join(thisdir, '..', 'LICENSE.txt'), thisdir)

    #
    # create the README file
    #
    NOTE_PKG_DEPS = """
Note: This package contains a subset of the
`Boost library <https://boost.org/>`_
(distributed under the `Boost software license <http://www.boost.org/users/license.html>`_)
and the `Eigen3 library <https://eigen.tuxfamily.org/>`_
(distributed under the `MPL license 2.0 <https://www.mozilla.org/en-US/MPL/2.0/>`_).
They are located in the source package directory ``tomographer/include/deps/``.
"""

    readme_content = ''
    with open(os.path.join(thisdir, 'README_.rst')) as f:
        readme_content = f.read()
    with open(os.path.join(thisdir, 'README.rst'), 'w') as fw:
        fw.write(readme_content)
        fw.write(NOTE_PKG_DEPS)

    
    #
    # All set.
    #
    return True
