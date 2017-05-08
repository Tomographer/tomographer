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

#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <signal.h>

#include <random>
#include <map>
#include <chrono>
#include <algorithm>
#include <string>
#include <functional>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/multiprocomp.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/mhrwtasks.h>
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/indepmeasllh.h>
#include <tomographer/densedm/tspacefigofmerit.h>
#include <tomographer/densedm/tspacellhwalker.h>

#include <tomographer/tools/boost_test_logger.h>

#include "test_multi_tasks_common.h"



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
      buflog.getContents(),
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
  
  BOOST_CHECK_EQUAL(buflog.getContents(), "[origin] test message\n");
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

  std::string buflog_str = buflog.getContents();

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


BOOST_FIXTURE_TEST_CASE(tasks_run, test_task_dispatcher_fixture)
{
  Tomographer::Logger::BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);
  Tomographer::MultiProc::OMP::TaskDispatcher<TestTask, TestBasicCData, 
                                              Tomographer::Logger::BoostTestLogger, long>
      task_dispatcher(&cData, logger, num_runs, 1);

  BOOST_MESSAGE("About to run tasks");

  task_dispatcher.run();

  check_correct_results();
}

BOOST_FIXTURE_TEST_CASE(make_task_dispatcher, test_task_dispatcher_fixture)
{
  typedef Tomographer::MultiProc::OMP::TaskDispatcher<TestTask, TestBasicCData, 
                                                      Tomographer::Logger::BoostTestLogger, int>  TaskDispatcherType;

  Tomographer::Logger::BoostTestLogger logger;

  auto task_dispatcher = Tomographer::MultiProc::OMP::makeTaskDispatcher<TestTask>(
      &cData, logger,
      num_runs, 1);

  // just check that the type was properly deduced (including all template parameters)
  BOOST_CHECK_EQUAL(std::string(typeid(task_dispatcher).name()),
                    std::string(typeid(TaskDispatcherType).name()));
}

struct TestTaskCheckAlignedStack : public TestTask {
  template<typename... Args>
  TestTaskCheckAlignedStack(Args&&... x)
    : TestTask(std::forward<Args>(x)...)
  { }

  template<typename LoggerType, typename TaskManagerIface>
  void run(const TestBasicCData * pcdata, LoggerType & logger, TaskManagerIface * iface)
  {
    char blah_data[5] = {0}; // some random stuff -- not really needed, it's just here to clutter the code and memory
    
    // test variable to make sure that the stack is aligned
    Eigen::Matrix4d m;

    // BOOST_TEST_MESSAGE is not thread-safe
    //    BOOST_TEST_MESSAGE( "m.data() == " << (uintptr_t)m.data() );
    logger.debug("TestTaskCheckAlignedStack::run()", "m.data() == %p", (void*)m.data());
    // Neither is BOOST_CHECK because it calls BOOST_TEST_MESSAGE !
    // BOOST_CHECK( (((uintptr_t)m.data()) & 0xf) == 0 ); // pointer to matrix data is aligned to multiple of 16 bytes
    if ( ! ( (((uintptr_t)m.data()) & 0xf) == 0 ) ) { // pointer to matrix data is aligned to multiple of 16 bytes
      // FAILED:
      throw std::runtime_error("m.data() memory not aligned to 0xf bytes!! test case failed.");
    }

    // and run the parent task
    TestTask::run(pcdata, logger, iface);

    (void)blah_data;
  }
};

BOOST_FIXTURE_TEST_CASE(inner_code_stack_aligned, test_task_dispatcher_fixture)
{
  Tomographer::Logger::BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);
  Tomographer::MultiProc::OMP::TaskDispatcher<TestTaskCheckAlignedStack, TestBasicCData,
                                              Tomographer::Logger::BoostTestLogger, long>
      task_dispatcher(&cData, logger, num_runs, 1);

  char blah_data[4] = {0}; // some random stuff -- not really needed, it's just here to clutter the code and memory
  char blah_data2[7] = {0}; // some random stuff -- not really needed, it's just here to clutter the code and memory

  BOOST_MESSAGE("About to run tasks.");

  task_dispatcher.run();

  BOOST_MESSAGE("All tasks finished, run() completed.");

  check_correct_results();

  (void)blah_data;
  (void)blah_data2;
}


BOOST_FIXTURE_TEST_SUITE(status_reporting, test_task_dispatcher_status_reporting_fixture) ;

BOOST_AUTO_TEST_CASE(status_report_periodic)
{
  Tomographer::Logger::BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);

  Tomographer::MultiProc::OMP::TaskDispatcher<StatusRepTestTask, StatusRepTestBasicCData,
                                              Tomographer::Logger::BoostTestLogger, long>
      task_dispatcher(&cData, logger, num_runs, 1);

  perform_test_status_report_periodic(task_dispatcher, logger) ;
}


BOOST_AUTO_TEST_CASE(interrupt_tasks_withthread)
{
  Tomographer::Logger::BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);

  Tomographer::MultiProc::OMP::TaskDispatcher<StatusRepTestTask, StatusRepTestBasicCData,
                                              Tomographer::Logger::BoostTestLogger, long>
      task_dispatcher(&cData, logger, num_runs, 1);

  perform_test_interrupt_tasks_withthread(task_dispatcher, logger) ;

}
BOOST_AUTO_TEST_CASE(status_report_withthread)
{
  Tomographer::Logger::BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);

  Tomographer::MultiProc::OMP::TaskDispatcher<StatusRepTestTask, StatusRepTestBasicCData,
                                              Tomographer::Logger::BoostTestLogger, long>
      task_dispatcher(&cData, logger, num_runs, 1);

  perform_test_status_report_withthread(task_dispatcher, logger);
}

//
// Also provide some testing for non-OpenMP enabled platforms:
//

BOOST_AUTO_TEST_CASE(status_report_withsigalrm)
{
  Tomographer::Logger::BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);

  Tomographer::MultiProc::OMP::TaskDispatcher<StatusRepTestTask, StatusRepTestBasicCData,
                                              Tomographer::Logger::BoostTestLogger, long>
      task_dispatcher(&cData, logger, num_runs, 1);

  perform_test_status_report_withsigalrm(task_dispatcher, logger);
}


BOOST_AUTO_TEST_SUITE_END(); // status_reporting

BOOST_AUTO_TEST_SUITE_END();



// =============================================================================
BOOST_AUTO_TEST_SUITE_END();


