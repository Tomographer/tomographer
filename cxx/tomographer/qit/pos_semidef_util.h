

#ifndef POS_SEMIDEF_UTIL
#define POS_SEMIDEF_UTIL

/** \file pos_semidef_util.h
 *
 * \brief Tools for dealing with positive semidefinite matrices.
 *
 */

#include <Eigen/Core>
#include <Eigen/Eigenvalues>


namespace Tomographer {
namespace Tools {


/** \brief Make sure that the given vector is numerically positive
 *
 * This function replaces all values in \a vec that are less than \a tolerance by the
 * value \a tolerance, in such a way that the sum of all the elements of the vector is
 * preserved.
 *
 * The original vector \a vec is modified.
 */
template<typename VectorType>
inline void force_pos_vec_keepsum(Eigen::Ref<VectorType> vec,
                                  const typename Eigen::NumTraits<typename VectorType::Scalar>::Real tolerance = 1e-8)
{
  typedef typename Eigen::NumTraits<typename VectorType::Scalar>::Real RealScalar;
  typedef typename VectorType::Index Index;
  
  RealScalar trace = 0;
  RealScalar new_excess_trace = 0;
  Index k;
  Index num_pos = 0;
  for (k = 0; k < vec.size(); ++k) {
    trace += vec(k);
    if (vec(k) < tolerance) {
      new_excess_trace += tolerance - vec(k);
    } else {
      ++num_pos;
    }
  }

  eigen_assert(trace >= vec.size()*tolerance);
  
  RealScalar remove_from_each = new_excess_trace / num_pos;
  for (k = 0; k < vec.size(); ++k) {
    if (vec(k) < tolerance) {
      vec(k) = tolerance;
    } else {
      vec(k) -= remove_from_each;
    }
  }  
}

/** \brief Make sure that rho is numerically positive semidefinite
 *
 * This function replaces all eigenvalues that are less than \a tolerance by the value \a
 * tolerance, in such a way that the trace of the matrix is preserved.
 *
 * The original object \a rho is untouched, and the fixed version is returned.
 */
template<typename MatrixType, typename Der>
inline MatrixType force_pos_semidef(const Eigen::MatrixBase<Der> & rho,
				    const typename Eigen::NumTraits<typename MatrixType::Scalar>::Real tolerance = 1e-8)
{
  typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;

  Eigen::SelfAdjointEigenSolver<MatrixType> eig_rho_ref(
      rho.template cast<typename MatrixType::Scalar>()
      );
  auto U = eig_rho_ref.eigenvectors();
  RealVectorType d = eig_rho_ref.eigenvalues();

  force_pos_vec_keepsum<RealVectorType>(d, tolerance);

  return U * d.asDiagonal() * U.adjoint();
}


/** \brief Safe version of operator square root for positive semidefinite matrices
 *
 * This function first makes sure that the object \a A is positive semidefinite (&agrave;
 * la \ref force_pos_semidef()), before taking the operator square root.
 *
 * \a A must be hermitian.
 */
template<typename MatrixType>
inline MatrixType safe_operator_sqrt(const Eigen::Ref<const MatrixType> & A,
                                     const typename Eigen::NumTraits<typename MatrixType::Scalar>::Real
                                     tolerance = 1e-8)
{
  Eigen::SelfAdjointEigenSolver<MatrixType> eig_A_ref( A );
  auto U = eig_A_ref.eigenvectors();
  auto d = eig_A_ref.eigenvalues();

  typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;

  force_pos_vec_keepsum<RealVectorType>(d, tolerance);

  return U * d.cwiseSqrt().asDiagonal() * U.adjoint();
}

/** \brief Safe version of operator inverse square root for positive semidefinite matrices
 *
 * This function first makes sure that the object \a A is positive semidefinite (&agrave;
 * la \ref force_pos_semidef()), before taking the operator inverse
 * square root.
 *
 * \a A must be hermitian.
 */
template<typename MatrixType>
inline MatrixType safe_operator_inv_sqrt(const Eigen::Ref<const MatrixType> & A,
                                     const typename Eigen::NumTraits<typename MatrixType::Scalar>::Real
                                     tolerance = 1e-8)
{
  Eigen::SelfAdjointEigenSolver<MatrixType> eig_A_ref( A );
  auto U = eig_A_ref.eigenvectors();
  auto d = eig_A_ref.eigenvalues();

  typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;

  force_pos_vec_keepsum<RealVectorType>(d, tolerance);

  for (typename MatrixType::Index k = 0; k < d.size(); ++k) {
    if (d(k) > tolerance) {
      d(k) = 1 / std::sqrt(d(k));
    }
  }

  return U * d.asDiagonal() * U.adjoint();
}




} // Tools
} // Tomographer





#endif
