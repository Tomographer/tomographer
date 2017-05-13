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

#ifndef TEST_MH_RANDOM_WALK_H
#define TEST_MH_RANDOM_WALK_H

#include <random>

#include <tomographer/mhrw.h>
#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/loggers.h>

#include <Eigen/Core>


struct StepSizeType {
  StepSizeType(double x = 0) : stepsize(x) { }
  double stepsize;
};

std::ostream & operator<<(std::ostream & stream, StepSizeType x) {
  stream << "howdy y'all your step size will be " << x.stepsize;
  return stream;
}

//
// Necessary utilities to perform a random walk on a lattice. just to perform test cases
// with integers so that we're sure we have deterministic results (e.g. from different
// compilers)
//
template<typename ScalarType, typename Rng, typename LoggerType = Tomographer::Logger::VacuumLogger>
struct TestLatticeMHRWBase
{
  typedef Eigen::Matrix<ScalarType,Eigen::Dynamic,1> PointType;
  typedef StepSizeType WalkerParams; // needed for MHWalker interface
  
  const Eigen::Array<int,Eigen::Dynamic,1> latticeDims;
  Rng & rng;

  Tomographer::Logger::LocalLogger<LoggerType> _logger;

  TestLatticeMHRWBase(const Eigen::Ref<const Eigen::Array<int,Eigen::Dynamic,1> > & dims,
		      Rng & rng_,
		      LoggerType & baselogger = Tomographer::Logger::vacuum_logger)
    : latticeDims(dims), rng(rng_), _logger(TOMO_ORIGIN, baselogger)
  {
  }
  // seem to need this for g++4.6
  TestLatticeMHRWBase(TestLatticeMHRWBase&& m)
    : latticeDims(std::move(m.latticeDims)),
      rng(m.rng),
      _logger(std::move(m._logger))
  {
  }

  inline void init() { }

  inline PointType startPoint()
  {
    return PointType::Zero(latticeDims.size());
  }

  inline void thermalizingDone() { }

  inline void done() { }

  inline PointType jumpFn(const PointType & curpt, const WalkerParams wp)
  {
    const auto step_size = wp.stepsize;
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
			   Rng & rng_,
			   LoggerType & logger = Tomographer::Logger::vacuum_logger)
    : Base(dims, rng_, logger),
      Sigma(Sigma_), SigmaInvScale(SigmaInvScale_),
      Offset(Offset_)
  {
  }

  // seem to need this for g++4.6
  TestLatticeMHRWGaussPeak(TestLatticeMHRWGaussPeak&& m)
    : Base(std::move(m)),
      Sigma(std::move(m.Sigma)),
      SigmaInvScale(m.SigmaInvScale),
      Offset(std::move(m.Offset))
  {
  }


  //typedef ScalarType FnValueType; // if scalartype=int, use int here also to make everything deterministic.
  typedef double FnValueType; // use 'double' otherwise it's impossible to make MHRW steps "smooth"
  enum { UseFnSyntaxType = Tomographer::MHUseFnLogValue } ;

  inline FnValueType fnLogVal(const PointType & pt)
  {
    FnValueType vval = - FnValueType((pt - Offset).transpose() * Sigma * (pt - Offset)) / SigmaInvScale;
    Base::_logger.subLogger(TOMO_ORIGIN).longdebug([&](std::ostream & stream) {
	stream << "pt = " << pt.transpose() << "; Offset = " << Offset.transpose()
	       << "; SigmaInvScale = " << SigmaInvScale << "; Sigma =\n"
	       << Sigma << " --> value is = "
	       << vval;
      });
    return vval;
  }

};


struct TestMHWalker : public TestLatticeMHRWGaussPeak<int> {
  typedef TestLatticeMHRWGaussPeak<int> Base;

  int count_jump;
    
  int Nthermchk;
  int Nrunchk;
  int Nsweepchk;

  TestMHWalker(int sweep_size, int check_n_therm, int check_n_run, std::mt19937 & rng)
    : TestLatticeMHRWGaussPeak(
        Eigen::Vector2i::Constant(100),
        (Eigen::Matrix2i() << 10, -5, 5, 10).finished(), 1,
        (Eigen::Vector2i() << 40, 50).finished(),
        rng
        ),
      count_jump(0),
      Nthermchk(check_n_therm),
      Nrunchk(check_n_run),
      Nsweepchk(sweep_size)
  {
  }
  TestMHWalker(TestMHWalker&& ) = default;
    
  inline void init()
  {
    Base::init();
    BOOST_CHECK_EQUAL(count_jump, 0);
    count_jump = 0;
  }
    
  inline PointType startPoint()
  {
    BOOST_CHECK_EQUAL(count_jump, 0);
    return PointType::Zero(latticeDims.size());
  }
    
  inline void thermalizingDone()
  {
    BOOST_CHECK_EQUAL(count_jump, Nthermchk*Nsweepchk);
  }
    
  inline void done()
  {
    BOOST_CHECK_EQUAL(count_jump, Nthermchk*Nsweepchk + Nrunchk*Nsweepchk);
  }
    
  template<typename PT>
  inline PointType jumpFn(PT&& curpt, const WalkerParams wp)
  {
    ++count_jump;
    return Base::jumpFn(std::forward<PointType>(curpt), wp.stepsize);
  }
};






struct TestMHRWStatsCollector
{
  int count_rawmoves;
  int count_samples;
    
  int Nthermchk;
  int Nrunchk;
  int Nsweepchk;
    
  TestMHRWStatsCollector(int sweep_size, int check_n_therm, int check_n_run)
    : count_rawmoves(0),
      count_samples(0),
      Nthermchk(check_n_therm),
      Nrunchk(check_n_run),
      Nsweepchk(sweep_size)
  {
  }
  void init()
  {
    BOOST_CHECK_EQUAL(count_rawmoves, 0);
    BOOST_CHECK_EQUAL(count_samples, 0);
  }
  void thermalizingDone()
  {
    BOOST_CHECK_EQUAL(count_rawmoves, Nthermchk*Nsweepchk);
    BOOST_CHECK_EQUAL(count_samples, 0);
  }
  void done()
  {
    BOOST_CHECK_EQUAL(count_rawmoves, Nthermchk*Nsweepchk+Nrunchk*Nsweepchk);
    BOOST_CHECK_EQUAL(count_samples, Nrunchk);
  }
  template<typename... Args>
  void processSample(Args&&... /*a*/)
  //CountIntType k, CountIntType n, const PointType & pt, FnValueType fnval, MHRandomWalk & rw)
  {
    ++count_samples;
  }
  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk>
  void rawMove(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted, double a,
                const PointType & /*newpt*/, FnValueType /*newptval*/, const PointType & /*curpt*/,
                FnValueType /*curptval*/, MHRandomWalk & /*rw*/)
  {
    if (count_rawmoves < Nthermchk*Nsweepchk) {
      BOOST_CHECK_EQUAL(k, count_rawmoves);
    } else {
      BOOST_CHECK_EQUAL(k, count_rawmoves-Nthermchk*Nsweepchk);
    }
    BOOST_CHECK_EQUAL((count_rawmoves < Nthermchk*Nsweepchk), is_thermalizing);
    BOOST_CHECK_EQUAL(is_live_iter, !is_thermalizing && ((k+1)%Nsweepchk == 0));

    BOOST_MESSAGE("a = " << a);
    if (a + std::numeric_limits<double>::epsilon() >= 1.0) {
      BOOST_CHECK(accepted);
    }

    ++count_rawmoves;
  }
};

struct TestMHRWStatsCollectorWithResult
  : public TestMHRWStatsCollector
{
  typedef bool ResultType;
  
  // set to \a true once "done()" is called
  ResultType _result;

  TestMHRWStatsCollectorWithResult(int sweep_size, int check_n_therm, int check_n_run)
    : TestMHRWStatsCollector(sweep_size, check_n_therm, check_n_run), _result(false)
  {
  }

  void done()
  {
    TestMHRWStatsCollector::done();
    _result = true;
  }

  ResultType getResult() const
  {
    return _result;
  }

  ResultType stealResult() const { return getResult(); }
};



#endif
