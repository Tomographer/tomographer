
#include <tomographerpy/common.h>
#include <tomographerpy/pymhrw.h>

#include "common_p.h"



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
      ;
  }

}
