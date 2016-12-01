
#include "common.h"

#include "eigpyconv.h"


// tests .... ---------------
double test_eigen(const Eigen::MatrixXd & x)
{
  std::cerr << "test_eigen() ...\n";
  std::ostringstream s;
  s << "x = \n" << x << ";\n";
  std::string ss(s.str());
  fprintf(stderr, "%s", ss.c_str());
  //
  std::cerr << "test_eigen() about to return.\n";
  return x.sum();//rows() * 10000 + x.cols() * 100 + x(1,0);
}
boost::python::object test_eigen2(const Eigen::MatrixXcd & x)
{
  boost::python::dict d;
  d["rows"] = x.rows();
  d["cols"] = x.cols();
  d["(0,0)"] = x(0,0);
  d["(1,0)"] = ((2 <= x.rows()) ? x(1,0) : -1);
  d["(0,1)"] = ((2 <= x.cols()) ? x(0,1) : -1);
  d["sum"] = x.sum();
  return d;
}

Eigen::MatrixXd testgetmatrix_d(const Eigen::MatrixXd & x)
{
  eigen_assert(x.rows() == x.cols());
  return x * 3;
}
Eigen::MatrixXcd testgetmatrix_cd(const Eigen::MatrixXcd & x)
{
  return x * 3;
}
Eigen::MatrixXi testgetmatrix_i(const Eigen::MatrixXi & x)
{
  return x * 3;
}
// --------------




void py_tomo_histogram();
void py_tomo_tomorun();




PyLogger tpy_logger;



BOOST_PYTHON_MODULE(tomographer)
{
  // python logging
  tpy_logger.initPythonLogger();
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);

  boost::python::class_<PyLogger>("PyLogger")
    .add_property("level", +[](const PyLogger & l) { return l.level(); },
                  +[](PyLogger & l, boost::python::object newlevel) {
                    l.setLevel(l.fromPythonLevel(newlevel));
                  });
  
  boost::python::scope().attr("cxxlogger") = boost::ref(tpy_logger);

  logger.debug("INIT TOMOGRAPHER");

  // Eigen converters
  register_eigen_converter();

  logger.debug("Registered eigen converters.");

  py_tomo_histogram();

  py_tomo_tomorun();

  // // expose an interface for our C++/python logger wrapper, in particular to set C++ log level
  // struct PyCxxLoggerInterface {
  //   PyCxxLoggerInterface() : the_tpy_logger(&tpy_logger) { }
  //   PyLogger *the_tpy_logger;
  // };
  // {
  //   boost::python::class_<PyCxxLoggerInterface>("_pycxxlogger")
  //     .add_property("level", +[](PyCxxLoggerInterface & l) {
  //         return l.the_tpy_logger->toPythonLevel(l.the_tpy_logger->level());
  //       },
  //       +[](PyCxxLoggerInterface & l, boost::python::object newlevel) {
  //         l.the_tpy_logger->setLevel(l.the_tpy_logger->fromPythonLevel(newlevel));
  //       });
  // }
  // boost::python::scope().attr("cxxlogger") = PyCxxLoggerInterface();
  

  // haha
  logger.debug("importing some final toys...");

  // some dummy tests for the eigen converters:
  boost::python::def("test_eigen", test_eigen);
  boost::python::def("test_eigen2", test_eigen2);
  boost::python::def("testgetmatrix_d", testgetmatrix_d);
  boost::python::def("testgetmatrix_cd", testgetmatrix_cd);
  boost::python::def("testgetmatrix_i", testgetmatrix_i);

  logger.debug("TOMOGRAPHER INIT COMPLETE.");
}
