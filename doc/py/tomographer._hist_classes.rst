Histogram Classes (`tomographer`)
=================================


This collection of classes take care of recording, storing and manipulating a histogram.
Typically such a class would be returned by a core task such as
:py:func:`tomographer.tomorun.tomorun()`.

.. rubric:: Skip to:

- :py:class:`tomographer.HistogramParams`

- :py:class:`tomographer.Histogram`

- :py:class:`tomographer.HistogramWithErrorBars`


.. versionchanged:: 5.0

   The histogram classes now use NumPy internal arrays for storing their data, so that the
   count data type is not statically fixed.  The `HistogramReal` class has been deprecated.

.. versionchanged:: 5.0

   The `AveragedSimpleHistogram`, `AveragedSimpleRealHistogram` and
   `AveragedErrorBarHistogram` classes have been deprecated.  The logic is that they
   aren't really needed especially on the Python side, and if we wish to make this
   functionality available it would make more sense to do so via the
   :tomocxx:`AggregatedHistogram classes
   <class_tomographer_1_1_aggregated_histogram_simple.html>`.


.. rubric:: Deprecated Aliases

The following aliases are provided for compatibility with older versions of Tomographer
(before Tomographer 5): `UniformBinsHistogramParams`, `UniformBinsHistogram`,
`UniformBinsRealHistogram`, and `UniformBinsHistogramWithErrorBars`.



.. automodule:: tomographer
    :members: HistogramParams, Histogram, HistogramWithErrorBars
    :show-inheritance:



