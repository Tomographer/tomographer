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


#include <tomographerpy/common.h>
#include <tomographerpy/pymultiproc.h>

#include "common_p.h"


void py_tomo_multiproc(py::module rootmodule)
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, *tpy::logger);
  logger.debug("py_tomo_multiproc() ...");

  logger.debug("multiproc module ... ");

  py::module multiprocmodule = rootmodule.def_submodule(
      "multiproc",
      "Classes and utilities for handling parallel tasks and for status reporting."
      );

  logger.debug("multiproc.FullStatusReport ...");
  { typedef tpy::FullStatusReport Kl;
    py::class_<tpy::FullStatusReport>(
        multiprocmodule,
        "FullStatusReport",
        "A full status report of running tasks.  This is typically passed on to a callback for displaying "
        "progressive status reports.\n\n"
        ".. seealso:: This Python class interfaces :tomocxx:`the C++ class Tomographer::MultiProc::FullStatusReport "
        "<struct_tomographer_1_1_multi_proc_1_1_full_status_report.html>`."
        "\n\n"
        "There is a slight difference between the C++ and Python API's on how the individual workers' running/idle "
        " are reported: here, we don't expose any `workers_running` list, rather, if a worker is idle, the corresponding "
        "object in `workers` is `None`."
        "\n\n"
        "All attributes of this class are read-only.\n\n"
        "\n\n"
        ".. py:attribute:: num_completed\n\n"
        "    The number of task instances that have already been entirely completed.\n\n"
        ".. py:attribute:: num_total_runs\n\n"
        "    The total number of task instances that we have to run\n\n"
        ".. py:attribute:: elapsed\n\n"
        "    How many seconds have elapsed since starting to run tasks\n\n"
        ".. py:attribute:: total_fraction_done\n\n"
        "    The total fraction of the tasks completed\n\n"
        ".. py:attribute:: workers\n\n"
        "    A list of individual reports for each running worker.  The length of this list is the number of workers "
        "which can handle running tasks (e.g. parallel threads).  Each item of the list is either `None` if the "
        "worker is idle, or a :py:class:`WorkerStatusReport` instance providing the worker's current status report."
        "\n\n"
        )
      .def_readonly("num_completed", & Kl::num_completed )
      .def_readonly("num_total_runs", & Kl::num_total_runs )
      .def_readonly("elapsed", & Kl::elapsed )
      .def_readonly("workers", & Kl::workers )
      .def_readonly("total_fraction_done", & Kl::total_fraction_done )
      .def("getHumanReport", [](const Kl & r) { return r.human_report; },
           "getHumanReport()\n\n"
           "A convenience function which formats the data contained in this structure in a human-readable form, "
           "providing an overview of the current status of the tasks.  The report is suitable to be displayed "
           "in a terminal, for example.\n\n"
           "The formatted report is returned as a string.")
      ;
  }

  logger.debug("multiproc.WorkerStatusReport ...");
  { typedef tpy::WorkerStatusReport Kl;
    py::class_<tpy::WorkerStatusReport>(
        multiprocmodule,
        "WorkerStatusReport",
        "Provides a summary of the current status of a worker processing a task."
        "\n\n"
        "This Python class is a wrapper for a corresponding C++ subclass class of "
        ":tomocxx:`Tomographer::MultiProc::TaskStatusReport "
        "<struct_tomographer_1_1_multi_proc_1_1_task_status_report.html>`, for example "
        ":tomocxx:`Tomographer::MHRWTasks::MHRandomWalkTask::StatusReport "
        "<struct_tomographer_1_1_m_h_r_w_tasks_1_1_m_h_random_walk_task_1_1_status_report.html>`. "
        "Note that the data is rearranged from the C++ API; only the attributes `fraction_done` and `msg` are "
        "exposed directly; the remaining data is exposed via a general-purpose dictionary `data`."
        "\n\n"
        ".. py:attribute:: worker_id\n\n"
        "    The identification number for this worker, typically the thread number.\n\n"
        ".. py:attribute:: fraction_done\n\n"
        "    The estimated fraction of the current task which is completed, given as a real number between 0 and 1.\n\n"
        ".. py:attribute:: msg\n\n"
        "    A message (provided as a string) which summarizes the currents status of the task\n\n"
        ".. py:attribute:: data\n\n"
        "    Additional data which is available, depending on the task type.\n\n"
        "\n\n"
        "    Functions which provide status reports using :py:class:`FullStatusReport` and "
        ":py:class:`WorkerStatusReport` should properly document which additional information is "
        "available in the `data` attribute. (See, for example, :py:func:`tomographer.tomorun.tomorun()`.)"
        )
      .def_readonly("worker_id", & Kl::worker_id )
      .def_readonly("fraction_done", & Kl::fraction_done )
      .def_readonly("msg", & Kl::msg )
      .def_readonly("data", & Kl::data )
      ;
  }
}
