
// this file should be included in tests before other Eigen or tomographer headers. It
// also automatically includes all the needed boost test framework tools.

#ifndef TEST_TOMOGRAPHER_H
#define TEST_TOMOGRAPHER_H

#define EIGEN_INITIALIZE_MATRICES_BY_NAN

#include <cstdlib>

// define the exception class, but don't override eigen's eigen_assert() macro itself
#include <tomographer/tools/eigen_assert_exception.h>


/** 
 * \brief Tool to dynamically request eigen_assert() to either assert() or to throw an
 *        exception.
 *
 * We want true failures to cause an assert() failure, because they can be traced much
 * more easily (core dump). However, sometimes we want to test that an eigen_assert() is
 * truly triggered. In that case, we dynamically request eigen_assert() to trigger an
 * exception.
 *
 * An exception can be triggered in a scope by using the idiom
 * \code
 *  {
 *    // new code block
 *    EigenAssertTest::setting_scope scopevar(true); // true -> throw exception, false -> assert()
 *    ... // eigen_assert() here throws an exception
 *  }
 *  // setting is popped to previous value
 * \endcode
 */


namespace EigenAssertTest {

struct setting_scope;
extern setting_scope * setting_scope_ptr;


struct setting_scope
{
  setting_scope(bool throws)
    : throws_exception(throws), parent_scope(setting_scope_ptr)
  {
    /*std::fprintf(stderr, "(): setting_scope_ptr=%p, throws_exception=%d\n", (void*)::EigenAssertTest::setting_scope_ptr, (::EigenAssertTest::setting_scope_ptr ? (int)::EigenAssertTest::setting_scope_ptr->throws_exception : 999999));*/  \
    setting_scope_ptr = this;
  }

  ~setting_scope()
  {
    /*std::fprintf(stderr, "~(): setting_scope_ptr=%p, throws_exception=%d\n", (void*)::EigenAssertTest::setting_scope_ptr, (::EigenAssertTest::setting_scope_ptr ? (int)::EigenAssertTest::setting_scope_ptr->throws_exception : 999999));*/ \
    setting_scope_ptr = parent_scope;
  }

  const bool throws_exception;
private:
  setting_scope * parent_scope;
};


} // namespace EigenAssertTest


#define eigen_assert(x)                                                 \
  do {                                                                  \
    /*std::fprintf(stderr, "setting_scope_ptr=%p, throws_exception=%d\n", (void*)::EigenAssertTest::setting_scope_ptr, (int)(::EigenAssertTest::setting_scope_ptr ? ::EigenAssertTest::setting_scope_ptr->throws_exception : 999999));*/ \
    if (::EigenAssertTest::setting_scope_ptr && ::EigenAssertTest::setting_scope_ptr->throws_exception) { \
      /*std::fprintf(stderr, "an eigen_assert() failure will cause an exception!\n");*/ \
      if (!(x)) {                                                       \
        throw (::Tomographer::Tools::eigen_assert_exception(#x, __FILE__, __LINE__)); \
      }                                                                 \
    } else {                                                            \
      assert((x) && "eigen_assert() failure");                          \
    }                                                                   \
  } while (false)


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
