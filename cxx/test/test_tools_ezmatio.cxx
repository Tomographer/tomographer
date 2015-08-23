

#include "test_tomographer.h"

#include <tomographer/tools/ezmatio.h>


BOOST_AUTO_TEST_SUITE(test_tools_ezmatio);

struct load_mat_test_file
{
  Tomographer::MAT::File f;

  load_mat_test_file()
    : f(TOMOGRAPHER_TEST_DATA_DIR "test_tools_ezmatio/test_tools_ezmatio_data.mat")
  {
  }

  ~load_mat_test_file()
  {
  }
};

BOOST_FIXTURE_TEST_SUITE(testf, load_mat_test_file);

BOOST_AUTO_TEST_CASE(simple_values)
{
  
}







BOOST_AUTO_TEST_SUITE_END();





BOOST_AUTO_TEST_SUITE_END();
