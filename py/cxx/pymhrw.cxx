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
            ".. py:function:: MHRWParams(mhrw_params=, n_sweep, n_therm, n_run, **kwargs)\n\n"
            "    Construct a `MHRWParams` instance, initializing the read-only members `mhwalker_params`, "
            "`n_sweep`, `n_therm` and `n_run` to the values given to the constructor."
            "\n\n"
            "|picklable|"
            "\n\n"
            ".. seealso:: See the corresponding C++ class :tomocxx:`Tomographer::MHRWParams "
            "<struct_tomographer_1_1_m_h_r_w_params.html>` for more information about these parameters.  (The "
            "interfaced class uses the template parameters `MHWalkerParams=py::object` and `CountIntType=%s`.)"
            "\n\n"
            ".. py:attribute:: mhwalker_params\n\n"
            "    See :tomocxx:`Tomographer::MHRWParams <struct_tomographer_1_1_m_h_r_w_params.html>`.\n\n"
            ".. py:attribute:: n_sweep\n\n"
            "    See :tomocxx:`Tomographer::MHRWParams <struct_tomographer_1_1_m_h_r_w_params.html>`.\n\n"
            ".. py:attribute:: n_therm\n\n"
            "    See :tomocxx:`Tomographer::MHRWParams <struct_tomographer_1_1_m_h_r_w_params.html>`.\n\n"
            ".. py:attribute:: n_run\n\n"
            "    See :tomocxx:`Tomographer::MHRWParams <struct_tomographer_1_1_m_h_r_w_params.html>`.\n\n",
            boost::core::demangle(typeid(tpy::MHRWParams::CountIntType).name()).c_str()
            ).c_str()
        )
      .def(py::init([](py::args args, py::kwargs kwargs) {
            py::object mhwalker_params = py::none();
            tpy::MHRWParams::CountIntType n_sweep = 0, n_therm = 0, n_run = 0;
            if (py::len(args) && py::len(kwargs)) {
              throw tpy::TomographerCxxError("Can't specify positional arguments along with keyword arguments "
                                             "for MHRWParams(...)");
            }
            if (py::len(args)) {
              if (py::len(args) != 4) {
                throw tpy::TomographerCxxError("Expected exactly four arguments in call to "
                                               "MHRWParams(mhwalker_params, n_sweep, n_therm, n_run)");
              }
              // exactly 4 args given: they are, in order, (mhwalker_params, n_sweep, n_therm, n_run)
              mhwalker_params = args[0];
              n_sweep = args[1].cast<tpy::MHRWParams::CountIntType>();
              n_therm = args[2].cast<tpy::MHRWParams::CountIntType>();
              n_run = args[3].cast<tpy::MHRWParams::CountIntType>();
            } else if (py::len(kwargs)) {
              n_sweep = kwargs.attr("pop")("n_sweep"_s, 0).cast<tpy::MHRWParams::CountIntType>();
              n_therm = kwargs.attr("pop")("n_therm"_s, 0).cast<tpy::MHRWParams::CountIntType>();
              n_run = kwargs.attr("pop")("n_run"_s, 0).cast<tpy::MHRWParams::CountIntType>();
              mhwalker_params = kwargs;
            } else { // no arguments at all -- default constructor
              mhwalker_params = py::dict();
              n_sweep = 0;
              n_therm = 0;
              n_run = 0;
            }
            return new Kl(mhwalker_params, n_sweep, n_therm, n_run);
          }))
      .def_readwrite("mhwalker_params", &Kl::mhwalker_params)
      .def_readwrite("n_sweep", &Kl::n_sweep)
      .def_readwrite("n_therm", &Kl::n_therm)
      .def_readwrite("n_run", &Kl::n_run)
      .def("__repr__", [](const Kl& p) {
          return streamstr("MHRWParams(mhwalker_params="<<py::str(p.mhwalker_params).cast<std::string>()
                           <<",n_sweep="<<p.n_sweep<<",n_therm="<<p.n_therm<<",n_run="<<p.n_run<<")") ;
        })
      .def(py::pickle(
               [](const Kl& mhrw_params) {
                 return py::make_tuple(mhrw_params.mhwalker_params,
                                       mhrw_params.n_sweep,
                                       mhrw_params.n_therm,
                                       mhrw_params.n_run);
               },
               [](py::tuple t) {
                 return tpy::internal::unpack_tuple_and_construct<Kl, py::object,
                                                                  tpy::MHRWParams::CountIntType,
                                                                  tpy::MHRWParams::CountIntType,
                                                                  tpy::MHRWParams::CountIntType>(t);
               }))
      ;
  }
}
