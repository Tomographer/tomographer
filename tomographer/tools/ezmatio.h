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
#include <initializer_list>
#include <utility>

extern "C" {
#  include <matio.h>
}

#include <Eigen/Core>

#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/eigenutil.h>
#include <tomographer/tools/fmt.h>
#include <tomographer/mathtools/pos_semidef_util.h>


/** \file ezmatio.h
 *
 * \brief Utilities for reading MATLAB \c "*.mat" data files. See \ref Tomographer::MAT.
 */



namespace Tomographer {
// namespace doc in doc/doxdocs/namespaces.cxx
namespace MAT {


/** \brief Base Exception class for errors within our MAT routines
 */
TOMOGRAPHER_EXPORT class Exception : public std::exception
{
  std::string p_heading;
  std::string p_message;
  std::string p_final;
public:
  Exception(std::string msg) : p_message(msg) { update_final(); }
  Exception(std::string heading, std::string msg) : p_heading(heading), p_message(msg) { update_final(); }
  virtual ~Exception() noexcept { }
  
  virtual const char * what() const noexcept { return p_final.c_str(); }

protected:
  inline void setHeading(const std::string heading) {
    p_heading = heading;
    update_final();
  }
  inline void setMessage(const std::string message) {
    p_message = message;
    update_final();
  }
private:
  inline void update_final() {
    p_final = p_heading + p_message;
  }
};

/** \brief Exception relating to a MATLAB variable in the data file 
 */
TOMOGRAPHER_EXPORT class VarError : public Exception
{
public:
  VarError(std::string msg) : Exception("", msg) { }
  VarError(std::string varname, std::string msg) : Exception(heading(varname), msg) { }
  virtual ~VarError() noexcept { }
  
  void setVarName(std::string varname) { setHeading(heading(varname)); }

private:
  static std::string heading(std::string varname) { return "Variable " + varname + ": "; }
};

//! Error while reading a variable from the MATLAB data file
TOMOGRAPHER_EXPORT class VarReadError : public VarError
{
public:
  VarReadError(const std::string varname)
    : VarError(varname, "Can't read variable")
  {
  }
  virtual ~VarReadError() noexcept { }
};

//! Type mismatch (wrong type requested) in a variable read from the MATLAB data file
TOMOGRAPHER_EXPORT class VarTypeError : public VarError
{
public:
  VarTypeError(const std::string varname, const std::string msg)
    : VarError(varname, msg)
  {
  }
  virtual ~VarTypeError() noexcept { }
};

//! Unknown type of a variable present in the data file
TOMOGRAPHER_EXPORT class VarMatTypeError : public VarError {
public:
  VarMatTypeError(const std::string msg)
    : VarError(msg)
  {
  }
  VarMatTypeError(const std::string varname, const std::string msg)
    : VarError(varname, msg)
  {
  }
  virtual ~VarMatTypeError() noexcept { }
};

//! Error while opening a MATLAB file
TOMOGRAPHER_EXPORT class FileOpenError : public Exception
{
public:
  FileOpenError(const std::string fname, const std::string errmsg = std::string())
    : Exception(heading(fname), "Error opening file" + (errmsg.size() ? ": "+errmsg : ""))
  {
  }
  virtual ~FileOpenError() noexcept { }

  void setFileName(const std::string fname) {
    setHeading(heading(fname));
  }

private:
  static std::string heading(std::string fname) { return "File `"+fname+"`: "; }
};


//! Invalid index or index list provided to a routine
TOMOGRAPHER_EXPORT class InvalidIndexError : public Exception {
public:
  InvalidIndexError(const std::string msg) : Exception("Invalid index: ", msg) { }
  virtual ~InvalidIndexError() noexcept { }
};





class Var;


/** \brief A MATLAB file open for reading data.
 *
 * Access variables with \ref var(), and get a list of variables with \ref
 * getVarInfoList().
 *
 * This class is in fact a wrapper for MatIO C function calls. This class owns a \a mat_t
 * pointer to an open file, which is open in the constructor and closed in the destructor.
 *
 * \note This C++ class is not copyable. You may pass it by value everywhere though as if
 *       it were copyable, because of C++11 move semantics.
 */
TOMOGRAPHER_EXPORT class File
{
public:
  /** \brief Open a data file for reading.
   *
   * The file will be immediately open for reading. You may directly call the \ref var(),
   * \ref getVarInfoList() etc. methods after constructing a File object.
   */
  File(const std::string fname)
  {
    errno = 0;
    p_matfp = Mat_Open(fname.c_str(), MAT_ACC_RDONLY);
    if ( p_matfp == NULL ) {
      throw FileOpenError(fname, strerror(errno));
    }
  }
  File(const File& other) = delete;

  /** \brief Move constructor.
   *
   * This object has C++11 move semantics, meaning that you can pass it around by
   * value. Read more on C++11 move semantics for more information.
   */
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

  /** \brief Find a variable by name.
   *
   * This returns a \ref Var object with the information corresponding to the given
   * variable.
   *
   * If \a load_data is \c true (the default), then the returned Var object also contains
   * the variable data. Otherwise, it just contains the information about the type,
   * dimensions, etc.
   */
  inline Var var(const std::string varname, bool load_data = true);


  /** \brief Get a list of all variables in this data file.
   *
   * The returned Var objects don't have data loaded. You must re-open them with \ref
   * var() to get the data.
   */
  inline std::vector<Var> getVarInfoList();

  /** \brief Direct access to the underlying C pointer used for the MatIO library.
   *
   * Please be careful with this. The pointer is owned by this object, and will be free'd
   * at the end of this object's lifetime.
   */
  mat_t * getMatPtr()
  {
    return p_matfp;
  }

  /** \brief Move assignment operator.
   *
   * This object has C++11 move semantics, meaning that you can pass it around by
   * value. Read more on C++11 move semantics for more information.
   */
  File& operator=(File&& other)
  {
    // move assignment operator. Steal the other object's MAT file handle
    p_matfp = other.p_matfp;
    other.p_matfp = NULL;
    return *this;
  }
  File& operator=(const File& other) = delete;
  
private:
  mat_t *p_matfp;
};


/** \brief Calculate the product of all dimensions
 *
 * Given two C++/STL-type iterators \a begin and \a end, calculate the product of the
 * sequence of elements. For an empty sequence (<em>begin==end</em>), the result is \c 1.
 */
template<typename It, typename ValueType = typename std::iterator_traits<It>::value_type>
inline ValueType getNumEl(It begin, It end)
{
  ValueType n = 1;
  for (It it = begin; it != end; ++it) {
    n *= (*it);
  }
  return n;
}


/** \brief An array of ints which specifies a list of dimensions.
 *
 * This utility may be used to specify a list of dimensions for a N-D array.  It
 * is a subclass of \ref std::vector<int>, and implements some additional
 * functions such as \ref numel(), \ref matchesWanted() and \ref
 * operator<<(std::ostream&,const DimList&) "operator<<".
 *
 * You may also use \ref DimList::operator<<(int dim) and
 * \ref DimList::operator<<(const std::vector<int>& moredims) to append dimensions
 * to the list.
 *
 *
 */
TOMOGRAPHER_EXPORT class DimList : public std::vector<int>
{
public:
  //! Initializes an empty dimension list
  DimList() : std::vector<int>() { }
  
  /** \brief Initialize a dimension list with another list of dimensions
   *
   * The \a dims may be another vector, or another DimList instance.  This also works with
   * move semantics.
   */
  template<typename VectorType>
  DimList(VectorType&& dims)
    : std::vector<int>(std::forward<VectorType>(dims))
  {
  }
  /** \brief Initialize the dimension list with an explicit initializer
   *
   * Enables construction and initialization with the syntax
   * \code
   *   DimList dims{2, 3, 6}; // 3 dimensions initialized to [2, 3, 6]
   *   DimList dims{{2, 3, 6}}; // 3 dimensions initialized to [2, 3, 6], alternative syntax
   * \endcode
   *
   * \todo Note to self: both syntaxes work, but I'm not sure which constructor exactly
   *       the second line calls ...?
   */
  template<typename T, TOMOGRAPHER_ENABLED_IF_TMPL(std::is_convertible<T,int>::value)>
  DimList(std::initializer_list<T> init)
    : std::vector<int>(init)
  {
  }
  /** \brief Initialize the dimension list with an iterator range
   *
   * The dimension list is initialized with begin and end iterators \a b and \a e.  See
   * \ref std::vector(It b, It e).
   */
  template<class It>
  DimList(It b, It e) : std::vector<int>(b, e)
  {
  }

  /** \brief Get the total number of elements in an array of these dimensions
   *
   * This is simply the product of all elements in the array. See \ref getNumEl().
   */
  inline int numel() const { return getNumEl(begin(), end()); }

  /** \brief Get the number of dimensions in this array.
   *
   * This is simply an alias for \ref std::vector<int>::size()
   *
   */
  inline int ndims() const { return (int)size(); }

  /** \brief Test whether our dimensions match the given required list of dimensions
   *
   */
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

  /** \brief Add (append) a dimension to the dimension list
   *
   * Pushes the dimension \a dim at the end of the dimension list, increasing the number
   * of dimensions by one.
   */
  DimList& operator<<(int dim) {
    push_back(dim);
    return *this;
  }
  /** \brief Add (append) dimensions to the dimension list
   *
   * Pushes the dimension list \a moredims at the end of the current dimension list,
   * increasing the number of dimensions by <em>moredims.size()</em>.
   */
  DimList& operator<<(const std::vector<int>& moredims) {
    insert(end(), moredims.begin(), moredims.end());
    return *this;
  }
};

//! C++ output stream operators for a \ref DimList
inline std::ostream& operator<<(std::ostream& out, const DimList& dlist)
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
 * This subclass of \ref std::vector<int> stores indices into a multi-dimensional array,
 * and adds utilities to access the data with a linear index (i.e. linear memory access
 * model) in either column-major or row-major format.
 *
 * If \a RowMajor is \c true, then the linear indices in this class are given and
 * calculated in row-major format; if \c false, in column-major format. See <a
 * href="http://eigen.tuxfamily.org/dox/group__TopicStorageOrders.html">Eigen's page on
 * storage orders</a>.
 *
 * \note that this class also stores the underlying dimensions of the tensor, which are
 *       needed to calculate the linear indices. Also, note that the multidimensional
 *       index list is stored, not the linear index. So repeated calls to \ref
 *       setLinearIndex() and \ref linearIndex() will do redundant calculations.
 *
 * \note Use \ref IndexListIterator if you want to iterate linearly through memory layout
 *       with both linear and indices-based access.
 *
 * \note There is no way to change the underlying dimensions once the object is
 *       constructed.
 */
template<bool IsRowMajor_ = false>
TOMOGRAPHER_EXPORT class IndexList : public std::vector<int>
{
public:
  //! Is this class calculating and expecting row-major (\c true) or column-major (\c false) format 
  static constexpr bool IsRowMajor = IsRowMajor_;
  //! Base vector type (superclass)
  typedef std::vector<int> VectorType;
  
  /** \brief Constructor with linear index
   *
   * \note if no linear index is specified (or a negative value is given), then the vector
   * is uninitialized.
   */
  IndexList(const std::vector<int>& dims = std::vector<int>(), int linearindex = -1)
    : std::vector<int>(dims.size()), p_dims(dims)
  {
    if (getNumEl(dims.begin(), dims.end()) <= 0) {
      throw InvalidIndexError("zero-sized array given by dimension list");
    }
    if (linearindex >= 0) {
      setLinearIndex(linearindex);
    }
  }
  /** \brief Constructor with multidimensional index specification
   *
   * The second argument is any (single-argument) initializer which can be used as
   * constructor in a \ref std::vector<int>.
   *
   */
  // the reason we dont have template<typename... VectorArgs> for std::vector<>
  // constructor is that g++ 4.6 if I remember correctly can't unpack arguments into a
  // fixed number of arguments (TODO: CHECK THIS!)
  template<typename VectorIntInitializer>
  IndexList(const std::vector<int>& dims, VectorIntInitializer&& index)
    : std::vector<int>(std::forward<VectorIntInitializer>(index)), p_dims(dims)
  {
    if (getNumEl(dims.begin(), dims.end()) <= 0) {
      throw InvalidIndexError("zero-sized array given by dimension list");
    }
  }

  /** \brief Set the linear index.
   *
   * This will set the underlying index list to the multi-dimensional index corresponding
   * to this linear index.
   *
   * (This is the row-major implementation)
   */
  TOMOGRAPHER_ENABLED_IF(IsRowMajor)
  void setLinearIndex(int linearindex)
  {
    const int ndims = (int)p_dims.size();
    for (int k = ndims-1; k >= 0; --k) {
      this->at(k) = linearindex % p_dims[k];
      linearindex /= p_dims[k]; // integer division
    }
  }
  /** \brief Set the linear index.
   *
   * This will set the underlying index list to the multi-dimensional index corresponding
   * to this linear index.
   *
   * (This is the column-major implementation)
   */
  TOMOGRAPHER_ENABLED_IF(!IsRowMajor)
  void setLinearIndex(int linearindex)
  {
    const int ndims = (int)p_dims.size();
    for (int k = 0; k < ndims; ++k) {
      this->at(k) = linearindex % p_dims[k];
      linearindex /= p_dims[k]; // integer division
      // std::cout << "k = " << k << "; p_dims = " << p_dims << "; at(k) = " << this->at(k)
      //           << "; linearindex=" << linearindex << "\n";
    }
  }

  /** \brief Linear index corresponding to the stored multidimensional indices.
   *
   * (This is the row-major implementation.)
   */
  TOMOGRAPHER_ENABLED_IF(IsRowMajor)
  int linearIndex() const
  {
    int linindex = 0;
    for (int k = 0; k < (int)p_dims.size(); ++k) {
      linindex *= p_dims[k];
      linindex += this->at(k);
    }
    return linindex;
  }
  /** \brief Linear index corresponding to the stored multidimensional indices.
   *
   * (This is the column-major implementation.)
   */
  TOMOGRAPHER_ENABLED_IF(!IsRowMajor)
  int linearIndex() const
  {
    int linindex = 0;
    for (int k = (int)p_dims.size()-1; k >= 0; --k) {
      linindex *= p_dims[k];
      linindex += this->at(k);
    }
    return linindex;
  }

  /** \brief Return a reference to \c *this.
   */
  inline const std::vector<int> & index() const
  {
    return *this;
  }

  /** \brief Get the underlying dimensions given to the constructor.
   *
   * There is no way to change the underlying dims.
   */
  inline const std::vector<int> & dims() const
  {
    return p_dims;
  }

  /** \brief Append index to list.
   *
   * Utility to construct an index list as
   * \code
   *   IndexList il; il << 1 << 2 << 5;
   * \endcode
   */
  IndexList& operator<<(int ind) {
    push_back(ind);
    return *this;
  }

  /** \brief Append a list of indices to list.
   *
   * Utility to construct an index list as
   * \code
   *   IndexList more = ...; // or std::vector<int> more = ...
   *   IndexList il; il << 1 << 2 << more;
   * \endcode
   */
  IndexList& operator<<(const std::vector<int>& moredims) {
    insert(end(), moredims.begin(), moredims.end());
    return *this;
  }



  // for internal use:

  template<bool IsRowMajor2 = false>
  static const std::vector<int> &  forward_index(const IndexList<IsRowMajor2> & index)
  {
    return index;
  }
  template<bool IsRowMajor2 = false>
  static std::vector<int> &&  forward_index(IndexList<IsRowMajor2> && index)
  {
    return std::move(index);
  }

private:
  const std::vector<int> p_dims;
};


//! C++ output stream operator for \ref IndexList
template<bool IsRowMajor>
inline std::ostream& operator<<(std::ostream& str, const IndexList<IsRowMajor> & indexlist)
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
template<bool IsRowMajor_ = false, typename IntType_ = int>
TOMOGRAPHER_EXPORT class IndexListIterator
{
public:
  static constexpr bool IsRowMajor = IsRowMajor_;
  typedef IntType_ IntType;
  typedef std::vector<IntType> VectorType;

private:
  const std::vector<IntType> p_dims;
  const IntType p_numel;

  std::vector<IntType> p_index;
  IntType p_linearIndex;

public:
  IndexListIterator(const std::vector<IntType>& dims)
    : p_dims(dims),
      p_numel(getNumEl(dims.begin(), dims.end())),
      p_index(dims.size(), 0),
      p_linearIndex(0)
  {
    if (p_numel <= 0) {
      std::ostringstream ss;
      ss << "Invalid dimensions: " << p_dims;
      throw InvalidIndexError(ss.str());
    }
  }

  inline const std::vector<IntType>& index() const
  {
    return p_index;
  }

  inline IntType linearIndex() const
  {
    return p_linearIndex;
  }
  
  inline IntType numel() const
  {
    return p_numel;
  }


  TOMOGRAPHER_ENABLED_IF(IsRowMajor)
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

  TOMOGRAPHER_ENABLED_IF(!IsRowMajor)
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
    return Tools::isPositive<IntType>(p_linearIndex) && p_linearIndex < p_numel;
  }


  // for internal use:

  template<bool IsRowMajor2 = false, typename IntType2 = int>
  static const std::vector<IntType2> &
  forward_index(const IndexListIterator<IsRowMajor2,IntType2> & index)
  {
    return index.p_index;
  }
  template<bool IsRowMajor2 = false, typename IntType2 = int>
  static std::vector<IntType2> &&
  forward_index(IndexListIterator<IsRowMajor2,IntType2> && index)
  {
    return std::move(index.p_index);
  }

};


//! C++ output stream operator for \ref IndexListIterator
template<bool IsRowMajor, typename IntType>
inline std::ostream& operator<<(std::ostream& str, const IndexListIterator<IsRowMajor, IntType> & indexlistit)
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
 * This class does two things:
 *
 * - it ensures that the variable in the data file has a type and shape which is
 *   compatible with the C++ data type requested (\a T)
 *
 * - it decodes the %MAT Matlab data via the C MatIO interface (see the \a matvar_t
 *   structure) to transform it into the requested native C++ object
 *
 * It may be that the decoding routine might not want to have directly \a T as a return
 * type, but rather return a type which is convertible to \a T or which can be used as an
 * initializer for a \a T. (Think, for example, an Eigen mapped type.)
 *
 * Another reason for not returning \a T directly is if \a T is a proxy type which
 * describes which type is requested along with some parameters (e.g. storage order). See
 * for example \ref GetStdVector.
 *
 * This default class template does nothing. Specialize it for your C++ type or use one of
 * the available specializations.
 *
 * The \a Enabled template parameter is provided for convenience, if you wish to employ
 * SFINAE to conditionally enable specializations. See the VarValueDecoder<T> for simple
 * numeric values for an example.  The idea is to use a construct
 * \code
 * template<...>
 * struct VarValueDecoder<..., typename std::enable_if<condition>::type >
 * \endcode
 * where the second argument (\c Enabled) is an invalid C++ expression (SFINAE) if the
 * required condition is not met, and resolves to \c void if the condition is met.  Don't
 * forget that because of the initial template declaration, when invoking
 * <code>VarValueDecoder<T></code>, the second paramter is always defaulted to \c void
 * regardless of the specialization.
 *
 * In addition, this class may have an additional typedef (or nested struct/class) member
 * named \a Params (no example available yet).  In that case, it is possible to pass a
 * second argument to the \ref Var::value() method (same for \ref value()), precisely of
 * the type \a Params.  This argument is passed as second argument to both \ref
 * checkShape() and \ref decodeValue(); these methods may use this additional argument
 * however they like.  For example, the \a Params may be flags or options specifying how
 * to encode the value into the type \a T, or may be the requested shape of the return
 * type, or whatever other additional info the decoding procedure may like to have.
 */
template<typename T, typename Enabled = void>
TOMOGRAPHER_EXPORT class VarValueDecoder
{
public:
  /** \brief Type returned by \ref decodeValue()
   *
   * The specialization of VarValueDecoder<T> should explicitly specify which type is
   * returned by the main decoding routine (see class doc).
   */
  typedef T RetType;

  /** \brief Check that the type and shape of the \a var are compatible with \a T
   *
   * Specializations of VarValueDecoder<T> should implement this function and check
   * whether the given variable can be decoded into the requested C++ type \a T. If there
   * is any error, throw a \ref VarTypeError exception.
   */
  static inline void checkShape(const Var & var)
  {
    (void)var; // silence "unused parameter" warnings
  }

  /** \brief Decode the variable \a var into the C++ type \a T
   *
   * Specializations of VarValueDecoder<T> should implement this function to read data in
   * the MATLAB variable \a var and return an instance of the requested C++ type \a T.
   *
   * Note the return type of this function need not be \a T itself, but any type you like,
   * presumably one that is convertible to \a T, assignable to \a T, or which can be used
   * as initializer for \a T. You may find this useful if you want to return an
   * initializer type such as, e.g., an \a Eigen generator expression.
   *
   * Do note, though, that with modern compilers performing return value optimization
   * (RVO), in most cases there shouldn't be any real overhead in returning a \a T
   * directly.
   */
  static inline RetType decodeValue(const Var & var);
};


namespace tomo_internal {

  template<typename T, typename Res = void>
  struct enable_if_is_type { typedef T check_type; typedef Res type; };

  // has_params_member<T>::value is true if T has a type member named Params,
  // i.e. typename T::Params, else it is false.
  template<typename T, typename Enabled = void>
  struct has_params_member {
    enum { value = 0 };
  };
  // if T has no Params member, following will fail by SFINAE (the default arguemnt of
  // parameter #2 always stays int)
  template<typename T>
  struct has_params_member<T, typename enable_if_is_type<typename T::Params>::type >
  {
    enum { value = 1 };
  };

template<typename Scalar>
struct isComplex { // use Eigen's existing implementation
  enum { value = Eigen::NumTraits<Scalar>::IsComplex };
};


}; // namespace tomo_internal



class Var;

/** \brief Access the value of the given variable, as a C++ type
 *
 * This is an alternative syntax for <em>var.value<T>()</em>.  See documentation for \ref
 * Var::value().
 */
template<typename T>
inline typename VarValueDecoder<T>::RetType value(const Var& var)
{
  VarValueDecoder<T>::checkShape(var);
  return VarValueDecoder<T>::decodeValue(var);
}

/** \brief Access the value of the given variable, as a C++ type
 *
 * This is an alternative syntax for <em>var.value<T>()</em>.  See documentation for \ref
 * Var::value().
 *
 * This overload allows to decode the value of the variable with some parameters
 * specifying what exactly is requested.  See documentation for \ref VarValueDecoder.
 */
template<typename T, TOMOGRAPHER_ENABLED_IF_TMPL( tomo_internal::has_params_member<VarValueDecoder<T> >::value )>
inline typename VarValueDecoder<T>::RetType value(const Var& var,
                                                  const typename VarValueDecoder<T>::Params & params)
{
  VarValueDecoder<T>::checkShape(var, params);
  return VarValueDecoder<T>::decodeValue(var, params);
}



/** \brief A MATLAB variable in the MAT file.
 *
 * This object is a wrapper around MatIO's API for reading data of a variable in the data
 * file.
 *
 * This object can be moved around using C++11 move semantics. Also, this object is
 * copyable. (But don't modify the object, as the data is shared.)
 *
 * Var objects may, or may not, have the actual data loaded (\ref hasData()). If no data
 * is loaded, information about the variable (type, shape, etc.) may be accessed. There is
 * currently no way to load the data subsequently for the same Var object, just
 * re-construct a new Var object.
 *
 * To read the data, you should use the template method \ref value(). The type you can
 * request is any type for which a corresponding \ref VarValueDecoder has been defined.
 */
TOMOGRAPHER_EXPORT class Var
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
  /** \brief Read variable from MATLAB data file.
   *
   * This constructor initializes this variable from the given open MATLAB data file \a
   * matf by looking up the variable named \a varname.
   *
   * If \a load_data is \c true, then the data is also loaded. Otherwise, only the type
   * and shape information is loaded. See \ref hasData().
   *
   * Calling this constructor directly is the same as calling \ref File::var().
   */
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

  //! Var objects are copyable. Beware though that the data is shared.
  Var(const Var& copy)
  {
    p_vardata = copy.p_vardata;
    p_vardata->refcount++;
  }

  //! Var implements C++11 move semantics.
  Var(Var&& other)
  {
    // steal the data
    p_vardata = other.p_vardata;
    other.p_vardata = NULL;
  }

  ~Var() {
    if (p_vardata != NULL) {
      p_vardata->refcount--;
      if (!p_vardata->refcount) {
        Mat_VarFree(p_vardata->p_matvar);
        delete p_vardata;
      }
    }
  }

  /** \brief Take in charge the given C \c matvar_t pointer.
   *
   * This static method yields an instance of \a Var directly initialized with the given
   * variable pointer. The object takes ownership of the pointer, and will call \a
   * Mat_VarFree when the last Var copy sharing this data will be destructed.
   */
  static Var takeOver(matvar_t * varinfo)
  {
    return Var(varinfo);
  }

  /** \brief The variable name.
   *
   * Returns the name of the MATLAB variable this object is referring to.
   */
  inline const std::string & varName() const
  {
    tomographer_assert(p_vardata != NULL);
    return p_vardata->p_varname;
  }

  /** \brief Number of dimensions of this object.
   *
   * This is called the &lsquo;rank&rsquo; of the tensor in MatIO terminology.
   */
  inline int ndims() const
  {
    tomographer_assert(p_vardata != NULL);
    return p_vardata->p_matvar->rank;
  }
  /** \brief Specific dimensions of this numeric array or tensor.
   *
   * This returns a DimList, which is essentially a \ref std::vector<int>.
   */
  inline DimList dims() const
  {
    tomographer_assert(p_vardata != NULL);
    return DimList(&p_vardata->p_matvar->dims[0],
		   &p_vardata->p_matvar->dims[p_vardata->p_matvar->rank]);
  }
  /** \brief The total number of elements in this array or tensor.
   *
   * Returns the product of all the dimensions of this array or tensor.
   */
  inline int numel() const
  {
    tomographer_assert(p_vardata != NULL);
    return dims().numel();
  }
  /** \brief Whether this variable is complex or real.
   *
   * Returns \c true if the data stored by this variable is complex.
   *
   * If the data is complex, then the variable pointer's data is in fact a pointer to a \c
   * mat_complex_split_t which stores the real and imaginary parts of the array
   * separately. See MatIO's documentation for more info.
   */
  inline bool isComplex() const
  {
    tomographer_assert(p_vardata != NULL);
    return p_vardata->p_matvar->isComplex;
  }
  /** \brief Whether this is a square matrix.
   *
   * Return \c true if this variable has 2 dimensions which are equal, or \c false otherwise.
   */
  inline bool isSquareMatrix() const
  {
    tomographer_assert(p_vardata != NULL);
    return p_vardata->p_matvar->rank == 2 && p_vardata->p_matvar->dims[0] == p_vardata->p_matvar->dims[1];
  }

  /** \brief Whether data for this Var object has been loaded.
   *
   * If not, then to access data (e.g. \ref value<T>()), then you need to re-obtain a
   * fresh Var object with data loaded.
   */
  inline bool hasData() const
  {
    tomographer_assert(p_vardata != NULL);
    return (p_vardata->p_matvar->data != NULL);
  }

  /** \brief Read this variable data as native C++ object.
   *
   * This function returns the data stored by the variable in a C++ object of type \a T.
   *
   * The type \a T may be any type for which a specialization of \ref VarValueDecoder
   * has been defined.
   *
   * Examples:
   * \code
   *   // Read a variable in the file named 'my_scalar_variable'
   *   Var scalar_var = matfile.var("my_scalar_variable");
   *   // get the scalar variable as a float. This will raise an exception if
   *   // 'my_scalar_variable' is an array with several elements.
   *   float scalar_f = scalar_var.value<float>();
   *   // same: get the scalar variable as a double.
   *   double scalar_f = scalar_var.value<double>();
   *
   *   // Read a variable in the file named 'my_matrix_variable'
   *   Var matrix_var = matfile.var("my_matrix_variable");
   *   // get the data as an Eigen::MatrixXd
   *   Eigen::MatrixXd matrix = matrix_var.value<Eigen::MatrixXd>();
   *   // get the data in an std::vector<double>, stored with row-major ordering
   *   std::vector<double> vecdata = matrix_var.value<GetStdVector<double,true> >();
   *
   *   // read complex variables using std::complex<floating-point-type> :
   *   Var cmatrix_var = matfile.var("my_complex_matrix_variable");
   *   Eigen::MatrixXcd cmatrix = cmatrix_var.value<Eigen::MatrixXcd>();
   *   std::vector<std::complex<double> > cvecdata
   *       = cmatrix_var.value<GetStdVector<std::complex<double>,true> >();
   * \endcode
   *
   * You may also equivalently call \ref Tomographer::MAT::value()
   * "Tomographer::MAT::value<T>":
   * \code
   *   Eigen::MatrixXd matrix = var.value<Eigen::MatrixXd>();
   *   // is equivalent to:
   *   Eigen::MatrixXd matrix = Tomographer::MAT::value<Eigen::MatrixXd>(var);
   * \endcode
   */
  template<typename T>
  inline typename VarValueDecoder<T>::RetType value() const
  {
    return Tomographer::MAT::value<T>(*this);
  }
  /** \brief Read this variable data as native C++ object.
   *
   * See \ref value<T>().
   *
   * This overload accepts an additional \a params parameter, but only if the
   * corresponding VarValueDecoder<T> accepts it. This is in case the behavior of the
   * decoding should depend on additional parameters given at runtime. Currently, no
   * built-in VarValueDecoder<T> specializations use this.
   *
   */
  template<typename T, TOMOGRAPHER_ENABLED_IF_TMPL( tomo_internal::has_params_member<VarValueDecoder<T> >::value )>
  inline typename VarValueDecoder<T>::RetType value(const typename VarValueDecoder<T>::Params & params)
  {
    return Tomographer::MAT::value<T>(*this, params);
  }

  /** \brief Access the underlying C pointer to the MatIO structure. Use with care.
   *
   * Please be careful as the data is shared between different copies of Var objects.
   *
   */
  const matvar_t * getMatvarPtr() const
  {
    tomographer_assert(p_vardata != NULL);
    return p_vardata->p_matvar;
  }

  /** \brief Move assignment operator as this object implements C++11 move semantics.
   *
   */  
  Var& operator=(Var&& other)
  {
    // steal the data
    p_vardata = other.p_vardata;
    other.p_vardata = NULL;
    return *this;
  }
  //! Var objects are copyable. Beware though that the data is shared.
  Var& operator=(const Var& copy)
  {
    p_vardata = copy.p_vardata;
    p_vardata->refcount++;
    return *this;
  }
};


// ---------------


inline Var File::var(const std::string varname, bool load_data)
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


template<typename T, typename Enabled>
// static
inline typename VarValueDecoder<T,Enabled>::RetType VarValueDecoder<T,Enabled>::decodeValue(const Var & var)
{
  throw std::runtime_error(std::string("Not Implemented: Please specialize MAT::VarValueDecoder<> for ")
			   + typeid(T).name() + " to decode variable " + var.varName());
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
//! Specialization of \ref MatType for \a MAT_T_DOUBLE
template<>  struct MatType<MAT_T_DOUBLE> { typedef double Type; };
//! Specialization of \ref MatType for \a MAT_T_DOUBLE
template<>  struct MatType<MAT_T_SINGLE> { typedef float Type; };
//! Specialization of \ref MatType for \a MAT_T_INT64
template<>  struct MatType<MAT_T_INT64> { typedef int64_t Type; };
//! Specialization of \ref MatType for \a MAT_T_INT32
template<>  struct MatType<MAT_T_INT32> { typedef int32_t Type; };
//! Specialization of \ref MatType for \a MAT_T_INT16
template<>  struct MatType<MAT_T_INT16> { typedef int16_t Type; };
//! Specialization of \ref MatType for \a MAT_T_INT8
template<>  struct MatType<MAT_T_INT8> { typedef int8_t Type; };
//! Specialization of \ref MatType for \a MAT_T_UINT64
template<>  struct MatType<MAT_T_UINT64> { typedef uint64_t Type; };
//! Specialization of \ref MatType for \a MAT_T_UINT32
template<>  struct MatType<MAT_T_UINT32> { typedef uint32_t Type; };
//! Specialization of \ref MatType for \a MAT_T_UINT16
template<>  struct MatType<MAT_T_UINT16> { typedef uint16_t Type; };
//! Specialization of \ref MatType for \a MAT_T_UINT8
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



/** \brief Interface to read out a single numeric value
 *
 */
template<typename T>
TOMOGRAPHER_EXPORT class VarValueDecoder<T,
#ifdef TOMOGRAPHER_PARSED_BY_DOXYGEN
                       _IS_NUMERIC_TYPE
#else
                       typename std::enable_if<(std::numeric_limits<T>::is_specialized ||
                                                tomo_internal::isComplex<T>::value)>::type
#endif
                       >
{
public:
  //! See \ref VarValueDecoder::RetType
  typedef T RetType;

  //! See \ref VarValueDecoder::checkShape()
  static inline void checkShape(const Var & var)
  {
    if (var.isComplex() && !tomo_internal::isComplex<T>::value) {
      throw VarTypeError(var.varName(),
                         streamstr("Can't store complex matrix in type " << typeid(T).name()));
    }
    if (var.numel() != 1) {
      throw VarTypeError(var.varName(),
                         streamstr("Expected scalar but got matrix"));
    }
  }
                             
  //! See \ref VarValueDecoder::decodeValue()
  static inline RetType decodeValue(const Var & var)
  {
    const matvar_t * matvar_ptr = var.getMatvarPtr();

    MAT_SWITCH_TYPE(matvar_ptr,
                    return get_value<Type>(matvar_ptr, var.varName());
        );
    // should never reach here
    tomographer_assert(false);
  }

private:
  template<typename MATType,
           TOMOGRAPHER_ENABLED_IF_TMPL(!tomo_internal::isComplex<RetType>::value &&
                                       !tomo_internal::isComplex<MATType>::value)>
  static inline RetType get_value(const matvar_t * matvar_ptr, const std::string & )
  {
    return RetType( ((const MATType *) matvar_ptr->data)[0] );
  }
  
  template<typename MATType,
           TOMOGRAPHER_ENABLED_IF_TMPL(tomo_internal::isComplex<RetType>::value &&
                                       !tomo_internal::isComplex<MATType>::value)>
  static inline RetType get_value(const matvar_t * matvar_ptr, const std::string & )
  {
    return RetType( typename Tools::ComplexRealScalar<RetType>::type(((const MATType *) matvar_ptr->data)[0]),
                    0 );
  }
  
  template<typename MATType,
           TOMOGRAPHER_ENABLED_IF_TMPL(!tomo_internal::isComplex<RetType>::value &&
                                       tomo_internal::isComplex<MATType>::value)>
  static inline RetType get_value(const matvar_t * /*matvar_ptr*/, const std::string & varname)
  {
    throw VarTypeError(varname, "Expected real scalar, got complex type");
  }
  
  template<typename MATType,
           TOMOGRAPHER_ENABLED_IF_TMPL(tomo_internal::isComplex<RetType>::value &&
                                       tomo_internal::isComplex<MATType>::value)>
  static inline RetType get_value(const matvar_t * matvar_ptr, const std::string & )
  {
    typedef typename Tools::ComplexRealScalar<MATType>::type MATRealType;
    const mat_complex_split_t * cdata = (mat_complex_split_t*) matvar_ptr->data;

    return RetType( typename Tools::ComplexRealScalar<RetType>::type( ((const MATRealType *) cdata->Re)[0] ),
                    typename Tools::ComplexRealScalar<RetType>::type( ((const MATRealType *) cdata->Im)[0] ) );
  }

};



// -----------------------------------------------


namespace tomo_internal {

//! Interface to directly access mat data with multidimensional index
template<typename OutType, typename MatInnerT>
class VarMatDataAccessor
{
  const Var & p_var;

  typedef typename Tools::ComplexRealScalar<MatInnerT>::type MatRealInnerT;

  const MatInnerT * p_r_ptr;
  const MatRealInnerT * p_cre_ptr;
  const MatRealInnerT * p_cim_ptr;
  
public:
  VarMatDataAccessor(const Var & var)
    : p_var(var)
  {
    const matvar_t * matvar_ptr = var.getMatvarPtr();
    tomographer_assert(matvar_ptr->data != NULL);
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
      tomographer_assert(p_cre_ptr != NULL);
      tomographer_assert(p_cim_ptr != NULL);
    }
  }

  virtual ~VarMatDataAccessor() { }

  template<typename IndexListType,
	   typename OutType__ = OutType, typename MatInnerT__ = MatInnerT,
           TOMOGRAPHER_ENABLED_IF_TMPL(!tomo_internal::isComplex<OutType__>::value &&
                                       !tomo_internal::isComplex<MatInnerT__>::value)>
  inline OutType value(IndexListType&& index) const
  {
    tomographer_assert(p_r_ptr != NULL);

    // real value.
    std::size_t lin = linear_index(std::forward<IndexListType>(index));
    return (OutType)p_r_ptr[lin];
  }
  
  template<typename IndexListType,
	   typename OutType__ = OutType, typename MatInnerT__ = MatInnerT,
           TOMOGRAPHER_ENABLED_IF_TMPL(!tomo_internal::isComplex<OutType__>::value &&
                                       tomo_internal::isComplex<MatInnerT__>::value)>
  inline OutType value(IndexListType&& ) const
  {
    throw VarTypeError(p_var.varName(), "Expected real type, got complex");
  }

  template<typename IndexListType,
	   typename OutType__ = OutType, typename MatInnerT__ = MatInnerT,
           TOMOGRAPHER_ENABLED_IF_TMPL(tomo_internal::isComplex<OutType__>::value &&
                                       !tomo_internal::isComplex<MatInnerT__>::value)>
  inline OutType value(IndexListType&& index) const
  {
    tomographer_assert(p_r_ptr != NULL);

    // real value.
    std::size_t lin = linear_index(std::forward<IndexListType>(index));
    return OutType( typename Eigen::NumTraits<OutType>::Real(p_r_ptr[lin]) , 0 );
  }

  template<typename IndexListType,
	   typename OutType__ = OutType, typename MatInnerT__ = MatInnerT,
           TOMOGRAPHER_ENABLED_IF_TMPL(tomo_internal::isComplex<OutType__>::value &&
                                       tomo_internal::isComplex<MatInnerT__>::value)>
  inline OutType value(IndexListType&& index) const
  {
    tomographer_assert(p_cre_ptr != NULL);
    tomographer_assert(p_cim_ptr != NULL);

    // complex value
    std::size_t lin = linear_index(std::forward<IndexListType>(index));
    return OutType( typename Eigen::NumTraits<OutType>::Real(p_cre_ptr[lin]),
                    typename Eigen::NumTraits<OutType>::Real(p_cim_ptr[lin]) );
  }

private:
  template<typename IndexListType>
  std::size_t linear_index(IndexListType && index) const
  {
    IndexList<false> ind_cmaj{
        p_var.dims(),
	std::remove_reference<IndexListType>::type::forward_index(index)
	};
    // std::cout << "linear_index: ind_cmaj=" << ind_cmaj << ", -> linear index = " << ind_cmaj.linearIndex() << "\n";
    return ind_cmaj.linearIndex();
  }
};

} // namespace tomo_internal


// -----------------------------------------------

/** \brief Describe shape of variable and whether it is complex
 *
 * Allows to check a matlab variable for correct shape, for example.
 *
 * This struct stores attributes of a variable, or attributes which one would require a
 * variable to have. You can create a VarShape which represents an actual MATLAB variable,
 * or you can also create a VarShape describing the characteristics which you'd want a
 * loaded MATLAB variable to have, and then test that shape agains the actual variable.
 *
 * This simple struct stores:
 *  - whether the variable is complex or not (\ref is_complex);
 *  - the dimensions of the variable, i.e. a list of integers (\ref dims);
 *  - whether the variable is or should be square, i.e. whether the two first dimensions
 *    are or have to be equal (\ref is_square).
 *
 * The dimensions \a dims may contain \a -1 entries, which indicate that the particular
 * dimension is not fixed (i.e. doesn't have a particular requirement).
 *
 * You may check whether a particular requirement given as a VarShape matches the actual
 * shape of a variable using \ref checkShape(const Var & var), or more generally whether
 * it matches another VarShape using \ref checkShape(const VarShape& varshape).
 *
 * Example:
 * \code
 *   VarShape shape(true, DimList() << 4 << 4, false);
 *   Var v = matfile.var("my_variable");
 *   try {
 *     std::cout << "Decoding '" << v.varName() << "' into a " << shape << "\n";
 *     shape.checkShape(v);
 *     // The variable 'my_variable' in the data file is indeed a 4x4 square
 *     // real or complex matrix.
 *     std::cout << "OK\n";
 *   } catch (const VarTypeError & err) {
 *     std::cout << "Error: " << err.what() << "\n";
 *   }
 * \endcode
 *
 * You may also stream a VarShape into an \ref std::ostream for formatting, as in the
 * example.
 */
TOMOGRAPHER_EXPORT struct VarShape
{
  /** \brief Whether the variable is or should be complex
   *
   * When describing a requirement, it is OK if the requirement is to be complex
   * but the variable is real, and the shapes will be reported to match. (This
   * is natural as a real MATLAB variable is easily decoded into a complex C++
   * object.)
   */
  const bool is_complex;

  /** \brief The dimensions of the variable, or requested dimensions
   *
   * If you are describing the requested dimensions of a variable, you can set some
   * dimensions to \a -1 to signify that they can assume any value. For example, if you
   * want a matrix with three rows but any number of columns, you may set
   * \code
   *   dims = DimList() << 3 << -1;
   * \endcode
   *
   * This property can be left empty to signify that there should be no constraint on the
   * dimensions of the MATLAB variable.
   *
   * See also \ref DimList::matchesWanted()
   */
  const DimList dims;

  /** \brief Whether the variable's two first dimensions are (or should be) the same
   *
   * This flag encodes a requirement that the two first dimensions of a matrix must be
   * equal. For example, to ensure that an input variable is a list of three square
   * matrices (of any but same size) encoded as <code>Mat(:,:,i)</code>, then you may set
   * \code
   *    dims = DimList() << -1 << -1 << 3;
   *    is_square = true;
   * \endcode
   *
   * If \a is_square is \a false, this does not indicate that the variable is requested to
   * have a non-square shape. If \a is_square is \a false, the variable may have any
   * shape.
   */
  const bool is_square;

  /** \brief Construct a VarShape object from given characteristics
   *
   * This simply initializes the fields and checks consistency of the given fields.
   */
  template<typename DimListType>
  VarShape(bool is_complex_, DimListType&& dims_, bool is_square_)
    : is_complex(is_complex_), dims(std::forward<DimListType>(dims_)), is_square(is_square_)
  {
    _check_consistency();
  }

  /** \brief Construct a VarShape object reflecting the actual shape of a variable in the
   *         data file.
   *
   * The fields are initialized from the values of the actual variable.
   */
  VarShape(const Var & var)
    : is_complex(var.isComplex()), dims(var.dims()), is_square(var.isSquareMatrix())
  {
  }


private:
  void _check_consistency()
  {
    if (is_square) {
      tomographer_assert(dims.size() == 0 || (dims.size() == 2 && (dims[0] == -1 || dims[1] == -1 || dims[0] == dims[1])));
    }
  }
  
public:
  
  /** \brief Verify that our requirements match the given other \a shape
   *
   * If the \a shape does not satisfy the requirements of this VarShape object,
   * then throw a VarTypeError. The requirements are described in the class
   * documentation and in the individual member documentations for \ref
   * is_complex, \ref dims and \ref is_square.
   */
  inline void checkShape(const VarShape & shape);

  /** \brief Shorthand for \ref checkShape(const VarShape& other) for a shape given by a
   *         variable.
   *
   * \note If you have a variable to check against, it is advantageous to use this method
   *       instead of \ref checkShape(const VarShape& shape) as any thrown exception will
   *       also contain the name of the variable and thus be more informative.
   */
  inline void checkShape(const Var & var)
  {
    try {
      checkShape(VarShape(var));
    } catch (VarTypeError & err) {
      err.setVarName(var.varName());
      throw;
    }
  }
};

//! C++ output stream operator for \ref VarShape .
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
    if (varshape.is_square) {
      str << " with first two dimensions square";
    }
  } else {
    // empty dims means no requirement
    str << "array";
  }
  if (varshape.dims.size()) {
    str << " of shape " << varshape.dims;
  }
  return str;
}

inline void VarShape::checkShape(const VarShape& other)
{
  const DimList mvardims = other.dims;
  
  if ((other.is_complex && !is_complex) ||
      (mvardims.size() != dims.size() && dims.size() > 0) ||
      (is_square && (mvardims.size() != 2 || mvardims[0] != mvardims[1])) ||
      (dims.size() > 0 && !mvardims.matchesWanted(dims)))  {
    
    std::stringstream errstr;

    errstr << "Expected "
           << *this
           << ", got "
           << other;

    //fprintf(stderr, "Bad var type for variable %s\n", var.varName().c_str());
    throw VarTypeError(std::string(), errstr.str());

  }
}



// -----------------------------------------------------------------------------

// get an std::vector<T> for a variable.

/** \brief Ask for this type in \ref Var::value<typename T>() to get an \ref std::vector
 *         of the data
 *
 * \tparam T_ is the type stored in the std::vector.
 *
 * \tparam IsRowMajor_ should be \c true if the elements of the matrix should be stored in
 * the vector in row-major ordering or \c false if they should be stored in column-major
 * ordering (the default, also Eigen's default)
 */
template<typename T_, bool IsRowMajor_ = false>
struct GetStdVector {
  typedef T_ type;
  static constexpr bool IsRowMajor = IsRowMajor_;
};

//! Specialization of \ref VarValueDecoder to obtain an \ref std::vector with the matrix data. See \ref GetStdVector.
template<typename T, bool IsRowMajor>
TOMOGRAPHER_EXPORT class VarValueDecoder<GetStdVector<T, IsRowMajor> >
{
public:
  typedef std::vector<T> RetType;

  static inline void checkShape(const Var & var)
  {
    if (var.isComplex() && !tomo_internal::isComplex<T>::value) {
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
                    
                    for(IndexListIterator<IsRowMajor> il(var.dims()); il.valid(); ++il) {
                      // std::cout << "index: " << il << "\n";
                      val[il.linearIndex()] = acc.value(il);
                    }
                    
                    return val;
        );
    // should never reach here
    tomographer_assert(false);
  }

};







// =============================================================================

// get Eigen types.

namespace tomo_internal {


inline DimList dims_stackedcols(DimList vdims)
{
  tomographer_assert(vdims.size() >= 1);
  DimList vdimsreshaped;
  if (vdims.size() == 1) {
    vdimsreshaped = vdims;
    vdimsreshaped << 1;
  } else if (vdims.size() == 2) {
    vdimsreshaped = vdims;
  } else if (vdims.size() > 2) {
    vdimsreshaped << getNumEl(vdims.data(), vdims.data()+vdims.size()-1) << vdims[vdims.size()-1];
  }
  tomographer_assert(vdimsreshaped[0] != -1 && vdimsreshaped[1] != -1);
  return vdimsreshaped;
}

template<typename MatrixType, typename MatType,
         TOMOGRAPHER_ENABLED_IF_TMPL(Eigen::NumTraits<typename MatrixType::Scalar>::IsComplex &&
                                     Eigen::NumTraits<MatType>::IsComplex)>
inline void init_eigen_matrix(MatrixType & matrix, const DimList & vdims,
                              const Var & var, const std::ptrdiff_t data_offset = 0)
{
  typedef typename MatrixType::Scalar Scalar;
  typedef typename Eigen::NumTraits<Scalar>::Real RealScalar;

  typedef typename Eigen::NumTraits<MatType>::Real MatRealType;

  const matvar_t * matvar_ptr = var.getMatvarPtr();
  
  const mat_complex_split_t * cdata = (mat_complex_split_t*) matvar_ptr->data;
  
  DimList vdimsreshaped = dims_stackedcols(vdims);
  
  matrix = (
      Eigen::Map<const Eigen::Matrix<MatRealType,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor> >(
          (const MatRealType *) cdata->Re + data_offset, vdimsreshaped[0], vdimsreshaped[1]
          ).template cast<std::complex<RealScalar> >()
      + std::complex<RealScalar>(0,1) *
      Eigen::Map<const Eigen::Matrix<MatRealType,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor> >(
          (const MatRealType *) cdata->Im + data_offset, vdimsreshaped[0], vdimsreshaped[1]
          ).template cast<std::complex<RealScalar> >()
      );
}

template<typename MatrixType, typename MatRealType,
         TOMOGRAPHER_ENABLED_IF_TMPL(!Eigen::NumTraits<typename MatrixType::Scalar>::IsComplex &&
                                     Eigen::NumTraits<MatRealType>::IsComplex)>
inline void init_eigen_matrix(MatrixType & /*matrix*/, const DimList & /*vdims*/,
                              const Var & var, const std::ptrdiff_t /*data_offset*/ = 0)
{
  throw VarTypeError(var.varName(), "Expected real type, but got complex.");
}

template<typename MatrixType, typename MatRealType,
         TOMOGRAPHER_ENABLED_IF_TMPL(!Eigen::NumTraits<MatRealType>::IsComplex)>
inline void init_eigen_matrix(MatrixType & matrix, const DimList & vdims,
                              const Var & var, const std::ptrdiff_t data_offset = 0)
{
  typedef typename MatrixType::Scalar Scalar;

  const matvar_t * matvar_ptr = var.getMatvarPtr();
  
  DimList vdimsreshaped = dims_stackedcols(vdims);
  
  matrix = (
      Eigen::Map<const Eigen::Matrix<MatRealType,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor> >(
          (const MatRealType *) matvar_ptr->data + data_offset, vdimsreshaped[0], vdimsreshaped[1]
          )
      ).template cast<Scalar>();
}

} // namespace tomo_internal



/** \brief Decoder for Eigen::Matrix types.
 *
 * \note This decoder is capable of decoding multidimensional tensors also with number of
 *       dimensions >2. In that case, the last dimension is the column index, and the
 *       first dimensions up to the before-to-last-one are collapsed in column-major
 *       ordering into the row index of the Eigen type.
 *
 */
template<typename Scalar, int Rows, int Cols, int Options, int MaxRows, int MaxCols>
TOMOGRAPHER_EXPORT class VarValueDecoder<Eigen::Matrix<Scalar,Rows,Cols,Options,MaxRows,MaxCols> >
{
public:

  typedef Eigen::Matrix<Scalar,Rows,Cols,Options,MaxRows,MaxCols> MatrixType;
  typedef typename Eigen::NumTraits<Scalar>::Real RealScalar;
  typedef MatrixType RetType;

  /* not needed in fact.
  struct Params {
    Params() : dims{Rows, Cols}
    {
    } 
    Params(int rows) : dims{rows, 1}
    {
      tomographer_assert(Cols == 1);
      tomographer_assert(Rows == Eigen::Dynamic || Rows == rows);
    }
    Params(int rows, int cols) : dims{rows, cols}
    {
      tomographer_assert(Rows == Eigen::Dynamic || Rows == rows);
      tomographer_assert(Cols == Eigen::Dynamic || Cols == cols);
    }
    Params(const DimList& dims_) : dims{dims_}
    {
      tomographer_assert(dims.size() == 2);
      tomographer_assert(Rows == Eigen::Dynamic || Rows == dims[0]);
      tomographer_assert(Cols == Eigen::Dynamic || Cols == dims[1]);
    }

    const DimList dims;
  };
  */

  static inline void checkShape(const Var & var) //, const Params & p = Params())
  {
    DimList matdims;
    matdims << (Rows!=Eigen::Dynamic ? Rows : -1)
            << (Cols!=Eigen::Dynamic ? Cols : -1);

    VarShape shape(Eigen::NumTraits<Scalar>::IsComplex,
                   matdims,
                   matdims[0] != -1 && matdims[0] == matdims[1]);

    DimList vdims = var.dims();
    if (vdims.size() > 2) {
      vdims = (DimList() << getNumEl(vdims.data(), vdims.data()+vdims.size()-1) << vdims[vdims.size()-1]);
    }
    try {
      shape.checkShape(VarShape(var.isComplex(), vdims, (vdims[0] != -1 && vdims[0] == vdims[1])));
    } catch (VarTypeError & err) {
      err.setVarName(var.varName());
      throw; // re-throw
    }
  }
                             
  static inline RetType decodeValue(const Var & var) //, const Params & p = Params())
  {
    const matvar_t * matvar_ptr = var.getMatvarPtr();
    DimList vdims{var.dims()};

    if (vdims.size() < 2) {
      throw VarTypeError(var.varName(), streamstr("Expected matrix, but variable shape is " << vdims));
    }
    // if we are stacking several dimensions in the column, force column-major ordering.
    if (vdims.size() > 2 &&
        ((MatrixType::Options & Eigen::RowMajorBit) == Eigen::RowMajor)) {
      throw VarTypeError(var.varName(),
                         "When collapsing several dimensions into Eigen columns, you must use "
                         "column-major ordering (sorry).");
    }

    const int cols = vdims[vdims.size()-1];
    // rest of dimensions.
    const int rows = getNumEl(vdims.data(), vdims.data()+vdims.size()-1);

    MatrixType matrix(rows, cols);

    MAT_SWITCH_TYPE(
        matvar_ptr,
        tomo_internal::init_eigen_matrix<MatrixType, Type>(
            matrix, vdims, var
            );
        );
     
    return matrix;
  }
  
};





/** \brief Decoder for a std::vector of elements of type Eigen::Matrix.
 *
 * This decoder decodes multidimensional tensors with number of dimensions >= 3. For
 * multidimensional tensors with number of dimensions == 3, the first two indices indicate
 * the inner matrix dimensions, and the last index indicates the position in the \a
 * std::vector. For multidimensional tensors also with number of dimensions >3, all
 * further dimensions are collapsed with the 3rd dimension in column-major ordering, as
 * the \a std::vector index.
 */
template<typename Scalar, int Rows, int Cols, int Options, int MaxRows, int MaxCols, typename Alloc>
TOMOGRAPHER_EXPORT class VarValueDecoder<std::vector<Eigen::Matrix<Scalar,Rows,Cols,Options,MaxRows,MaxCols>, Alloc> >
{
public:

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

    DimList vdims = var.dims();
    if (vdims.size() < 1) {
      throw VarTypeError(var.varName(), "Invalid (empty) variable dimensions");
    }
    if (vdims.size() == 1) {
      vdims << 1 << 1;
    } else if (vdims.size() == 2) {
      vdims << 1;
    } else { //if (vdims.size() > 2) {
      vdims = (DimList() << vdims[0] << vdims[1] << getNumEl(vdims.data()+2, vdims.data()+vdims.size()));
    }

    // check shape now:
    try {
      shape.checkShape(VarShape(var.isComplex(), vdims, false));
    } catch (VarTypeError & err) {

      err.setVarName(var.varName());
      throw; // re-throw
    }

  }
                             
  static inline RetType decodeValue(const Var & var)
  {
    DimList vardims{var.dims()};

    tomographer_assert(vardims.size() >= 1);

    DimList innerdims;
    std::size_t outerdim = 1;
    if (vardims.size() == 1) {
      innerdims << vardims[0] << 1;
    } else if (vardims.size() == 2) {
      innerdims = vardims;
    } else {
      // more dimensions
      innerdims << vardims[0] << vardims[1];
      outerdim = getNumEl(vardims.data()+2, vardims.data()+vardims.size());
    }

    RetType value(outerdim);

    std::ptrdiff_t innernumel = innerdims[0]*innerdims[1];

    std::size_t j;
    const matvar_t * matvar_ptr = var.getMatvarPtr();

    MAT_SWITCH_TYPE(
        matvar_ptr,
        for (j = 0; j < outerdim; ++j) {
          tomo_internal::init_eigen_matrix<MatrixType,Type>(
              value[j], // matrix reference
              innerdims, // dimensions of matrix reference
              var, // data
              j*innernumel // offset
              );
        }
        );

    return value;
  }
  
};


// get a (guaranteed) positive semidefinite matrix from the variable, along with its
// matrix square root

template<typename EigenType> class EigenPosSemidefMatrixWithSqrt;
template<typename EigenType> class VarValueDecoder<EigenPosSemidefMatrixWithSqrt<EigenType> >;


/** \brief Read a positive semidefinite matrix along with its matrix square root
 *
 * Use this type for \ref VarValueDecoder "VarValueDecoder<T>" to decode a positive
 * semidefinite matrix from a given variable, make sure that it is indeed numerically
 * positive semidefinite, and calculate as well its matrix square root.
 *
 * We read a square matrix, calculate its eigenvalue decomposition, and re-create a new
 * matix with the same eigenvectors, where all eigenvalues are at least a given \a
 * tolerance (the other eigenvalues are adjusted so as to perserve the overall trace).
 * Internally, we use the \ref Tomographer::MathTools::forcePosSemiDef() family of
 * utilities (preserving the total trace of the matrix).
 *
 * You should not construct this type directly, just use it with \ref
 * VarValueDecoder<EigenPosSmidefMatrixWithSqrt<EigenType> >.
 */
template<typename EigenType_>
TOMOGRAPHER_EXPORT class EigenPosSemidefMatrixWithSqrt
{
public:
  typedef EigenType_ EigenType;
  typedef typename Eigen::NumTraits<typename EigenType::Scalar>::Real RealScalarType;

  /** \brief Parameters to read the positive semidefinite matrix
   *
   * The main parameter is the tolerance.  The tolerance is the minimal acceptable
   * eigenvalue of the returned matrix (in any case must be positive).
   *
   * The tolerance defaults to Eigen's tolerance, typically <code>1e-12</code> for \c
   * double type.
   *
   * Internally, we use the \ref Tomographer::MathTools::forcePosSemiDef() family of
   * utilities (preserving the total trace of the matrix).  This means that the minimal
   * eigenvalue of the returned matrix will be the \a tolerance.
   */
  struct Params {
    //! Constructor, specify the tolerance
    Params(RealScalarType tolerance_ = Eigen::NumTraits<RealScalarType>::dummy_precision())
      : tolerance(tolerance_) { }
    //! The tolerance
    const RealScalarType tolerance;
  };

  //! The readily-initialized eigenvalue decomposition of the matrix read from the MAT file
  Eigen::SelfAdjointEigenSolver<EigenType> eig;

  //! The matrix itself
  EigenType mat;
  //! The matrix square root of the read matrix
  EigenType sqrt;

private:
  friend class VarValueDecoder<EigenPosSemidefMatrixWithSqrt<EigenType> >;

  EigenPosSemidefMatrixWithSqrt(EigenType && m, const Params & p)
    : eig( m )
  {
    typedef typename Eigen::SelfAdjointEigenSolver<EigenType>::RealVectorType RealVectorType;
    
    auto U = eig.eigenvectors();
    auto d = eig.eigenvalues();
  
    Tomographer::MathTools::forcePosVecKeepSum<RealVectorType>(d, p.tolerance);
  
    mat = U * d.asDiagonal() * U.adjoint();
    sqrt = U * d.cwiseSqrt().asDiagonal() * U.adjoint();
  }
};
//! Specialization of \ref VarValueDecoder for extracting a positive semidefinite matrix along with sqrt
template<typename EigenType_>
TOMOGRAPHER_EXPORT class VarValueDecoder<EigenPosSemidefMatrixWithSqrt<EigenType_> >
{
public:
  typedef EigenType_ EigenType;
  typedef EigenPosSemidefMatrixWithSqrt<EigenType> RetType;

  typedef typename EigenPosSemidefMatrixWithSqrt<EigenType>::Params Params;

  typedef VarValueDecoder<EigenType> EigenVarDecoder;

  static inline void checkShape(const Var & var, const Params & = Params())
  {
    // make sure we can decode the given variable
    EigenVarDecoder::checkShape(var);
    // make sure that var is a square matrix
    const DimList d = var.dims();
    if (d.size() != 2 || d[0] != d[1]) {
      throw VarTypeError(var.varName(), std::string("variable to decode is not a square matrix"));
    }
  }
                             
  static inline RetType decodeValue(const Var & var, const Params & p = Params())
  {
    return EigenPosSemidefMatrixWithSqrt<EigenType>(EigenVarDecoder::decodeValue(var), p);
  }

};



  

} // namespace MAT
} // namespace Tomographer




#endif
