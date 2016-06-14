/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#ifndef TEST_MH_RANDOM_WALK_H
#define TEST_MH_RANDOM_WALK_H

#include <random>

#include <tomographer2/mhrw.h>
#include <tomographer2/tools/cxxutil.h>
#include <tomographer2/tools/loggers.h>

#include <Eigen/Core>

//
// Necessary utilities to perform a random walk on a lattice. just to perform test cases
// with integers so that we're sure we have deterministic results (e.g. from different
// compilers)
//
template<typename ScalarType, typename Rng, typename LoggerType = Tomographer::Logger::VacuumLogger>
struct TestLatticeMHRWBase
{
  typedef Eigen::Matrix<ScalarType,Eigen::Dynamic,1> PointType;
  typedef double StepRealType; // needed for MHWalker interface
  
  const Eigen::Array<int,Eigen::Dynamic,1> latticeDims;
  Rng rng;

  Tomographer::Logger::LocalLogger<LoggerType> _logger;

  TestLatticeMHRWBase(const Eigen::Ref<const Eigen::Array<int,Eigen::Dynamic,1> > & dims,
		      typename Rng::result_type seed = 0,
		      LoggerType & baselogger = Tomographer::Logger::vacuum_logger)
    : latticeDims(dims), rng(seed), _logger(TOMO_ORIGIN, baselogger)
  {
  }

  inline void init() { }

  inline PointType startpoint()
  {
    return PointType::Zero(latticeDims.size());
  }

  inline void thermalizing_done() { }

  inline void done() { }

  template<typename RealType>
  inline PointType jump_fn(const PointType & curpt, const RealType step_size)
  {
    auto rnddist = _get_step_rnddist(step_size);

    _logger.longdebug([&](std::ostream & stream) {
	stream << "jump_fn(), step_size="<<step_size<<", rnddist="<<rnddist;
      });

    PointType newpt(latticeDims.size());
    for (int k = 0; k < latticeDims.size(); ++k) {

      ScalarType delta = rnddist(rng);
      _logger.longdebug([&](std::ostream&stream){stream << "delta["<<k<<"] = " << delta; });

      ScalarType newcoord = curpt(k) + delta;
      if (newcoord < 0) {
	newcoord = 0; // don't wrap around, because fn might be discontinuous += latticeDims(k);
      }
      if (newcoord >= latticeDims(k)) {
	newcoord = latticeDims(k)-1; // don't wrap around
      }
      newpt(k) = newcoord;
    }

    return newpt;
  }

  //internal:

  //make sure it's a signed scalar type
  TOMO_STATIC_ASSERT_EXPR((int)ScalarType(-1) == -1);

  template<typename RealType, TOMOGRAPHER_ENABLED_IF_TMPL(std::is_integral<ScalarType>::value)>
  inline std::uniform_int_distribution<ScalarType> _get_step_rnddist(RealType step_size)
  { 
    ScalarType istep = ScalarType(1+step_size);
    return std::uniform_int_distribution<ScalarType>(-istep, istep);
  }

  template<typename RealType, TOMOGRAPHER_ENABLED_IF_TMPL(!std::is_integral<ScalarType>::value)>
  inline std::uniform_real_distribution<ScalarType> _get_step_rnddist(RealType step_size)
  { 
    // normal floating type, no correction needed
    ScalarType step = ScalarType(step_size);
    return std::uniform_real_distribution<ScalarType>(-step, step);
  }

};


// M-H Random walk on a lattice (Periodic Boundary Conditions), using a Gaussian peak with
// some parameters as distribution.
//
// Complies to MHWalker type interface.
//
template<typename ScalarType, typename Rng = std::mt19937,
	 typename LoggerType = Tomographer::Logger::VacuumLogger>
struct TestLatticeMHRWGaussPeak
  : public TestLatticeMHRWBase<ScalarType, Rng, LoggerType>
{
  typedef TestLatticeMHRWBase<ScalarType, Rng, LoggerType> Base;
  typedef Eigen::Matrix<ScalarType,Eigen::Dynamic,Eigen::Dynamic> SigmaType;
  typedef typename Base::PointType PointType;

  const SigmaType Sigma;
  const ScalarType SigmaInvScale;
  const PointType Offset;

  TestLatticeMHRWGaussPeak(const Eigen::Ref<const Eigen::Array<int,Eigen::Dynamic,1> > & dims,
			   const Eigen::Ref<const SigmaType> & Sigma_,
			   ScalarType SigmaInvScale_,
			   const Eigen::Ref<const PointType> & Offset_,
			   int seed = 0,
			   LoggerType & logger = Tomographer::Logger::vacuum_logger)
    : Base(dims, seed, logger),
      Sigma(Sigma_), SigmaInvScale(SigmaInvScale_),
      Offset(Offset_)
  {
  }

  //typedef ScalarType FnValueType; // if scalartype=int, use int here also to make everything deterministic.
  typedef double FnValueType; // use 'double' otherwise it's impossible to make MHRW steps "smooth"
  enum { UseFnSyntaxType = Tomographer::MHUseFnLogValue } ;

  inline FnValueType fnlogval(const PointType & pt)
  {
    FnValueType vval = - FnValueType((pt - Offset).transpose() * Sigma * (pt - Offset)) / SigmaInvScale;
    Base::_logger.sublogger(TOMO_ORIGIN).longdebug([&](std::ostream & stream) {
	stream << "pt = " << pt.transpose() << "; Offset = " << Offset.transpose()
	       << "; SigmaInvScale = " << SigmaInvScale << "; Sigma =\n"
	       << Sigma << " --> value is = "
	       << vval;
      });
    return vval;
  }

};


#endif
