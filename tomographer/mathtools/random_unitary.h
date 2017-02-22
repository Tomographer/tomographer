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

#ifndef RANDOM_UNITARY_H
#define RANDOM_UNITARY_H

/** \file random_unitary.h
 *
 * \brief Produce a random unitary according to the Haar measure.
 *
 */


#include <iostream>
#include <random>

#include <Eigen/Core>

#include <tomographer/tools/eigenutil.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/tools/cxxutil.h> // tomographer_assert()


namespace Tomographer {
namespace MathTools {


/** \brief Generate a Haar-distributed random unitary
 *
 * \param U reference to an Eigen type (already sized to a square matrix)
 *
 * \param rng a std::random random number generator (such as std::mt19937)
 *
 * \param logger a reference to a logger (\ref pageLoggers) where we can log what we're
 *        doing.
 */
template<typename DerU, typename Rng, typename Log>
inline void randomUnitary(Eigen::MatrixBase<DerU> & U, Rng & rng, Log & logger)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(DerU); }

  tomographer_assert(U.rows() == U.cols());
  const int n = U.rows();

  logger.longdebug("randomUnitary()", "n = %d", n);
  
  typedef typename DerU::Scalar Scalar;
  typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> MatrixType;
  typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> VectorType;

  // first, get a matrix of normally distributed random numbers
  MatrixType A(n,n);

  std::normal_distribution<> normdist(0.0, 1.0);
  A = Tomographer::Tools::denseRandom<MatrixType>(rng, normdist, n, n);

  //  logger.longdebug("randomUnitary()", [&](std::ostream& str) {
  //      str << "got A = \n" << A;
  //    });

  // Gram-Schmidt orthogonalization

  for (int j = 0; j < n; ++j) {

    VectorType v = VectorType::Zero(n);
    v = A.col(j);
    //auto v = A.col(j);

    for (int k = 0; k < j; ++k) {
      Scalar p = U.col(k).adjoint() * v;
      v = v - p*U.col(k);
    }

    U.col(j) = v / v.norm();

    //    logger.longdebug("randomUnitary()", [&](std::ostream & str) {
    //	str << "dealt with column " << j << " = " << v.transpose() << "\n"
    //	    << "\t--> " << U.col(j).transpose() << "\n"
    //	    << "\tnorm = " << U.col(j).squaredNorm() << " == " << U.col(j).adjoint() * U.col(j);
    //      });
  }

  logger.longdebug("randomUnitary()", [&](std::ostream& str) {
      str << "randomUnitary: got U = \n" << U << "\n"
  	  << "Check: U*U.adjoint() ==\n" << U*U.adjoint() << "\n"
  	  << "Check: U.adjoint()*U ==\n" << U.adjoint()*U;
    });
}

//! Overload of randomUnitary(U, rng, logger) which discards all logging messages.
template<typename Der1, typename Rng>
inline void randomUnitary(Eigen::MatrixBase<Der1> & U, Rng & rng)
{
  randomUnitary<Der1, Rng>(U, rng, Logger::vacuum_logger);
}


} // namespace MathTools
} // namespace Tomographer


#endif
