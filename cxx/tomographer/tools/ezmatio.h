#ifndef _MATIOPP_H
#define _MATIOPP_H

#include <inttypes.h>

#include <cerrno>

#include <complex>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

extern "C" {
#include <matio.h>
}

#include <tomographer/tools/fmt.h>


namespace Tomographer
{
namespace MAT
{

//! Base Exception class for errors within our MAT routines
class Exception : public std::exception {
  std::string p_heading;
  std::string p_msg;
public:
  Exception(const std::string& heading, const std::string& msg)
    : p_heading(heading), p_msg(msg)
  {
  }
  virtual ~Exception() throw() { }
  virtual const char * what() const throw() {
    return (p_heading + ": " + p_msg).c_str();
  }
};

class VarError : public Exception {
public:
  VarError(const std::string& msg) : Exception("Variable error", msg) { }
  virtual ~VarError() throw() { }
};
class VarReadError : public VarError {
public:
  VarReadError(const std::string& varname)
    : VarError(streamstr("Can't read variable `"<<varname<<"`"))
  {
  }
  virtual ~VarReadError() throw() { }
};
class VarTypeError : public VarError {
public:
  VarTypeError(const std::string& varname, const std::string& expected)
    : VarError(streamstr("Expected "<<expected<<" for variable `"<<varname<<"`"))
  {
    fprintf(stderr, "varname=%s\n", varname.c_str());
  }
  virtual ~VarTypeError() throw() { }
};
class VarMatTypeError : public VarError {
public:
  VarMatTypeError(const std::string& msg)
    : VarError(msg)
  {
  }
  virtual ~VarMatTypeError() throw() { }
};

class FileOpenError : public Exception {
public:
  FileOpenError(const std::string& fname, const std::string& errmsg = "")
    : Exception("File Error", "Error opening file `" + fname + (errmsg.size() ? "': "+errmsg : ""))
    {
    }
  virtual ~FileOpenError() throw() { }
};


class InvalidOperationError : public Exception {
public:
  InvalidOperationError(const std::string& msg)
    : Exception("Invalid operation", msg)
  {
  }
  virtual ~InvalidOperationError() throw() { }
};

class BadIndexError : public Exception {
public:
  BadIndexError(const std::string& msg) : Exception("Bad index", msg) { }
  virtual ~BadIndexError() throw() { }
};


class Var;


class File
{
public:
  File(const std::string& fname)
  {
    errno = 0;
    p_matfp = Mat_Open(fname.c_str(), MAT_ACC_RDONLY);
    if ( p_matfp == NULL ) {
      throw FileOpenError(fname, strerror(errno));
    }
  }

  ~File()
  {
    Mat_Close(p_matfp);
  }

  inline Var var(const std::string& varname, bool load_data = true);

  mat_t * get_mat_ptr()
  {
    return p_matfp;
  }
private:
  mat_t *p_matfp;
};



template<class It>
int get_numel(It b, It e)
{
  int n = 1;
  for (It it = b; it != e; ++it) {
    n *= (*it);
  }
  return n;
}


class DimList : public std::vector<int> {
public:
  DimList(const std::vector<int>& dims = std::vector<int>())
    : std::vector<int>(dims)
  {
  }
  template<class It>
  DimList(It b, It e) : std::vector<int>(b, e)
  {
  }

  inline int numel() const { return get_numel(begin(), end()); }

  inline int ndims() const { return (int)size(); }

  DimList& operator<<(int dim) {
    push_back(dim);
    return *this;
  }
  DimList& operator<<(const std::vector<int>& moredims) {
    insert(end(), moredims.begin(), moredims.end());
    return *this;
  }
};

std::ostream& operator<<(std::ostream& out, const DimList& dlist)
{
  out << "[";
  for (DimList::const_iterator it = dlist.begin(); it != dlist.end(); ++it) {
    if (it != dlist.begin()) {
      out << " ";
    }
    if (*it == -1) {
      // usually meant for arbitrary size requirement. In any case, it is not a physical
      // array size.
      out << '*';
    } else {
      out << *it;
    }
  }
  out << "]";
  return out;
}


class IndexList : public std::vector<int> {
public:
  IndexList(const std::vector<int>& dims = std::vector<int>(), int linearindex = -1, bool firstmajor = false)
    : std::vector<int>(dims.size()), p_dims(dims)
  {
    if (get_numel(dims.begin(), dims.end()) <= 0) {
      throw BadIndexError("Invalid indexing of zero-sized array given by dimension list");
    }
    if (linearindex >= 0) {
      setLinearIndex(linearindex, firstmajor);
    }
  }
  void setLinearIndex(int linearindex, bool firstmajor = false)
  {
    const int ndims = (int)p_dims.size();
    if ( firstmajor ) {
      for (int k = 0; k < ndims; ++k) {
	int thisind = linearindex % p_dims[k];
	this->at(k) = thisind;
	linearindex /= p_dims[k]; // integer division
      }
    } else {
      for (int k = ndims-1; k >= 0; --k) {
	int thisind = linearindex % p_dims[k];
	this->at(k) = thisind;
	linearindex /= p_dims[k]; // integer division
      }
    }
  }
  int linearindex(bool firstmajor = false) const
  {
    int linindex = 0;
    if ( firstmajor ) {
      for (int k = (int)p_dims.size()-1; k >= 0; --k) {
	linindex *= p_dims[k];
	linindex += this->at(k);
      }
    } else {
      for (int k = 0; k < (int)p_dims.size(); ++k) {
	linindex *= p_dims[k];
	linindex += this->at(k);
      }
    }
    return linindex;
  }
  void increment(bool firstmajor = false)
  {
    const int ndims = p_dims.size();
    if (firstmajor) {
      for (int k = 0; k < ndims; ++k) {
	at(k)++;
	if (this->operator[](k) < p_dims[k]) {
	  // if this increment succeeded and stays in range, ok and stop.
	  break;
	} else {
	  // otherwise continue the loop and increment the next value, while resetting
	  // this one to zero.
	  at(k) = 0;
	}
      }
    } else {
      for (int k = ndims-1; k >= 0; --k) {
	at(k)++;
	if (this->operator[](k) < p_dims[k]) {
	  // if this increment succeeded and stays in range, ok and stop.
	  break;
	} else {
	  // otherwise continue the loop and increment the next value, while resetting
	  // this one to zero.
	  at(k) = 0;
	}
      }
    }
  }

  IndexList& operator<<(int dim) {
    push_back(dim);
    return *this;
  }
  IndexList& operator<<(const std::vector<int>& moredims) {
    insert(end(), moredims.begin(), moredims.end());
    return *this;
  }

private:
  std::vector<int> p_dims;
};




template<class OutType>
class VarDataAccessor {
public:
  virtual ~VarDataAccessor() { }
  virtual OutType operator()(int linindex) const = 0;
  inline OutType value(int linindex) const { return this->operator()(linindex); }
};

template<class OutComplexT, class MatInnerT>
class ComplexValueAccessor : public VarDataAccessor<OutComplexT> {
public:
  ComplexValueAccessor(const MatInnerT *, const MatInnerT *)
  {
    throw VarTypeError("unknown", streamstr("Got unexpected complex type, expected "
					    <<typeid(OutComplexT).name()) );
  }
  OutComplexT operator()(int) const {
    throw VarTypeError("unknown", streamstr("Got unexpected complex type, expected "
					    <<typeid(OutComplexT).name()) );
    return OutComplexT();
  }
};
template<class OutComplexInnerT, class MatInnerT>
class ComplexValueAccessor<std::complex<OutComplexInnerT>, MatInnerT>
  : public VarDataAccessor<std::complex<OutComplexInnerT> >
{
  const MatInnerT *p_re;
  const MatInnerT *p_im;
  const std::complex<OutComplexInnerT> ImagUnit;
public:
  ComplexValueAccessor(const MatInnerT *re, const MatInnerT *im)
    : p_re(re), p_im(im), ImagUnit(0,1) { }
    
  std::complex<OutComplexInnerT> operator()(int linindex) const {
    return std::complex<OutComplexInnerT>(p_re[linindex], p_im[linindex]);
  }
};
  
template<class OutType, class MatType>
class SimpleValueAccessor : public VarDataAccessor<OutType> {
  const MatType *p_data;
public:
  SimpleValueAccessor(const MatType * data) : p_data(data) { }

  OutType operator()(int linindex) const {
    return p_data[linindex]; // let C++ handle the type conversion.
  }
};


template<int MatTType = -1>  struct MatType { };
template<>  struct MatType<MAT_T_DOUBLE> { typedef double Type; };
template<>  struct MatType<MAT_T_SINGLE> { typedef float Type; };
template<>  struct MatType<MAT_T_INT64> { typedef int64_t Type; };
template<>  struct MatType<MAT_T_INT32> { typedef int32_t Type; };
template<>  struct MatType<MAT_T_INT16> { typedef int16_t Type; };
template<>  struct MatType<MAT_T_INT8> { typedef int8_t Type; };
template<>  struct MatType<MAT_T_UINT64> { typedef uint64_t Type; };
template<>  struct MatType<MAT_T_UINT32> { typedef uint32_t Type; };
template<>  struct MatType<MAT_T_UINT16> { typedef uint16_t Type; };
template<>  struct MatType<MAT_T_UINT8> { typedef uint8_t Type; };
  

#define MAT_SWITCH_TYPE(typ, ...)                                       \
  { switch (typ) {                                                      \
    case MAT_T_DOUBLE: { typedef typename MatType<MAT_T_DOUBLE>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_SINGLE: { typedef typename MatType<MAT_T_SINGLE>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_INT64: { typedef typename MatType<MAT_T_INT64>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_INT32: { typedef typename MatType<MAT_T_INT32>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_INT16: { typedef typename MatType<MAT_T_INT16>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_INT8: { typedef typename MatType<MAT_T_INT8>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_UINT64: { typedef typename MatType<MAT_T_UINT64>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_UINT32: { typedef typename MatType<MAT_T_UINT32>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_UINT16: { typedef typename MatType<MAT_T_UINT16>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_UINT8: { typedef typename MatType<MAT_T_UINT8>::Type Type; { __VA_ARGS__; } break; } \
    default:                                                            \
      throw VarMatTypeError( streamstr("Uknown/unsupported encoded type from matio: " \
				       <<(typ)) );			\
    }                                                                   \
  }
#define MAT_SWITCH_COMPLEXABLE_TYPE(typ, ...)                           \
  { switch (typ) {                                                      \
    case MAT_T_DOUBLE: { typedef typename MatType<MAT_T_DOUBLE>::Type Type; { __VA_ARGS__; } break; } \
    case MAT_T_SINGLE: { typedef typename MatType<MAT_T_SINGLE>::Type Type; { __VA_ARGS__; } break; } \
    default:                                                            \
      throw VarMatTypeError( streamstr("Uknown/unsupported encoded type from matio: " \
				       <<(typ)) );			\
    }                                                                   \
  }
      

template<class OutT>
class ValueDecoder {
  const matvar_t *p_matvarinfo;
  const void * p_data;
  VarDataAccessor<OutT> * p_accessor;
public:
  ValueDecoder(const matvar_t *matvarinfo, const void * data, int dataoffset = 0)
    : p_matvarinfo(matvarinfo), p_data(data)
  {
    if ( ! matvarinfo->isComplex ) {
      // value is real, not complex.

      MAT_SWITCH_TYPE(matvarinfo->data_type,
		      p_accessor = new SimpleValueAccessor<OutT, Type>( (const Type*)data + dataoffset );
	  );
    } else {
      // value is complex.
      const mat_complex_split_t *cdata = (mat_complex_split_t*) matvarinfo->data;

      MAT_SWITCH_COMPLEXABLE_TYPE(matvarinfo->data_type,
				  p_accessor = new ComplexValueAccessor<OutT, Type>(
				      (const Type*)cdata->Re + dataoffset,
				      (const Type*)cdata->Im + dataoffset
				      );
	  );
    }
  }

  ~ValueDecoder()
  {
    delete p_accessor;
  }

  const VarDataAccessor<OutT> * accessor() { return p_accessor; }
};


class Var
{
private:
  struct VarData {
    VarData() : refcount(0), p_matvar(NULL) { }
    int refcount;
    matvar_t * p_matvar;
    std::string p_varname;
  };
  VarData *p_vardata;

public:
  Var(File *matf, const std::string& varname, bool load_data = true)
  {
    p_vardata = new VarData;
    p_vardata->refcount++;
    if (load_data) {
      p_vardata->p_matvar = Mat_VarRead(matf->get_mat_ptr(), varname.c_str());
    } else {
      p_vardata->p_matvar = Mat_VarReadInfo(matf->get_mat_ptr(), varname.c_str());
    }
    if (p_vardata->p_matvar == NULL) {
      throw VarReadError(varname);
    }
    p_vardata->p_varname = varname;
  }
  Var(const Var& copy)
  {
    p_vardata = copy.p_vardata;
    p_vardata->refcount++;
  }
  virtual ~Var() {
    p_vardata->refcount--;
    if (!p_vardata->refcount) {
      Mat_VarFree(p_vardata->p_matvar);
      delete p_vardata;
    }
  }

  inline std::string varname() const {
    return std::string(p_vardata->p_varname);
  }

  inline int ndims() const {
    return p_vardata->p_matvar->rank;
  }
  inline DimList dims() const {
    return DimList(&p_vardata->p_matvar->dims[0],
		   &p_vardata->p_matvar->dims[p_vardata->p_matvar->rank]);
  }
  inline int numel() const {
    return dims().numel();
  }
  inline bool isComplex() const {
    return p_vardata->p_matvar->isComplex;
  }

  const matvar_t * get_matvar_ptr() const
  {
    return p_vardata->p_matvar;
  }

};


inline
Var File::var(const std::string& varname, bool load_data)
{
  return Var(this, varname, load_data);
}

/* get value types */

template<class T, bool want_complex>
struct _mat_helper_get_value_base {
  static inline T get_value(const Var& var)
  {
    const matvar_t * mvar = var.get_matvar_ptr();
      
    if (mvar->isComplex != want_complex || mvar->rank > 2 || mvar->rank == 0 ||
	(mvar->rank == 1 && mvar->dims[0] != 1) ||
	(mvar->rank == 2 && (mvar->dims[0] != 1 || mvar->dims[1] != 1))
	) {
      std::stringstream errstr;
      if (want_complex) {
	errstr << "complex ";
      } else {
	errstr << "real ";
      }
      errstr << "value";
      throw VarTypeError(var.varname(), errstr.str());
    }
      
    return ValueDecoder<T>(mvar, mvar->data).accessor()->value(0);
  }
};

template<class T>
struct _mat_helper_get_value
  : public _mat_helper_get_value_base<T,false> { };

template<class cT>
struct _mat_helper_get_value<std::complex<cT> >
  : public _mat_helper_get_value_base<std::complex<cT>,true> { };

template<class T>
inline T value(const Var& var)
{
  return _mat_helper_get_value<T>::get_value(var);
}

  
/* get vector/matrix types */

inline bool _mat_helper_matches_wanted_dims(const DimList& a, const DimList& want)
{
  if (a.size() != want.size()) {
    return false;
  }
  for (unsigned int k = 0; k < a.size(); ++k) {
    if (want[k] >= 0 && a[k] != want[k]) {
      return false;
    }
  }
  return true;
}

template<class T>
struct _mat_complex_tester { inline static bool is_complex() { return false; } };
template<class T>
struct _mat_complex_tester<std::complex<T> > { inline static bool is_complex() { return true; } };

inline std::ostream& fmt_req_shape(std::ostream& str, bool is_complex, int ndims, const DimList& wantdims,
				   bool is_square)
{
  str << ((is_complex)?std::string("complex "):std::string("real "));
  if (ndims == 1) {
    str << "vector";
  } else if (ndims == 2) {
    if (is_square) {
      str << "square ";
    }
    str << "matrix";
  } else if (ndims > 0) {
    str << ndims << "-D array";
  } else {
    str << "array";
  }
  if (wantdims.size()) {
    str << " of shape " << wantdims;
  }
  return str;
}

template<class T>
struct _mat_helper_check_vector {
  static inline void check(const Var& var,
			   int ndims,
			   const DimList& wantdims,
			   bool is_square)
  {
    const matvar_t * mvar = var.get_matvar_ptr();
    const DimList dims = var.dims();

    const bool is_complex = _mat_complex_tester<T>::is_complex();

    if ((mvar->isComplex && !is_complex) ||
	(ndims > 0 && mvar->rank != ndims) ||
	(is_square && ndims == 2 && mvar->rank == 2 && mvar->dims[0] != mvar->dims[1]) ||
	(wantdims.size() > 0 && !_mat_helper_matches_wanted_dims(dims, wantdims))) {
      std::stringstream errstr;

      fmt_req_shape(errstr, is_complex, ndims, wantdims, is_square);
      errstr << ", got ";
      fmt_req_shape(errstr, mvar->isComplex, mvar->rank, var.dims(),
		    (mvar->rank == 2 && mvar->dims[0] == mvar->dims[1]));
      fprintf(stderr, "Bad var type for variable %s\n", var.varname().c_str());
      throw VarTypeError(var.varname(), errstr.str());
    }
  }
};

template<class T>
inline void _mat_helper_copy_vector(const Var& var,
				    T * target,
				    size_t max_target_len,
				    size_t data_offset,
				    bool first_major)
{
  const matvar_t * mvar = var.get_matvar_ptr();
  const DimList dims = var.dims();

  ValueDecoder<T> decoder(mvar, mvar->data, data_offset);
  VarDataAccessor<T> * accessor = decoder.accessor();

  const int numel = dims.numel();
  if (numel == 0) {
    // don't bother copying anything, and mostly don't try to access the data, if
    // there is no data in the first place.
    return;
  }

  if (numel > max_target_len) {
    std::stringstream str;
    str << "Not enough memory space: numel=" << numel << " but max_target_len=" << max_target_len;
    throw VarTypeError(var.varname(), str.str());
  }

  IndexList datavarindex(dims, 0, !first_major);
  for (size_t k = 0; k < numel; ++k) {
    target[k] = accessor(datavarindex.linearindex(first_major));
    datavarindex.increment(first_major);
  }
}

template<class T>
inline std::vector<T> getStdVector(const Var& var,
				   int ndims = -1,
				   const DimList& wantdims = DimList(),
				   bool is_square = false,
				   bool first_major = false)
{
  std::vector<T> data(var.numel());
  _mat_helper_check_vector<T>::check(var, ndims, wantdims, is_square);
  _mat_helper_copy_vector(var, &data[0], data.size(), 0, first_major);
  return data;
}


template<class T>
inline std::vector<std::vector<T> > getListOfStdVectors(const Var& var,
							int innerndims = -1,
							const DimList& wantinnerdims = DimList(),
							bool inner_is_square = false,
							bool inner_first_major = false)
{
  const DimList dims = var.dims();

  _mat_helper_check_vector<T>::check(var,
				     (innerndims >= 0) ? innerndims+1 : -1 ,
				     wantinnerdims.size() ?  DimList() << wantinnerdims << -1  :  DimList(),
				     inner_is_square);

  if (var.numel() == 0) {
    // don't bother trying to copy data
    return std::vector<std::vector<T> >();
  }

  DimList innerdims = dims;
  size_t last_dim = innerdims.back();
  innerdims.pop_back();
    
  size_t k;
  std::vector<std::vector<T> > the_data(last_dim);
    
  const size_t lenofonevector = innerdims.numel();

  for (k = 0; k < last_dim; ++k) {
    the_data[k] = std::vector<T>(lenofonevector);
    _mat_helper_copy_vector<T>(var, &the_data[k][0], lenofonevector, k*lenofonevector, inner_first_major);
  }
    
  return the_data;
}


template<class OutType, class MatType, int Major>
struct _mat_eigen_matrix_caster_base
{
  static inline Eigen::Matrix<OutType,Eigen::Dynamic,Eigen::Dynamic,Major>
  real_map(const Var&, MatType * data, size_t data_offset, const DimList& dims)
  {
    return (Eigen::Map<Eigen::Matrix<MatType,Eigen::Dynamic,Eigen::Dynamic,Major> >(
		data + data_offset, dims[0], dims[1]
		)
	).template cast<OutType>();
  }
  static inline Eigen::Matrix<OutType,Eigen::Dynamic,Eigen::Dynamic,Major>
  complex_map(const Var& var, MatType * /*redata*/, MatType * /*imdata*/, size_t /*data_offset*/,
	      const DimList& /*dims*/) {
    throw VarTypeError(var.varname(), "Can't assign complex value to non-complex !");
  };
};
template<class OutType, class MatType, int Major>
struct _mat_eigen_matrix_caster : public _mat_eigen_matrix_caster_base<OutType,MatType,Major>
{
};
template<class OutCplx, class MatType, int Major>
struct _mat_eigen_matrix_caster<std::complex<OutCplx>,MatType,Major>
  : public _mat_eigen_matrix_caster_base<std::complex<OutCplx>,MatType,Major>
{
  // additional handling of converting complex values.
  static inline Eigen::Matrix<std::complex<OutCplx>,Eigen::Dynamic,Eigen::Dynamic,Major>
  complex_map(const Var&, MatType * redata, MatType * imdata, size_t data_offset,
	      const DimList& dims)
  {
    return (Eigen::Map<Eigen::Matrix<MatType,Eigen::Dynamic,Eigen::Dynamic,Major> >(
		redata + data_offset, dims[0], dims[1]
		).template cast<std::complex<MatType> >()
	    + std::complex<MatType>(0,1) *
	    Eigen::Map<Eigen::Matrix<MatType,Eigen::Dynamic,Eigen::Dynamic,Major> >(
		imdata + data_offset, dims[0], dims[1]
		).template cast<std::complex<MatType> >()
	).template cast<std::complex<OutCplx> >();
  }
};

template<class EigMatrix>
inline void _mat_helper_get_eigen_matrix(const Var& var, size_t data_offset, EigMatrix * matrix)
{
  const matvar_t * mvar = var.get_matvar_ptr();
  const DimList dims(&mvar->dims[0], &mvar->dims[mvar->rank]);

  const bool rowmajor = (bool)(EigMatrix::Options & Eigen::RowMajor);

  // ### FIXME: UGLY: using internals... :(  better way?
  typedef typename Eigen::internal::traits<EigMatrix>::Scalar Scalar;

  if (!mvar->isComplex) {
    MAT_SWITCH_TYPE(mvar->data_type,
		    if (rowmajor) {
		      *matrix = _mat_eigen_matrix_caster<Scalar,Type,Eigen::RowMajor>
			::real_map(var, (Type*)mvar->data, data_offset, dims);
		    } else {
		      *matrix = _mat_eigen_matrix_caster<Scalar,Type,Eigen::ColMajor>
			::real_map(var, (Type*)mvar->data, data_offset, dims);
		    }
	);
  } else {
    const mat_complex_split_t * cdata = (const mat_complex_split_t*) mvar->data;

    MAT_SWITCH_COMPLEXABLE_TYPE(
	mvar->data_type,
	if (rowmajor) {
	  *matrix = _mat_eigen_matrix_caster<Scalar,Type,Eigen::RowMajor>::complex_map(
	      var, (Type*)cdata->Re, (Type*)cdata->Im, data_offset,
	      dims);
	} else {
	  *matrix = _mat_eigen_matrix_caster<Scalar,Type,Eigen::ColMajor>::complex_map(
	      var, (Type*)cdata->Re, (Type*)cdata->Im, data_offset,
	      dims);
	}
	);
  }
}

template<class EigMatrix>
inline void getEigenMatrix(const Var& var, EigMatrix * matrix)
{
  // ### FIXME: UGLY: using internals... :(  better way?
  typedef typename Eigen::internal::traits<EigMatrix>::Scalar Scalar;

  DimList wantdims;
  wantdims << ((EigMatrix::RowsAtCompileTime != Eigen::Dynamic) ? EigMatrix::RowsAtCompileTime : -1)
	   << ((EigMatrix::ColsAtCompileTime != Eigen::Dynamic) ? EigMatrix::ColsAtCompileTime : -1) ;
    
  _mat_helper_check_vector<Scalar>::check(var, 2, wantdims, false);

  _mat_helper_get_eigen_matrix(var, 0, matrix);
}

template<class EigMatrix, class Alloc>
inline void getListOfEigenMatrices(const Var& var, std::vector<EigMatrix,Alloc> * matrices,
				   bool is_square = false)
{
  const DimList dims = var.dims();

  // ### FIXME: UGLY: using internals... :(  better way?
  typedef typename Eigen::internal::traits<EigMatrix>::Scalar Scalar;

  int inner_ndims = -1;
  DimList innerwantdims;
  if (EigMatrix::RowsAtCompileTime != Eigen::Dynamic &&
      EigMatrix::ColsAtCompileTime != Eigen::Dynamic) {
    inner_ndims = ((int)(EigMatrix::RowsAtCompileTime > 1) +
		   (int)(EigMatrix::ColsAtCompileTime > 1));
    innerwantdims << (EigMatrix::RowsAtCompileTime != Eigen::Dynamic ?
		      EigMatrix::RowsAtCompileTime : -1)
		  << (EigMatrix::ColsAtCompileTime != Eigen::Dynamic ?
		      EigMatrix::ColsAtCompileTime : -1);
  }

  _mat_helper_check_vector<Scalar>::check(var,
					  (inner_ndims >= 0) ? inner_ndims+1 : -1 ,
					  (innerwantdims.size()? DimList() << innerwantdims << -1 : DimList()),
					  is_square);
    
  if (var.numel() == 0) {
    // don't bother trying to copy any data and provoking segfaults
    matrices->clear();
    return;
  }

  DimList innerdims = dims;
  size_t last_dim = innerdims.back();
  innerdims.pop_back();
    
  size_t k;
  matrices->resize(last_dim);
    
  const size_t lenofonevector = innerdims.numel();

  for (k = 0; k < last_dim; ++k) {
    _mat_helper_get_eigen_matrix(var, k*lenofonevector, &(*matrices)[k]);
  }
}
  

} // namespace MAT
} // namespace Tomographer




#endif
