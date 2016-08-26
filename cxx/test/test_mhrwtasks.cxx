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
#include <iostream>
#include <random>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/mhrwtasks.h>
#include <tomographer2/tools/loggers.h>
#include <tomographer2/multiprocomp.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/densedm/tspacellhwalker.h>


// -----------------------------------------------------------------------------
// fixture(s)

typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> DenseLLH;
typedef Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes> OurValueCalculator;

typedef Tomographer::UniformBinsHistogram<OurValueCalculator::ValueType> OurHistogramType;

struct MyCData : public Tomographer::MHRWTasks::CDataBase<> {
  MyCData(const DenseLLH & llh_, const typename DMTypes::MatrixType & ref_T,
	  OurHistogramType::Params p, MHRWParamsType mhrwp, int base_seed)
    : Tomographer::MHRWTasks::CDataBase<>(mhrwp, base_seed),
      llh(llh_), vcalc(ref_T), histogram_params(p)
  {
  }

  DenseLLH llh;
  OurValueCalculator vcalc;

  OurHistogramType::Params histogram_params;

  typedef OurHistogramType MHRWStatsCollectorResultType;

  template<typename LoggerType>
  inline Tomographer::ValueHistogramMHRWStatsCollector<OurValueCalculator,LoggerType,OurHistogramType>
  createStatsCollector(LoggerType & logger) const
  {
    return Tomographer::ValueHistogramMHRWStatsCollector<OurValueCalculator,LoggerType,OurHistogramType>(
	histogram_params,
	vcalc,
	logger
	);
  }

  template<typename Rng, typename LoggerType>
  inline Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType>
  createMHWalker(Rng & rng, LoggerType & log) const
  {
    return Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType>(
	llh.dmt.initMatrixType(),
	llh,
	rng,
	log
	);
  }

};


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mhrwtasks)

BOOST_AUTO_TEST_CASE(instanciation)
{
  // use a strict logging mechanism (statically discard messages below WARNING). Change
  // this for debugging.
  Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::DEBUG);
  Tomographer::Logger::MinimumSeverityLogger<Tomographer::Logger::BufferLogger,
					     Tomographer::Logger::WARNING>  logger(buflog);
  // for debugging, use e.g. this:
  //Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::LONGDEBUG);
  //auto & logger = buflog;

  // some initializations

  logger.info("dmmhrwtask0", "testing our integrator with Pauli meas. on a qubit ... ");

  DMTypes dmt;
  DenseLLH llh(dmt);

  typename DenseLLH::VectorParamListType Exn(6,dmt.dim2());
  Exn <<
    0.5, 0.5,  std::sqrt(0.5),  0,
    0.5, 0.5, -std::sqrt(0.5),  0,
    0.5, 0.5,  0,               std::sqrt(0.5),
    0.5, 0.5,  0,              -std::sqrt(0.5),
    1,   0,    0,               0,
    0,   1,    0,               0
    ;
  typename DenseLLH::FreqListType Nx(6);
  // try to reproduce the nice "1qubit-test9-pureup-extreme-onlyupmeas" curve
  Nx << 0, 0, 0, 0, 250, 0;

  llh.setMeas(Exn, Nx);

  DMTypes::MatrixType ref_T(dmt.initMatrixType());
  ref_T <<
    1, 0,
    0, 0;

  // Now, create the task object.

  // seed for random number generator
  const int base_seed = 1000; // fixed seed for deterministic results in this test case.
  //taskcdat.base_seed = std::chrono::system_clock::now().time_since_epoch().count();

  // parameters of the random walk
  MyCData taskcdat(llh, ref_T,
		   OurHistogramType::Params(0.98, 1.0, 50),  // parameters for the fidelity histogram
		   MyCData::MHRWParamsType(0.05, 20, 100, 1000),
		   base_seed);

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<MyCData, std::mt19937> OurMHRWTask;

  OurMHRWTask thetask(134, &taskcdat, logger);

  //  thetask.run();
  
  BOOST_CHECK( false && "WRITE ME" ) ;
}

BOOST_AUTO_TEST_CASE(base)
{
  BOOST_CHECK( false && "WRITE ME" ) ;
}

BOOST_AUTO_TEST_SUITE_END()

