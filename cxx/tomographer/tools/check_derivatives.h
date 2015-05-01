
#ifndef CHECK_DERIVATIVES_H
#define CHECK_DERIVATIVES_H

#include <iostream>

#include <Eigen/Core>


namespace Tomographer
{
namespace Tools
{

/** \brief Check given derivatives with numerically-calculated finite differences
 *
 * \todo DOC................................
 *
 * If the derivatives don't match, a report is written to std::cerr.
 *
 * fn has signature void fn(MatrixBase & result, const MatrixBase & point), where result
 * is of shape (valdims x 1)
 *
 * \return TRUE if all was OK, or FALSE if there were significant differences detected
 * (more than the given tolerance)
 */
template<typename Der1, typename Der2, typename fnType>
bool check_derivatives(const Eigen::ArrayBase<Der1> & derivatives, const Eigen::MatrixBase<Der2> & point,
                       fnType fn, size_t valdims,
                       typename Eigen::MatrixBase<Der1>::Scalar delta = 1e-6,
                       typename Eigen::MatrixBase<Der1>::Scalar rel_tol = 1e-6
                       )
{
  bool ok = true;

  const size_t n = derivatives.rows();
  const size_t ds = derivatives.cols();
  eigen_assert(point.rows() == (int)ds);
  eigen_assert(point.cols() == (int)1);

  Eigen::VectorXd val0(valdims);
  Eigen::VectorXd dval1(valdims);
  Eigen::VectorXd dvalFromDer(valdims);

  Eigen::VectorXd dir(point.rows());

  // calculate the base point
  fn(val0, point);

  size_t i;
  for (i = 0; i < ds; ++i) {
    // numerically calculate the differences ...

    // direction in which to probe
    dir.setZero(); dir(i) = 1.0;

    fn(dval1, point + delta*dir);
    dval1 -= val0;

    dvalFromDer = derivatives.matrix() * (delta*dir);

    double thediff = (dval1 - dvalFromDer).norm();

    if (thediff/delta > rel_tol ) {
      // Error in the derivative
      ok = false;
      std::cerr << "Error in derivative check: Derivative wrong in direction\n"
		<< "dir = " << dir.transpose() << "   [basis vector #"<<i<<"]\n"
		<< "\tpoint = \t" << point.transpose() << "\n"
		<< "\tval0  = \t" << val0.transpose() << "\n"
		<< "\tdval1 = \t" << dval1.transpose() << "\n"
		<< "\tdvalFromDer = \t"<<dvalFromDer.transpose() << "\n"
		<< "\tderivative in this direction =\n\t\t\t\t" << derivatives.transpose().block(i,0,1,n) << "\n"
		<< "--> difference: \t" << thediff << "\n"
		<< "--> difference [relative to delta]: \t" << thediff/delta << "\n\n";
    }
  }

  return ok;
}



} // Tools
} // Tomographer



#endif
