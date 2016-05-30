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

#include <tomographer/densedm/dmmhrw.h>

#include <tomographer/densedm/indepmeasllh.h>


// -----------------------------------------------------------------------------
// fixture(s)


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_densedm_dmmhrw)

BOOST_AUTO_TEST_CASE(basic)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt;
  
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> DenseLLH;
  DenseLLH llh(dmt);

  llh.initMeasVector(6);
  llh.Exn <<
    0.5, 0.5,  0.707107,  0,
    0.5, 0.5, -0.707107,  0,
    0.5, 0.5,  0,         0.707107,
    0.5, 0.5,  0,        -0.707107,
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  llh.Nx << 1500, 800, 300, 300, 10, 30;

  typedef Tomographer::Logger::BufferLogger LoggerType;
  LoggerType buflog(Tomographer::Logger::DEBUG);

  std::mt19937 rng(0); // seeded rng, deterministic results
  
  Tomographer::DenseDM::StateSpaceLLHMHWalker<DenseLLH, std::mt19937, LoggerType>
    dmmhrw(DMTypes::MatrixType::Zero(), llh, rng, buflog);

  DMTypes::VectorParamType x;
  x << 0.5, 0.5, 0, 0; // maximally mixed state.
  DMTypes::MatrixType T;
  T << boost::math::constants::half_root_two<double>(), 0.5, 0, 0; // maximally mixed state.

  BOOST_CHECK_CLOSE(
  
  
  DMTypes::RealScalar value = llh.calc_llh(x);
  
  BOOST_MESSAGE(buflog.get_contents());

  BOOST_CHECK_CLOSE(value, 4075.70542169248, 1e-4);

  //std::cout << "llh @ mixed state = " << std::setprecision(15) << value << "\n";

}


BOOST_AUTO_TEST_SUITE_END()

