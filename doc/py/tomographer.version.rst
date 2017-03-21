Version and compilation information (`tomographer.version`)
===========================================================

.. py:data:: tomographer.version.version_str

   The full Tomographer version, as a string (for example ``"v4.0"``, or for pre-releases,
   ``"v4.0a7-1-g7b32960"``).

   The same version string is available simply as ``tomographer.__version__``.


.. py:data:: tomographer.version.version_info

   A named tuple with fields `(major,minor)` with the major and minor version of
   Tomographer.


.. py:data:: tomographer.version.compile_info
   
   A dictionary giving information on how the Python interface to Tomographer was
   compiled.  Currently the following fields are provided:

     - `compile_info['cflags']`: the extra compilation flags specified in order to compile
       the C++ module.  This includes setting the C++11 or C++14 standard, as well as any
       further vectorization / optimization flags.

       These flags are suitable to pass to `Extension(extra_compile_options=...)` if you
       are compiling your own extension with setuptools which uses the `tomographer`
       Python interface.

     - `compile_info['eigen']`: the version of Eigen used to compile the `tomographer`
       Python extension module, and which is included in the package data.  This is a
       string which includes information about the enabled vectorization instructions (for
       instance: ``"Eigen 3.3.1 (SIMD: AVX SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2)"``).

     - `compile_info['boost']`: the version of the Boost headers used to compile the
       `tomographer` Python extension module, and which is included in the package data
       (for example: ``"1_63"``).

     
