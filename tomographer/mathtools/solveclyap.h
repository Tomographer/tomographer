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

#ifndef TOMOGRAPHER_MATHTOOLS_SOLVECLYAP_H
#define TOMOGRAPHER_MATHTOOLS_SOLVECLYAP_H

/** \file solveclyap.h
 *
 * \brief Routine for solving a complex Lyapunov equation. See \ref
 *        Tomographer::SolveCLyap::solve().
 *
 */

#include <string>

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

#include <tomographer/tools/cxxutil.h> // tomographer_assert()


extern "C" void ztrsyl_(char *TRANA, char *TRANB, int *ISGN, int *M, int *N,
                        double *A, int *lda, double *B, int *ldb,
                        double *C, int *ldc, double *scale, int *info);

namespace Tomographer {
namespace MathTools {
namespace SolveCLyap {

/** \brief Error while attempting to solve complex Lyapunov/Sylvester equation
 *
 * See \ref solve().
 */
TOMOGRAPHER_EXPORT class SolveError : public std::exception {
  std::string p_msg;
public:
  SolveError(const std::string& msg)
    : p_msg(msg)
  {
  }
  virtual ~SolveError() throw() { }
  virtual const char * what() const throw() {
    return p_msg.c_str();
  }
};

namespace tomo_internal {

  static const std::string ztrsyl_argnames[] = {
    "TRANA", "TRANB", "ISGN", "M", "N", "A", "lda", "B", "ldb", "C", "ldc", "scale", "info"
  };


  template<bool debug_perform_check>
  struct solve_check_helper {
    template<typename XT, typename AT, typename CT, typename LoggerType>
    static inline void check(const XT&, const AT&, const CT&, LoggerType&)
    {
      // no-op.
    }
  };
  // specialize for if we want debug checks
  template<>
  struct solve_check_helper<true> {
    template<typename XT, typename AT, typename CT, typename LoggerType>
    static inline void check(const XT& X, const AT& A, const CT& C, LoggerType & logger)
    {
      //logger.debug("LyapCSolve::solve/check", "checking solution ...");

      double n1 = (A.adjoint() * X + X * A  -  C).norm();
      double n2 = A.norm() + C.norm();
      double ok = n1 / n2;
      logger.debug("LyapCSolve::solve/check",
		   "(A.adjoint() * X + X * A  -  C).norm()/(A.norm()+C.norm()) == %g/%g == %g ; norm(C)=%g",
		   n1, n2, ok, (double)C.norm());
      if (ok > 0.1) {
	logger.warning("LyapCSolve::solve/check", "Bad solution quality! rel norm error = %g", (double)ok);
      }
    }
  };

} // namespace tomo_internal


/** \brief Solve complex Lyapunov equation of the form <code>A'*X + X*A == C</code>
 *
 * Solves the Lyapunov equation \c "A.adjoint() * X + X * A == C" on the range of \a
 * A. The matrices \a A, \a X and \a C must be square. \a A must be Hermitian.
 *
 * The initial value of \a X is unimportant, and on output contains the solution of the
 * given equation.
 *
 * If an error occurs, a \ref SolveError is thrown.
 *
 * This function requires you to link to the LAPACK library (external symbol used is \c
 * ztrsyl_).
 *
 * Note that only complex double type is supported.  In the future we might support other
 * complex types, by calling the other LAPACK routines.
 *
 * If the template parameter \a debug_perform_check is set to \c true, then some debugging
 * consistency checks are performed.
 */
template<bool debug_perform_check = false, typename LoggerType>
TOMOGRAPHER_EXPORT
void solve(Eigen::Ref<Eigen::MatrixXcd> X, const Eigen::Ref<const Eigen::MatrixXcd> & A,
	   const Eigen::Ref<const Eigen::MatrixXcd> & C,
	   LoggerType & logger, const double tol = 1e-8)
{
  // do an eigenvalue decomposition of A

  const int d = A.rows(); // dimension of problem

  tomographer_assert(X.rows() == X.cols() && X.rows() == d);
  tomographer_assert(A.rows() == A.cols() && A.rows() == d);
  tomographer_assert(C.rows() == C.cols() && C.rows() == d);

  typedef Eigen::MatrixXcd MatType;

  Eigen::SelfAdjointEigenSolver<MatType> eigA(A);

  if (eigA.info() == Eigen::NoConvergence) {
    throw SolveError("Can't diagonalize matrix A: No Convergence");
  }

  const typename Eigen::SelfAdjointEigenSolver<MatType>::RealVectorType& eigvalsA = eigA.eigenvalues();
  const typename Eigen::SelfAdjointEigenSolver<MatType>::MatrixType& eigU = eigA.eigenvectors();

  int M = 0;
  for (int k = 0; k < d; ++k) {
    if (eigvalsA(k) > tol) {
      ++M;
    }
  }
 
  //  cout << "there are M="<< M << " nonzero eigenvalues\n";

  MatType D = MatType::Zero(M, M);
  MatType W = MatType::Zero(d, M);

  D.setZero();

  int counter = 0;
  for (int k = 0; k < d; ++k) {
    if (eigvalsA(k) < tol) {
      continue;
    }
    // include this eigenvector
    W.col(counter) = eigU.col(k);
    D(counter, counter) = eigvalsA(k);
    ++counter;
  }

  //  cout << "W = \n" << W << "\nD = \n"<< D << "\n";

  // we have that A = W * D * W^{-1} , where D is full rank (Yes!)

  // Original equation:  A^H*X + X*A == C    (with A^H==A)
  // replace eig of A:  (W*D*W^{-1})*X + X*(W*D*W^{-1}) == C
  // apply W^{-1} . W :     D*(W^{-1}*X*W) + (W^{-1}*X*W)*D == W^{-1}*C*W
  //
  // we can now solve the Sylvester equation with ZTRSYL, for W^{-1}*X*W =: Z

  MatType Z(M, M);
  
  Z = W.adjoint() * C * W;

  int plusone = +1;

  int info = 0;
  double scale = 0.0;

  int lda = D.outerStride();
  int ldb = lda;//D.outerStride();
  int ldc = Z.outerStride();

  ztrsyl_((char*)"C", (char*)"N", &plusone, &M, &M, (double*)D.data(), &lda, (double*)D.data(), &ldb,
          (double*)Z.data(), &ldc, &scale, &info);

  if (info >= 0) {
    if (info == 1) {
      logger.warning("SolveCLyap::solve()", "Warning: A and B have common or very close eigenvalues; "
		     "perturbed values were used to solve the equation");
    }
    // success
    X.noalias() = W * Z * W.adjoint() / scale;

    // maybe do some debug consistency check
    tomo_internal::solve_check_helper<debug_perform_check>::check(X, A, C, logger);
    return;
  }

  // error with argument # (-info)
  throw SolveError("Argument " + tomo_internal::ztrsyl_argnames[(-info)-1] + " to ztrsyl_ was invalid.");
}

} // namespace SolveCLyap
} // namespace MathTools
} // namespace Tomographer

#endif
