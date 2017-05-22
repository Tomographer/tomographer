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

#include <tomographer/mhrwacceptratiowalkerparamscontroller.h>
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

struct DummyMHWalkerParams {
  DummyMHWalkerParams(std::string x = "") : hey(std::move(x)) { }

  std::string hey{};
};

inline std::ostream &operator<<(std::ostream & stream, const DummyMHWalkerParams & p) { return stream << "hey:"<<p.hey; }

struct test_params_adjuster
{
  bool initparams_called{false};
  Tomographer::MHRWParams<DummyMHWalkerParams,long> init_params{};

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void initParams(MHRWParamsType & params, const MHWalker & , const MHRandomWalkType & )
  {
    BOOST_TEST_MESSAGE("test_params_adjuster::initParams: params = " << params) ;
    initparams_called = true;
    init_params = params;
  }

  bool adjustcall_called{false};
  Tomographer::MHRWParams<DummyMHWalkerParams,long> adjustcall_set_params{ std::string("zzz-set-params-zzz"),
      /* */                                                                 111, 222, 333 };
  double adjustcall_accept_ratio{0};
  long adjustcall_iter_k{0};

  template<typename MHRWParamsType, typename MHRWAcceptRatioWalkerParamsControllerType,
           typename MHWalker, typename MHRandomWalkType>
  inline void adjustParamsForAcceptRatio(MHRWParamsType & params, double accept_ratio,
                                         const MHRWAcceptRatioWalkerParamsControllerType & /*controller*/,
                                         const MHWalker & /*mhwalker*/, long iter_k,
                                         const MHRandomWalkType & /*mhrw*/)
  {
    BOOST_TEST_MESSAGE("test_params_adjuster::adjustParamsForAcceptRatio") ;
    adjustcall_called = true;
    params = adjustcall_set_params;
    adjustcall_accept_ratio = accept_ratio;
    adjustcall_iter_k = iter_k;
  }
};

struct mhrwacceptratiowalkerparamsctrl_fixture
{
  SimulatorMovAvgStatsColl mvavg{0}; // buffer-size simulated at 1024 -- see above

  Tomographer::Logger::BoostTestLogger logger{Tomographer::Logger::LONGDEBUG};

  test_params_adjuster params_adjuster{};

  // the controller
  Tomographer::MHRWAcceptRatioWalkerParamsController<test_params_adjuster,
                                                     SimulatorMovAvgStatsColl,
                                                     Tomographer::Logger::BoostTestLogger,
                                                     long>
    ctrl{mvavg, logger, params_adjuster,
      // desired min/max
      0.3,
      0.4,
      // acceptable min/max
      0.2,
      0.5,
      // ensure fraction of therm sweeps at final step size
      0.9
      };

  DummyMHWalker dmhwalker;
  DummyMHRandomWalk drw;

  // n_sweep=150 and movavg.bufferSize() not multiple of one another -- don't change these values!!!
  // below, we need n_therm multiple of 1024
  Tomographer::MHRWParams<DummyMHWalkerParams,long> p{std::string("xxx-initial-params-xxx"), 150, 1024, 8192};

  mhrwacceptratiowalkerparamsctrl_fixture()
  {
  }

};


// -----------------------------------------------------------------------------
// test suites

BOOST_AUTO_TEST_SUITE(test_mhrwacceptratiowalkerparamscontroller)
// =============================================================================

BOOST_AUTO_TEST_CASE(constants)
{
  BOOST_CHECK_LE(Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMin,
                 Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMin) ;
  BOOST_CHECK_LE(Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMin,
                 Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMax) ;
  BOOST_CHECK_LE(Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMax,
                 Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMax) ;
  BOOST_CHECK_LE(0.1, Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::EnsureNThermFixedParamsFraction) ;
  BOOST_CHECK_LE(Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::EnsureNThermFixedParamsFraction, 1.0) ;
}

BOOST_FIXTURE_TEST_CASE(constmembers, mhrwacceptratiowalkerparamsctrl_fixture)
{
  // it adjusts parameters during thermalization, and not during runs
  BOOST_CHECK_EQUAL( (ctrl.AdjustmentStrategy & Tomographer::MHRWControllerAdjustRWStageMASK) ,
                     Tomographer::MHRWControllerAdjustWhileThermalizing ) ;
  // it adjusts parameters during iterations (not at samples anyway, there are none during thermalization)
  BOOST_CHECK_EQUAL( (ctrl.AdjustmentStrategy & Tomographer::MHRWControllerAdjustFrequencyMASK) ,
                     Tomographer::MHRWControllerAdjustEveryIteration ) ;

  MY_BOOST_CHECK_FLOATS_EQUAL(ctrl.desiredAcceptRatioMin(), 0.3, tol) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(ctrl.desiredAcceptRatioMax(), 0.4, tol) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(ctrl.acceptableAcceptRatioMin(), 0.2, tol) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(ctrl.acceptableAcceptRatioMax(), 0.5, tol) ;

  MY_BOOST_CHECK_FLOATS_EQUAL(ctrl.ensureNThermFixedParamsFraction(), 0.9, tol);

  BOOST_CHECK_EQUAL(ctrl.originalNTherm(), 0) ; // only set after init(), see init() test case below
}

BOOST_FIXTURE_TEST_CASE(ctrldefault, mhrwacceptratiowalkerparamsctrl_fixture)
{
  Tomographer::MHRWAcceptRatioWalkerParamsController<test_params_adjuster,
                                                     SimulatorMovAvgStatsColl,
                                                     Tomographer::Logger::BoostTestLogger,
                                                     long>
    ctrldefault{mvavg, logger, params_adjuster};

  MY_BOOST_CHECK_FLOATS_EQUAL(ctrldefault.desiredAcceptRatioMin(),
                              Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMin,
                              tol) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(ctrldefault.desiredAcceptRatioMax(),
                              Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMax,
                              tol) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(ctrldefault.acceptableAcceptRatioMin(),
                              Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMin,
                              tol) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(ctrldefault.acceptableAcceptRatioMax(),
                              Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMax,
                              tol) ;

  MY_BOOST_CHECK_FLOATS_EQUAL(ctrldefault.ensureNThermFixedParamsFraction(),
                              Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::EnsureNThermFixedParamsFraction,
                              tol);
}

BOOST_FIXTURE_TEST_CASE(init_params, mhrwacceptratiowalkerparamsctrl_fixture)
{
  const auto & cdmhwalker = dmhwalker;
  const auto & cdrw = drw;

  // call init()
  ctrl.init(p, cdmhwalker, cdrw) ;

  // init() shouldn't modify the params
  BOOST_CHECK_EQUAL(p.mhwalker_params.hey, std::string("xxx-initial-params-xxx")) ;
  BOOST_CHECK_EQUAL(p.n_sweep, 150) ;
  BOOST_CHECK_EQUAL(p.n_therm, 1024) ;
  BOOST_CHECK_EQUAL(p.n_run, 8192) ;

  // initParams() called correctly?
  BOOST_CHECK(params_adjuster.initparams_called) ;
  BOOST_CHECK_EQUAL(params_adjuster.init_params.mhwalker_params.hey, std::string("xxx-initial-params-xxx")) ;
  BOOST_CHECK_EQUAL(params_adjuster.init_params.n_sweep, 150) ;
  BOOST_CHECK_EQUAL(params_adjuster.init_params.n_therm, 1024) ;
  BOOST_CHECK_EQUAL(params_adjuster.init_params.n_run, 8192) ;

  // now originalNTherm() should be correct
  BOOST_CHECK_EQUAL(ctrl.originalNTherm(), p.n_therm) ;
}

BOOST_FIXTURE_TEST_CASE(adjusts_params, mhrwacceptratiowalkerparamsctrl_fixture)
{
  const auto & cdmhwalker = dmhwalker;
  const auto & cdrw = drw;
  // always call init()
  ctrl.init(p, cdmhwalker, cdrw) ;

  mvavg.accept_ratio_value = 0.12; // bad acceptance ratio

  ctrl.adjustParams<true,false>(p, dmhwalker, 1024, drw); // iter_k a multiple of bufferSize

  // adjustParamsForAcceptRatio() called correctly?
  
  // callback should be called
  BOOST_CHECK(params_adjuster.adjustcall_called) ;

  // p should be adjusted
  BOOST_CHECK_EQUAL(p.mhwalker_params.hey, params_adjuster.adjustcall_set_params.mhwalker_params.hey);
  BOOST_CHECK_EQUAL(p.n_sweep, params_adjuster.adjustcall_set_params.n_sweep);
  BOOST_CHECK_EQUAL(p.n_therm, params_adjuster.adjustcall_set_params.n_therm);
  BOOST_CHECK_EQUAL(p.n_run, params_adjuster.adjustcall_set_params.n_run);

  // these values should be correct
  BOOST_CHECK_EQUAL(params_adjuster.adjustcall_iter_k, 1024) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(params_adjuster.adjustcall_accept_ratio, 0.12, tol) ;
}

BOOST_FIXTURE_TEST_CASE(allows_done_thermalization_right_1, mhrwacceptratiowalkerparamsctrl_fixture)
{
  const auto & cp = p;
  const auto & cdmhwalker = dmhwalker;
  const auto & cdrw = drw;
  // always call init()
  ctrl.init(p, cdmhwalker, cdrw) ;
  // don't make adjustParams() mess with parameters, we're keeping count here...
  params_adjuster.adjustcall_set_params = p;

  mvavg.accept_ratio_value = 0.21; // bad acceptance ratio: not desired

  // prevents from stopping while acceptance rate is unacceptable
  BOOST_CHECK( ! ctrl.allowDoneThermalization(cp, cdmhwalker, p.n_therm*p.n_sweep, cdrw) );

  // adjust params here -- shouldn't have an effect later because the accept_ratio is
  // acceptable.  The argument is a multiple of 1024 (the bufferSize) because n_therm is
  // multiple of 1024.
  ctrl.adjustParams<true,false>(p, dmhwalker, p.n_therm*p.n_sweep, drw);

  mvavg.accept_ratio_value = 0.31; // now ok

  // prevents from stopping while acceptance rate is unacceptable
  BOOST_CHECK( ctrl.allowDoneThermalization(cp, cdmhwalker, p.n_therm*p.n_sweep+1024, cdrw) );
}
BOOST_FIXTURE_TEST_CASE(allows_done_thermalization_right_2, mhrwacceptratiowalkerparamsctrl_fixture)
{
  const auto & cp = p;
  const auto & cdmhwalker = dmhwalker;
  const auto & cdrw = drw;
  // always call init()
  ctrl.init(p, cdmhwalker, cdrw) ;
  // don't make adjustParams() mess with parameters, we're keeping count here...
  params_adjuster.adjustcall_set_params = p;

  mvavg.accept_ratio_value = 0.19; // bad acceptance ratio: not even acceptable

  // adjust params here -- sets the last correction to now, preventing finishing
  // thermalization right away even if we get good accept ratio.  The argument is a
  // multiple of 1024 (the bufferSize) because n_therm is multiple of 1024.
  ctrl.adjustParams<true,false>(p, dmhwalker, p.n_therm*p.n_sweep, drw);

  mvavg.accept_ratio_value = 0.34; // acceptance ratio ok (desired)

  // doesn't allow done, because we need more thermalization runs
  BOOST_CHECK( ! ctrl.allowDoneThermalization(cp, cdmhwalker, (p.n_therm+1)*p.n_sweep, cdrw) );

  // allows done after n_therm+0.9*n_therm sweeps because enough therm sweeps have passed at acceptable accept ratio
  BOOST_CHECK( ctrl.allowDoneThermalization(cp, cdmhwalker, (long)((p.n_therm*1.9+1)*p.n_sweep+1), cdrw) );
}

BOOST_FIXTURE_TEST_CASE(allows_done_runs, mhrwacceptratiowalkerparamsctrl_fixture)
{
  const auto & cp = p;
  const auto & cdmhwalker = dmhwalker;
  const auto & cdrw = drw;
  // always call init()
  ctrl.init(p, cdmhwalker, cdrw) ;

  // allows done after n_therm + 0.9*n_therm because enough therm runs have passed at acceptable accept ratio
  BOOST_CHECK( ctrl.allowDoneRuns(cp, cdmhwalker, p.n_run*p.n_sweep, cdrw) );
}


// =============================================================================
BOOST_AUTO_TEST_SUITE_END()

