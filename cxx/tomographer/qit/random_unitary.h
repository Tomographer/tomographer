
#ifndef RANDOM_UNITARY_H
#define RANDOM_UNITARY_H

#include <iostream>
#include <random>

#include <Eigen/Core>

#include <tomographer/qit/util.h>
#include <tomographer/tools/loggers.h>


namespace Tomographer
{


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
inline void random_unitary(Eigen::MatrixBase<DerU> & U, Rng & rng, Log & logger)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(DerU); }

  assert(U.rows() == U.cols());
  const int n = U.rows();

  logger.longdebug("random_unitary()", "n = %d", n);
  
  typedef typename DerU::Scalar Scalar;
  typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> MatrixType;
  typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> VectorType;

  // first, get a matrix of normally distributed random numbers
  MatrixType A(n,n);

  std::normal_distribution<> normdist(0.0, 1.0);
  A = Tomographer::dense_random<MatrixType>(rng, normdist, n, n);

  //  logger.longdebug("random_unitary()", [&](std::ostream& str) {
  //      str << "got A = \n" << A;
  //    });

  // gram-schmidt orthogonalization

  for (int j = 0; j < n; ++j) {

    VectorType v = VectorType::Zero(n);
    v = A.col(j);
    //auto v = A.col(j);

    for (int k = 0; k < j; ++k) {
      Scalar p = U.col(k).adjoint() * v;
      v = v - p*U.col(k);
    }

    U.col(j) = v / v.norm();

    //    logger.longdebug("random_unitary()", [&](std::ostream & str) {
    //	str << "dealt with column " << j << " = " << v.transpose() << "\n"
    //	    << "\t--> " << U.col(j).transpose() << "\n"
    //	    << "\tnorm = " << U.col(j).squaredNorm() << " == " << U.col(j).adjoint() * U.col(j);
    //      });
  }

  logger.longdebug("random_unitary()", [&](std::ostream& str) {
      str << "random_unitary: got U = \n" << U << "\n"
  	  << "Check: U*U.adjoint() ==\n" << U*U.adjoint() << "\n"
  	  << "Check: U.adjoint()*U ==\n" << U.adjoint()*U;
    });
}

//! Overload of random_unitary(U, rng, logger) which discards all logging messages.
template<typename Der1, typename Rng>
inline void random_unitary(Eigen::MatrixBase<Der1> & U, Rng & rng)
{
  random_unitary<Der1, Rng>(U, rng, Logger::vacuum_logger);
}



} // namespace Tomographer

#endif
