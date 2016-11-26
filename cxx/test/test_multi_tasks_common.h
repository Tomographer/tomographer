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

#ifndef TEST_MULTI_TASKS_COMMON_H
#define TEST_MULTI_TASKS_COMMON_H

#include <string>

#include <tomographer2/tools/fmt.h>



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
    //BOOST_MESSAGE("Running task.") ; // BOOST_TEST_MESSAGE may not be thread-safe!!!!!
    logger.info("Running task.") ;
    logger.debug("TestTask::run", "running task.");
    _result.value = ( _input.a + _input.b ) * pcdata->c ;
    _result.msg = Tomographer::Tools::fmts("((a=%d)+(b=%d))*(c=%d) == %d",
                                           _input.a, _input.b, pcdata->c, _result.value);
    logger.info("Task finished.") ;
    //    BOOST_MESSAGE("Task finished.") ;
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
    BOOST_CHECK_GE(task_no, 0); BOOST_CHECK_LT(task_no, (int)check_correct_results.size());
    BOOST_CHECK_EQUAL(taskresult.value, check_correct_results[task_no].value);
    BOOST_CHECK_EQUAL(pcdata, pcdata_) ;
    BOOST_MESSAGE("Collected result from task " << task_no << ": " << taskresult.msg) ;
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



#endif
