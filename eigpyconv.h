


#include <cstring>

#include <iostream>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <numpy/arrayobject.h>

#include <boost/core/demangle.hpp>

#include <boost/python.hpp>
#include <boost/python/numeric.hpp>

#include <Eigen/Core>

#include <tomographer2/tools/cxxutil.h>


struct EigenNumpyConversionError : public std::exception
{
  EigenNumpyConversionError(std::string msg)
    : _msg(std::move(msg)) { }
  virtual ~EigenNumpyConversionError() { }

  const char * what() const throw() { return _msg.c_str(); }
private:
  std::string _msg;
};

void hlp_EigenNumpyConversionError_py_translate(EigenNumpyConversionError exc)
{
  PyErr_SetString(PyExc_RuntimeError, exc.what());
}



// convert C++ type -> NumPy type code.
template<typename Scalar> struct NpyCode { enum { TypeCode = -1 }; };
// specializations
template<> struct NpyCode<int8_t> { enum { TypeCode = NPY_INT8 }; };
template<> struct NpyCode<int16_t> { enum { TypeCode = NPY_INT16 }; };
template<> struct NpyCode<int32_t> { enum { TypeCode = NPY_INT32 }; };
template<> struct NpyCode<int64_t> { enum { TypeCode = NPY_INT64 }; };
template<> struct NpyCode<uint8_t> { enum { TypeCode = NPY_UINT8 }; };
template<> struct NpyCode<uint16_t> { enum { TypeCode = NPY_UINT16 }; };
template<> struct NpyCode<uint32_t> { enum { TypeCode = NPY_UINT32 }; };
template<> struct NpyCode<uint64_t> { enum { TypeCode = NPY_UINT64 }; };
template<> struct NpyCode<npy_float> { enum { TypeCode = NPY_FLOAT }; };
template<> struct NpyCode<npy_double> { enum { TypeCode = NPY_DOUBLE }; };
template<> struct NpyCode<npy_longdouble> { enum { TypeCode = NPY_LONGDOUBLE }; };
template<> struct NpyCode<std::complex<npy_float> > { enum { TypeCode = NPY_CFLOAT }; };
template<> struct NpyCode<std::complex<npy_double> > { enum { TypeCode = NPY_CDOUBLE }; };
template<> struct NpyCode<std::complex<npy_longdouble> > { enum { TypeCode = NPY_CLONGDOUBLE }; };


// convert NumPy type code to C++ type
template<typename CallbackTmpl, typename... CallbackTmplArgs>
void npyToCxxType(int npy_type, CallbackTmplArgs &&... args) {
  switch (npy_type) {
  case NPY_INT8: CallbackTmpl::template run<int8_t>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_INT16: CallbackTmpl::template run<int16_t>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_INT32: CallbackTmpl::template run<int32_t>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_INT64: CallbackTmpl::template run<int64_t>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_UINT8: CallbackTmpl::template run<uint8_t>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_UINT16: CallbackTmpl::template run<uint16_t>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_UINT32: CallbackTmpl::template run<uint32_t>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_UINT64: CallbackTmpl::template run<uint64_t>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_FLOAT: CallbackTmpl::template run<npy_float>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_DOUBLE: CallbackTmpl::template run<npy_double>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_LONGDOUBLE: CallbackTmpl::template run<npy_longdouble>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_CFLOAT: CallbackTmpl::template run<std::complex<npy_float> >(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_CDOUBLE: CallbackTmpl::template run<std::complex<npy_double> >(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_CLONGDOUBLE:
    CallbackTmpl::template run<npy_clongdouble>(std::forward<CallbackTmplArgs>(args)...); break;
  default:  throw EigenNumpyConversionError("Unknown NumPy type code: "+std::to_string(npy_type));
  };
};




template<typename T1, typename T2>
struct IsNumConvertible {
  enum { value = std::is_convertible<T1, T2>::value };
};
template<typename T1, typename R2>
struct IsNumConvertible<T1, std::complex<R2> > {
  enum { value = std::is_convertible<T1, R2>::value };
};
template<typename R1, typename R2>
struct IsNumConvertible<std::complex<R1>, std::complex<R2> > {
  enum { value = std::is_convertible<R1, R2>::value };
};

TOMO_STATIC_ASSERT_EXPR(IsNumConvertible<double,std::complex<double> >::value) ;
TOMO_STATIC_ASSERT_EXPR(IsNumConvertible<std::complex<int>,std::complex<double> >::value) ;
TOMO_STATIC_ASSERT_EXPR(!IsNumConvertible<std::complex<double>,double>::value) ;



template<typename MatrixType>
struct CopyNumpyDataToEigen
{
  typedef typename MatrixType::Scalar EigScalar;

  template<typename NPScalar, TOMOGRAPHER_ENABLED_IF_TMPL(IsNumConvertible<NPScalar,EigScalar>::value)>
  static inline void run(void * storage, PyArrayObject * array,
                         const npy_intp dims[2], const npy_intp strides[2])
  {
    std::cerr << "DEBUG: decoding NumPy array: " << boost::core::demangle(typeid(NPScalar).name())
              << "(sizeof="<<sizeof(NPScalar)<<") "
              << "dims=["<<dims[0]<<","<<dims[1]<<"] strides=["<<strides[0]<<","<<strides[1]<<"] into "
              << boost::core::demangle(typeid(EigScalar).name())
              << "(sizeof="<<sizeof(EigScalar)<<") .\n";
    
    // prepare the Eigen mapped object, of element type NPScalar
    auto mapped = 
      Eigen::Map<
        Eigen::Matrix<NPScalar,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor>, // RowMajor: fixes strides array order
        0,// Map options -- cannot assume aligned memory
        Eigen::Stride<Eigen::Dynamic,Eigen::Dynamic>
      >(  (NPScalar*) PyArray_DATA(array),
          dims[0], dims[1],
          Eigen::Stride<Eigen::Dynamic,Eigen::Dynamic>(strides[0]/sizeof(NPScalar), strides[1]/sizeof(NPScalar))
          );

    std::cerr << "DEBUG: mapped = \n" << mapped << "\n";

    // in-place construct at *storage
    (void) new (storage) MatrixType(mapped.template cast<EigScalar>());
  }

  template<typename NPScalar, TOMOGRAPHER_ENABLED_IF_TMPL(!IsNumConvertible<NPScalar,EigScalar>::value)>
  static inline void run(void * , PyArrayObject * , const npy_intp [2], const npy_intp [2]) {
    throw EigenNumpyConversionError(
        std::string() + "Cannot convert `" + boost::core::demangle(typeid(NPScalar).name()) + "' "
        + "to `" + boost::core::demangle(typeid(EigScalar).name()) + "'"
        );
  }
};

template<typename MatrixType>
struct eigen_python_converter
{
  typedef boost::python::to_python_converter<MatrixType, eigen_python_converter<MatrixType> > to_python;

  typedef typename MatrixType::Scalar EigScalar;
  
  //
  // convert Eigen -> Python Numpy
  //

  static PyObject* convert(const MatrixType& matrix) {
    npy_intp shape[2] = { (npy_intp)matrix.rows(), (npy_intp)matrix.cols() };
    npy_intp strides[2] = {
      (npy_intp)sizeof(EigScalar) * ( ((MatrixType::Options & Eigen::RowMajor) == 0)
                                      ? matrix.innerStride() : matrix.outerStride() ),
      (npy_intp)sizeof(EigScalar) * ( ((MatrixType::Options & Eigen::RowMajor) == 0)
                                      ? matrix.outerStride() : matrix.innerStride() )
    };
    PyArray_Descr * descr = PyArray_DescrNewFromType(NpyCode<EigScalar>::TypeCode);
    boost::python::object obj(
        boost::python::handle<>(
            PyArray_NewFromDescr(&PyArray_Type, descr,
                                 2, shape, strides, (void*)matrix.data(), 0, NULL)
            )
        );
    return boost::python::incref(obj.ptr());
  }

  //
  // convert Python Numpy -> Eigen
  //

  static void* convertible(PyObject* py_obj_ptr)
  {
    if (!PyArray_Check(py_obj_ptr))
      return NULL;

    // when this converter is installed, we must attempt to translate ANY numpy array into
    // an Eigen object, and fail at conversion if an error occurs.

    return py_obj_ptr;
  }

  static void construct(PyObject* py_obj_ptr,
                        boost::python::converter::rvalue_from_python_stage1_data* data)
  {
    PyArrayObject *array = reinterpret_cast<PyArrayObject*>(py_obj_ptr);
    void *storage = ((boost::python::converter::rvalue_from_python_storage<MatrixType>*)data)->storage.bytes;

    int nd = PyArray_NDIM(array);
    if (nd < 1) { // zero-D object??
      throw EigenNumpyConversionError("Invalid number of dimensions: "+std::to_string(nd));
    }

    const npy_intp * arraydimensions = PyArray_DIMS(array);
    const npy_intp * arraystrides = PyArray_STRIDES(array);

    std::cerr << "DEBUG: decoding NumPy array, nd=" << nd << ", "
              << "dims=[" << arraydimensions[0] << "," << (nd>1 ? arraydimensions[1] : -1) << "], "
              << "strides=[" << arraystrides[0] << ", " << (nd>1 ? arraystrides[1] : -1) << "]\n";

    // prepare as if for 1-D array
    npy_intp dims[2] = { arraydimensions[0], 1 };
    npy_intp strides[2] = { arraystrides[0], 0 };
    
    if (nd == 1) { // column vector
      // all ok
    } else if (nd == 2) { // matrix (or row vector)
      dims[1] = arraydimensions[1];
      strides[1] = arraystrides[1];
    } else {
      throw EigenNumpyConversionError("Cannot handle arrays with more than 2 dimensions. nd="+std::to_string(nd));
    }

    npyToCxxType<CopyNumpyDataToEigen<MatrixType> >(PyArray_TYPE(array), storage, array, dims, strides);

    data->convertible = storage;
  }

  static void from_python() {
    boost::python::converter::registry::push_back(
        &eigen_python_converter<MatrixType>::convertible,
        &eigen_python_converter<MatrixType>::construct,
        boost::python::type_id<MatrixType>()
        );
  }
};






// add the types you want to add
void register_eigen_converter()
{
  import_array(); //< required, or conversion leads to segfault
  
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

  boost::python::register_exception_translator<EigenNumpyConversionError>(hlp_EigenNumpyConversionError_py_translate);
}

