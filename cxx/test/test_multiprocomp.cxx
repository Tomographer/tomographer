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
#include "test_multi_tasks_common.h"



// -----------------------------------------------------------------------------
// fixtures:



std::function<void()> sigalarm_act;

void sigalarm_act_cfn(int signum)
{
  printf("[SIGALRM]\n");
  if (signum == SIGALRM) {
    sigalarm_act();
  }
}


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

struct TestTaskCheckAlignedStack : public TestTask {
  template<typename... Args>
  TestTaskCheckAlignedStack(Args&&... x)
    : TestTask(std::forward<Args>(x)...)
  { }

  template<typename LoggerType, typename TaskManagerIface>
  void run(const TestBasicCData * pcdata, LoggerType & logger, TaskManagerIface * iface)
  {
    char blah_data[4] = {0}; // some random stuff -- not really needed, it's just here to clutter the code and memory
    
    // make sure that the stack is aligned
    Eigen::Matrix4d m;
    BOOST_MESSAGE( "m.data() == " << (uintptr_t)m.data() );
    BOOST_CHECK( (((uintptr_t)m.data()) & 0xf) == 0 ); // pointer to matrix data is aligned to multiple of 16 bytes

    // and run the parent task
    TestTask::run(pcdata, logger, iface);

    (void)blah_data;
  }
};

BOOST_FIXTURE_TEST_CASE(inner_code_stack_aligned, test_task_dispatcher_fixture)
{
  BoostTestLogger logger(Tomographer::Logger::DEBUG);
  Tomographer::MultiProc::OMP::TaskDispatcher<TestTaskCheckAlignedStack, TestBasicCData,
                                              TestResultsCollector,
                                              BoostTestLogger, long>
      task_dispatcher(&cData, &resultsCollector, logger, num_runs, 1);

  char blah_data[4] = {0}; // some random stuff -- not really needed, it's just here to clutter the code and memory
  char blah_data2[7] = {0}; // some random stuff -- not really needed, it's just here to clutter the code and memory

  task_dispatcher.run();

  (void)blah_data;
  (void)blah_data2;
}


struct StatusRepTestBasicCData {
  StatusRepTestBasicCData() { }

  bool getTaskInput(int k) const {
    return (k == 0);
  }

  //  void do_request_status_report() const
  //  {
  //    fn();
  //  }
  //
  //  std::function<void()> fn;
};
struct StatusRepTestTask {
  typedef bool Input;

  typedef Tomographer::MultiProc::TaskStatusReport StatusReportType;
  
  typedef bool ResultType;

  template<typename LoggerType>
  StatusRepTestTask(Input input, const StatusRepTestBasicCData * , LoggerType & )
    : _input(input)
  {
  }

  template<typename LoggerType, typename TaskManagerIface>
  void run(const StatusRepTestBasicCData * , LoggerType & logger, TaskManagerIface * iface)
  {
    _result = false;
    // "slaves" : check for status reports, and generate one once requested.  Continue
    // adding to our internal counter until we submit a status report; after that just
    // stop.
    unsigned long count = 0;
    std::time_t time_start;
    std::time(&time_start);
    std::time_t now = time_start;
    int elapsed = 0;
    do {
      std::time(&now);
      elapsed = now - time_start;
      if (iface->statusReportRequested()) {
        logger.longdebug("StatusRepTestTask::run", "Status report requested");
        StatusReportType s(elapsed / 5.0,
                           Tomographer::Tools::fmts("elapsed = %d [%.2f%%]; count = %lu = %#lx",
                                                    elapsed, 100*elapsed/5.0, count, count));
        logger.longdebug("StatusRepTestTask::run", "s.msg = %s", s.msg.c_str());
        iface->submitStatusReport(s);
        logger.longdebug("StatusRepTestTask::run", "report submitted.");
        _result = true;
      }
      ++count;
      if ((count & 0xffff) == 0) {
        logger.longdebug("StatusRepTestTask::run", "count = %lu", count);
      }
    } while (now - time_start < 5);
  }

  ResultType getResult() const { return _result; }

private:
  Input _input;
  ResultType _result;
};
struct StatusRepTestResultsCollector {
  StatusRepTestResultsCollector()
  {
  }
  void init(int, int, const StatusRepTestBasicCData * )
  {
  }
  template<typename ResultType>
  void collectResult(int, const ResultType& taskresult, const StatusRepTestBasicCData *)
  {
    BOOST_CHECK_EQUAL(taskresult, true);
  }
  void runsFinished(int, const StatusRepTestBasicCData * )
  {
  }
};


#ifdef _OPENMP
BOOST_AUTO_TEST_CASE(status_report_withthread)
{
  StatusRepTestBasicCData cData;
  const int num_runs = 10;
  StatusRepTestResultsCollector resultsCollector;
  
  BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);

  Tomographer::MultiProc::OMP::TaskDispatcher<StatusRepTestTask, StatusRepTestBasicCData,
                                              StatusRepTestResultsCollector,
                                              BoostTestLogger, long>
      task_dispatcher(&cData, &resultsCollector, logger, num_runs, 1);

  // cData.fn = [&task_dispatcher]() {
  //   task_dispatcher.requestStatusReport();
  // };

  task_dispatcher.setStatusReportHandler(
      [&logger](const Tomographer::MultiProc::FullStatusReport<StatusRepTestTask::StatusReportType>& r) {
        logger.info("status_report test case", [&](std::ostream & stream) {
            stream << "Full status report recieved. num_completed = " << r.num_completed
                   << ", num_total_runs = " << r.num_total_runs << "\n";
            for (std::size_t k = 0; k < r.workers_running.size(); ++k) {
              if (!r.workers_running[k]) {
                stream << "Worker #" << k << " idle\n";
              } else {
                stream << "Worker #" << k << ":  " << r.workers_reports[k].fraction_done * 100 << "%, "
                       << r.workers_reports[k].msg << "\n";
              }
            }
          });
      });

   omp_set_nested(1);

   std::sig_atomic_t finished = 0;

#pragma omp parallel num_threads(2)
   {
     if (omp_get_thread_num() == 0) {
       // take care of sending status report requests

       while (!finished) {
         sleep(1);
         task_dispatcher.requestStatusReport();
       }
     } else if (omp_get_thread_num() == 1) {
       task_dispatcher.run();
       finished = 1;
     } else {
       assert( false ) ;
     }
   }

   logger.debug("test case:status_report", "Test case done.");
}
#endif

#ifndef __MINGW32__
// MinGW32 does not have SIGALRM / alarm()
BOOST_AUTO_TEST_CASE(status_report_withsigalrm)
{
  StatusRepTestBasicCData cData;
  const int num_runs = 10;
  StatusRepTestResultsCollector resultsCollector;
  
  BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);

  Tomographer::MultiProc::OMP::TaskDispatcher<StatusRepTestTask, StatusRepTestBasicCData,
                                              StatusRepTestResultsCollector,
                                              BoostTestLogger, long>
      task_dispatcher(&cData, &resultsCollector, logger, num_runs, 1);

  task_dispatcher.setStatusReportHandler(
      [&logger](const Tomographer::MultiProc::FullStatusReport<StatusRepTestTask::StatusReportType>& r) {
        logger.info("status_report test case", [&](std::ostream & stream) {
            stream << "Full status report recieved. num_completed = " << r.num_completed
                   << ", num_total_runs = " << r.num_total_runs << "\n";
            for (std::size_t k = 0; k < r.workers_running.size(); ++k) {
              if (!r.workers_running[k]) {
                stream << "Worker #" << k << " idle\n";
              } else {
                stream << "Worker #" << k << ":  " << r.workers_reports[k].fraction_done * 100 << "%, "
                       << r.workers_reports[k].msg << "\n";
              }
            }
          });
      });

  {
    auto finally = Tomographer::Tools::finally([](){
        alarm(0);
        signal(SIGALRM, SIG_DFL);
      });
    
    sigalarm_act = [&task_dispatcher]() {
      task_dispatcher.requestStatusReport();
      alarm(2);
      signal(SIGALRM, sigalarm_act_cfn);
    };
    
    alarm(1);
    signal(SIGALRM, sigalarm_act_cfn);
    
    task_dispatcher.run();
  }
}
#endif

#if !defined(_OPENMP) && defined(__MINGW__)
BOOST_AUTO_TEST_CASE(status_report_not_implemented)
{
  BOOST_CHECK(false && "Status report check NOT IMPLEMENTED on your platform, sorry");
}
#endif


BOOST_AUTO_TEST_SUITE_END();



// =============================================================================
BOOST_AUTO_TEST_SUITE_END();


