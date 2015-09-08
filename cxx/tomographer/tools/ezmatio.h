/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#ifndef _TOMOGRAPHER_TOOLS_EZMATIO_H
#define _TOMOGRAPHER_TOOLS_EZMATIO_H

#include <cstdint>
#include <cerrno>

#include <complex>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <sstream>

extern "C" {
#  include <matio.h>
}

#include <Eigen/Core>

#include <tomographer/qit/util.h>
#include <tomographer/tools/fmt.h>


namespace Tomographer
{
// namespace doc in doc/doxdocs/namespaces.cxx
namespace MAT
{

//! Base Exception class for errors within our MAT routines
class Exception : public std::exception
{
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

//! Exception relating to a MATLAB variable in the data file
class VarError : public Exception {
public:
  VarError(const std::string& msg) : Exception("Variable error", msg) { }
  virtual ~VarError() throw() { }
};

//! Error while reading a variable from the MATLAB data file
class VarReadError : public VarError {
public:
  VarReadError(const std::string& varname)
    : VarError(streamstr("Can't read variable `"<<varname<<"`"))
  {
  }
  virtual ~VarReadError() throw() { }
};

//! Type mismatch (wrong type requested) in a variable read from the MATLAB data file
class VarTypeError : public VarError {
public:
  VarTypeError(const std::string& varname, const std::string& expected)
    : VarError(streamstr("Expected "<<expected<<" for variable `"<<varname<<"`"))
  {
    //fprintf(stderr, "varname=%s\n", varname.c_str());
  }
  virtual ~VarTypeError() throw() { }
};

//! Unknown type of a variable present in the data file
class VarMatTypeError : public VarError {
public:
  VarMatTypeError(const std::string& msg)
    : VarError(msg)
  {
  }
  virtual ~VarMatTypeError() throw() { }
};

//! Error while opening a MATLAB file
class FileOpenError : public Exception {
public:
  FileOpenError(const std::string& fname, const std::string& errmsg = "")
    : Exception("File Error", "Error opening file `" + fname + (errmsg.size() ? "': "+errmsg : ""))
    {
    }
  virtual ~FileOpenError() throw() { }
};

/*
class InvalidOperationError : public Exception {
public:
  InvalidOperationError(const std::string& msg)
    : Exception("Invalid operation", msg)
  {
  }
  virtual ~InvalidOperationError() throw() { }
  };
*/

//! Invalid index or index list provided to a routine
class InvalidIndexError : public Exception {
public:
  InvalidIndexError(const std::string& msg) : Exception("Bad index", msg) { }
  virtual ~InvalidIndexError() throw() { }
};


class Var;


/** \brief A MATLAB file open for reading data.
 *
 * Access variables with \ref var()
 */
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
  File(const File& other) = delete;
  File(File&& other)
  {
    // move constructor. Steal the other object's MAT file handle
    p_matfp = other.p_matfp;
    other.p_matfp = NULL;
  }

  ~File()
  {
    if ( p_matfp != NULL ) {
      Mat_Close(p_matfp);
    }
  }

  inline Var var(const std::string& varname, bool load_data = true);

  inline std::vector<Var> getVarInfoList();

  mat_t * getMatPtr()
  {
    return p_matfp;
  }
  
private:
  mat_t *p_matfp;
};


/** \brief Calculate the product of all dimensions
 *
 * Given two C++/STL-type iterators \a begin and \a end, calculate the product of the
 * sequence of elements. For an empty sequence (<em>begin==end</em>), the result is \c 1.
 */
template<typename It, typename ValueType = typename std::iterator_traits<It>::value_type>
ValueType get_numel(It begin, It end)
{
  ValueType n = 1;
  for (It it = begin; it != end; ++it) {
    n *= (*it);
  }
  return n;
}


/** \brief An array of ints which specifies a list of dimensions.
 *
 */
class DimList : public std::vector<int>
{
public:
  DimList() : std::vector<int>() { }
  
  template<typename VectorType>
  DimList(VectorType&& dims)
    : std::vector<int>(std::forward<VectorType>(dims))
  {
  }
  template<class It>
  DimList(It b, It e) : std::vector<int>(b, e)
  {
  }

  inline int numel() const { return get_numel(begin(), end()); }

  inline int ndims() const { return (int)size(); }

  inline bool matchesWanted(const DimList& wanted) const
  {
    if (size() != wanted.size()) {
      return false;
    }
    for (std::size_t k = 0; k < size(); ++k) {
      if (wanted[k] >= 0 && this->operator[](k) != wanted[k]) {
        return false;
      }
    }
    return true;
  }

  DimList& operator<<(int dim) {
    push_back(dim);
    return *this;
  }
  DimList& operator<<(const std::vector<int>& moredims) {
    insert(end(), moredims.begin(), moredims.end());
    return *this;
  }
};

//! C++ output stream operators for a \ref DimList
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

/** \brief A list of indices with an API for linear or subindices access.
 *
 * Stores also the dimensions. (.....................)
 *
 * \todo DOC.......... in particular what \a RowMajor does......
 *
 */
template<bool RowMajor = false>
class IndexList : public std::vector<int>
{
public:
  IndexList(const std::vector<int>& dims = std::vector<int>(), int linearindex = -1)
    : std::vector<int>(dims.size()), p_dims(dims)
  {
    if (get_numel(dims.begin(), dims.end()) <= 0) {
      throw InvalidIndexError("Invalid indexing of zero-sized array given by dimension list");
    }
    if (linearindex >= 0) {
      setLinearIndex(linearindex);
    }
  }
  template<typename VectorIntInitializer>
  IndexList(const std::vector<int>& dims, VectorIntInitializer&& index)
    : std::vector<int>(std::forward<VectorIntInitializer>(index)), p_dims(dims)
  {
    if (get_numel(dims.begin(), dims.end()) <= 0) {
      throw InvalidIndexError("Invalid indexing of zero-sized array given by dimension list");
    }
  }
  void setLinearIndex(int linearindex)
  {
    const int ndims = (int)p_dims.size();
    if ( RowMajor ) {
      for (int k = ndims-1; k >= 0; --k) {
	this->at(k) = linearindex % p_dims[k];
	linearindex /= p_dims[k]; // integer division
      }
    } else {
      for (int k = 0; k < ndims; ++k) {
	this->at(k) = linearindex % p_dims[k];
	linearindex /= p_dims[k]; // integer division
      }
    }
  }
  int linearIndex() const
  {
    int linindex = 0;
    if ( RowMajor ) {
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

  inline const IndexList & index() const
  {
    return *this;
  }

  inline IndexList && index() &&
  {
    return std::move(*this);
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


template<bool RowMajor>
std::ostream& operator<<(std::ostream& str, const IndexList<RowMajor> & indexlist)
{
  str << "[";
  for (std::size_t j = 0; j < indexlist.size(); ++j) {
    str << indexlist[j];
    if (j < indexlist.size()) {
      str << ", ";
    } else {
      str << ";";
    }
  }
  return str << "==" << indexlist.linearIndex() << "]";
}


/** \brief Utility to iterate over a multidim array by increasing linear index
 *
 */
template<bool RowMajor = false, typename IntType = int>
class IndexListIterator
{
  const std::vector<IntType> p_dims;
  const IntType p_numel;

  std::vector<IntType> p_index;
  IntType p_linearIndex;

public:
  IndexListIterator(const std::vector<IntType>& dims = std::vector<IntType>())
    : p_dims(dims),
      p_numel(get_numel(dims.begin(), dims.end())),
      p_index(dims.size(), 0),
      p_linearIndex(0)
  {
    if (p_numel <= 0) {
      std::ostringstream ss;
      ss << "Invalid dimensions: " << p_dims;
      throw InvalidIndexError(ss.str());
    }
  }

  inline const std::vector<IntType> & index() const
  {
    return p_index;
  }
  inline std::vector<IntType> && index() &&
  {
    return std::move(p_index);
  }

  inline IntType linearIndex() const
  {
    return p_linearIndex;
  }
  
  inline IntType numel() const
  {
    return p_numel;
  }


  TOMOGRAPHER_ENABLED_IF(RowMajor)
  IntType increment()
  {
    for (int k = p_dims.size() - 1; k >= 0; --k) {
      p_index[k]++;
      if (p_index[k] < p_dims[k]) {
        // if this increment succeeded and stays in range, ok and stop.
        break;
      } else {
        // otherwise continue the loop and increment the next value, while resetting
        // this one to zero.
        p_index[k] = 0;
      }
    }
    return ++p_linearIndex;
  }

  TOMOGRAPHER_ENABLED_IF(!RowMajor)
  IntType increment()
  {
    for (int k = 0; k < (int)p_dims.size(); ++k) {
      p_index[k]++;
      if (p_index[k] < p_dims[k]) {
        // if this increment succeeded and stays in range, ok and stop.
        break;
      } else {
        // otherwise continue the loop and increment the next value, while resetting
        // this one to zero.
        p_index[k] = 0;
      }
    }
    return ++p_linearIndex;
  }

  IntType operator++() {
    return increment();
  }

  bool valid() const
  {
    return Tools::is_positive<IntType>(p_linearIndex) && p_linearIndex < p_numel;
  }
};


template<bool RowMajor, typename IntType>
std::ostream& operator<<(std::ostream& str, const IndexListIterator<RowMajor, IntType> & indexlistit)
{
  std::vector<int> indexlist{indexlistit.index()};

  str << "[";
  for (std::size_t j = 0; j < indexlist.size(); ++j) {
    str << indexlist[j];
    if (j < indexlist.size()) {
      str << ", ";
    } else {
      str << ";";
    }
  }
  return str << "==" << indexlistit.linearIndex() << "]";
}





/** \brief Specializable template which takes care of decoding values
 *
 * Specialize this template to your favorite return type to define a decoder for that
 * type.
 *
 */
template<typename T, typename Enabled = void>
struct VarValueDecoder
{
  typedef T RetType;

  static inline void checkShape(const Var &)
  {
  }

  static inline RetType decodeValue(const Var &  /* var */)
  {
    throw std::runtime_error(std::string("Not Implemented: Please specialize MAT::VarValueDecoder<> for ")
                             + typeid(T).name());
  }
};






/** \brief A MATLAB variable in the MAT file.
 *
 */
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

  Var(matvar_t * varinfo) // take over the struct ownership
  {
    if (varinfo == NULL) {
      throw VarReadError("<unknown>");
    }
    p_vardata = new VarData;
    p_vardata->refcount++;
    p_vardata->p_matvar = varinfo;
    p_vardata->p_varname = std::string(varinfo->name);
  }

public:
  Var(File & matf, const std::string& varname, bool load_data = true)
  {
    p_vardata = new VarData;
    p_vardata->refcount++;
    if (load_data) {
      p_vardata->p_matvar = Mat_VarRead(matf.getMatPtr(), varname.c_str());
    } else {
      p_vardata->p_matvar = Mat_VarReadInfo(matf.getMatPtr(), varname.c_str());
    }
    if (p_vardata->p_matvar == NULL) {
      delete p_vardata;
      p_vardata = NULL;
      throw VarReadError(varname);
    }
    p_vardata->p_varname = varname;
  }

  Var(const Var& copy)
  {
    p_vardata = copy.p_vardata;
    p_vardata->refcount++;
  }

  Var(Var&& other)
  {
    // steal the data
    p_vardata = other.p_vardata;
    other.p_vardata = NULL;
  }

  virtual ~Var() {
    if (p_vardata != NULL) {
      p_vardata->refcount--;
      if (!p_vardata->refcount) {
        Mat_VarFree(p_vardata->p_matvar);
        delete p_vardata;
      }
    }
  }

  static Var takeOver(matvar_t * varinfo)
  {
    return Var(varinfo);
  }


  inline const std::string & varName() const
  {
    assert(p_vardata != NULL);
    return p_vardata->p_varname;
  }

  inline int ndims() const
  {
    assert(p_vardata != NULL);
    return p_vardata->p_matvar->rank;
  }
  inline DimList dims() const
  {
    assert(p_vardata != NULL);
    return DimList(&p_vardata->p_matvar->dims[0],
		   &p_vardata->p_matvar->dims[p_vardata->p_matvar->rank]);
  }
  inline int numel() const
  {
    assert(p_vardata != NULL);
    return dims().numel();
  }
  inline bool isComplex() const
  {
    assert(p_vardata != NULL);
    return p_vardata->p_matvar->isComplex;
  }
  inline bool isSquareMatrix() const
  {
    assert(p_vardata != NULL);
    return p_vardata->p_matvar->rank == 2 && p_vardata->p_matvar->dims[0] == p_vardata->p_matvar->dims[1];
  }

  inline bool hasData() const
  {
    assert(p_vardata != NULL);
    return (p_vardata->p_matvar->data != NULL);
  }

  template<typename T>
  inline typename VarValueDecoder<T>::RetType value() const;

  const matvar_t * getMatvarPtr() const
  {
    assert(p_vardata != NULL);
    return p_vardata->p_matvar;
  }

};


template<typename T>
inline typename VarValueDecoder<T>::RetType value(const Var& var)
{
  VarValueDecoder<T>::checkShape(var);
  return VarValueDecoder<T>::decodeValue(var);
}

template<typename T>
inline typename VarValueDecoder<T>::RetType Var::value() const
{
  return Tomographer::MAT::value<T>(*this);
}


// ---------------


inline Var File::var(const std::string& varname, bool load_data)
{
  return Var(*this, varname, load_data);
}

inline std::vector<Var> File::getVarInfoList()
{
  std::vector<Var> varlist;

  Mat_Rewind(p_matfp);
  matvar_t * p = NULL;
  while ( (p = Mat_VarReadNextInfo(p_matfp)) != NULL ) {
    varlist.push_back(Var::takeOver(p));
  }
  
  return varlist;
}




// =============================================================================
// Some basic utilities for dealing with MATLAB variables
// =============================================================================



/** \brief Map matio's constants to C/C++ types
 *
 * The constants \a MAT_T_DOUBLE, \a MAT_T_SINGLE, etc. describing MATLAB's types are
 * mapped to C/C++ types via this template. You can get the C/C++ type by using the
 * constant as template parameter, e.g.:
 * \code
 *   typedef MatType<MAT_T_DOUBLE>::Type double_type; // double
 *   typedef MatType<MAT_T_SINGLE>::Type single_type; // float
 *   typedef MatType<MAT_T_UINT32>::Type uint32_type; // uint32_t
 * \endcode
 */
template<int MatTypeId = -1>  struct MatType { };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_DOUBLE
template<>  struct MatType<MAT_T_DOUBLE> { typedef double Type; };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_DOUBLE
template<>  struct MatType<MAT_T_SINGLE> { typedef float Type; };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_INT64
template<>  struct MatType<MAT_T_INT64> { typedef int64_t Type; };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_INT32
template<>  struct MatType<MAT_T_INT32> { typedef int32_t Type; };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_INT16
template<>  struct MatType<MAT_T_INT16> { typedef int16_t Type; };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_INT8
template<>  struct MatType<MAT_T_INT8> { typedef int8_t Type; };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_UINT64
template<>  struct MatType<MAT_T_UINT64> { typedef uint64_t Type; };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_UINT32
template<>  struct MatType<MAT_T_UINT32> { typedef uint32_t Type; };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_UINT16
template<>  struct MatType<MAT_T_UINT16> { typedef uint16_t Type; };
//! Specialization of \ref MatType<int MatTypeId> for MAT_T_UINT8
template<>  struct MatType<MAT_T_UINT8> { typedef uint8_t Type; };


/** \brief Useful hack to get C++ real scalar type from dynamical MAT type ID
 *
 * This works like a switch(){ ... } statement, where the \a typ is checked against \a
 * MAT_T_DOUBLE, \a MAT_T_SINGLE, etc. and where the given code (...) is executed with the
 * identifier \a Type set to the given C++ type.
 */
#define MAT_SWITCH_REAL_TYPE(typ, ...)                                  \
  do { switch (typ) {                                                   \
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
  } while (false)

/** \brief Useful hack to get C++ complex type from dynamical MAT type ID
 *
 * The MAT type ID is checked against MAT_T_DOUBLE and MAT_T_SINGLE only, and the
 * corresponding code executed with the identifier \a Type defined as the appropriate C++
 * std::complex<T> type.
 */
#define MAT_SWITCH_COMPLEX_TYPE(typ, ...)                           \
  do { switch (typ) {                                                      \
    case MAT_T_DOUBLE: { typedef std::complex<typename MatType<MAT_T_DOUBLE>::Type> Type; { __VA_ARGS__; } break; } \
    case MAT_T_SINGLE: { typedef std::complex<typename MatType<MAT_T_SINGLE>::Type> Type; { __VA_ARGS__; } break; } \
    default:                                                            \
      throw VarMatTypeError( streamstr("Uknown/unsupported encoded type from matio: " \
				       <<(typ)) );			\
    }                                                                   \
  } while (false)


/** \brief Useful hack to get C++ type from dynamical MAT type ID
 *
 * If the variable corresponding to \a matvar_ptr is of complex type, then this macro
 * behaves just like \a MAT_SWITCH_REAL_TYPE. If it is of complex type, then it behaves
 * just like \a MAT_SWITCH_COMPLEX_TYPE.
 *
 */
#define MAT_SWITCH_TYPE(matvar_ptr, ...)                                \
  do {                                                                  \
    if (!(matvar_ptr)->isComplex) {                                     \
      switch ((matvar_ptr)->data_type) {                                \
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
      default:                                                          \
        throw VarMatTypeError( streamstr("Uknown/unsupported encoded type from matio: " \
                                         << (matvar_ptr)->data_type) ); \
      }                                                                 \
    } else {                                                            \
      switch ((matvar_ptr)->data_type) {                                \
      case MAT_T_DOUBLE: { typedef std::complex<typename MatType<MAT_T_DOUBLE>::Type> Type; { __VA_ARGS__; } break; } \
      case MAT_T_SINGLE: { typedef std::complex<typename MatType<MAT_T_SINGLE>::Type> Type; { __VA_ARGS__; } break; } \
      default:                                                          \
        throw VarMatTypeError( streamstr("Uknown/unsupported encoded type from matio: " \
                                         << (matvar_ptr)->data_type) ); \
      }                                                                 \
    }                                                                   \
  } while (false)
      



// =============================================================================
// now, let's define some ways to get variable values.
// =============================================================================



//! Interface to read out a single value.
template<typename T>
struct VarValueDecoder<T,
                       typename std::enable_if<(std::numeric_limits<T>::is_specialized ||
                                                Tools::is_complex<T>::value)>::type
                       >
{
  typedef T RetType;

  static inline void checkShape(const Var & var)
  {
    if (var.isComplex() && !Tools::is_complex<T>::value) {
      throw VarTypeError(var.varName(),
                         streamstr("Can't store complex matrix in type " << typeid(T).name()));
    }
    if (var.numel() != 1) {
      throw VarTypeError(var.varName(),
                         streamstr("Expected scalar but got matrix"));
    }
  }
                             
  static inline RetType decodeValue(const Var & var)
  {
    const matvar_t * matvar_ptr = var.getMatvarPtr();

    MAT_SWITCH_TYPE(matvar_ptr,
                    return get_value<Type>(matvar_ptr, var.varName());
        );
    // should never reach here
    assert(false);
  }

private:
  template<typename MATType,
           TOMOGRAPHER_ENABLED_IF_TMPL(!Tools::is_complex<RetType>::value &&
                                       !Tools::is_complex<MATType>::value)>
  static inline RetType get_value(const matvar_t * matvar_ptr, const std::string & )
  {
    return (RetType) ((const MATType *) matvar_ptr->data)[0];
  }
  
  template<typename MATType,
           TOMOGRAPHER_ENABLED_IF_TMPL(Tools::is_complex<RetType>::value &&
                                       !Tools::is_complex<MATType>::value)>
  static inline RetType get_value(const matvar_t * matvar_ptr, const std::string & )
  {
    return RetType( ((const MATType *) matvar_ptr->data)[0],
                    0 );
  }
  
  template<typename MATType,
           TOMOGRAPHER_ENABLED_IF_TMPL(!Tools::is_complex<RetType>::value &&
                                       Tools::is_complex<MATType>::value)>
  static inline RetType get_value(const matvar_t * /*matvar_ptr*/, const std::string & varname)
  {
    throw VarTypeError(varname, "Expected real scalar, got complex type");
  }
  
  template<typename MATType,
           TOMOGRAPHER_ENABLED_IF_TMPL(Tools::is_complex<RetType>::value &&
                                       Tools::is_complex<MATType>::value)>
  static inline RetType get_value(const matvar_t * matvar_ptr, const std::string & )
  {
    typedef typename Tools::complex_real_scalar<MATType>::type MATRealType;
    const mat_complex_split_t * cdata = (mat_complex_split_t*) matvar_ptr->data;

    return RetType( ((const MATRealType *) cdata->Re)[0],
                    ((const MATRealType *) cdata->Im)[0] );
  }

};



// -----------------------------------------------


namespace tomo_internal {

//! Interface to directly access mat data with multidimensional index
template<typename OutType, typename MatInnerT>
class VarMatDataAccessor
{
  const Var & p_var;

  typedef typename Tools::complex_real_scalar<MatInnerT>::type MatRealInnerT;

  const MatInnerT * p_r_ptr;
  const MatRealInnerT * p_cre_ptr;
  const MatRealInnerT * p_cim_ptr;
  
public:
  VarMatDataAccessor(const Var & var)
    : p_var(var)
  {
    const matvar_t * matvar_ptr = var.getMatvarPtr();
    assert(matvar_ptr->data != NULL);
    if (!matvar_ptr->isComplex) {
      // real type
      p_r_ptr = (const MatInnerT*) matvar_ptr->data;
      p_cre_ptr = NULL;
      p_cim_ptr = NULL;
    } else {
      const mat_complex_split_t * cdata = (mat_complex_split_t*) matvar_ptr->data;
      p_r_ptr = NULL;
      p_cre_ptr = (const MatRealInnerT*) cdata->Re;
      p_cim_ptr = (const MatRealInnerT*) cdata->Im;
      assert(p_cre_ptr != NULL);
      assert(p_cim_ptr != NULL);
    }
  }

  virtual ~VarMatDataAccessor() { }

  template<typename IndexListType,
           TOMOGRAPHER_ENABLED_IF_TMPL(!Tools::is_complex<OutType>::value &&
                                       !Tools::is_complex<MatInnerT>::value)>
  inline OutType value(IndexListType&& index) const
  {
    assert(p_r_ptr != NULL);

    // real value.
    std::size_t lin = linear_index(std::forward<IndexListType>(index));
    return p_r_ptr[lin];
  }
  
  template<typename IndexListType,
           TOMOGRAPHER_ENABLED_IF_TMPL(!Tools::is_complex<OutType>::value &&
                                       Tools::is_complex<MatInnerT>::value)>
  inline OutType value(IndexListType&& ) const
  {
    throw VarTypeError(p_var.varName(), "Expected real type, got complex");
  }

  template<typename IndexListType,
           TOMOGRAPHER_ENABLED_IF_TMPL(Tools::is_complex<OutType>::value &&
                                       !Tools::is_complex<MatInnerT>::value)>
  inline OutType value(IndexListType&& index) const
  {
    assert(p_r_ptr != NULL);

    // real value.
    std::size_t lin = linear_index(std::forward<IndexListType>(index));
    return OutType( p_r_ptr[lin] , 0 );
  }

  template<typename IndexListType,
           TOMOGRAPHER_ENABLED_IF_TMPL(Tools::is_complex<OutType>::value &&
                                       Tools::is_complex<MatInnerT>::value)>
  inline OutType value(IndexListType&& index) const
  {
    assert(p_cre_ptr != NULL);
    assert(p_cim_ptr != NULL);

    // complex value
    std::size_t lin = linear_index(std::forward<IndexListType>(index));
    return OutType(p_cre_ptr[lin], p_cim_ptr[lin]);
  }

private:
  template<typename IndexListType>
  std::size_t linear_index(IndexListType && index) const
  {
    IndexList<false> ind_cmaj{p_var.dims(), std::forward<IndexListType>(index.index())};
    return ind_cmaj.linearIndex();
  }
};

} // namespace tomo_internal


// -----------------------------------------------

/** \brief Describe shape and type of variable
 *
 * Allows to check a matlab variable for correct shape, for example.
 *
 */
struct VarShape
{
  const bool is_complex;
  const DimList dims;
  const bool is_square;

  template<typename DimListType>
  VarShape(bool is_complex_, DimListType&& dims_, bool is_square_)
    : is_complex(is_complex_), dims(std::forward(dims_)), is_square(is_square_)
  {
    _check_consistency();
  }

  template<class T, typename DimListType>
  VarShape(DimListType&& dims_, bool is_square_)
    : is_complex(Tools::is_complex<T>::value), dims(std::forward(dims_)), is_square(is_square_)
  {
    _check_consistency();
  }
  
  VarShape(const Var & var)
    : is_complex(var.isComplex()), dims(var.dims()), is_square(var.isSquareMatrix())
  {
  }


private:
  void _check_consistency() {
    if (is_square) {
      assert(dims.size() == 0 || (dims.size() == 2 && (dims[0] == -1 || dims[1] == -1 || dims[0] == dims[1])));
    }
  }
  
public:
  
  /** \brief Verify the shape of the MATLAB variable.
   *
   * If the MATLAB variable \a var does not have the shape described by this VarShape
   * object, or if the variable is complex and \a T is not, then throw a VarTypeError.
   */
  inline void checkShape(const Var& var);
};

inline std::ostream& operator<<(std::ostream& str, const VarShape & varshape)
{
  str << ((varshape.is_complex) ? std::string("complex ") : std::string("real "));
  if (varshape.dims.size() == 1) {
    str << "vector";
  } else if (varshape.dims.size() == 2) {
    if (varshape.is_square) {
      str << "square ";
    }
    str << "matrix";
  } else if (varshape.dims.size() > 0) {
    str << varshape.dims.size() << "-D array";
  } else {
    str << "array";
  }
  if (varshape.dims.size()) {
    str << " of shape " << varshape.dims;
  }
  return str;
}

inline void VarShape::checkShape(const Var& var)
{
  const matvar_t * mvar = var.getMatvarPtr();
  const DimList mvardims = var.dims();
  
  if ((mvar->isComplex && !is_complex) ||
      ((std::size_t)mvar->rank != dims.size() && dims.size() > 0) ||
      (is_square && (mvar->rank != 2 || mvar->dims[0] != mvar->dims[1])) ||
      (dims.size() > 0 && !mvardims.matchesWanted(dims)))  {
    
    std::stringstream errstr;

    errstr << *this;
    errstr << ", got ";
    errstr << VarShape(var);

    //fprintf(stderr, "Bad var type for variable %s\n", var.varName().c_str());
    throw VarTypeError(var.varName(), errstr.str());

  }
}



// -----------------------------------------------------------------------------

// get an std::vector<T> for a variable.

/** \brief Ask for this type in \ref Var::value() to get an std::vector<T>
 *
 */
template<typename T_, bool RowMajor_ = false>
struct GetStdVector {
  typedef T_ type;
  static constexpr bool RowMajor = RowMajor_;
};

template<typename T, bool RowMajor>
struct VarValueDecoder<GetStdVector<T, RowMajor> >
{
  typedef std::vector<T> RetType;

  static inline void checkShape(const Var & var)
  {
    if (var.isComplex() && !Tools::is_complex<T>::value) {
      throw VarTypeError(var.varName(),
                         std::string("can't store complex matrix in type ")
                         + typeid(T).name());
    }
  }
                             
  static inline std::vector<T> decodeValue(const Var & var)
  {
    const matvar_t * matvar_ptr = var.getMatvarPtr();
    std::size_t numel = var.numel();
    std::vector<T> val(numel);

    MAT_SWITCH_TYPE(matvar_ptr,
                    tomo_internal::VarMatDataAccessor<T, Type> acc(var);
                    
                    for(IndexListIterator<RowMajor> il(var.dims()); il.valid(); ++il) {
                      val[il.linearIndex()] = acc.value(il);
                    }
                    
                    return val;
        );
    // should never reach here
    assert(false);
  }

};







// =============================================================================

// get Eigen types.

namespace tomo_internal {


template<typename MatrixType, typename MatRealType,
         TOMOGRAPHER_ENABLED_IF_TMPL(Eigen::NumTraits<typename MatrixType::Scalar>::IsComplex)>
void init_eigen_matrix(MatrixType & matrix, const DimList & vdims,
                       const matvar_t * matvar_ptr, const std::ptrdiff_t data_offset = 0)
{
  typedef typename MatrixType::Scalar Scalar;
  typedef typename Eigen::NumTraits<Scalar>::Real RealScalar;
  
  const mat_complex_split_t * cdata = (mat_complex_split_t*) matvar_ptr->data;
  
  assert(vdims.size() == 2);
  assert(vdims[0] != -1 && vdims[1] != -1);
  
  matrix = (
      Eigen::Map<Eigen::Matrix<MatRealType,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor> >(
          (const MatRealType *) cdata->Re + data_offset, vdims[0], vdims[1]
          ).template cast<std::complex<RealScalar> >()
      + std::complex<RealScalar>(0,1) *
      Eigen::Map<Eigen::Matrix<MatRealType,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor> >(
          (const MatRealType *) cdata->Im + data_offset, vdims[0], vdims[1]
          ).template cast<std::complex<RealScalar> >()
      );
}

template<typename MatrixType, typename MatRealType,
         TOMOGRAPHER_ENABLED_IF_TMPL(!Eigen::NumTraits<typename MatrixType::Scalar>::IsComplex)>
void init_eigen_matrix(MatrixType & matrix, const DimList & vdims,
                       const matvar_t * matvar_ptr, const std::ptrdiff_t data_offset = 0)
{
  typedef typename MatrixType::Scalar Scalar;
  
  assert(vdims.size() == 2);
  assert(vdims[0] != -1 && vdims[1] != -1);
  
  matrix = (
      Eigen::Map<Eigen::Matrix<MatRealType,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor> >(
          (const MatRealType *) matvar_ptr->data + data_offset, vdims[0], vdims[1]
          )
      ).template cast<Scalar>();
}

} // namespace tomo_internal



template<typename Scalar, int Rows, int Cols, int Options, int MaxRows, int MaxCols>
struct VarValueDecoder<Eigen::Matrix<Scalar,Rows,Cols,Options,MaxRows,MaxCols> >
{
  typedef Eigen::Matrix<Scalar,Rows,Cols,Options,MaxRows,MaxCols> MatrixType;
  typedef typename Eigen::NumTraits<Scalar>::Real RealScalar;
  typedef MatrixType RetType;

  static inline void checkShape(const Var & var)
  {
    DimList matdims;
    matdims << (Rows!=Eigen::Dynamic ? Rows : -1)
            << (Cols!=Eigen::Dynamic ? Cols : -1);

    VarShape shape(Eigen::NumTraits<Scalar>::IsComplex,
                   matdims,
                   matdims.size() == 2 && matdims[0] == matdims[1]);

    shape.checkShape(var);
  }
                             
  static inline RetType decodeValue(const Var & var)
  {
    matvar_t * matvar_ptr = var.getMatvarPtr();
    DimList vdims{var.dims()};

    assert(vdims.size() >= 2);
    const int rows = vdims[0];
    // rest of dimensions.
    int cols = get_numel(vdims.data()+1, vdims.data()+vdims.size());

    constexpr int RowMajorOrColMajor = ( ((Options & Eigen::RowMajorBit) == Eigen::RowMajor)
                                         ? Eigen::RowMajor
                                         : Eigen::ColMajor );
    
    MatrixType matrix(rows, cols);

    if (var.isComplex()) {
      const mat_complex_split_t * cdata = (mat_complex_split_t*) matvar_ptr->data;
      MAT_SWITCH_COMPLEX_TYPE(
          matvar_ptr->data_type,
          tomo_internal::init_eigen_matrix<MatrixType, std::complex<Type> >(
              *matrix, vdims, matvar_ptr
              );
          );
    } else {
      MAT_SWITCH_REAL_TYPE(
          matvar_ptr->data_type,
          tomo_internal::init_eigen_matrix<MatrixType,Type>(
              *matrix, vdims, matvar_ptr
              );
          );
    }
     
    return matrix;
  }
  
};





template<typename Scalar, int Rows, int Cols, int Options, int MaxRows, int MaxCols, typename Alloc>
struct VarValueDecoder<std::vector<Eigen::Matrix<Scalar,Rows,Cols,Options,MaxRows,MaxCols>, Alloc> >
{
  typedef Eigen::Matrix<Scalar,Rows,Cols,Options,MaxRows,MaxCols> MatrixType;
  typedef typename Eigen::NumTraits<Scalar>::Real RealScalar;

  typedef std::vector<MatrixType,Alloc> RetType;

  static inline void checkShape(const Var & var)
  {
    DimList matdims;
    matdims << (Rows!=Eigen::Dynamic ? Rows : -1)
            << (Cols!=Eigen::Dynamic ? Cols : -1)
            << -1;

    VarShape shape(Eigen::NumTraits<Scalar>::IsComplex,
                   matdims,
                   false);

    shape.checkShape(var);
  }
                             
  static inline RetType decodeValue(const Var & var)
  {
    DimList vardims{var.dims()};

    assert(vardims.size() >= 1);

    constexpr int numinnerdims = int(Rows != 1) + int(Cols != 1);

    DimList innerdims;
    int k;
    for (k = 0; k < vardims.size(); ++k) {
      if (vardims[k] != 1) {
        innerdims << vardims[k];
      }
      if (innerdims.size() == numinnerdims) {
        break;
      }
    }
    DimList inner_rowcols;
    inner_rowcols << (Rows == 1  ? 1 : innerdims[0]);
    inner_rowcols << (Cols == 1
                      ? 1 : (Rows == 1
                             ? innerdims[0]
                             : innerdims[1]));

    std::size_t outerdim = get_numel(vardims.data() + k, vardims.data() + vardims.size());

    RetType value(outerdim);
    std::size_t j;

    matvar_t * matvar_ptr = var.getMatvarPtr();

    if (var.isComplex()) {
      const mat_complex_split_t * cdata = (mat_complex_split_t*) matvar_ptr->data;
      MAT_SWITCH_COMPLEX_TYPE(
          matvar_ptr->data_type,
          typedef Type MATType;
          for (j = 0; j < outerdim; ++j) {
            tomo_internal::init_eigen_matrix<MatrixType,MATType>(value[j], inner_rowcols, matvar_ptr);
          }
          );
    } else {
      MAT_SWITCH_REAL_TYPE(
          matvar_ptr->data_type,
          typedef Type MatType;
          for (j = 0; j < outerdim; ++j) {
            tomo_internal::init_eigen_matrix<MatrixType,MatType>(value[j], inner_rowcols, matvar_ptr);
          }
          );
    }

    return value;
  }
  
};
  

} // namespace MAT
} // namespace Tomographer




#endif
