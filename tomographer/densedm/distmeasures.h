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

#ifndef TOMOGRAPHER_DENSEDM_DISTMEASURES_H
#define TOMOGRAPHER_DENSEDM_DISTMEASURES_H


/** \file distmeasures.h
 *
 * \brief Distance measures in quantum information for states represented as dense matrices
 *
 */

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/SVD>
#include <unsupported/Eigen/MatrixFunctions>


namespace Tomographer {
namespace DenseDM {

/** \brief Calculate the trace distance between two density matrices
 *
 * The trace distance between \f$ \rho \f$ and \f$ \sigma \f$ is defined as
 * \f[
 *   \delta(\rho,\sigma) = \frac{1}{2} \left\Vert \rho - \sigma \right\Vert_1 \ .
 * \f]
 *
 */
template<typename ValueType, typename Derived1, typename Derived2>
inline ValueType traceDistance(const Eigen::MatrixBase<Derived1> & rho1, const Eigen::MatrixBase<Derived2> & rho2)
{
  return boost::math::constants::half<ValueType>() *
    (rho1 - rho2).template selfadjointView<Eigen::Lower>()
    .eigenvalues().cwiseAbs().sum();
}


/** \brief Fidelity between two density matrices
 *
 * Calculates \f$ F(\rho,\sigma) = \left\Vert\sigma^{1/2}\rho^{1/2}\right\Vert_1 \f$.
 *
 * \note This is the Nielsen & Chuang fidelity, sometimes also called "root fidelity."
 */
template<typename ValueType, typename Derived, typename Derived2>
inline ValueType fidelity(const Eigen::MatrixBase<Derived>& rho, const Eigen::MatrixBase<Derived2>& sigma)
{
  // The Schatten one-norm is the sum of the singular values.
  Eigen::SelfAdjointEigenSolver<Derived> rho_eig(rho);
  Eigen::SelfAdjointEigenSolver<Derived2> sigma_eig(sigma);
  return ValueType( (rho_eig.operatorSqrt()*sigma_eig.operatorSqrt()).jacobiSvd().singularValues().sum() );
}

/** \brief Fidelity between two quantum states given by their \ref pageParamsT.
 *
 * The \f$ T \f$ -parameterization of \f$ \rho \f$ is a matrix \f$ T \f$ which satisfies
 * \f[
 *     \rho = T T^\dagger .
 * \f]
 *
 * This function calculates the same fidelity function as \ref fidelity(), but accepts the
 * \ref pageParamsT of the quantum states instead. The formula used by this function
 * acts directly on the \f$ T \f$ 's:
 * \f[
 *    F(T_1 T_1^\dagger, T_2 T_2^\dagger)
 *        = \left\Vert\rho^{1/2}\sigma^{1/2}\right\Vert_1
 *        = \left\Vert T_1^\dagger T_2\right\Vert_1 .
 * \f]
 *
 * \note This is the Nielsen & Chuang fidelity, sometimes also called "root fidelity."
 */
template<typename ValueType, typename Der1, typename Der2>
inline ValueType fidelityT(const Eigen::MatrixBase<Der1>& T1, const Eigen::MatrixBase<Der2>& T2)
{
  // Calculate ||sigma^{1/2} rho^{1/2}||_1 == || T1^\dagger * T2 ||_1
  // and Schatten one-norm is the sum of the singular values.
  ValueType val = (T1.adjoint()*T2).template cast<std::complex<ValueType> >().jacobiSvd().singularValues().sum();
  return val;
}





} // namespace DenseDM
} // namespace Tomographer







#endif
