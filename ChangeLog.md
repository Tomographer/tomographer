
Change Log
==========


## v5.4 (2018-05-04)

  * Upgraded code for *pybind11* ≥ 2.2

  * Bugfixes in MPI multiproc implementation

  * Minor enhancements in the API


## v5.3 (2017-08-30)

  * Fix bug: the automatic base rng seed was ignored, causing the rngs to always
    be seeded with the seeds 0, 1, ... etc.

  * Python's `tomographer.tomorun.tomorun()` now provides clearer error messages
    when used with incorrect arguments

  * Fix bug: `MHRandomWalk` would call `allowDone*()` on controllers after every
    iteration, instead of after every sweep

  * Fix bug: `MHRWValueErrorBinsConvergedController` could previously interrupt
    at an iteration count which was not a multiple of the binning analysis
    sample size, causing some samples not to be taken into account in the error
    bar

  * New C++ muliprocessing implementation based on [MPI][] −
    `Tomographer::MultiProc::MPI` (provided as an extra tool, not used in
    `tomorun` nor the Python interface)

  * `MHRWTasks::CDataBase` now accepts a list of random seeds to provide for
    each task

  * Improved `tomorun`'s implementation of rng seeding using random device

  * Python/C++ bindings API: `tpy::CountIntType` and `tpy::RealType` are
    deprecated in favor of the more specific `tpy::IterCountIntType`,
    `tpy::HistCountIntType` etc.

[MPI]: https://computing.llnl.gov/tutorials/mpi/


## v5.2 (2017-08-22)

  * Better handling of Python exceptions inside C++ threads

  * Consolidated some thread-related code in order to address some bugs in
    status reporting

  * Added maximum run time for binning-converged controller to prevent
    excessively long runs (`tomorun` option
    `--control-binning-converged-max-add-run-iters`)

  * The `tomorun` executable is able to auto-detect number of binning levels,
    like the Python interface

  * Better built-in fit models, and support for calculation of the quantum error
    bars in some degenerate cases (Python module `tomographer.querrorbars`)

  * Other small enhancements:

    - minor optimization in MultiplexorValueCalculator

    - detect if iteration count will exceed capacity of integer type used

    - `tomographer.tomorun.tomorun()` uses better defaults for max number of
      unconverged bin error bars


## v5.1 (2017-06-24)

  * Python package installation: Fix compilation flags (add `-stdlib=libc++` by
    default on Mac OS X)

  * More meaningful iteration/sweep progress display:

        run sweep    19822/32768 [+th:512] : 61.10% done

    replaces

        iteration 2060800/(3328000=100*(512+32768)) : 61.92% done

  * Documentation updates

  * Some small internal clean-ups

  * Add static minimum logging level compilation configuration option for
    `tomorun` to improve run speed for custom builds;
    added
    [documentation for custom `tomorun` build configurations][tomorun_custom_build]

[tomorun_custom_build]: https://tomographer.github.io/tomographer/api-doc/current/html/page_tomorun_config_build.html


## v5.0 (2017-05-27)

  * Added "light jumps" random walk algorithm. The new algorithm chooses
    differently which next candidate point (for jumping to): instead of
    exploring all the neighboring points uniformly on the hypersphere, we only
    go to neighboring points in directions corresponding to the basis vectors.
    This is much faster for large system sizes, and explores the same
    distribution.

  * Added support for *random walk controllers*, controlling for instance the
    random walk step size, or controlling its duration to make sure that all
    error bars calculated from a binning analysis have converged.  These
    controllers are implemented both in `tomorun` and in Python's
    `tomographer.tomorun.tomorun()`

  * `tomorun` program: New internal structure for defining figures of merit, it
    is now much more easy to add a new figure of merit

  * `tomorun` program: Can now seed the random number generator via a system
    random device (requries custom compilation)

  * Updated documentation

  * Various fixes to the build system

  * Added more minimal examples (`test/minimal_tomorun_controlled.cxx` &
    `test/minimal_single_random_walk.cxx`)

  * Added `ChangeLog.md`

### C++ API Changes

  * Added `Tomographer::DenseDM::TSpace::LLHWalkerLight` class for "light
    jumps", which is much faster for large system sizes

  * Added the *MHRWControllers* concept, and implementations
    `Tomographer::MHRWStepSizeController`,
    `Tomographer::MHRWValueErrorBinsConvergedController`, as well as
    `Tomographer::MHRWAcceptRatioWalkerParamsController`

  * `Tomographer::UniformBinsHistogram` → `Tomographer::Histogram` (etc.),
    deprecated aliases available

  * Added `Tomographer::AggregatedHistogram{Simple|WithErrorBars}` classes

  * Some API clean-ups

**Major Backwards-Incompatible API Changes to Some Components:**

  * Changed `Tomographer::MHRWParams` template parameters, now allowing for
    arbitrary *MHWalkerParams* specifying whatever parameters the MHWalker needs
    in order to carry out its random walk

  * Status reporting API changed. Introduced
    `Tomographer::Tools::StatusProvider` and `Tomographer::Tools::StatusQuery`.
    Now the `MHRandomWalk` is itself aware of status reporting.  Added
    `Tomographer::PeriodicStatusReportMHRWStatsCollector` and
    `Tomographer::PredStatusReportMHRWStatsCollector`.

  * `Tomographer::MHRWTasks::ValueHistogramTasks` was removed and replaced by
    `Tomographer::MHRWTasks::ValueHistogramTools` with a new revised API with
    much improved flexibility. (For instance, it is now possible to have several
    stats collectors and to customize what results on is interested in.)

  * `Tomographer::MHRWTasks` was reworked for added flexibiliy (e.g., the
    *CData* now needs to specify `setupRandomWalkAndRun()` etc. instead of
    `createMHWalker()` and friends, so that it can add controllers and keep
    references to each other etc.)

### Python API Changes

  * New histogram classes with NumPy buffers, replacing the simple C++ wrappers.
    Renamed `tomographer.UniformBinsHistogram` → `tomographer.Histogram`,
    removed the different classes for different underlying types (now all
    handled by `tomographer.Histogram`).  Deprecated aliases available.

  * Deprectated the `AveragedXXXXXXHistogram` classes

  * `tomographer.MHRWParams` now stores any arbitrary *MHWalkerParams* using an
    arbitrary Python object (normally a dictionary).  It is backwards-compatible
    in several ways (MHWalkerParams as additional keyword arguments, or a step
    size given as first argument to MHRWParams())

  * (python) added compiler info in `tomographer.version.compile_info`

  * Doc: Example for writing your own C++ code with a Python module interface

  * `PyLogger` made explicitly not GIL-aware, use `GilProtectedPyLogger` if GIL
    is released

  * Added test cases (e.g. unpickling data pickled using earlier versions of
    Tomographer)


## v4.1 (2017-04-08)

  * Improved Python packaging and fixed some bugs in `setup.py`

  * Minor changes to the Python interface


## v4.0 (2017-03-21)

  * Switched to [`pybind11`](https://github.com/pybind/pybind11) for Python
    bindings (thanks to Chris Granade for suggesting this great library),
    dropping the dependency on Boost.Python.

  * Python package available with `pip`.  Added module `tomographer.include` as
    well as member `tomographer.version.compile_info` for packaging.

  * Any custom figure of merit can be specified as a user-defined function to
    the `tomographer.tomorun.tomorun()` in the Python interface

  * OpenMP is no longer required neither for `tomorun` nor for the Python
    interface.  This allows to compile with Apple's default clang compiler on
    Mac OS X.  (OpenMP can still be used instead of C++11 threads, if
    preferred.)


## v3.0 (2017-02-22)

  * New Python interface (using Boost.Python)

  * Test integration with Travis CI

  * Overhaul of some C++ components, some enhancements.  Added for instance
    `Tomographer::MHRWTasks::ValueHistogramTasks`


## v2.0 (2016-09-03)

  * Minor improvements to the main `tomorun` program

  * Revised framework API for better organization, consistency and flexibility


## v1.1 (2015-11-20)

  * Changed a few default parameter values to more sensible values

  * Improved error reporting for faulty inputs

  * Fixed small bug where the "purified distance" figure of merit would always
    use the MLE as reference state

  * Small changes to documentation and build system


## v1.0 (2015-09-23)

  * First release of Tomographer.
