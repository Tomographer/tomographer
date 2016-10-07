
### Two-qubit example


## Running the example

# Create the data file

Perform the following, using MATLAB.

- make sure you have installed the libraries
  [QLib](http://www.tau.ac.il/~quantum/qlib/intro.html) and [CVX](http://cvxr.com/cvx/)

- run the script `make_data_file.m`.

- You should now have a file `thedata.mat`. This is the input file you can specify to
  `tomorun`.

# Running `tomorun` and configuration files

There are a few configuration files provided, named `tomorun-config-XYZ`.  You may run
each one of them with the system command

    > tomorun --config tomorun-config-XYZ

After running this command, the file `tomorun-config-XYZ-histogram.csv` should appear.
This is the histogram collected during the random walk, and is the main output of the
`tomorun` program.

# Analyzing the output of `tomorun`

For some figures of merit, the histogram output can be fit to a simple model involving few
paramters, the quantum error bars. We have provided scripts to perform this task, and to
display some nice graphs.  For each tomorun configuration, there is a script
`analysis_config_XYZ.m`.  Just run those scripts in MATLAB to analyze the output of
`tomorun`.

    >>> analyze_config_XYZ
