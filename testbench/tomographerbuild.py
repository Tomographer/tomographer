
from __future__ import print_function

import os
import os.path
import json
import datetime
import hashlib
import multiprocessing # cpu_count for make -jX
import subprocess
import shutil
import timeit


TOMOGRAPHER_URL = os.environ.get('TOMOGRAPHER_URL',
                                 'file://'+os.path.expanduser("~/Research/projects/tomographer"))

GIT = os.environ.get('GIT', "git")
CMAKE = os.environ.get('CMAKE', "cmake")
MAKE = os.environ.get('MAKE', "make")
CURL = os.environ.get('CURL', "curl")
TAR = os.environ.get('TAR', "tar")
UNZIP = os.environ.get('UNZIP', "unzip")

base_cmake_args = []

benchwork_dir = '_benchwork'

def do_run(cmdargv, **kwargs):

    add_environ = kwargs.pop('add_environ', {})
    if add_environ:
        kwargs['env'] = dict(os.environ, **add_environ)

    print("Running  %r  ... kwargs=%r"%(cmdargv,kwargs))
    subprocess.check_call(cmdargv, **kwargs)
    print("")

class TomographerBuild(object):
    def __init__(self, tomographer_version, cmake_args, add_environ={}):

        self.tomographer_version = tomographer_version
        self.cmake_args = cmake_args
        self.add_environ = add_environ

        m = hashlib.sha256()
        m.update(self.tomographer_version)
        m.update("\n".join(self.cmake_args))
        m.update("\n".join((str(k)+"\n"+str(v) for k,v in self.add_environ.items())))
        self.key = m.hexdigest()[-16:]

        self.workdir = os.path.join(benchwork_dir, self.key)
        self.tomographer_version_dir = 'tomographer-'+self.tomographer_version
        self.work_tomographer_dir = os.path.join(self.workdir, self.tomographer_version_dir)
        self.work_tomographer_build = os.path.join(self.work_tomographer_dir, 'build')
        
        try:
            self._compile()
        except Exception:
            print("Error compiling Tomographer ", self.tomographer_version)
            shutil.rmtree(self.workdir)
            raise

    def _compile(self):
        if not os.path.isdir(self.workdir):

            print("NEW BUILD: Tomographer %s, key=%s"%(self.tomographer_version, self.key))

            os.mkdir(self.workdir)
            # dump info in there so we know which version it is
            with open(os.path.join(self.workdir, 'BUILD_INFO'), 'w') as f:
                json.dump({'key': self.key,
                           'tomographer_version': self.tomographer_version,
                           'cmake_args': self.cmake_args,
                           'build_datetime': datetime.datetime.now().isoformat()}, f,
                          indent=4)

            # git clone
            do_run([GIT, "clone", TOMOGRAPHER_URL, self.tomographer_version_dir],
                   cwd=self.workdir, add_environ=self.add_environ)
            # git checkout <CORRECT-VERSION>
            do_run([GIT, "checkout", self.tomographer_version],
                   cwd=self.work_tomographer_dir, add_environ=self.add_environ)

            os.mkdir(self.work_tomographer_build)
            # cmake
            do_run([CMAKE, '..', ] + self.cmake_args,
                   cwd=self.work_tomographer_build, add_environ=self.add_environ)

            # make
            do_run([MAKE, '-j'+str(multiprocessing.cpu_count()), 'VERBOSE=1'],
                   cwd=self.work_tomographer_build, add_environ=self.add_environ)


class TomographerTomorunReleaseExe(object):
    def __init__(self, tomographer_version, plat, archivefmt):
        archivefilename = 'tomographer-'+tomographer_version+'-'+plat+'.'+archivefmt
        url = ('https://github.com/Tomographer/tomographer/releases/download/'+
               tomographer_version+'/'+archivefilename)
        
        self.key = 'rel-'+tomographer_version+'-'+plat

        self.workdir = os.path.join(benchwork_dir, self.key)

        self.work_tomographer_dir = os.path.join(os.workdir, 'tomographer-'+tomographer_version+'-'+plat)

        if not os.path.isdir(self.workdir):
            os.mkdir(self.workdir)
            do_run([CURL, '-L', '-O', url], cwd=self.workdir)
            if archivefmt == 'tar.gz':
                do_run([TAR, 'xfz', archivefilename], cwd=self.workdir)
            elif archivefmt == 'tar.bz2':
                do_run([TAR, 'xfj', archivefilename], cwd=self.workdir)
            elif archivefmt == 'zip':
                do_run([UNZIP, archivefilename], cwd=self.workdir)
            else:
                raise ValueError("Unknown archive format: "+archivefmt)






def timeit_call(args, cwd, repeat=3, number=1):
    timeit_call.args = args
    timeit_call.cwd = cwd

    print("timeit_call(args={!r}, cwd={!r}, repeat={}, number={}".format(args, cwd, repeat, number))

    xx = timeit.repeat('subprocess.check_call(args, cwd=cwd)',
                      setup="""
import subprocess
from tomographerbuild import timeit_call
args = timeit_call.args
cwd = timeit_call.cwd
""", repeat=repeat, number=number)

    print("\nTimeIt Report:  repeat=%d, number=%d\n"
          "\targs=%r\n"
          "\tcwd=%r"%(repeat, number, args, cwd))
    for x in xx:
        print("Time: %.3g s"%(float(x)/number))
    best = min([ float(x)/number for x in xx])
    print("Best time: ", best, "s")
    print("\n")

    return {
        'best': best,
        'avg': sum([ float(x)/number for x in xx]) / repeat
    }
