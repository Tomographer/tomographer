/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef PY_HISTOGRAM_H
#define PY_HISTOGRAM_H


#include <tomographerpy/common.h>

#include <tomographer/histogram.h>


namespace tpy {

//! Histogram Params. See \ref Tomographer::HistogramParams
typedef Tomographer::HistogramParams<RealType> HistogramParams;

//! A Histogram with integer counts. See \ref Tomographer::Histogram
typedef Tomographer::Histogram<RealType, CountIntType> Histogram;

//! A Histogram with real counts. See \ref Tomographer::Histogram
typedef Tomographer::Histogram<RealType, RealType> HistogramReal;

//! A Histogram with real counts and error bars. See \ref Tomographer::HistogramWithErrorBars
typedef Tomographer::HistogramWithErrorBars<RealType, RealType> HistogramWithErrorBars;


/** \brief Average of histograms with underlying histograms without error bars
 *         and with integer counts. See \ref Tomographer::AveragedHistogram
 */
typedef Tomographer::AveragedHistogram<Histogram, RealType> AveragedSimpleHistogram;

/** \brief Average of histograms with underlying histograms without error bars
 *         and with real counts. See \ref Tomographer::AveragedHistogram
 */
typedef Tomographer::AveragedHistogram<HistogramReal, RealType> AveragedSimpleRealHistogram;

/** \brief Average of histograms with underlying histograms with error bars (and
 *         with real counts). See \ref Tomographer::AveragedHistogram
 */
typedef Tomographer::AveragedHistogram<HistogramWithErrorBars, RealType> AveragedErrorBarHistogram;


} // namespace Py


#endif
