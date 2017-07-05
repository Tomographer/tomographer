
Tomographer v5.0
----------------

Release 5.0 brings several new features, most notably:

  - A new random walk algorithm which greatly speeds up the random walk for
    larger systems (e.g. 6 qubits);
    
  - Automatic adjustment of step size based on acceptance ratio; automatic
    detection of random walk end once the error bars from the binning analysis
    have converged.

The API was also significantly revised for added functionality and flexibility
(see [change log][ChangeLog]).

To get see how to started, [check out this page][get-started].

If you're interested in using the stand-alone executable program, download the
release `tomographer-tomorun-v5.0` below corresponding to your system.

If you'd like to install the Python package, you don't need to download
anything; simply run:

    pip install numpy pybind11
    pip install tomographer

See [the downloads page][downloads] for more options and more info.

[ChangeLog]: https://github.com/Tomographer/tomographer/blob/master/ChangeLog.md
[get-started]: https://tomographer.github.io/tomographer/get-started
[downloads]: https://tomographer.github.io/tomographer/download
