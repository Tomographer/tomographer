
#include <cstdio>
#include <random>
#include <iostream>
#include <vector>

#include "test_tomographer.h"

#include <tomographer/mhrw.h>

#include "test_mh_random_walk.h" // our test-case random walk


BOOST_AUTO_TEST_SUITE(test_mhrw)
// -----------------------------------------------------------------------------

struct test_mhrandomwalk_fixture
{
  struct TestMHWalker : public TestLatticeMHRWGaussPeak<int> {
    typedef TestLatticeMHRWGaussPeak<int> Base;

    int count_jump;
    
    int Nthermchk;
    int Nrunchk;
    int Nsweepchk;
    
    TestMHWalker(int sweep_size, int check_n_therm, int check_n_run)
      : TestLatticeMHRWGaussPeak(
	  Eigen::Vector2i::Constant(100),
	  (Eigen::Matrix2i() << 10, -5, 5, 10).finished(),
	  (Eigen::Vector2i() << 40, 50).finished(),
	  414367 // seed, fixed -> deterministic
	  ),
	count_jump(0),
	Nthermchk(check_n_therm),
	Nrunchk(check_n_run),
	Nsweepchk(sweep_size)
    {
    }
    
    inline void init()
    {
      Base::init();
      BOOST_CHECK_EQUAL(count_jump, 0);
      count_jump = 0;
    }
    
    inline PointType startpoint()
    {
      BOOST_CHECK_EQUAL(count_jump, 0);
      return PointType::Zero(latticeDims.size());
    }
    
    inline void thermalizing_done()
    {
      BOOST_CHECK_EQUAL(count_jump, Nthermchk*Nsweepchk);
    }
    
    inline void done()
    {
      BOOST_CHECK_EQUAL(count_jump, Nthermchk*Nsweepchk + Nrunchk*Nsweepchk);
    }
    
    template<typename PT>
    inline PointType jump_fn(PT&& curpt, const RealScalar step_size)
    {
      ++count_jump;
      return Base::jump_fn(std::forward<PointType>(curpt), step_size);
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
    void thermalizing_done()
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
    void process_sample(Args... /*a*/)
    //CountIntType k, CountIntType n, const PointType & pt, FnValueType fnval, MHRandomWalk & rw)
    {
      ++count_samples;
    }
    template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk>
    void raw_move(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted, double a,
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
  
};


BOOST_FIXTURE_TEST_CASE(mhrandomwalk, test_mhrandomwalk_fixture)
{
  typedef Tomographer::Logger::VacuumLogger LoggerType;
  LoggerType logger;

  typedef std::mt19937 Rng;
  Rng rng(3040); // fixed seed

  typedef Tomographer::MHRandomWalk<Rng, TestMHWalker, TestMHRWStatsCollector, LoggerType, int>
    MHRandomWalkType;

  const int ntherm = 50;
  const int nrun = 100;
  const int nsweep = 10;

  TestMHWalker mhwalker(nsweep, ntherm, nrun);
  TestMHRWStatsCollector stats(nsweep, ntherm, nrun);
  MHRandomWalkType rw(nsweep, ntherm, nrun, 2, mhwalker, stats, rng, logger);

  BOOST_CHECK_EQUAL(rw.n_sweep(), nsweep);
  BOOST_CHECK_EQUAL(rw.n_therm(), ntherm);
  BOOST_CHECK_EQUAL(rw.n_run(), nrun);

  BOOST_CHECK(!rw.has_acceptance_ratio());

  rw.run();

}



// BOOST_AUTO_TEST_CASE(MultiMHRW)
// {
//   struct statscollector {
//     int cnt...............................;
//   };

// }


// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END()

