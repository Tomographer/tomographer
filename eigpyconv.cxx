
#include "common.h"
#include "eigpyconv.h"


// add the types you want to add
void register_eigen_converter()
{
  // NOTE: import_array() must be called from within the module's init function
  // (https://mail.scipy.org/pipermail/numpy-discussion/2010-December/054349.html):
  // import_array(); //< required, or conversion leads to segfault
  
  // matrices...
  eigen_python_converter< Eigen::Matrix<long,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<long,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<int,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<int,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<unsigned long,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<unsigned long,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<unsigned int,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<unsigned int,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<float,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<float,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<std::complex<float>,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<std::complex<float>,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<std::complex<double>,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<std::complex<double>,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  // ... and vectors
  eigen_python_converter< Eigen::Matrix<long,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<long,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<int,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<int,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<unsigned long,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<unsigned long,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<unsigned int,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<unsigned int,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<float,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<float,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<double,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<double,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<std::complex<float>,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<std::complex<float>,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<std::complex<double>,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<std::complex<double>,Eigen::Dynamic,1> >::from_python();
  

  boost::python::register_exception_translator<EigenNumpyConversionError>(+[](const EigenNumpyConversionError & exc) {
      PyErr_SetString(PyExc_RuntimeError, exc.what());
    });
  
  boost::python::register_exception_translator<Tomographer::Tools::EigenAssertException>(
      +[](const Tomographer::Tools::EigenAssertException & exc) {
        PyErr_SetString(PyExc_RuntimeError, exc.what());
      });
}

