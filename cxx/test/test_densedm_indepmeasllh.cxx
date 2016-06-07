/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#include <cmath>

#include <string>
#include <sstream>
#include <random>

#include <boost/math/constants/constants.hpp>

// include before <Eigen/*> !
#include "test_tomographer.h"

#include <tomographer2/densedm/indepmeasllh.h>


// -----------------------------------------------------------------------------
// fixture(s)


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_densedm_indeapmeasllh)

BOOST_AUTO_TEST_CASE(basic)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt;
  
  Tomographer::DenseDM::IndepMeasLLH<DMTypes> dat(dmt);

  dat.initMeasVector(6);
  dat.Exn <<
    0.5, 0.5,  0.707107,  0,
    0.5, 0.5, -0.707107,  0,
    0.5, 0.5,  0,         0.707107,
    0.5, 0.5,  0,        -0.707107,
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  dat.Nx << 1500, 800, 300, 300, 10, 30;

  DMTypes::VectorParamType x(dmt.initVectorParamType());
  x << 0.5, 0.5, 0, 0; // maximally mixed state
  
  DMTypes::RealScalar value = dat.calc_llh(x);
  
  BOOST_CHECK_CLOSE(value, 4075.70542169248, 1e-4);
  //std::cout << "llh @ mixed state = " << std::setprecision(15) << value << "\n";
}

BOOST_AUTO_TEST_CASE(basic_dyn)
{
  typedef Tomographer::DenseDM::DMTypes<Eigen::Dynamic> DMTypes;
  DMTypes dmt(2);
  
  Tomographer::DenseDM::IndepMeasLLH<DMTypes> dat(dmt);

  dat.initMeasVector(6);
  dat.Exn <<
    0.5, 0.5,  0.707107,  0,
    0.5, 0.5, -0.707107,  0,
    0.5, 0.5,  0,         0.707107,
    0.5, 0.5,  0,        -0.707107,
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  dat.Nx << 1500, 800, 300, 300, 10, 30;

  DMTypes::VectorParamType x(dmt.initVectorParamType());
  x << 0.5, 0.5, 0, 0; // maximally mixed state
  
  const DMTypes::VectorParamType xconst(x); // make sure calc_llh() accepts const argument
  DMTypes::RealScalar value = dat.calc_llh(xconst);
  
  BOOST_CHECK_CLOSE(value, 4075.70542169248, 1e-4);
  //std::cout << "llh @ mixed state = " << std::setprecision(15) << value << "\n";
}

BOOST_AUTO_TEST_SUITE_END()

