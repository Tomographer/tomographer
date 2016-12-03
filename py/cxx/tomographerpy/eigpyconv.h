
#ifndef EIGPYCONV_H
#define EIGPYCONV_H

#include <tomographerpy/common.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

// get a demangle() function from Boost, either with boost::core::demangle() (boost >=
// 1.56) or boost::units::detail::demangle() (boost before that)
#include <boost/version.hpp>
#if BOOST_VERSION >= 105600
#include <boost/core/demangle.hpp>
#else
#include <boost/units/detail/utility.hpp>
namespace boost { namespace core {  using boost::units::detail::demangle; } }
#endif

#include <boost/python/numeric.hpp>

#include <Eigen/Core>

#include <tomographer/tools/cxxutil.h>



struct EigenNumpyConversionError : public std::exception
{
  EigenNumpyConversionError(std::string msg)
    : _msg(msg) { }
  virtual ~EigenNumpyConversionError() throw() { }

  const char * what() const throw() { return _msg.c_str(); }
private:
  std::string _msg;
};



// convert C++ type -> NumPy type code.
template<typename Scalar> struct NpyCode {
  enum { TypeCode = NPY_NOTYPE };
  static const char * getCodeName() { static char s[] = "<INVALID>"; return s; }
};
// specializations
#define DECL_NPY_CODE_SPECIALIZED_TYPE(typ, CODE)                       \
  template<> struct NpyCode<typ> { enum { TypeCode = CODE };            \
    static const char * getCodeName() { static char s[] = #CODE; return s; } };

DECL_NPY_CODE_SPECIALIZED_TYPE(bool, NPY_BOOL)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_int8, NPY_INT8)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_int16, NPY_INT16)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_int32, NPY_INT32)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_int64, NPY_INT64)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_uint8, NPY_UINT8)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_uint16, NPY_UINT16)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_uint32, NPY_UINT32)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_uint64, NPY_UINT64)
#if NPY_FLOAT16 != NPY_UINT16
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_float16, NPY_FLOAT16)
#endif
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_float32, NPY_FLOAT32)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_float64, NPY_FLOAT64)
DECL_NPY_CODE_SPECIALIZED_TYPE(npy_longdouble, NPY_LONGDOUBLE)
DECL_NPY_CODE_SPECIALIZED_TYPE(std::complex<npy_float32>, NPY_COMPLEX64)
DECL_NPY_CODE_SPECIALIZED_TYPE(std::complex<npy_float64>, NPY_COMPLEX128)
DECL_NPY_CODE_SPECIALIZED_TYPE(std::complex<npy_longdouble>, NPY_CLONGDOUBLE)


// convert NumPy type code to C++ type
template<typename CallbackTmpl, typename... CallbackTmplArgs>
void npyToCxxType(int npy_type, CallbackTmplArgs &&... args) {
  switch (npy_type) {
  case NPY_BOOL: CallbackTmpl::template run<bool>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_INT8: CallbackTmpl::template run<npy_int8>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_INT16: CallbackTmpl::template run<npy_int16>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_INT32: CallbackTmpl::template run<npy_int32>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_INT64: CallbackTmpl::template run<npy_int64>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_UINT8: CallbackTmpl::template run<npy_uint8>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_UINT16: CallbackTmpl::template run<npy_uint16>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_UINT32: CallbackTmpl::template run<npy_uint32>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_UINT64: CallbackTmpl::template run<npy_uint64>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_FLOAT16: CallbackTmpl::template run<npy_float16>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_FLOAT32: CallbackTmpl::template run<npy_float32>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_FLOAT64: CallbackTmpl::template run<npy_float64>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_LONGDOUBLE: CallbackTmpl::template run<npy_longdouble>(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_COMPLEX64:
    CallbackTmpl::template run<std::complex<npy_float32> >(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_COMPLEX128:
    CallbackTmpl::template run<std::complex<npy_float64> >(std::forward<CallbackTmplArgs>(args)...); break;
  case NPY_CLONGDOUBLE:
    CallbackTmpl::template run<std::complex<npy_longdouble> >(std::forward<CallbackTmplArgs>(args)...); break;

  case NPY_NOTYPE: default:
    throw EigenNumpyConversionError("Unknown NumPy type code: "+std::to_string(npy_type));
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



template<typename EigDenseType>
struct CopyNumpyDataToEigen
{
  typedef typename EigDenseType::Scalar EigScalar;

  template<typename NPScalar, TOMOGRAPHER_ENABLED_IF_TMPL(IsNumConvertible<NPScalar,EigScalar>::value)>
  static inline void run(void * storage, PyArrayObject * array,
                         const npy_intp dims[2], const npy_intp strides[2])
  {
    // LET'S NOT MAKE PYTHON CALLS FROM BOOST/PYTHON CONVERSION ROUTINES
    //    auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);

    //    std::cerr << "DEBUG: decoding NumPy array: " << boost::core::demangle(typeid(NPScalar).name())
    //              << "(sizeof="<<sizeof(NPScalar)<<") "
    //              << "dims=["<<dims[0]<<","<<dims[1]<<"] strides=["<<strides[0]<<","<<strides[1]<<"] into "
    //              << boost::core::demangle(typeid(EigScalar).name())
    //              << "(sizeof="<<sizeof(EigScalar)<<") .\n";
    
    //    logger.debug("CopyNumpyDataToEigen::run() ...");

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

    //    logger.longdebug([&](std::ostream & stream) { stream << "mapped = \n" << mapped; });

    // in-place construct at *storage
    (void) new (storage) EigDenseType(mapped.template cast<EigScalar>());

    //    logger.debug("CopyNumpyDataToEigen::run() completed.");
  }

  template<typename NPScalar, TOMOGRAPHER_ENABLED_IF_TMPL(!IsNumConvertible<NPScalar,EigScalar>::value)>
  static inline void run(void * , PyArrayObject * , const npy_intp [2], const npy_intp [2]) {
    std::string s;
    s += "Cannot convert `";
    s += boost::core::demangle(typeid(NPScalar).name());
    s += "' to `";
    s += boost::core::demangle(typeid(EigScalar).name());
    s += "'";
    throw EigenNumpyConversionError(s);
  }
};

template<typename EigDenseType>
struct eigen_python_converter
{
  typedef boost::python::to_python_converter<EigDenseType, eigen_python_converter<EigDenseType> > to_python;

  typedef typename EigDenseType::Scalar EigScalar;
  
  //
  // convert Eigen -> Python Numpy
  //

  static PyObject* convert(const EigDenseType & matrix)
  {
    PyArray_Descr * descr = PyArray_DescrFromType(NpyCode<EigScalar>::TypeCode);

    const int elsize = descr->elsize;

    int nd = -1;
    npy_intp shape[2] = {0};
    npy_intp strides[2] = {0};

    std::size_t arr_data_sz; 

    if (EigDenseType::ColsAtCompileTime == 1) {
      // this in fact a true vector object -- generate a 1D NumPy array
      //
      // Note: We make a difference between a Eigen::MatrixXd with one column and a
      // Eigen::VectorXd.  The former converts to a np array of shape (size, 1,) while the
      // latter converts to a 1-D array.

      nd = 1;
      shape[0] = (npy_intp)matrix.rows();
      strides[0] = elsize * ( ((EigDenseType::Options & Eigen::RowMajor) == 0)
                              ? matrix.innerStride() : matrix.outerStride());
      if (shape[0] == 0) {
        arr_data_sz = 0;
      } else {
        arr_data_sz = (shape[0]-1)*strides[0] + elsize;
      }

      //fprintf(stderr, "Vector has strides (inner=%d,outer=%d), sz=%d, data sz=%u\n", (int)matrix.innerStride(),
      //        (int)matrix.outerStride(), (int)matrix.rows(), (unsigned int) arr_data_sz);

    } else {

      // a Matrix object --> convert to a 2D numpy array (even if the matrix has in fact
      // one column)

      nd = 2;
      shape[0] = matrix.rows();
      shape[1] = matrix.cols();
      strides[0] = elsize * ( ((EigDenseType::Options & Eigen::RowMajor) == 0)
                              ? matrix.innerStride() : matrix.outerStride() );
      strides[1] = elsize * ( ((EigDenseType::Options & Eigen::RowMajor) == 0)
                   ? matrix.outerStride() : matrix.innerStride() );
      if (shape[0] == 0 || shape[1] == 0) {
        arr_data_sz = 0;
      } else {
        arr_data_sz = (shape[0]-1)*strides[0] + (shape[1]-1)*strides[1] + elsize;
      }
    }

    // copy data because the scope of the Eigen::MatrixXd is completely unknown (it could
    // be temporary return value, for example)
    PyObject *pyarr = PyArray_NewFromDescr(&PyArray_Type, descr,
                                           nd, shape, strides, NULL, 0, NULL);

    // just copy the freakin' bytes:
    if (arr_data_sz > 0) {
      std::memcpy( PyArray_DATA(reinterpret_cast<PyArrayObject*>(pyarr)), (void*)matrix.data(), arr_data_sz );
    }

    return pyarr; // no need to go via boost::python::object, right?
  }

  //
  // convert Python Numpy -> Eigen
  //

  static void* convertible(PyObject* py_obj_ptr)
  {
    if (!PyArray_Check(py_obj_ptr)) {
      return NULL;
    }

    // when this converter is installed, we must attempt to translate ANY numpy array into
    // an Eigen object, and fail at conversion if an error occurs.
    return py_obj_ptr;
  }

  static void construct(PyObject* py_obj_ptr,
                        boost::python::converter::rvalue_from_python_stage1_data* data)
  {
    PyArrayObject *array = reinterpret_cast<PyArrayObject*>(py_obj_ptr);
    void *storage = ((boost::python::converter::rvalue_from_python_storage<EigDenseType>*)data)->storage.bytes;

    int nd = PyArray_NDIM(array);
    if (nd < 1) { // zero-D object??
      throw EigenNumpyConversionError("Invalid number of dimensions: "+std::to_string(nd));
    }

    const npy_intp * arraydimensions = PyArray_DIMS(array);
    const npy_intp * arraystrides = PyArray_STRIDES(array);

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

    npyToCxxType<CopyNumpyDataToEigen<EigDenseType> >(PyArray_TYPE(array), storage, array, dims, strides);

    data->convertible = storage;
  }

  static void from_python() {
    boost::python::converter::registry::push_back(
        &eigen_python_converter<EigDenseType>::convertible,
        &eigen_python_converter<EigDenseType>::construct,
        boost::python::type_id<EigDenseType>()
        );
  }
};





void register_eigen_converter();



#endif
