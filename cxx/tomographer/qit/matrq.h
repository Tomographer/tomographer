#ifndef QUTIL_H
#define QUTIL_H

#include <complex>

#include <Eigen/Core>

template<int FixedDim_ = Eigen::Dynamic, int FixedMaxParamList_ = Eigen::Dynamic,
         typename RealScalar_ = double, typename IntFreqType_ = int> struct MatrQ;

namespace tomo_internal
{
  template<typename Derived>
  struct matrq_traits { };

  template<int FixedDim_, int FixedMaxParamList_,
           typename RealScalar_, typename IntFreqType_>
  struct matrq_traits<MatrQ<FixedDim_, FixedMaxParamList_, RealScalar_, IntFreqType_> >
  {
    enum {
      FixedDim = FixedDim_,
      FixedMaxParamList = FixedMaxParamList_
    };
    typedef RealScalar_ RealScalar;
    typedef std::complex<RealScalar> ComplexScalar;
    typedef IntFreqType_ IntFreqType;
  };
};


template<typename Derived>
struct MatrQBase
{
  enum {
    FixedDim = tomo_internal::matrq_traits<Derived>::FixedDim,
    FixedMaxParamList = tomo_internal::matrq_traits<Derived>::FixedMaxParamList
  };
  typedef typename tomo_internal::matrq_traits<Derived>::RealScalar RealScalar;
  typedef typename tomo_internal::matrq_traits<Derived>::ComplexScalar ComplexScalar;
  typedef typename tomo_internal::matrq_traits<Derived>::IntFreqType IntFreqType;

  /** \brief Complex dim x dim Matrix */
  typedef Eigen::Matrix<std::complex<RealScalar>, FixedDim, FixedDim> MatrixType;
  
  /** \brief Real dim*dim Vector */
  typedef Eigen::Matrix<RealScalar, FixedDim*FixedDim, 1> VectorParamType;
  
  /** \brief dynamic Matrix with rows = dim*dim Vectors (row-major)
   * [maximum FixedMaxParamList rows, or Dynamic]
   */
  typedef Eigen::Matrix<RealScalar, Eigen::Dynamic, FixedDim*FixedDim, Eigen::RowMajor,
                        FixedMaxParamList, FixedDim*FixedDim> VectorParamListType;

  /** \brief dynamic Array of integers [maximum FixedMaxParamList entries or Dynamic]
   */
  typedef Eigen::Array<IntFreqType, Eigen::Dynamic, 1, 0 /*Options*/,
                       FixedMaxParamList, 1> FreqListType;
};


template<typename Derived, bool has_fixed_dim>
struct MatrQBaseDimStore : public MatrQBase<Derived>
{
  inline size_t dim() const { return MatrQBase<Derived>::FixedDim; }

  MatrQBaseDimStore(size_t dim_)
  {
    eigen_assert(MatrQBase<Derived>::FixedDim != Eigen::Dynamic && dim_ == dim());
  }

  /** When assigned to a MatrixType, should initialize it with zeros. */
  inline typename MatrQBase<Derived>::MatrixType::ConstantReturnType initMatrixType() const
  {
    return MatrQBase<Derived>::MatrixType::Zero();
  }
  /** When assigned to a VectorParamType, should initialize it with zeros. */
  inline typename MatrQBase<Derived>::VectorParamType::ConstantReturnType initVectorParamType() const
  {
    return MatrQBase<Derived>::VectorParamType::Zero();
  }
  /** When assigned to a VectorParamListType, should initialize it with zeros. */
  inline typename MatrQBase<Derived>::VectorParamListType::ConstantReturnType initVectorParamListType(size_t len) const
  {
    return MatrQBase<Derived>::VectorParamListType::Zero(len,
                                                         MatrQBase<Derived>::FixedDim*MatrQBase<Derived>::FixedDim);
  }
  /** When assigned to a FreqListType, should initialize it with zeros. */
  inline typename MatrQBase<Derived>::FreqListType::ConstantReturnType initFreqListType(size_t len) const
  {
    return MatrQBase<Derived>::FreqListType::Zero(len);
  }
};
// specialization for dynamic sized types -- give good initializers
template<typename Derived>
struct MatrQBaseDimStore<Derived, false> : public MatrQBase<Derived>
{
private:
  const size_t _dim;
public:

  inline size_t dim() const { return _dim; }

  MatrQBaseDimStore(size_t dim_)
    : _dim(dim_)
  {
    eigen_assert(MatrQBase<Derived>::FixedDim == Eigen::Dynamic);
  }

  /** When assigned to a MatrixType, should initialize it with zeros. */
  inline typename MatrQBase<Derived>::MatrixType::ConstantReturnType initMatrixType() const
  {
    return MatrQBase<Derived>::MatrixType::Zero(_dim, _dim);
  }
  /** When assigned to a VectorParamType, should initialize it with zeros. */
  inline typename MatrQBase<Derived>::VectorParamType::ConstantReturnType initVectorParamType() const
  {
    return MatrQBase<Derived>::VectorParamType::Zero(_dim*_dim);
  }
  /** When assigned to a VectorParamListType, should initialize it with zeros. */
  inline typename MatrQBase<Derived>::VectorParamListType::ConstantReturnType initVectorParamListType(size_t len) const
  {
    return MatrQBase<Derived>::VectorParamListType::Zero(len, _dim*_dim);
  }
  /** When assigned to a FreqListType, should initialize it with zeros. */
  inline typename MatrQBase<Derived>::FreqListType::ConstantReturnType initFreqListType(size_t len) const
  {
    return MatrQBase<Derived>::FreqListType::Zero(len);
  }
};



/** \brief Defines the data types for a particular problem.
 *
 * ............
 */
template<int FixedDim_, int FixedMaxParamList_, typename RealScalar_, typename IntFreqType_>
struct MatrQ
  : public MatrQBaseDimStore<MatrQ<FixedDim_, FixedMaxParamList_, RealScalar_, IntFreqType_>,
                             FixedDim_ != Eigen::Dynamic>
{
  MatrQ(size_t dim_)
    : MatrQBaseDimStore<MatrQ<FixedDim_, FixedMaxParamList_, RealScalar_, IntFreqType_>,
                        FixedDim_ != Eigen::Dynamic>(dim_)
  {
  }
};




typedef MatrQ<Eigen::Dynamic, Eigen::Dynamic, double, int> DefaultMatrQ;

typedef MatrQ<2,6,double,int> QubitPaulisMatrQ;





#endif
