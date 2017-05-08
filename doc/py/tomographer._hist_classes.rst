Histogram Classes (`tomographer`)
=================================


This collection of classes take care of recording, storing and manipulating a histogram.
Typically such a class would be returned by a core task such as
:py:func:`tomographer.tomorun.tomorun()`.


.. automodule:: tomographer
    :members: HistogramParams, Histogram, HistogramReal, HistogramWithErrorBars, AveragedSimpleHistogram, AveragedSimpleRealHistogram, AveragedErrorBarHistogram
    :show-inheritance:



Deprecated Aliases
------------------

The following aliases are provided for compatibility with older versions of Tomographer
(before Tomographer 5): `UniformBinsHistogramParams`, `UniformBinsHistogram`,
`UniformBinsRealHistogram`, and `UniformBinsHistogramWithErrorBars`.
