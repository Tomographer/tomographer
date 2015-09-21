---
title: "Getting Started"
permalink: /get-started/
layout: default
---

I've got experimental data to analyze.
======================================

Here are the steps to follow in order to get started:

1. [Download and install]({{ site.baseurl }}/download) Tomographer.

2. Get familiar with `tomorun`. Get an idea of what the program does and how to use it with:

        > tomorun --help

2. Create a MATLAB data file with your experimental data, as specified in `tomorun`'s help
   text. This includes in particular the MATLAB variables `dim`, `Emn`, `Nm`, and `rho_MLE`.

   The MAT file format is widely recognized by Mathematica, numpy/scipy, ... so that it
   shouldn't be an issue to generate the data file.

   Don't forget to save in the appropriate MATLAB file version so that it can be read by
   `tomorun`. Just to be safe, in MATLAB, you should use the save command with an explicit
   version option:

        >> save('my_data_file.mat', ..., '-v6')

   (This depends on the version and configuration of the MatIO library `tomorun` was
   compiled with.)

3. Create a configuration file for tomorun for your experiment. See the
   [`README`]({{site.github_blob_baseurl}}{{site.tomographer_latest_version}}/README.md)
   the tomographer sources for an example. (This is simply a list of `tomorun` options
   with corresponding values, and optional comments)

4. Run `tomorun`.

        > tomorun --config my_config_file

   This will run the Metropolis-Hastings random walk according to the given settings.

   Hit `Ctrl-C` to get an intermediate progress report, and to get updates on if the
   parameters of the random walk are appropriate (you should have an acceptance ratio
   roughly in between 0.25 and 0.4). Hit `Ctrl-C` twice in short succession to interrupt.

   The output is controlled by the options `--write-histogram` and
   `--write-histogram-from-config-file-name`. This will produce a tab-separated table of
   values as documented in `tomorun --help`.

5. Analyse the output.

   Depending on what your figure of merit is, and what you indend to do, fit the numerical
   estimate of the distribution of the figure of merit with the appropriate model. A
   MATLAB tool is provided in the source tomographer distribution to help you
   (`tools/analyze_tomorun_histogram.m`), which uses the Curve Fitting Toolbox to analyze
   the output from `tomorun`.
