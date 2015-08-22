

#include "test_tomographer.h"

#include <tomographer/tools/util.h>




TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("void class::subclass::subclass(int)") == "class::subclass::subclass");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("conststr ns::subclass::method()") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int ns::subclass::method(const int&, void, conststr *)") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int ns::subclass::operator==(int)") == "ns::subclass::operator==");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int operator==(const ns::subclass&, char)") == "operator==(const ns::subclass&, char)");
