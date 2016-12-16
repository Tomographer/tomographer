Metropolis-Hastings Random Walk related classes (`tomographer`)
===============================================================


The only Metropolis-Hastings Random Walk C++ class which we interface here is the
structure which specifies the parameters of the random walk: step size, sweep size, number
of thermalization sweeps and number of live sweeps.

If you want to customize the random walk procedure (e.g. define a new random walk scheme),
currently you need to write that in C++. (But anyway that's probably what you'd want to
do, because C++ is *way* faster than Python for simple boring computing tasks.)



.. automodule:: tomographer
    :members: MHRWParams
    :show-inheritance:
