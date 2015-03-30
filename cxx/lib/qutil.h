#ifndef QUTIL_H
#define QUTIL_H

#include <complex>

#include <Eigen/Core>


template<typename Derived>
struct MatrQBase
{
  /** \brief Complex dim x dim Matrix */
  typedef Eigen::Matrix<std::complex<Derived::RealScalar>,Derived::FixedDim,Derived::FixedDim> MatrixType;
  
  /** \brief Real dim*dim Vector */
  typedef Eigen::Matrix<Derived::RealScalar,Derived::FixedDim*Derived::FixedDim,1> VectorParamType;
  
  /** \brief dynamic Matrix with rows = dim*dim Vectors (row-major)
   * [maximum FixedMaxParamList rows, or Dynamic]
   */
  Matrix<Derived::RealScalar, Eigen::Dynamic, Derived::FixedDim*Derived::FixedDim, Eigen::RowMajor,
         Derived::FixedMaxParamList, Derived::FixedDim*Derived::FixedDim> VectorParamListType;

  /** \brief dynamic Array of integers [maximum FixedMaxParamList entries or Dynamic]
   */
  Array<Derived::IntFreqType, Eigen::Dynamic, 1, 0 /*Options*/,
        Derived::FixedMaxParamList, 1> FreqListType;


  const size_t dim;
  MatrQBase(size_t dim_)
    : dim(dim_)
  {
    eigen_assert(Derived::FixedDim == Eigen::Dynamic || dim_ == Derived::FixedDim);
  }
};


template<typename Derived, bool has_fixed_dim>
struct MatrQBaseWithInitializers : public MatrQBase<Derived>
{
  MatrixQBaseWithInitializers(size_t dim_) : MatrixQBase<Derived>(dim_) { }

  inline auto mkMatrixType()
  {
    return MatrixType::Zero();
  }
  inline auto mkVectorParamType()
  {
    return VectorParamType::Zero();
  }
  inline auto mkVectorParamListType(size_t len)
  {
    return MatrixType::Zero(len);
  }
  inline auto mkFreqListType(size_t len)
  {
    return FreqListType::Zero(len);
  }
};
// specialization for dynamic sized types -- give good initializers
template<typename Derived>
struct MatrQBaseWithInitializers<Derived, false> : public MatrQBase<Derived>
{
  MatrixQBaseWithInitializers(size_t dim_) : MatrixQBase<Derived>(dim_) { }

  inline auto mkMatrixType()
  {
    return MatrixType::Zero(dim, dim);
  }
  inline auto mkVectorParamType()
  {
    return VectorParamType::Zero(dim*dim);
  }
  inline auto mkVectorParamListType(size_t len)
  {
    return MatrixType::Zero(len, dim*dim);
  }
  inline auto mkFreqListType(size_t len)
  {
    return FreqListType::Zero(len);
  }
}




template<int FixedDim_ = Eigen::Dynamic, int FixedMaxParamList_ = Eigen::Dynamic,
         typename RealScalar_ = double, typename IntFreqType_ = int>
struct MatrQ
  : public MatrQBaseWithInitializers<MatrQ<FixedDim_, FixedMaxParamList_, RealScalar_, IntFreqType_>,
                                     FixedDim_ != Eigen::Dynamic>
{
  enum {
    FixedDim = FixedDim_,
    FixedMaxParamList = FixedMaxParamList_
  };
  typedef RealScalar_ RealScalar;
  typedef IntFreqType_ IntFreqType

  MatrQ(size_t dim_)
    : MatrQBaseWithInitializers<MatrQ<FixedDim_, FixedMaxParamList_, RealScalar_, IntFreqType_>,
                                FixedDim_ != Eigen::Dynamic>(dim_)
  {
  }

};




typedef MatrQ<Eigen::Dynamic, Eigen::Dynamic, double> DefaultMatrQ;







#endif
