
// this file should be included in tests before other Eigen or tomographer headers. It
// also automatically includes all the needed boost test framework tools.

#ifndef TEST_TOMOGRAPHER_H
#define TEST_TOMOGRAPHER_H

#define EIGEN_INITIALIZE_MATRICES_BY_NAN

#ifndef TOMOGRAPHER_TEST_EIGEN_ASSERT_ASSERT
// we want `eigen_assert()` to raise an `eigen_assert_exception` here
#include <tomographer/tools/eigen_assert_exception.h>
#endif

#include <Eigen/Core>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/output_test_stream.hpp>


template<typename Derived1, typename Derived2>
boost::test_tools::predicate_result
check_eigen_dense_equal(const Eigen::DenseBase<Derived1> & a, const Eigen::DenseBase<Derived2> & b,
                        const double tol)
{
  BOOST_MESSAGE("Comparing two Eigen dense objects.");

  if (a.rows() != b.rows()) {
    boost::test_tools::predicate_result res(false);
    res.message() << "a.rows() [="<<a.rows()<<"]  !=  b.rows() [="<<b.rows()<<"]";
    return res;
  }

  if (a.cols() != b.cols()) {
    boost::test_tools::predicate_result res(false);
    res.message() << "a.cols() [="<<a.cols()<<"]  !=  b.cols() [="<<b.cols()<<"]";
    return res;
  }

  Eigen::Array<typename Derived1::Scalar, Eigen::Dynamic, Eigen::Dynamic> a_eval(a.rows(), a.cols());
  Eigen::Array<typename Derived2::Scalar, Eigen::Dynamic, Eigen::Dynamic> b_eval(b.rows(), b.cols());
  a_eval = a;
  b_eval = b;

  typedef decltype(typename Derived1::Scalar(0) + typename Derived2::Scalar(0))  PromotedScalar;

  Eigen::Array<PromotedScalar, Eigen::Dynamic, Eigen::Dynamic> diff  =  a_eval - b_eval;

  if (diff.isMuchSmallerThan(1.0f, tol)) {
    return true;
  }
  boost::test_tools::predicate_result res(false);
  res.message() << "matrices are different: a=\n" << a_eval << "\n\t!=  b=\n" << b << "\n"
                << "\t[diff = \n" << diff << ",\n"
                << "\tnorm of difference = " << diff.matrix().norm() << "]\n";
  return res;

}



#define MY_BOOST_CHECK_EIGEN_EQUAL(a, b, tol)       \
  BOOST_CHECK( check_eigen_dense_equal((a), (b), (tol)) )




// tolerance, in *PERCENT*
static const double tol_percent = 1e-12;
static const double tol = tol_percent * 0.01;




#endif
