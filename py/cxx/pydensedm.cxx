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


#include <cstdint>

#include <tomographerpy/common.h>
#include <tomographerpy/pydensedm.h>

#include <pybind11/eval.h>

#include "common_p.h"


void py_tomo_densedm(py::module rootmodule)
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, *tpy::logger);
  logger.debug("py_tomo_densedm() ...");

  logger.debug("densedm module ... ");

  py::module densedmmodule = rootmodule.def_submodule(
      "densedm",
      ( "Classes and utilities for handling a tomography setup in which quantum states are represented "
        "via their density operator stored as dense matrices in memory."
        "\n\n"
        "The :py:class:`DMTypes` class stores the quantum system dimension, and is used by the other types."
        "\n\n"
        "The :py:class:`IndepMeasLLH` class is capable of storing measurement data (resulting from "
        "independent measurement effects on each system) and calculating "
        "the corresponding log-likelihood function. (Note that this still allows for correlated measurements "
        "such as adaptive tomography, it just prohibits truly quantum joint measurements over the different "
        "copies.)"
        "\n\n"
        "The :py:class:`ParamX` allows to calculate the X-parameterization of a Hermitian matrix "
        "(and back). See :tomocxx:`the definition of the X Parameterization "
        "<page_params_x.html>` for more info.  The X-parameterization is used, e.g., by the :py:class:`IndepMeasLLH` "
        "class to store the POVM effects and to calculate inner products more efficiently." )
      ) ;

  logger.debug("densedm.DMTypes ...");
  { typedef tpy::DMTypes Kl;
    py::class_<Kl>(
        densedmmodule,
        "DMTypes",
        "Stores the dimension of the quantum system, the square of the "
        "dimension and the number of degrees of freedom."
        "\n\n"
        "All attributes of this class are read-only."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. py:function:: DMTypes(dim)"
        "\n\n"
        "    Construct a DMTypes object storing the given dimension for the quantum system."
        "\n\n"
        ".. py:attribute:: dim\n\n"
        "    The dimension of the quantum system.\n\n"
        ".. py:attribute:: dim2\n\n"
        "    The square of the dimension.\n\n"
        ".. py:attribute:: ndof\n\n"
        "    The number of degrees of freedom.  This is `dim2-1`\n\n"
        "\n\n"
        )
      .def(py::init<int>(), "dim"_a)
      .def_property_readonly("dim", & Kl::dim )
      .def_property_readonly("dim2", & Kl::dim2 )
      .def_property_readonly("ndof", & Kl::ndof )
      .def("__repr__", [](const Kl & p) { return streamstr("DMTypes(dim="<<p.dim()<<")"); })
      .def("__getinitargs__", [](py::object p) { return py::make_tuple(p.attr("dim")); })
      ;
  }
  logger.debug("densedm.ParamX ...");
  { typedef tpy::ParamX Kl;
    py::class_<Kl>(
        densedmmodule,
        "ParamX",
        "Calculates the X-parameterization of hermitian matrices (and back)."
        "\n\n"
        "This Python class is a wrapper for the C++ class "
        ":tomocxx:`Tomographer::DenseDM::ParamX "
        "<class_tomographer_1_1_dense_d_m_1_1_param_x.html>`.\n"
        "\n\n"
        ".. py:function:: ParamX(dmt)\n\n"
        "    Constructor. Pass on a :py:class:`DMTypes` instance specifying the dimension of the "
        "quantum system.\n\n"
        )
      .def(py::init<tpy::DMTypes>(), "dmt"_a)
      .def("HermToX", +[](const Kl& p, const Eigen::MatrixXcd& Herm) {
          return p.HermToX(Herm);
        }, "Herm"_a,
        // docstring
        "HermToX(Herm)"
        "\n\n"
        "Calculate the X-parameterization of the Hermitian matrix `Herm`.  Returns a 1-D "
        "`numpy.array` object.  Only the lower triangluar portion of the matrix is accessed by "
        "this method."
          )
      .def("XToHerm", +[](const Kl& p, const Eigen::VectorXd& x) {
          return p.XToHerm(x);
        }, "x"_a,
        // docstring
        "XToHerm(x)"
        "\n\n"
        "Calculate the Hermitian matrix corresponding to the given X-parameterization vector.  The vector "
        "is expected to be a `numpy.array` object.  Returns a 2-D "
        "`numpy.array` object containing the full Hermitian matrix."
          )
      ;
  }
  logger.debug("densedm.IndepMeasLLH ...");
  { typedef tpy::IndepMeasLLH Kl;
    py::class_<Kl>(
        densedmmodule,
        "IndepMeasLLH",
        "Stores measurement data and calculates the log-likelihood function."
        "\n\n"
        "Measurements are specified as a list of observed POVM effects along with frequencies, i.e., "
        "how many times each POVM effect was observed."
        "\n\n"
        "POVM effects are stored internally in X parameterization. See :tomocxx:`here for more details "
        "<page_params_x.html>`."
        "\n\n"
        "This Python class is a wrapper for the C++ class "
        ":tomocxx:`Tomographer::DenseDM::IndepMeasLLH "
        "<class_tomographer_1_1_dense_d_m_1_1_indep_meas_l_l_h.html>`.\n"
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. py:function:: IndepMeasLLH(dmt)\n\n"
        "    Constructor. Specify the system dimension in the `dmt` argument. The latter "
        " must be an :py:class:`DMTypes` instance."
        "\n\n"
        ".. py:attribute:: dmt\n\n"
        "    The :py:class:`DMTypes` instance storing the dimension of the system. This is a "
        " read-only attribute.\n\n"
        ".. py:attribute:: numEffects\n\n"
        "    The number of separate POVM effects recorded. See :py:meth:`resetMeas()`, "
        " :py:meth:`addMeasEffect()` and :py:meth:`setMeas()`.\n\n"
        )
      .def(py::init<tpy::DMTypes>(), "dmt"_a)
      .def_readonly("dmt", & Kl::dmt )
      .def_property_readonly("numEffects", & Kl::numEffects )
      .def("Exn", [](const Kl& l) -> Eigen::MatrixXd { return l.Exn(); })
      .def("Exn", [](const Kl& l, int k) -> Eigen::VectorXd { return l.Exn(k); }, "k"_a,
           "Exn([k])"
           "\n\n"
           "If `k` is not specified, then return the matrix of all POVM effects in X-parameterization.  Each "
           "row of the returned matrix is a POVM effect in X-parameterization. "
           "\n\n"
           "If `k` is specified, then only the given POVM effect indexed by `k` is returned. It is given "
           "in X parameterization, as a 1-D array."
           "\n\n"
           "In any case, the returned value is a `numpy.array` object."
          )
      .def("Nx", [](const Kl& l) -> Eigen::VectorXi { return l.Nx(); })
      .def("Nx", [](const Kl& l, int k) -> CountIntType { return l.Nx(k); }, "k"_a,
           "Nx([k])"
           "\n\n"
           "If `k` is not specified, then return a list of frequencies associated to each row of "
           "the matrix returned by :py:meth:`Exn()`.  The return value is a 1-D NumPy array."
           "\n\n"
           "If `k` is specified, then return the frequency associated to the POVM effect indexed by `k`. "
           "The returned value is an integer."
          )
      .def("resetMeas", & Kl::resetMeas,
           "resetMeas()"
           "\n\n"
           "Forget any stored POVM effects.  The internal `Exn` and `Nx` objects are cleared.  You may "
           "start adding POVM effects with :py:meth:`setMeas()` or :py:meth:`addMeasEffect()`."
          )
      .def("addMeasEffect", [](Kl & l, Eigen::MatrixXcd E_x_or_m, CountIntType n, bool check_validity) {
          if (E_x_or_m.cols() == 1) {
            // it is a X-param vector
            tomographer_assert( E_x_or_m.imag().norm() < 1e-6
                                && "Imaginary components given in X-parameterization" ) ;
            l.addMeasEffect(tpy::DMTypes::VectorParamType(E_x_or_m.real()), n, check_validity);
          } else {
            // it is a matrix
            l.addMeasEffect(tpy::DMTypes::MatrixType(E_x_or_m), n, check_validity);
          }
        }, "E"_a, "n"_a, "check_validity"_a = true,
        "addMeasEffect(E, n, [check_validity=True])"
        "\n\n"
        "Add an observed POVM effect and a corresponding frequency. "
        "The argument `E` may be a 1-D NumPy array, in wihch case it is assumed to carry "
        "the X parameterization of the POVM "
        "effect.  Otherwise, `E` should be a complex square matrix describing the POVM effect "
        "in its usual matrix form.  In any case, `n` is an integer specifying how many times this POVM "
        "effect was observed (it may be set to 1).\n\n"
        "If `check_validity` is `True`, then some consistency checks are performed on the POVM effect, "
        "such as verifying it for positive semidefiniteness."
          )
      .def("setMeas", [](Kl & l, const py::object& E, py::object Nx, bool check_validity) {
          // we can just iterate on E and addMeasEffect() for each item, and use the
          // same trick as in the above overload to detect whether each item is a
          // X-param-vector or complex matrix.  This works for an Exn matrix (will iter
          // on rows), a 3-D numpy array (will iter on first dimension), as well as a
          // list of matrices (will iter on list).
          l.resetMeas();
          const std::size_t len = py::len(E);
          tomographer_assert(len == py::len(Nx)) ;
          for (std::size_t k = 0; k < len; ++k) {

            Eigen::MatrixXcd E_x_or_m = E[py::cast(k)].cast<Eigen::MatrixXcd>();
            CountIntType Nk = Nx[py::cast(k)].cast<CountIntType>();
            if (PyErr_Occurred() != NULL) {
              // tell pybind11 that the exception is already set
              throw py::error_already_set();
            }
              
            if (E_x_or_m.cols() == 1) {
              // it is a X-param vector
              tomographer_assert( E_x_or_m.imag().norm() < 1e-6
                                  && "Imaginary components given in X-parameterization" ) ;
              l.addMeasEffect(tpy::DMTypes::VectorParamType(E_x_or_m.real()), Nk, check_validity);
            } else {
              // it is a matrix
              l.addMeasEffect(tpy::DMTypes::MatrixType(E_x_or_m), Nk, check_validity);
            }

          }
        }, "E"_a, "Nx"_a, "check_validity"_a = true,
        "setMeas(E, Nx, [check_validity=True])"
        "\n\n"
        "Set all the measurement data in one go and clear any previously given measurement data."
        "\n\n"
        "The object `E` is iterated over (if it is a `NumPy` array, the iteration goes over the first dimension) "
        "and each element is interpreted as a POVM effect.  Each element is understood as for "
        ":py:meth:`addMeasEffect()` either as the X-parameterization if a POVM effect, if a 1-D "
        "array is specified, or as the matrix representation of the POVM effect.  For example, `E` may be\n\n"
        "  - a list of complex 2-D `NumPy` arrays, specifying the list of POVM effects in matrix representation;\n\n"
        "  - a 3-D `NumPy` array, where the first index designates the POVM effect, and the second and third "
        "    dimension are the matrix dimensions of each POVM effect;\n\n"
        "  - a matrix (`NumPy` array) of the same form as returned by :py:meth:`Exn()`, i.e., with each "
        "    row being the the X-parameterization of a POVM effect;\n\n"
        "  - a list of 1-D `NumPy` arrays specifying the X parameterization of each POVM effect."
        "\n\n"
        "The argument `Nx` must be a list (or `numpy.array`) specifying the corresponding frequencies "
        "for each POVM effect. `Nx` must be of the same length as `E` (or, if `E` is a "
        "`NumPy` array, as the first dimension of `E`)."
        "\n\n"
        "If `check_validity` is `True`, then some consistency checks are performed on the POVM effects, "
        "such as verifying them for positive semidefiniteness."
          )
      .def("logLikelihoodX", [](const Kl& l, Eigen::VectorXd x) {
          // no need to test if x has imaginary components, the conversion to
          // Eigen::VectorXd would have failed.
          return l.logLikelihoodX(x);
        }, "x"_a,
        "logLikelihoodX(x)"
        "\n\n"
        "Calculate the log-likelihood function.  The argument is the X parameterization of the state "
        "at which the log-likelihood should be evaluated.  The log-likelihood function is defined as "
        ":math:`\\ln\\Lambda(\\rho) = \\sum_k N_k \\ln\\operatorname{tr}(E_k\\cdot\\rho)`, where "
        ":math:`E_k` is the POVM effect indexed by `k` and :math:`N_k` is the corresponding frequency."
          )
      .def("logLikelihoodRho", [](const Kl& l, Eigen::MatrixXcd rho) {
          return l.logLikelihoodX(Tomographer::DenseDM::ParamX<tpy::DMTypes>(l.dmt).HermToX(rho));
        }, "x"_a,
        "logLikelihoodRho(rho)"
        "\n\n"
        "Calculate the log-likelihood function at the state `rho`, specified by its density matrix given as "
        "a `NumPy` array.  This overload converts its argument "
        "to X-parameterization and calls :py:meth:`logLikelihoodX()`."
          )
      .def("__repr__", [](const Kl & p) {
          return streamstr("<IndepMeasLLH dim="<<p.dmt.dim()<<" numEffects="<<p.numEffects()
                           <<" Ntot="<<p.Nx().sum()<<">") ;
        })
      .def("__getinitargs__", [](py::object p) { return py::make_tuple(p.attr("dmt")); })
      .def("__getstate__", [](py::object p) {
          return py::make_tuple(p.attr("Exn")(), p.attr("N")());
        })
      .def("__setstate__", [](py::object p, py::tuple t) {
          if (py::len(t) != 2) {
            throw TomographerCxxError(streamstr("Invalid pickle state: expected 2 args, got " << py::len(t)));
          }
          p.attr("setMeas")(t[0], t[1], false);
        })
      ;
  }
}
