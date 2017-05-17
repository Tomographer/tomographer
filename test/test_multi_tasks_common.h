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

#ifndef TEST_MULTI_TASKS_COMMON_H
#define TEST_MULTI_TASKS_COMMON_H

#include <string>
#include <functional>
#include <chrono>

#include <tomographer/tools/fmt.h>

// don't include multiprocomp.h, because when compiling test_multiproc I don't want to
// have included multiprocomp.h (be sure that there is no symbol pollution)
#ifdef TOMOGRAPHER_USE_WINDOWS_SLEEP
// use MS Window's Sleep() function
#include <windows.h>
#define TOMOGRAPHERTESTS_SLEEP_FOR_MS(x) Sleep((x))
#else
// normal C++11 API function, not available on mingw32 w/ win threads
#include <thread>
#define TOMOGRAPHERTESTS_SLEEP_FOR_MS(x) std::this_thread::sleep_for(std::chrono::milliseconds((x)))
#endif


#ifdef _OPENMP
#include <omp.h>
#endif




typedef
#if defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 6 && !defined(__clang__)
    std::chrono::monotonic_clock // for g++ 4.6
#else
    std::chrono::steady_clock
#endif
    StdClockType;


int UnitTimeSleepMs = 300;





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

  typedef Tomographer::MultiProc::TaskStatusReport StatusReportType;
  
  struct ResultType {
    ResultType(int value_) : msg(), value(value_) { }
    ResultType(int value_, std::string msg_) : msg(msg_), value(value_) { }

    ResultType(ResultType && ) = default; // expressedly allow move construction

    std::string msg;
    int value;

    // test that the multiproc implementation does not depend on these
    ResultType(const ResultType & ) = delete;
    ResultType & operator=(const ResultType & ) = delete;

    ResultType explicitCopy() const { return ResultType(value, msg); }
  };

  template<typename LoggerType>
  TestTask(Input input, const TestBasicCData * , LoggerType & logger)
    : _input(input), _result(-1)
  {
    logger.debug("TestTask::TestTask", "constructor called") ;
  }

  template<typename LoggerType, typename TaskManagerIface>
  void run(const TestBasicCData * pcdata, LoggerType & logger, TaskManagerIface * )
  {
    //BOOST_MESSAGE("Running task.") ; // BOOST_TEST_MESSAGE may not be thread-safe!!!!!
    logger.info("TestTask::run", "Running task.") ;
    logger.debug("TestTask::run", "running task.");
    _result.value = ( _input.a + _input.b ) * pcdata->c ;
    _result.msg = Tomographer::Tools::fmts("((a=%d)+(b=%d))*(c=%d) == %d",
                                           _input.a, _input.b, pcdata->c, _result.value);
    logger.info("TestTask::run", "Task finished.") ;
    //    BOOST_MESSAGE("Task finished.") ;
  }

  inline ResultType getResult() const { return _result.explicitCopy(); }
  inline ResultType stealResult() { return std::move(_result); }

  Input _input;
  ResultType _result;
  
};

// struct TestResultsCollector {
//   TestResultsCollector(std::vector<TestTask::ResultType> check_correct_results_, int num_runs_,
//                        const TestBasicCData * pcdata_)
//     : init_called(0), collectres_called(0), runsfinished_called(0),
//       num_runs(num_runs_), pcdata(pcdata_),
//       check_correct_results(std::move(check_correct_results_))
//   {
//   }
//   void init(int num_total_runs, int n_chunk, const TestBasicCData * pcdata_)
//   {
//     BOOST_CHECK_EQUAL(num_total_runs, num_runs) ;
//     BOOST_CHECK_EQUAL(n_chunk, 1) ;
//     BOOST_CHECK_EQUAL(pcdata, pcdata_) ;
//     ++init_called;
//   }
//   template<typename ResultType>
//   void collectResult(int task_no, const ResultType& taskresult, const TestBasicCData * pcdata_)
//   {
//     BOOST_CHECK_GE(task_no, 0); BOOST_CHECK_LT(task_no, (int)check_correct_results.size());
//     BOOST_CHECK_EQUAL(taskresult.value, check_correct_results[task_no].value);
//     BOOST_CHECK_EQUAL(pcdata, pcdata_) ;
//     BOOST_TEST_MESSAGE("Collected result from task " << task_no << ": " << taskresult.msg) ;
//     ++collectres_called;
//   }
//   void runsFinished(int num_total_runs, const TestBasicCData * pcdata_)
//   {
//     BOOST_CHECK_EQUAL(num_total_runs, num_runs) ;
//     BOOST_CHECK_EQUAL(pcdata, pcdata_) ;
//     ++runsfinished_called;
//   }
//
//   int init_called;
//   int collectres_called;
//   int runsfinished_called;
//   const int num_runs;
//   const TestBasicCData * pcdata;
//   const std::vector<TestTask::ResultType> check_correct_results;
// };



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

  const std::vector<int> correct_result_values;
  
  test_task_dispatcher_fixture()
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

  void check_correct_results(const std::vector<TestTask::ResultType*> & results)
  {
    for (std::size_t k = 0; k < correct_result_values.size(); ++k) {
      BOOST_CHECK_EQUAL(results[k]->value, correct_result_values[k]) ;
    }
  }

  template<typename TaskDispatcherType>
  void check_correct_results_collected(const TaskDispatcherType & task_dispatcher)
  {
    // collectedTaskResults()
    const std::vector<TestTask::ResultType*> results = task_dispatcher.collectedTaskResults();
    check_correct_results(results);

    // numTaskRuns()
    BOOST_CHECK_EQUAL(results.size(), (std::size_t)task_dispatcher.numTaskRuns());

    // collectedTaskResult(k)
    const std::size_t N = task_dispatcher.numTaskRuns();
    for (std::size_t k = 0; k < N; ++k) {
      BOOST_CHECK_EQUAL(task_dispatcher.collectedTaskResult(k).value, correct_result_values[k]) ;
    }
  }
};




//
// Utilities for testing the status reporting mechanism of a task dispatcher
//




#ifndef __MINGW32__
// MinGW32 does not have SIGALRM / alarm()
std::function<void()> sigalarm_act;

void sigalarm_act_cfn(int signum)
{
  //  printf("[SIGALRM]\n");
  if (signum == SIGALRM) {
    sigalarm_act();
  }
}
#endif






struct StatusRepTestBasicCData {
  StatusRepTestBasicCData() : UnitTimeSleepMs_(UnitTimeSleepMs) { }

  int UnitTimeSleepMs_;

  int getTaskInput(int k) const {
    return 2*UnitTimeSleepMs_ + (k/3)*UnitTimeSleepMs_;
  }
};
struct StatusRepTestTask {

  typedef Tomographer::MultiProc::TaskStatusReport StatusReportType;
  
  typedef bool ResultType;

  template<typename LoggerType>
  StatusRepTestTask(int input, const StatusRepTestBasicCData * , LoggerType & logger)
    : _input(input) // input is task number
  {
    logger.debug("StatsRepTestTask constructor", "Task will run for %d ms", input);
  }

  template<typename LoggerType, typename TaskManagerIface>
  void run(const StatusRepTestBasicCData * , LoggerType & logger, TaskManagerIface * iface)
  {
    _result = false;

    // Check for status reports, and generate one once requested.  Run the task
    // like this for five seconds.

    unsigned long count = 0;
    StdClockType::time_point time_start = StdClockType::now();
    int elapsed_ms = 0;
    int ms_to_run = _input;
    do {
      elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(StdClockType::now() - time_start).count();
      if (iface->statusReportRequested()) {
        logger.longdebug("StatusRepTestTask::run", "Task #%02d: Status report requested", _input);
        StatusReportType s(elapsed_ms / (double)ms_to_run,
                           Tomographer::Tools::fmts("elapsed = %d [%.2f%%]; count = %lu = %#lx",
                                                    elapsed_ms, 100.0*elapsed_ms/ms_to_run, count, count));
        logger.longdebug("StatusRepTestTask::run", "s.msg = %s", s.msg.c_str());
        iface->submitStatusReport(s);
        logger.longdebug("StatusRepTestTask::run", "report submitted.");
        _result = true;
      }
      ++count;
    } while (elapsed_ms < ms_to_run);
  }

  ResultType getResult() const { return _result; }

  ResultType stealResult() const { return getResult(); }

private:
  int _input;
  ResultType _result;
};


struct test_task_dispatcher_status_reporting_fixture {
  StatusRepTestBasicCData cData;
  const int num_runs;
  
  test_task_dispatcher_status_reporting_fixture()
    : cData(), num_runs(6)
  {
  }


  template<typename TaskDispatcher, typename LoggerType>
  inline void set_report_handler(TaskDispatcher& task_dispatcher, LoggerType & logger)
  {
    auto plogger = &logger; // see http://stackoverflow.com/q/21443023/1694896
    task_dispatcher.setStatusReportHandler(
        [plogger](const Tomographer::MultiProc::FullStatusReport<StatusRepTestTask::StatusReportType>& r) {
          plogger->info("status_report test case", [&](std::ostream & stream) {
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
  }


  template<typename TaskDispatcher, typename LoggerType>
  inline void perform_test_status_report_periodic(TaskDispatcher & task_dispatcher, LoggerType & logger)
  {
    logger.info("test case:status_report_periodic", "Starting test case.");

    set_report_handler(task_dispatcher, logger);

    task_dispatcher.requestPeriodicStatusReport(UnitTimeSleepMs); // every unit time

    task_dispatcher.run();

    for ( auto res : task_dispatcher.collectedTaskResults() ) {
      BOOST_CHECK( res );
    }

    logger.info("test case:status_report_periodic", "Test case done.");
  }

  template<typename TaskDispatcher, typename LoggerType>
  inline void perform_test_interrupt_tasks_withthread(TaskDispatcher & task_dispatcher, LoggerType & logger)
  {
#ifdef _OPENMP
    logger.info("test case:interrupt_tasks_withthread", "Starting test case.");

    set_report_handler(task_dispatcher, logger);

    omp_set_dynamic(0);
    omp_set_nested(1);

    bool tasks_interrupted = false;

    auto starttime = StdClockType::now();

#pragma omp parallel num_threads(2) default(shared)
    {
      if (omp_get_thread_num() == 0) {
        // take care of sending the interrupt request

        TOMOGRAPHERTESTS_SLEEP_FOR_MS(UnitTimeSleepMs/2);
        task_dispatcher.requestInterrupt();

      } else if (omp_get_thread_num() == 1) {
        // run the slave tasks

        try {
          task_dispatcher.run();
        } catch (const Tomographer::MultiProc::TasksInterruptedException & e) {
          tasks_interrupted = true;
        }

      } else {
        // never here
        assert( false ) ;
      }
    }

    auto endtime = StdClockType::now();

#pragma omp critical
    {
      BOOST_CHECK(tasks_interrupted);
    }
   
    logger.debug("test case:interrupt_tasks_withthread", [&](std::ostream & stream) {
        stream << "Tasks (hopefully) interrupted after "
               << std::chrono::duration_cast<std::chrono::seconds>(endtime-starttime).count()
               << " seconds.";
      });

    logger.info("test case:interrupt_tasks_withthread", "Test case done.");

#else // _OPENMP
    (void)task_dispatcher; (void)logger;
    BOOST_TEST_MESSAGE("test case interrupt_tasks_withthread: nothing to do because OpenMP is disabled");
    BOOST_CHECK( true ) ; // dummy test case
#endif
  }

  template<typename TaskDispatcher, typename LoggerType>
  inline void perform_test_status_report_withthread(TaskDispatcher & task_dispatcher, LoggerType & logger)
  {
#ifdef _OPENMP
    logger.info("test case:status_report_withthread", "Starting test case.");

    set_report_handler(task_dispatcher, logger);

    omp_set_dynamic(0);
    omp_set_nested(1);

    volatile std::sig_atomic_t finished = 0;

#pragma omp parallel num_threads(2) default(shared)
    {
      if (omp_get_thread_num() == 0) {
        // take care of sending status report requests

        while (!finished) {
          TOMOGRAPHERTESTS_SLEEP_FOR_MS(UnitTimeSleepMs);
          task_dispatcher.requestStatusReport();
        }

      } else if (omp_get_thread_num() == 1) {
        // run the slave tasks

        task_dispatcher.run();

        finished = 1;

        for ( auto res : task_dispatcher.collectedTaskResults() ) {
          BOOST_CHECK( res );
        }

      } else {
        // never here
        assert( false ) ;
      }
    }

    logger.debug("test case:status_report_withthread", "Test case done.");
#else // _OPENMP
    (void)task_dispatcher; (void)logger;
    BOOST_TEST_MESSAGE("test case status_report_withthread: nothing to do because OpenMP is disabled");
    BOOST_CHECK( true ) ; // dummy test case
#endif
  }

  template<typename TaskDispatcher, typename LoggerType>
  inline void perform_test_status_report_withsigalrm(TaskDispatcher & task_dispatcher, LoggerType & logger)
  {
#ifndef __MINGW32__ // MinGW32 does not have SIGALRM / alarm()
    logger.info("test case:status_report_withsigalrm", "Starting test case.");

    // sigalarm only accepts integer seconds so make that our time step unit
    cData.UnitTimeSleepMs_ = 1000;

    set_report_handler(task_dispatcher, logger);

    {
      auto finally = Tomographer::Tools::finally([](){
          alarm(0);
          signal(SIGALRM, SIG_DFL);
        });
    
      sigalarm_act = [&task_dispatcher]() {
        task_dispatcher.requestStatusReport();
        alarm(1);
        signal(SIGALRM, sigalarm_act_cfn);
      };
    
      alarm(1);
      signal(SIGALRM, sigalarm_act_cfn);
    
      task_dispatcher.run();

      for ( auto res : task_dispatcher.collectedTaskResults() ) {
        BOOST_CHECK( res );
      }
    }

    logger.info("test case:status_report_withsigalrm", "Test case done.");
#else // MINGW
    (void)task_dispatcher; (void)logger;
    BOOST_TEST_MESSAGE("test case status_report_withsigalrm: nothing to do because signal/alarm is not supported");
    BOOST_CHECK( true ) ; // dummy test case

#if !defined(_OPENMP)
    // but on MINGW & !OPENMP, we have no way of testing status report checking ... so fail here
    BOOST_CHECK(false && "Status report check NOT IMPLEMENTED on your platform, sorry");
#endif // at least some form of status report checked
#endif // MINGW
  }
  
};















#endif
