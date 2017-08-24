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

#ifndef PYDENSEDM_H
#define PYDENSEDM_H

#include <string>

#include <tomographerpy/common.h>

#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/indepmeasllh.h>
#include <tomographer/densedm/param_herm_x.h>


namespace tpy {

//! The \ref Tomographer::DenseDM::DMTypes we should use by default, with dynamic sized matrices
typedef Tomographer::DenseDM::DMTypes<Eigen::Dynamic, RealScalar> DMTypes;

//! The \ref Tomographer::DenseDM::IndepMeasLLH type we should use by default, with dynamic sized matrices
typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes, RealScalar, FreqCountIntType> IndepMeasLLH;

//! A vector of frequencies (using Eigen::Matrix instead of Eigen::Array as in Tomographer::DenseDM::IndepMeasLLH)
typedef Eigen::Matrix<FreqCountIntType, Eigen::Dynamic, 1> FreqCountIntVectorType;

//! A \ref Tomographer::DenseDM::ParamX set with our dynamic-sized types
typedef Tomographer::DenseDM::ParamX<DMTypes> ParamX;

}


#endif
