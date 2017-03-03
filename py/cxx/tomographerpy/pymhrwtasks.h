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

#ifndef PYMHRWTASKS_H
#define PYMHRWTASKS_H

#include <string>

#include <tomographerpy/common.h>

#include <tomographer/mhrw.h>
#include <tomographer/mhrw_bin_err.h>
#include <tomographer/mhrwtasks.h>

#include <tomographerpy/pyhistogram.h>
#include <tomographerpy/pymhrw.h>


namespace tpy {

namespace tomo_internal {
struct DummyValueCalculator {
  typedef RealType ValueType;
  template<typename PointType>
  inline ValueType getValue(const PointType & ) const { return 0; }
};
} // tomo_internal

typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<
    tomo_internal::DummyValueCalculator, CountIntType, RealType, Eigen::Dynamic, Eigen::Dynamic
  >::Result
  ValueHistogramWithBinningMHRWStatsCollectorResult;



typedef Tomographer::MHRWTasks::MHRandomWalkTaskResult<
    ValueHistogramWithBinningMHRWStatsCollectorResult, CountIntType, RealType
  >
  MHRandomWalkValueHistogramTaskResult;


} // Py











#endif
