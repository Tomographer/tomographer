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

  /** \brief dynamic Matrix with rows = dim*dim Vectors (row-major) */
  Matrix<Derived::RealScalar, Eigen::Dynamic, Derived::FixedDim*Derived::FixedDim, Eigen::RowMajor,
         Derived::FixedMaxParamList, Derived::FixedDim*Derived::FixedDim> VectorParamListType;


  const size_t dim;
  MatrQBase(size_t dim_)
    : dim(dim_)
  {
    eigen_assert(Derived::FixedDim == Eigen::Dynamic || dim_ == Derived::FixedDim);
  }
};


template<int FixedDim_ = Eigen::Dynamic, int FixedMaxParamList_ = Eigen::Dynamic, typename RealScalar_ = double>
struct MatrQ : public MatrQBase<MatrQ<FixedDim_, FixedMaxParamList_, RealScalar_> >
{
  enum {
    FixedDim = FixedDim_,
    FixedMaxParamList = FixedMaxParamList_
  };
  typedef RealScalar_ RealScalar;

  MatrQ(size_t dim_) : MatrQBase(dim_)
  {
  }

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
};

// specialization for dynamic size
template<typename RealScalar_ = double>
struct MatrQ<Eigen::Dynamic, Eigen::Dynamic, RealScalar>
  : public MatrQBase<MatrQ<Eigen::Dynamic, Eigen::Dynamic, RealScalar_> >
{
  enum { FixedDim = FixedDim_};
  typedef RealScalar_ RealScalar;

  MatrQ(size_t dim_) : MatrQBase(dim_)
  {
  }

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
};




typedef MatrQ<Eigen::Dynamic, Eigen::Dynamic, double> DefaultMatrQ;







#endif
