---
title: "Getting Started"
permalink: /get-started/
layout: default
---

I've got experimental data to analyze.
======================================

Please select your preferred option, perhaps the one that suits best your existing workflow:

- [Tomographer routines invoked via Python code, with data provided as NumPy arrays,
  subsequent analysis performed via Python routines](#python-version); or

- [Tomographer routines run from command-line, data provided in MATLAB file format,
  subsequent analysis performed separately with e.g. MATLAB](#command-line-tomorun-matlab).


## <a name="python-version"></a> The Python Way

Tomographer provides an interface for easy invocation from Python code.  Here are the
steps to get started and analyze data from your experiment:

1.  Install the necessary Python packages.

    First, install `tomographer`'s dependencies.  Depending on whether you're
    using [*conda*](https://www.anaconda.com/){:target="_blank"} or *pip* by
    itself, you should run the following commands:

        # for *conda* users:

        > conda install numpy gcc libgcc
        > conda install -c conda-forge pybind11

        # OR, for *pip* users:

        > pip install numpy pybind11

    All `pip install` commands (e.g. `pip install SOMEPACKAGE`) might have to be
    prefixed by `sudo -H` (e.g. `sudo -H pip install SOMEPACKAGE`) if you need
    administrator priviledges, or you can use the option `--user` to install
    into a user directory (e.g. `pip install SOMEPACKAGE --user`).

    Now, install `tomographer` itself, using `pip` (this should also work within
    a *conda* environment):
    
        > pip install tomographer

    In case of trouble, you should [seek additional information and instructions
    in the README
    file](https://github.com/Tomographer/tomographer/blob/master/README.md#installing-and-using-the-python-package){:target="_blank"}.

2.  Open your favorite Python environment (e.g. edit a Python script or open an IPython or
    Jupyter notebook session), and start off with [this one-qubit
    example]({{site.github_blob_baseurl}}master/examples/simple-qubit/simple_example.py).

    Note: The script is designed to display well within a Jupyter notebook, including
    real-time feedback on the progress of the random walk.

3.  Edit at wish to get a hang of how the Python call works.  Read the documentation of
    the [`tomographer.tomorun.tomorun()`]({{ site.baseurl }}/api-doc/{{
    site.tomographer_latest_version
    }}/html/py/tomographer.tomorun.html#tomographer.tomorun.tomorun) Python function, the
    main entry point to our procedure from Python code.

4.  As in the example, the histogram fit, as well as the calculation of the quantum error
    bars can be handled by [some provided python
    routines]({{site.baseurl}}/api-doc/{{site.tomographer_latest_version}}/html/py/tomographer.querrorbars.html)

5. Write up and publish paper. We will be very happy if you cite us :)

   > 1. Philippe Faist and Renato Renner. Practical and Reliable Error Bars in
   >    Quantum Tomography. Phys. Rev. Lett. 117:1, 010404 (2016).
   >    arXiv:1509.06763
   > 
   > 2. Philippe Faist. The Tomographer Project. Available at:
   >    https://github.com/Tomographer/tomographer/




## <a name="command-line-tomorun-matlab"></a> Using the Command-Line Program `tomorun`


The Tomographer project provides the `tomorun` program which analyses data from a quantum
tomography experiment.

1. [Download and install]({{ site.baseurl }}/download) Tomographer/tomorun.  You
   do not need to set up the Python interface.

2. Get familiar with `tomorun`. Get an idea of what the program does and how to use it with:

        > tomorun --help

3. Create a MATLAB data file with your experimental data, as specified in `tomorun`'s help
   text, encoded in the MATLAB variables `dim`, `Emn`, and `Nm`.

   Don't forget to properly model noise in your measurement device into effective POVM
   effects.

   The MAT file format is widely recognized by Mathematica, numpy/scipy, ... so that it
   shouldn't be an issue to generate the data file.

   Don't forget to save in the appropriate MATLAB file version so that it can be read by
   `tomorun`. Just to be safe, in MATLAB, you should use the `save` command with an
   explicit version option:

        >> save('my_data_file.mat', ..., '-v6')

   (This depends on the version and configuration of the
   [MatIO](http://matio.sourceforge.net/){:target="_blank"} library `tomorun` was compiled
   with.)

4. Create a configuration file for tomorun for your experiment (or edit [this example
   configuration]({{site.baseurl}}/tomorun-config-sample){:target="_blank"}). This is
   simply a list of `tomorun` options with corresponding values, and optional comments.

   Use the `--data-file-name` option to specify the name of the file where you saved your
   data (step above). To keep things simple, put the configuration file and data file in
   the same directory.

   The option `--value-type` allows you to specify the figure of merit you want to
   track. Set the histogram range and number of bins using `--value-hist`.

   Fine-tune the Metropolis-Hastings random walk with the options `--step-size`,
   `--n-sweep`, `--n-therm`, and `--n-run`. Specify the number of random walk instances to
   run with `--n-repeats` (typically one per available CPU). Use a binning analysis
   because that's better (it's on by default).

   The output is controlled by the options `--write-histogram` and
   `--write-histogram-from-config-file-name`. When using the option
   `--write-histogram=<filename>`, then the file written will be
   `<filename>-histogram.csv`, and will be overwritten if it already exists.

5. Run `tomorun`.

        > tomorun --config my_config_file

   This will run the Metropolis-Hastings random walk according to the given settings.

   Hit `Ctrl-C` to get an intermediate progress report, and to see if the parameters of
   the random walk are appropriate. You should have an acceptance ratio roughly in between
   0.2 and 0.4. Hit `Ctrl-C` twice in short succession to abort, for instance if you want
   to change the parameters of the random walk and start again.

   The output consists in a tab-separated table of values as documented in `tomorun
   --help`. The output file is specified with the options `--write-histogram` and
   `--write-histogram-from-config-file-name`.

6. Analyse the output.  (Read [{{site.paper_ref}}]({{site.paper_url}}){:target="_blank"}
   ([{{site.paper_arxiv_ref}}]({{site.paper_arxiv_url}}){:target="_blank"}) to understand
   the theory behind this method.)

   Depending on what your figure of merit is, and what you indend to do, fit the numerical
   estimate of the distribution of the figure of merit with the appropriate model. A
   MATLAB tool is provided in the source tomographer distribution to help you
   ([`tools/analyze_tomorun_histogram.m`]({{site.github_blob_baseurl}}{{site.tomographer_latest_version}}/tools/analyze_tomorun_histogram.m){:target="_blank"}),
   which uses the Curve Fitting Toolbox to analyze the output from `tomorun`.
   
   A Python script which is capable of analyzing output from `tomorun` is also
   provided
   ([`tools/analyze_tomorun_histogram_simple.py`]({{site.github_blob_baseurl}}{{site.tomographer_latest_version}}/tools/analyze_tomorun_histogram_simple.py){:target="_blank"})
   *(note this requires the `tomographer` python package to be installed, see
   instructions above)*.  Run the script with `--help` to get information on its
   possible command-line options.


7. Write up and publish paper. We will be very happy if you cite us :)

   > 1. Philippe Faist and Renato Renner. Practical and Reliable Error Bars in
   >    Quantum Tomography. Phys. Rev. Lett. 117:1, 010404 (2016).
   >    arXiv:1509.06763
   > 
   > 2. Philippe Faist. The Tomographer Project. Available at:
   >    https://github.com/Tomographer/tomographer/

