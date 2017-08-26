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

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mpi/multiprocmpi.h>


#include "test_multi_tasks_common.h"


// -----------------------------------------------------------------------------
// fixture(s)



// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mpi_multiprocmpi)

BOOST_FIXTURE_TEST_CASE(tasks_run, test_task_dispatcher_fixture)
{
  mpi::environment env;
  mpi::communicator world;

  Tomographer::Logger::BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);
  Tomographer::MultiProc::MPI::TaskDispatcher<TestTask, TestBasicCData,
                                              Tomographer::Logger::BoostTestLogger, long>
    task_dispatcher(world.rank() == 0 ? &cData : NULL, world, logger, num_runs);

  BOOST_MESSAGE("About to run MPI tasks");

  task_dispatcher.run();

  if (world.rank() == 0) {
    check_correct_results_collected(task_dispatcher);
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

