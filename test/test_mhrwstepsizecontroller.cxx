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

#include <cmath>

#include <string>
#include <iostream>
#include <random>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mhrwstepsizecontroller.h>
#include <tomographer/tools/boost_test_logger.h>



// -----------------------------------------------------------------------------
// fixture(s)

struct SimulatorMovAvgStatsColl
{
  double accept_ratio_value;

  // called by the controller
  inline int bufferSize() const { return 1024; } // pretend we calculated the value from this many samples 
  inline bool hasMovingAverageAcceptanceRatio() const { return true; }
  inline double movingAverageAcceptanceRatio() const { return accept_ratio_value; }
};

struct DummyMHWalker { };
struct DummyMHRandomWalk { };


struct mhrwstepsizectrl_fixture
{
  SimulatorMovAvgStatsColl mvavg{0};

  Tomographer::Logger::BoostTestLogger logger;

  // the controller
  Tomographer::MHRWStepSizeController<SimulatorMovAvgStatsColl,Tomographer::Logger::BoostTestLogger>
    ctrl{mvavg, logger};

  DummyMHWalker dmhwalker;
  DummyMHRandomWalk drw;

  Tomographer::MHRWParams<Tomographer::MHWalkerParamsStepSize<double>,int> p{0.01, 150, 2048, 32768};

  mhrwstepsizectrl_fixture()
  {
  }

};


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mhrwstepsizecontroller)

BOOST_AUTO_TEST_CASE(constants)
{
  BOOST_CHECK_LE(Tomographer::MHRWStepSizeControllerDefaults::AcceptableAcceptanceRatioMin,
                 Tomographer::MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMin) ;
  BOOST_CHECK_LE(Tomographer::MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMin,
                 Tomographer::MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMax) ;
  BOOST_CHECK_LE(Tomographer::MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMax,
                 Tomographer::MHRWStepSizeControllerDefaults::AcceptableAcceptanceRatioMax) ;
  BOOST_CHECK_LE(0.1, Tomographer::MHRWStepSizeControllerDefaults::EnsureNThermFixedParamsFraction) ;
  BOOST_CHECK_LE(Tomographer::MHRWStepSizeControllerDefaults::EnsureNThermFixedParamsFraction, 1.0) ;
}

BOOST_FIXTURE_TEST_CASE(corrects_lowar, mhrwstepsizectrl_fixture)
{
  // it adjusts parameters during thermalization, and not during runs
  BOOST_CHECK_EQUAL( (ctrl.AdjustmentStrategy & Tomographer::MHRWControllerAdjustRWStageMASK) ,
                     Tomographer::MHRWControllerAdjustWhileThermalizing ) ;
  // it adjusts parameters during iterations (not at samples anyway, there are none during thermalization)
  BOOST_CHECK_EQUAL( (ctrl.AdjustmentStrategy & Tomographer::MHRWControllerAdjustFrequencyMASK) ,
                     Tomographer::MHRWControllerAdjustEveryIteration ) ;

  const auto & cp = p;
  const auto & cdmhwalker = dmhwalker;
  const auto & cdrw = drw;

  // init() shouldn't modify the params, because there are enough samples
  ctrl.init(p, cdmhwalker, cdrw) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(p.mhwalker_params.step_size, 0.01, tol) ;
  BOOST_CHECK_EQUAL(p.n_sweep, 150) ;
  BOOST_CHECK_EQUAL(p.n_therm, 2048) ;
  BOOST_CHECK_EQUAL(p.n_run, 32768) ;

  mvavg.accept_ratio_value = 0.12; // too low acceptance ratio

  ctrl.adjustParams<true,false>(p, dmhwalker, 1024, drw); // iter_k must be mult of bufferSize
  // check that step size decreased
  BOOST_CHECK_LT(p.mhwalker_params.step_size, 0.0095) ;
  // check that sweep was compensated to same product step*sweep
  MY_BOOST_CHECK_FLOATS_EQUAL(p.n_sweep*p.mhwalker_params.step_size, 1.5, p.mhwalker_params.step_size) ;

  // prevents from stopping while acceptance rate is unacceptable
  BOOST_CHECK( ! ctrl.allowDoneThermalization(cp, cdmhwalker, p.n_therm*p.n_sweep, cdrw) );

  // but allows so once acceptance ratio is good
  mvavg.accept_ratio_value = 0.28;
  BOOST_CHECK( ctrl.allowDoneThermalization(cp, cdmhwalker, (p.n_therm+1)*p.n_sweep, cdrw) );
}

BOOST_FIXTURE_TEST_CASE(corrects_highar, mhrwstepsizectrl_fixture)
{
  const auto & cp = p;
  const auto & cdmhwalker = dmhwalker;
  const auto & cdrw = drw;

  // init() shouldn't modify the params, because there are enough samples
  ctrl.init(p, cdmhwalker, cdrw) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(p.mhwalker_params.step_size, 0.01, tol) ;
  BOOST_CHECK_EQUAL(p.n_sweep, 150) ;
  BOOST_CHECK_EQUAL(p.n_therm, 2048) ;
  BOOST_CHECK_EQUAL(p.n_run, 32768) ;

  mvavg.accept_ratio_value = 0.75; // too high acceptance ratio

  ctrl.adjustParams<true,false>(p, dmhwalker, 1024, drw); // iter_k must be mult of bufferSize
  // check that step size decreased
  BOOST_CHECK_GT(p.mhwalker_params.step_size, 0.0105) ;
  // check that sweep was compensated to same product step*sweep
  MY_BOOST_CHECK_FLOATS_EQUAL(p.n_sweep*p.mhwalker_params.step_size, 1.5, p.mhwalker_params.step_size) ;

  // prevents from stopping while acceptance rate is unacceptable
  BOOST_CHECK( ! ctrl.allowDoneThermalization(cp, cdmhwalker, p.n_therm*p.n_sweep, cdrw) );

  // but allows so once acceptance ratio is good
  mvavg.accept_ratio_value = 0.28;
  BOOST_CHECK( ctrl.allowDoneThermalization(cp, cdmhwalker, (p.n_therm+1)*p.n_sweep, cdrw) );
}

BOOST_AUTO_TEST_SUITE_END()

