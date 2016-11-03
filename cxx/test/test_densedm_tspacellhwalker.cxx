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

#include <tomographer2/densedm/tspacellhwalker.h>

#include <unsupported/Eigen/MatrixFunctions>

#include <tomographer2/densedm/param_herm_x.h>
#include <tomographer2/densedm/densellh.h>
#include <tomographer2/densedm/indepmeasllh.h>

#include <tomographer2/tools/boost_test_logger.h>


// -----------------------------------------------------------------------------
// fixture(s)


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_densedm_tspacellhwalker)

TOMO_STATIC_ASSERT_EXPR(Tomographer::DenseDM::TSpace::LLHMHWalker<
                        Tomographer::DenseDM::IndepMeasLLH<Tomographer::DenseDM::DMTypes<2> >,
                        std::mt19937, Tomographer::Logger::VacuumLogger
                        >::UseFnSyntaxType == (int)Tomographer::MHUseFnLogValue) ;

BOOST_AUTO_TEST_CASE(tspacellhmhwalker)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt;
  
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> DenseLLH;
  DenseLLH llh(dmt);

  const double SQRT22 = boost::math::constants::half_root_two<double>();

  DenseLLH::VectorParamListType Exn(6, dmt.dim2());
  Exn <<
    0.5, 0.5,  SQRT22,  0,
    0.5, 0.5, -SQRT22,  0,
    0.5, 0.5,  0,       SQRT22,
    0.5, 0.5,  0,      -SQRT22,
    1,   0,    0,       0,
    0,   1,    0,       0
    ;
  DenseLLH::FreqListType Nx(6);
  Nx << 1500, 800, 300, 300, 10, 30;

  llh.setMeas(Exn, Nx, false);

  typedef Tomographer::Logger::BoostTestLogger LoggerType;
  LoggerType logger(Tomographer::Logger::DEBUG);

  std::mt19937 rng(46570); // seeded rng, deterministic results
  
  Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH, std::mt19937, LoggerType>
    dmmhrw(DMTypes::MatrixType::Zero(), llh, rng, logger);

  DMTypes::MatrixType rho(dmt.initMatrixType());
  rho << 0.8, dmt.cplx(0,0.1),
    dmt.cplx(0,-0.1), 0.2;

  DMTypes::VectorParamType x(dmt.initVectorParamType());
  x = Tomographer::DenseDM::ParamX<DMTypes>(dmt).HermToX(rho);
  DMTypes::MatrixType T(dmt.initMatrixType());
  T = rho.sqrt();

  dmmhrw.init();
  dmmhrw.thermalizingDone();

  const DMTypes::MatrixType Tconst(T); // make sure that fnlogval() accepts const argument
  BOOST_CHECK_CLOSE(dmmhrw.fnLogVal(Tconst), llh.logLikelihoodX(x), tol_percent);

  {
    // Check that the jump distribution is symmetric.  Idea: draw many samples, and
    // average them in linear space (in which T is a hypersphere): normalize that, and you
    // should get T again.
    DMTypes::MatrixType sumT(dmt.initMatrixType());
    const int N_SAMPLES = 10000;
    int num_samples;
    for (num_samples = 0; num_samples < N_SAMPLES; ++num_samples) {
      DMTypes::MatrixType newT = dmmhrw.jumpFn(T, 0.2);
      BOOST_CHECK_CLOSE(newT.norm(), 1.0, tol_percent);
      sumT += newT;
    }
    //sumT /= num_samples; // ### unnecessary, already included in next step:
    sumT /= sumT.norm();
    MY_BOOST_CHECK_EIGEN_EQUAL(sumT, T, 1.0/std::sqrt((double)num_samples));
  }

  dmmhrw.done();
}


BOOST_AUTO_TEST_SUITE_END()

