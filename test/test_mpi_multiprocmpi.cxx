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
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/string.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mpi/multiprocmpi.h>

#include <tomographer/tools/boost_test_logger.h>

#include "test_multi_tasks_common.h"



// -----------------------------------------------------------------------------
// fixture(s) and helper(s)


// serialization for MyTaskInput

namespace boost {
namespace serialization {
template<class Archive>
void serialize(Archive & ar, MyTaskInput & x, const unsigned int /*version*/)
{
  ar & x.a;
  ar & x.b;
}
} // namespace serialization
} // namespace boost
namespace boost { namespace mpi {
  template <>
  struct is_mpi_datatype<MyTaskInput> : mpl::true_ { };
} }


//
// simple result type which is serializable & default-constructible
//
struct SimpleTestTaskResultType {
  SimpleTestTaskResultType(int value_ = -1) : msg(), value(value_) { }
  std::string msg;
  int value;
};

namespace boost {
namespace serialization {
template<class Archive>
void serialize(Archive & ar, SimpleTestTaskResultType & x, const unsigned int /*version*/)
{
  ar & x.msg;
  ar & x.value;
}
} // namespace serialization
} // namespace boost
// namespace boost { namespace mpi {
//   template <>
//   struct is_mpi_datatype<SimpleTestTaskResultType> : mpl::true_ { };
// } }



//
// TestBasicCDataMPI must have default constructor, and must be serializable
//
struct TestBasicCDataMPI {

  TestBasicCDataMPI(int c_ = -1) : c(c_), inputs() { }
  int c;

  std::vector<MyTaskInput> inputs;

  template<typename IntType>
  MyTaskInput getTaskInput(IntType k) const {
    return inputs[(std::size_t)k];
  }

private:
  friend class boost::serialization::access;
  template<class Archive>
  inline void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & c;
    ar & inputs;
  }
};


struct TestTaskMPI {
  //
  // A very simple task.  The task is to calculate the sum of two inputs, "a" and "b", and
  // multiply the result by some common number "c" stored in TestBasicCData.
  //

  typedef TestBasicCDataMPI TestCDataType;

  typedef MyTaskInput Input;

  typedef Tomographer::MultiProc::TaskStatusReport StatusReportType;

  typedef SimpleTestTaskResultType ResultType;
  
  template<typename LoggerType>
  TestTaskMPI(Input input, const TestCDataType * , LoggerType & logger)
    : _input(input), _result(-1)
  {
    logger.debug("TestTask::TestTask", "constructor called") ;
  }

  template<typename LoggerType, typename TaskManagerIface>
  void run(const TestCDataType * pcdata, LoggerType & logger, TaskManagerIface * mgriface)
  {
    //BOOST_MESSAGE("Running task.") ; // BOOST_TEST_MESSAGE may not be thread-safe!!!!!
    logger.info("TestTask::run", "Running task.") ;
    logger.debug("TestTask::run", "running task.");

    const int NN = 1000000;

    for (int i = 0; i < NN; ++i) {
      _result.value = ( _input.a + _input.b ) * pcdata->c ;
      _result.msg = Tomographer::Tools::fmts("((a=%d)+(b=%d))*(c=%d) == %d",
                                             _input.a, _input.b, pcdata->c, _result.value);

      if (i % 1000 == 0 && mgriface->statusReportRequested()) {
        mgriface->submitStatusReport(
            StatusReportType((double)i/NN,
                             streamstr("working very hard ... " << i << "/" << NN))
            );
      }

    }

    logger.info("TestTask::run", "Task finished.") ;
    //    BOOST_MESSAGE("Task finished.") ;
  }

  inline ResultType getResult() const { return _result; }
  inline ResultType stealResult() { return std::move(_result); }

  Input _input;
  ResultType _result;
  
};



struct test_task_dispatcher_MPI_fixture {
  TestBasicCDataMPI cData;
  const int num_runs;

  const std::vector<int> correct_result_values;
  
  test_task_dispatcher_MPI_fixture()
    : cData(1000),
      num_runs(10),
      correct_result_values(mkvec<int>()
                            <<3000
                            <<30000
                            <<3000
                            <<9000
                            <<3000
                            <<20000
                            <<3000
                            <<3000
                            <<17000
                            <<3000)
  {
    cData.inputs = mkvec<MyTaskInput>()
      << MyTaskInput(1, 2)
      << MyTaskInput(10, 20)
      << MyTaskInput(1, 2)
      << MyTaskInput(4, 5)
      << MyTaskInput(1, 2)
      << MyTaskInput(-1, 21)
      << MyTaskInput(1, 2)
      << MyTaskInput(1, 2)
      << MyTaskInput(8, 9)
      << MyTaskInput(1, 2);
  }

  template<typename LocalLoggerType>
  void check_correct_results(const std::vector<TestTaskMPI::ResultType*> & results,
                             LocalLoggerType & logger)
  {
    BOOST_CHECK_EQUAL(correct_result_values.size(), results.size()) ;
    for (std::size_t k = 0; k < results.size(); ++k) {
      logger.debug("checking results[%zu]=%p", k, results[k]);
      BOOST_CHECK_EQUAL(results[k]->value, correct_result_values[k]) ;
    }
  }

  template<typename TaskDispatcherType, typename LoggerType>
  void check_correct_results_collected(const TaskDispatcherType & task_dispatcher,
                                       LoggerType & baselogger)
  {
    auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, baselogger);

    // collectedTaskResults()
    const std::vector<TestTaskMPI::ResultType*> results = task_dispatcher.collectedTaskResults();

    logger.debug("checking that results are correct... results.size()=%zu", results.size()) ;
    check_correct_results(results, logger);

    logger.debug("using numTaskRuns()...") ;
    // numTaskRuns()
    BOOST_CHECK_EQUAL(results.size(), (std::size_t)task_dispatcher.numTaskRuns());

    logger.debug("using collectedTaskResult()...") ;
    // collectedTaskResult(k)
    const std::size_t N = (std::size_t)task_dispatcher.numTaskRuns();
    for (std::size_t k = 0; k < N; ++k) {
      BOOST_CHECK_EQUAL(task_dispatcher.collectedTaskResult(k).value, correct_result_values[k]) ;
    }

    logger.debug("done");
  }
};




// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mpi_multiprocmpi)

BOOST_FIXTURE_TEST_CASE(tasks_run, test_task_dispatcher_MPI_fixture)
{
  mpi::environment env;
  mpi::communicator world;

  Tomographer::Logger::FileLogger filelogger(stderr, Tomographer::Logger::DEBUG);//LONGDEBUG);
  typedef Tomographer::Logger::OriginPrefixedLogger<Tomographer::Logger::FileLogger>
    LoggerType;

  LoggerType logger(filelogger, streamstr(world.rank() << "/" << world.size()<<"|"));


  TestBasicCDataMPI * pcdata;
  if (world.rank() == 0) {
    pcdata = &cData;
  } else {
    pcdata = NULL;
  }


  Tomographer::MultiProc::MPI::TaskDispatcher<TestTaskMPI, TestBasicCDataMPI,
                                              LoggerType, long>
    task_dispatcher(pcdata, world, logger, num_runs);

  BOOST_MESSAGE("About to run MPI tasks");

  task_dispatcher.run();

  if (world.rank() == 0) {
    logger.debug("test case", "about to collect & check results") ;
    check_correct_results_collected(task_dispatcher, logger);
    logger.debug("test case", "collect & checked results done.") ;
  } else {
    // should generate tomographer_assert failure (eigen_assert)
    EigenAssertTest::setting_scope settingvariable(true); // eigen_assert() should throw an exception.
    BOOST_CHECK_THROW(
        task_dispatcher.collectedTaskResult(0),
        ::Tomographer::Tools::EigenAssertException
        );
  }
}

BOOST_AUTO_TEST_SUITE_END()

