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

#include <tomographer/tools/fmt.h>

#include "common_p.h"





void py_tomo_mhrw(py::module rootmodule)
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, *tpy::logger);
  logger.debug("py_tomo_mhrw() ...");

  logger.debug("MHRWParams ...");
  { typedef tpy::MHRWParams Kl;
    py::class_<tpy::MHRWParams>(
        rootmodule,
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
      .def(py::init<>())
      .def(py::init<RealType,CountIntType,CountIntType,CountIntType>(),
           "step_size"_a, "n_sweep"_a, "n_therm"_a, "n_run"_a)
      .def_readwrite("step_size", &Kl::step_size)
      .def_readwrite("n_sweep", &Kl::n_sweep)
      .def_readwrite("n_therm", &Kl::n_therm)
      .def_readwrite("n_run", &Kl::n_run)
      .def("__repr__", [](const Kl& p) {
          return streamstr("MHRWParams(step_size="<<Tomographer::Tools::fmts("%.3g", (double)p.step_size)
                           <<",n_sweep="<<p.n_sweep<<",n_therm="<<p.n_therm<<",n_run="<<p.n_run<<")") ;
        })
      .def("__getstate__", [](const Kl& mhrw_params) {
          return py::make_tuple(mhrw_params.step_size,
                                mhrw_params.n_sweep,
                                mhrw_params.n_therm,
                                mhrw_params.n_run);
        })
      .def("__setstate__", [](Kl & p, py::tuple t) {
          tpy::internal::unpack_tuple_and_construct<Kl, RealType,CountIntType,CountIntType,CountIntType>(p, t);
          // if (t.size() != 4) {
          //   throw std::runtime_error("Invalid pickle state!");
          // }
          // // Invoke the in-place constructor. Note that this is needed even
          // // when the object just has a trivial default constructor
          // new (&p) Kl(t[0].cast<RealType>(),
          //             t[1].cast<CountIntType>(),
          //             t[2].cast<CountIntType>(),
          //             t[3].cast<CountIntType>());
        });
    ;
  }
}
