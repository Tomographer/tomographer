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


#ifndef _TOMOGRAPHER_MATHTOOLS_POS_SEMIDEF_UTIL
#define _TOMOGRAPHER_MATHTOOLS_POS_SEMIDEF_UTIL

/** \file pos_semidef_util.h
 *
 * \brief Tools for dealing with positive semidefinite matrices.
 *
 */

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

#include <tomographer/tools/cxxutil.h> // tomographer_assert()


namespace Tomographer {
namespace MathTools {


/** \brief Make sure that the given vector is numerically positive
 *
 * This function replaces all values in \a vec that are less than \a tolerance by the
 * value \a tolerance, in such a way that the sum of all the elements of the vector is
 * preserved.
 *
 * The original vector \a vec is modified.
 */
template<typename VectorType>
inline void forcePosVecKeepSum(Eigen::Ref<VectorType> vec,
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

  tomographer_assert(trace >= vec.size()*tolerance);
  
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
inline MatrixType forcePosSemiDef(const Eigen::MatrixBase<Der> & rho,
				    const typename Eigen::NumTraits<typename MatrixType::Scalar>::Real tolerance = 1e-8)
{
  typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;

  Eigen::SelfAdjointEigenSolver<MatrixType> eig_rho_ref(
      rho.template cast<typename MatrixType::Scalar>()
      );
  auto U = eig_rho_ref.eigenvectors();
  RealVectorType d = eig_rho_ref.eigenvalues();

  forcePosVecKeepSum<RealVectorType>(d, tolerance);

  return U * d.asDiagonal() * U.adjoint();
}


/** \brief Safe version of operator square root for positive semidefinite matrices
 *
 * This function first makes sure that the object \a A is positive semidefinite (&agrave;
 * la \ref forcePosSemiDef()), before taking the operator square root.
 *
 * \a A must be hermitian.
 */
template<typename MatrixType>
inline MatrixType safeOperatorSqrt(const Eigen::Ref<const MatrixType> & A,
                                     const typename Eigen::NumTraits<typename MatrixType::Scalar>::Real
                                     tolerance = 1e-8)
{
  Eigen::SelfAdjointEigenSolver<MatrixType> eig_A_ref( A );
  auto U = eig_A_ref.eigenvectors();
  auto d = eig_A_ref.eigenvalues();

  typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;

  forcePosVecKeepSum<RealVectorType>(d, tolerance);

  return U * d.cwiseSqrt().asDiagonal() * U.adjoint();
}

/** \brief Safe version of operator inverse square root for positive semidefinite matrices
 *
 * This function first makes sure that the object \a A is positive semidefinite (&agrave;
 * la \ref forcePosSemiDef()), before taking the operator inverse
 * square root.
 *
 * \a A must be hermitian.
 */
template<typename MatrixType>
inline MatrixType safeOperatorInvSqrt(const Eigen::Ref<const MatrixType> & A,
                                      const typename Eigen::NumTraits<typename MatrixType::Scalar>::Real
                                      tolerance = 1e-8)
{
  Eigen::SelfAdjointEigenSolver<MatrixType> eig_A_ref( A );
  auto U = eig_A_ref.eigenvectors();
  auto d = eig_A_ref.eigenvalues();

  typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;

  forcePosVecKeepSum<RealVectorType>(d, tolerance);

  for (typename MatrixType::Index k = 0; k < d.size(); ++k) {
    if (d(k) > tolerance) {
      d(k) = 1 / std::sqrt(d(k));
    }
  }

  return U * d.asDiagonal() * U.adjoint();
}




} // MathTools
} // Tomographer





#endif
