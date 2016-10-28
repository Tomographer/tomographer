
# Two-qubit example for `tomorun` usage

Here is the two-qubit example presented in Section 5 of the supplemental material to
https://dx.doi.org/10.1103/PhysRevLett.117.010404 .

## Running the example

### Create the data file

The data file is already in this git repository, so you don't need to do this manually to
run the example.  The following steps are provided as an illustration for how to create
your own data file.

Perform the following, using MATLAB.

- make sure you have installed the libraries
  [QLib](http://www.tau.ac.il/~quantum/qlib/intro.html) and
  [CVX](http://cvxr.com/cvx/);

- make sure that the directory `tools/matlabtools` (of the Tomographer project)
  is in your MATLAB path;

- run the script `make_data_file.m` -- edit at will;

- You should now have a file `thedata.mat`. This is the input file you can
  specify to `tomorun`.

### Running `tomorun` and configuration files

There are a few configuration files provided, named `tomorun-config-XYZ`.  You
may run each one of them with the system command

    > tomorun --config tomorun-config-XYZ

While the command is running, you can hit CTRL+C to get a status report. If you
wish to interrupt the running program, hit CTRL+C twice in short succession.

After running this command, the file `tomorun-config-XYZ-histogram.csv` should
appear.  This is the histogram collected during the random walk, and is the main
output of the `tomorun` program.

### Analyzing the output of `tomorun`

For some figures of merit, the histogram output can be fit to a simple model
involving few paramters, the quantum error bars. We have provided scripts to
perform this task, and to display some nice graphs.  For each tomorun
configuration, there is a script `analysis_config_XYZ.m`.  Just run those
scripts in MATLAB to analyze the output of `tomorun`.

    >>> analyze_config_XYZ

For these scripts to run, make sure that the directories `tools` as well as
`tools/matlabtools` (of the Tomographer project) are in your MATLAB path.


