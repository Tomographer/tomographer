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
#include <tomographerpy/pymhrw.h>

#include "common_p.h"


struct MHRWParams_pickle_suite : boost::python::pickle_suite
{
  static boost::python::tuple getinitargs(const Py::MHRWParams & mhrw_params)
  {
    return boost::python::make_tuple(mhrw_params.step_size,
                                     mhrw_params.n_sweep,
                                     mhrw_params.n_therm,
                                     mhrw_params.n_run);
  }
};



void py_tomo_mhrw()
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);
  logger.debug("py_tomo_mhrw() ...");

  using boost::python::arg;

  logger.debug("MHRWParams ...");
  { typedef Py::MHRWParams Kl;
    boost::python::class_<Py::MHRWParams>(
        "MHRWParams",
        Tomographer::Tools::fmts(
            "Parameters for a Metropolis-Hastings random walk.\n\n"
            ".. py:function:: MHRWParams(step_size, n_sweep, n_therm, n_run)\n\n"
            "    Construct a `MHRWParams` instance, initializing the read-only members `step_size`, `n_sweep`, "
            "`n_therm` and `n_run` to the values given to the constructor."
            "\n\n"
            "|picklable|"
            "\n\n"
            ".. seealso:: See the corresponding C++ class :tomocxx:`Tomographer::MHRWParams "
            "<struct_tomographer_1_1_m_h_r_w_params.html>` for more information about these parameters.  (The "
            "interfaced class uses the template parameters `CountIntType=%s` and `StepRealType=%s`.)"
            "\n\n"
            ".. py:attribute:: step_size\n\n"
            "    See :tomocxx:`Tomographer::MHRWParams <struct_tomographer_1_1_m_h_r_w_params.html>`.\n\n"
            ".. py:attribute:: n_sweep\n\n"
            "    See :tomocxx:`Tomographer::MHRWParams <struct_tomographer_1_1_m_h_r_w_params.html>`.\n\n"
            ".. py:attribute:: n_therm\n\n"
            "    See :tomocxx:`Tomographer::MHRWParams <struct_tomographer_1_1_m_h_r_w_params.html>`.\n\n"
            ".. py:attribute:: n_run\n\n"
            "    See :tomocxx:`Tomographer::MHRWParams <struct_tomographer_1_1_m_h_r_w_params.html>`.\n\n",
            boost::core::demangle(typeid(CountIntType).name()).c_str(),
            boost::core::demangle(typeid(RealType).name()).c_str()
            ).c_str()
        )
      .def(boost::python::init<>())
      .def(boost::python::init<RealType,CountIntType,CountIntType,CountIntType>(
               (arg("step_size"), arg("n_sweep"), arg("n_therm"), arg("n_run"))))
      .add_property("step_size", +[](const Kl & p) { return p.step_size; },
                    +[](Kl & p, RealType step_size) { p.step_size = step_size; })
      .add_property("n_sweep", +[](const Kl & p) { return p.n_sweep; },
                    +[](Kl & p, CountIntType n_sweep) { p.n_sweep = n_sweep; })
      .add_property("n_therm", +[](const Kl & p) { return p.n_therm; },
                    +[](Kl & p, CountIntType n_therm) { p.n_therm = n_therm; })
      .add_property("n_run", +[](const Kl & p) { return p.n_run; },
                    +[](Kl & p, CountIntType n_run) { p.n_run = n_run; })
      .def_pickle(MHRWParams_pickle_suite())
      ;
  }

}
