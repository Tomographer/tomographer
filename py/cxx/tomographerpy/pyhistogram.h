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

typedef Tomographer::UniformBinsHistogramParams<RealType> UniformBinsHistogramParams;

typedef Tomographer::UniformBinsHistogram<RealType, CountIntType> UniformBinsHistogram;

typedef Tomographer::UniformBinsHistogram<RealType, RealType> UniformBinsRealHistogram;

// UniformBinsWithErrorBars
typedef Tomographer::UniformBinsHistogramWithErrorBars<RealType, RealType> UniformBinsHistogramWithErrorBars;


// AveragedSimpleHistogram (underlying histogram w/o error bars)
typedef Tomographer::AveragedHistogram<UniformBinsHistogram, RealType> AveragedSimpleHistogram;

typedef Tomographer::AveragedHistogram<UniformBinsRealHistogram, RealType> AveragedSimpleRealHistogram;

typedef Tomographer::AveragedHistogram<UniformBinsHistogramWithErrorBars, RealType> AveragedErrorBarHistogram;


} // namespace Py


#endif
