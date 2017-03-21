
"""
The convenience module `tomographer.include` facilitates the development of new
`Python`/`C++` modules using tomographer along with tomographerpy headers.
"""

import os.path

#
# The included components are defined here -- this present list is also used by setup.py
#
BOOST_DEPS_COMPONENTS = ['algorithm', 'math', 'core', 'exception']
"""
Those components of boost which are included in the header dependencies directory of
this package.
"""


def get_include(keys=False):
    """
    Return the directory(ies) to add to C++ include path in order to compile
    extensions using `tomographer`/`tomographerpy` headers.  This function is
    intended to be used in your `setup.py` in order to compile your extension.

    Tomographer's dependencies, the Eigen and Boost headers, are included in the
    package data so that you can easily compile other extensions using
    Tomographer.  This function will also return the necessary include paths to
    reach those dependencies.  (Note: Only a subset of the boost headers are
    included, as defined by :py:data:`BOOST_DEPS_COMPONENTS`).

    If `keys=False` (the default), then a list of include paths are returned.
    If `keys=True`, then a dictionary is returned with the keys `['tomographer',
    'tomographerpy', 'boost', 'eigen']` and with corresponding values which are
    the include paths for each component.

    See also: Information about compile flags in
    :py:data:`tomographer.version.compile_info['cflags']
    <tomographer.version.compile_info>`.
    """

    root_include = os.path.dirname(os.path.realpath(__file__))

    if keys:
        return { 'tomographer': root_include,
                 'tomographerpy': root_include,
                 'boost': os.path.join(root_include, 'deps', 'boost'),
                 'eigen': os.path.join(root_include, 'deps', 'eigen3'), }
    
    return list(set(get_include(keys=True).values()))




def find_include_dir(headername, pkgnames=[], testfn=os.path.isdir, return_with_suffix=None, add_paths=[]):
    """
    Find an include directory for a third-party library.

    This function works by looking for `headername` using the test function `testfn`.  By
    default, it looks for a directory named `headername`.

    Arguments:

      - `headername`: file or directory name to look for
    
      - `pkgnames`: package names under which these headers may have been packaged, e.g.,
        by homebrew.  This is used to add guess paths.

      - `testfn`: how to determine that `headername` was found.  The default,
        :py:func:`os.path.isdir`, checks that `headername` is a directory.

      - `return_with_suffix`: you may set this to a path fragment to be appended to the
        directory found

      - `add_paths`: additional search paths to use

    Returns: The directory relative to which we found `headername`, possibly supplemented
    by the given suffix in `return_with_suffix`.  `None` is returned if the include files
    were not found.
    """
    guesses = (
        # homebrew version
        [ os.path.join('/usr/local/opt', pkgname, 'include') for pkgname in pkgnames] + [
            # default paths
            '/usr/local/include',
            '/usr/include',
            '/opt/include',
            os.path.expanduser('~/.local/include'),
        ] + add_paths)
    for i in guesses:
        dn = os.path.join(i, headername)
        if testfn(dn): # found
            if return_with_suffix:
                return os.path.join(i, return_with_suffix)
            return i

    return None


_staticlib_suffixes = ['.a']
_dylib_suffixes = ['.so', '.dylib', '.dll']

def find_lib(libname, pkgnames, libnamesuffixes=[], add_paths=[], prefer_static=True):
    """
    Find a third-party library.

    This function looks for the library `libname` using common library naming schemes
    (e.g. lib`libname`.dylib) in common search paths.

    Returns the full path to the library or `None` if not found.

    Arguments:

      - `libname`: the base name of the library, without any ``lib`` prefix or any suffix
    
      - `pkgnames`: as for :py:func:`find_include_dir`, specify possible package names
        which may contain this library

      - `libnamesuffixes`: possible library suffixes for `libname` (but not filename
        suffix). This is used for instance some times to distinguish a multithreaded
        version of a library from a non-multithreaded one)

      - `add_paths`: additional paths in which to look

      - `prefer_static`: if `True`, then static library suffixes are tried before the
        dynamic ones; if `False`, the opposite.
    """
    guesses = (
        # homebrew version
        [ os.path.join('/usr/local/opt', pkgname, 'include') for pkgname in pkgnames] + [
        os.path.join('/usr/local/opt', pkgname, 'lib'),
        # default paths
        '/usr/local/lib',
        '/usr/lib',
        '/opt/lib',
        '/usr/lib{suffix}/x86_64-linux-gnu',
        os.path.expanduser('~/.local/lib'),
    ] + add_paths)
    is_64bits = sys.maxsize > 2**32 # see https://docs.python.org/2/library/platform.html
    libdirsuffixes = ['', '64' if is_64bits else '32']
    for i in guesses:
        for s in libdirsuffixes:
            if '{suffix' not in i:
                i += '{suffix}'
            #print("find_lib:TRYING i={!r}".format(i))
            basedn = i.format(suffix=s)
            #
            for lnp in ['lib', '']: # lib-name-prefixes
                for lns in libnamesuffixes + ['']: # lib-name-suffixes
                    fnslist = (_staticlib_suffixes+_dylib_suffixes) if prefer_static else (_dylib_suffixes+_staticlib_suffixes)
                    for fns in fnslist: # file name suffixes for library
                        dn = os.path.join(basedn+s, lnp+libname+lns+fns)
                        #print("find_lib:TRYING dn={!r}".format(dn))
                        if os.path.isfile(dn):
                            return dn
    return None



def ensure_str(s):
    """
    Simple conversion of the value `s` to a `str`, that is, a byte string on Python 2 and
    a unicode string on Python 3.

    Input is expected to be a byte-string or unicode string in both Py2 or Py3.
    """
    if not isinstance(s, str):
        return s.decode('utf-8')
    return s



class Vars(object):
    """
    Minimal handling of compilation config variables, importable from CMake cache.

    Constructor arguments:
      - `vars`: the list of variable names which we will be storing.
      - `cachefile`: a CMake cache file to look values into.  If `None`, no CMake cache is
        consulted.

    The variables are looked up:
       - in the environment variables
       - in the CMake cache, if any is specified
       - as per any given default value (with :py:meth:`setDefault()`))

    Example usage::

        cmake_cache_file = os.environ.get('CMAKE_CACHE_FILE', None)
        vv = Vars([
            'GIT',
            'EIGEN3_INCLUDE_DIR',
        ], cachefile=cmake_cache_file)
        vv.setDefault('GIT', lambda : find_executable('git'))
        vv.setDefault('EIGEN3_INCLUDE_DIR',
                      lambda : find_include_dir('eigen3', pkgnames=['eigen','eigen3'],
                                                return_with_suffix='eigen3'))
    """
    def __init__(self, vars, **kwargs):
        # self.d: values read & cached
        self.d = {}

        dc = {}
        if 'cachefile' in kwargs:
            self._cachefile = kwargs['cachefile']
            if self._cachefile:
                dc = self._readcache(self._cachefile)

        for k in vars:
            if k in os.environ:
                self.d[k] = os.environ.get(k)
            elif k in dc:
                self.d[k] = ensure_str(dc[k])
            else:
                self.d[k] = None

    def setDefault(self, k, defvalue):
        """
        If no value has been detected for key `k`, set the value `defvalue`.  `defvalue` may
        be a callable which is invoked only if the value is actually needed.
        """
        if k not in self.d or self.d.get(k, None) is None:
            if callable(defvalue):
                self.d[k] = defvalue()
            else:
                self.d[k] = defvalue

    def _readcache(self, cachefile):
        dc = {}
        import re
        rx = re.compile(r'^(?P<name>[A-Za-z0-9_]+)(:(?P<type>[A-Za-z_]+))=(?P<value>.*)$')
        with open(cachefile) as f:
            for line in f:
                m = rx.match(line)
                if not m:
                    continue
                dc[m.group('name')] = ensure_str(m.group('value'))
        return dc

    def get(self, k):
        """
        Get the value associated with the variable `k` (specified in environment, read from
        cache, or default value).
        """
        return self.d.get(k)

    def message(self, tight=False):
        s = "Configuration variables:\n\n"
        s += ("\n".join([ "    {}={}".format(k,v) for k,v in self.d.items() ]))
        s += "\n\nYou may specify configuration variable values using environment variables."

        if hasattr(self, '_cachefile'):
            if not self._cachefile:
                s += ("\n\nYou may also read variables from a CMakeCache.txt file with\n"
                      "    CMAKE_CACHE_FILE=path/to/CMakeCache.txt")
            else:
                s += ("\n\n    (read cache file {})".format(cmake_cache_file))
        
        if tight:
            return s

        return "\n" + s + "\n"
