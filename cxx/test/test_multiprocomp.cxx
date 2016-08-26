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

#include <cstdio>
#include <cstring>

#include <random>
#include <map>
#include <chrono>
#include <algorithm>
#include <string>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/multiprocomp.h>
#include <tomographer2/tools/loggers.h>
#include <tomographer2/mhrwtasks.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/densedm/tspacellhwalker.h>

#include "boost_test_logger.h"


// -----------------------------------------------------------------------------
// fixtures:



// -----------------------------------------------------------------------------
// test cases:


BOOST_AUTO_TEST_SUITE(test_multiprocomp)
// =============================================================================

BOOST_AUTO_TEST_SUITE(OMPThreadSanitizerLogger)

BOOST_AUTO_TEST_CASE(relays_logs)
{
  Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::DEBUG);
  Tomographer::MultiProc::OMP::ThreadSanitizerLogger<Tomographer::Logger::BufferLogger> testtasklogger(buflog);
  testtasklogger.longdebug("origin", "longdebug level");
  testtasklogger.debug("origin", "debug level");
  testtasklogger.info("origin", "info level");
  testtasklogger.warning("origin", "warning level");
  testtasklogger.error("origin", "error level");
  
  BOOST_CHECK_EQUAL(
      buflog.get_contents(),
      "[origin] debug level\n"
      "[origin] info level\n"
      "[origin] warning level\n"
      "[origin] error level\n"
      );
}

BOOST_AUTO_TEST_CASE(fixes_level)
{
  Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::LONGDEBUG);

  Tomographer::MultiProc::OMP::ThreadSanitizerLogger<Tomographer::Logger::BufferLogger> testtasklogger(buflog);

  // this should NOT have any effect for testtasklogger, because OMP::ThreadSanitizerLogger
  // should fix the level at construction time for thread-safety/consistency reasons.
  buflog.setLevel(Tomographer::Logger::WARNING);

  testtasklogger.longdebug("origin", "test message");
  
  BOOST_CHECK_EQUAL(buflog.get_contents(), "[origin] test message\n");
}

BOOST_AUTO_TEST_CASE(parallel)
{
  // 
  // Make sure that the output of the log is not mangled. We sort the lines because of
  // course the order is undefined, but each line should be intact (thanks to
  // OMP::ThreadSanitizerLogger's wrapping into "#pragma omp critical" sections).
  //

  Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::LONGDEBUG);
  
#pragma omp parallel shared(buflog)
  {
    Tomographer::MultiProc::OMP::ThreadSanitizerLogger<Tomographer::Logger::BufferLogger> testtasklogger(buflog);
    testtasklogger.longdebug("main()", "test task logger from core #%06d of %06d",
			     omp_get_thread_num(), omp_get_num_threads());
  }

  std::string buflog_str = buflog.get_contents();

  BOOST_MESSAGE("buflog contents: \n" << buflog_str);
  BOOST_CHECK(buflog_str.size());

  std::vector<std::string> lines;
  std::stringstream ss(buflog_str);
  std::string line;
  while (std::getline(ss, line, '\n')) {
    lines.push_back(line);
  }

  std::sort(lines.begin(), lines.end()); // std::string's operator< does lexicographical comparision

  std::ostringstream sorted_stream;
  std::copy(lines.begin(), lines.end(), std::ostream_iterator<std::string>(sorted_stream, "\n"));
  std::string sorted = sorted_stream.str();


  std::string reference_str;
  for (int k = 0; k < (int)lines.size(); ++k) {
    reference_str += Tomographer::Tools::fmts("[main()] test task logger from core #%06d of %06d\n",
					      k, (int)lines.size());
  }

  BOOST_CHECK_EQUAL(sorted, reference_str);
}


BOOST_AUTO_TEST_SUITE_END();


// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(t__MultiProc__OMP__TaskDispatcher);

BOOST_AUTO_TEST_CASE(write_me)
{
  BOOST_CHECK(false) ;
}

struct MyTaskInput {
  MyTaskInput(int a_ = 0, int b_ = 0) : a(a_), b(b_) { }
  int a;
  int b;
};

struct TestBasicCData {
  TestBasicCData(int c_) : c(c_), inputs() { }
  int c;

  std::vector<MyTaskInput> inputs;

  MyTaskInput getTaskInput(int k) const {
    return inputs[k];
  }
};
struct TestTask {
  //
  // A very simple task.  The task is to calculate the sum of two inputs, "a" and "b", and
  // multiply the result by some common number "c" stored in TestBasicCData.
  //

  typedef MyTaskInput Input;

  typedef Tomographer::MultiProc::StatusReport StatusReportType;
  
  struct ResultType {
    ResultType(int value_ = -1) : msg(), value(value_) { }
    std::string msg;
    int value;
  };

  template<typename LoggerType>
  TestTask(Input input, const TestBasicCData * , LoggerType & logger)
    : _input(input)
  {
    logger.debug("TestTask::TestTask", "constructor called") ;
  }

  template<typename LoggerType, typename TaskManagerIface>
  void run(const TestBasicCData * pcdata, LoggerType & logger, TaskManagerIface * )
  {
    BOOST_MESSAGE("Running task.") ;
    logger.debug("TestTask::run", "running task.");
    _result.value = ( _input.a + _input.b ) * pcdata->c ;
    _result.msg = Tomographer::Tools::fmts("((a=%d)+(b=%d))*(c=%d) == %d",
                                           _input.a, _input.b, pcdata->c, _result.value);
  }

  ResultType getResult() const { return _result; }

  Input _input;
  ResultType _result;
  
};
struct TestResultsCollector {
  TestResultsCollector(std::vector<TestTask::ResultType> check_correct_results_, int num_runs_,
                       const TestBasicCData * pcdata_)
    : init_called(0), collectres_called(0), runsfinished_called(0),
      num_runs(num_runs_), pcdata(pcdata_),
      check_correct_results(std::move(check_correct_results_))
  {
  }
  void init(int num_total_runs, int n_chunk, const TestBasicCData * pcdata_)
  {
    BOOST_CHECK_EQUAL(num_total_runs, num_runs) ;
    BOOST_CHECK_EQUAL(n_chunk, 1) ;
    BOOST_CHECK_EQUAL(pcdata, pcdata_) ;
    ++init_called;
  }
  template<typename ResultType>
  void collectResult(int task_no, const ResultType& taskresult, const TestBasicCData * pcdata_)
  {
    BOOST_CHECK_GE(task_no, 0); BOOST_CHECK_LT(task_no, check_correct_results.size());
    BOOST_CHECK_EQUAL(taskresult.value, check_correct_results[task_no].value);
    BOOST_CHECK_EQUAL(pcdata, pcdata_) ;
    BOOST_MESSAGE("Collected result: " << taskresult.msg) ;
    ++collectres_called;
  }
  void runsFinished(int num_total_runs, const TestBasicCData * pcdata_)
  {
    BOOST_CHECK_EQUAL(num_total_runs, num_runs) ;
    BOOST_CHECK_EQUAL(pcdata, pcdata_) ;
    ++runsfinished_called;
  }

  int init_called;
  int collectres_called;
  int runsfinished_called;
  const int num_runs;
  const TestBasicCData * pcdata;
  const std::vector<TestTask::ResultType> check_correct_results;
};

// http://stackoverflow.com/a/23036970/1694896
template<typename T>
class mkvec {
public:
  typedef mkvec<T> my_type;
  my_type& operator<< (const T& val) {
    data_.push_back(val);
    return *this;
  }
  my_type& operator<< (const std::vector<T>& inVector) {
    this->data_.reserve(this->data_.size() + inVector.size());
    this->data_.insert(this->data_.end(), inVector.begin(), inVector.end());
    return *this;
  }
  operator std::vector<T>() const {
    return data_;
  }
private:
    std::vector<T> data_;
};


struct test_task_dispatcher_fixture {
  TestBasicCData cData;
  const int num_runs;
  TestResultsCollector resultsCollector;
  
  test_task_dispatcher_fixture()
    : cData(1000),
      num_runs(10),
      resultsCollector(mkvec<TestTask::ResultType>()
                       <<3000
                       <<30000
                       <<3000
                       <<9000
                       <<3000
                       <<20000
                       <<3000
                       <<3000
                       <<17000
                       <<3000,
                       num_runs, &cData)
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
};

BOOST_FIXTURE_TEST_CASE(tasks_run, test_task_dispatcher_fixture)
{
  BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);
  Tomographer::MultiProc::OMP::TaskDispatcher<TestTask, TestBasicCData, TestResultsCollector,
                                              BoostTestLogger, long>
      task_dispatcher(&cData, &resultsCollector, logger, num_runs, 1);

  BOOST_MESSAGE("About to run tasks");

  task_dispatcher.run();

  BOOST_CHECK_EQUAL(resultsCollector.init_called, 1);
  BOOST_CHECK_EQUAL(resultsCollector.collectres_called, num_runs);
  BOOST_CHECK_EQUAL(resultsCollector.runsfinished_called, 1);
}

BOOST_FIXTURE_TEST_CASE(make_task_dispatcher, test_task_dispatcher_fixture)
{
  typedef Tomographer::MultiProc::OMP::TaskDispatcher<TestTask, TestBasicCData, TestResultsCollector,
                                                      BoostTestLogger, int>  TaskDispatcherType;

  BoostTestLogger logger;

  auto task_dispatcher = Tomographer::MultiProc::OMP::makeTaskDispatcher<TestTask>(
      &cData, &resultsCollector, logger,
      num_runs, 1);

  // just check that the type was properly deduced (including all template parameters)
  BOOST_CHECK_EQUAL(std::string(typeid(task_dispatcher).name()),
                    std::string(typeid(TaskDispatcherType).name()));
}

BOOST_AUTO_TEST_CASE(inner_code_stack_aligned)
{
  BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE(status_report)
{
  // test also FullStatusReport.
  BOOST_CHECK(false);
}

BOOST_AUTO_TEST_SUITE_END();



// =============================================================================
BOOST_AUTO_TEST_SUITE_END();


