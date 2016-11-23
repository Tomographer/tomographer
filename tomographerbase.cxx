
#include "common.h"



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



BOOST_PYTHON_MODULE(tomographer)
{
  std::cerr << "INIT TOMOGRAPHER\n";

  // Eigen converters
  //long ok = []() -> long { import_array(); return 1; } (); if (!ok) { return; } // error
  register_eigen_converter();

  std::cerr << "Registered eigen converters.\n";

  py_tomo_histogram();

  py_tomo_tomorun();

  std::cerr << "some final toys...\n";

  // some dummy tests for the eigen converters:
  boost::python::def("test_eigen", test_eigen);
  boost::python::def("test_eigen2", test_eigen2);
  boost::python::def("testgetmatrix_d", testgetmatrix_d);
  boost::python::def("testgetmatrix_cd", testgetmatrix_cd);
  boost::python::def("testgetmatrix_i", testgetmatrix_i);

  std::cerr << "TOMOGRAPHER INIT COMPLETE.\n";
}
