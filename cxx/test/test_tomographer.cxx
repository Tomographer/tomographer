

// the sole role of this file is to define the main() function for the tests.
// See http://www.boost.org/doc/libs/1_56_0/libs/test/doc/html/utf/user-guide/test-runners.html

#define BOOST_TEST_MODULE test_tomographer
#include <boost/test/unit_test.hpp>

#include "test_tomographer.h"

EigenAssertTest::setting_scope * EigenAssertTest::setting_scope_ptr = NULL;
