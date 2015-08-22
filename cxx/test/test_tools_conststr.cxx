

#include "test_tomographer.h"


#include <tomographer/tools/conststr.h>
#include <tomographer/tools/util.h>


TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("abcdef") == Tomographer::Tools::conststr("abcdef"));
TOMO_STATIC_ASSERT_EXPR(!(Tomographer::Tools::conststr("ksfldnfa") == Tomographer::Tools::conststr("abcdef")));
TOMO_STATIC_ASSERT_EXPR(!(Tomographer::Tools::conststr("abcdef") == Tomographer::Tools::conststr("abcde")));
TOMO_STATIC_ASSERT_EXPR(!(Tomographer::Tools::conststr("abcde") == Tomographer::Tools::conststr("abcdef")));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("fdknslf")[0] == 'f');
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("fdknslf")[1] == 'd');
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789")[8] == '8');
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789")[9] == '9');
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").is_in_range(0u));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").is_in_range(1u));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").is_in_range(9u));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::conststr("0123456789").is_in_range(10u));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::conststr("0123456789").is_in_range(std::string::npos));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").clamp_to_range(0) == 0);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").clamp_to_range(1) == 1);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").clamp_to_range(13) == 9);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").startswith(Tomographer::Tools::conststr("01234")));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::conststr("0123456789").startswith(Tomographer::Tools::conststr("abcdef")));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::conststr("012").startswith(Tomographer::Tools::conststr("0123456789")));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("xyz0123456789").startswith(Tomographer::Tools::conststr("01234"), 3));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").startswith(Tomographer::Tools::conststr("9"),9));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("xyz0123456789").startswith(Tomographer::Tools::conststr("X1234"), 3, 1));
// substr(start, count) or substr_e(start, end)
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").substr(0,3) == Tomographer::Tools::conststr("012"));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").substr(2,3) == Tomographer::Tools::conststr("234"));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").substr_e(2,5) == Tomographer::Tools::conststr("234"));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").substr(2) == Tomographer::Tools::conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").substr(2, 8) == Tomographer::Tools::conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").substr(2, 10) == Tomographer::Tools::conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").substr(2, std::string::npos) == Tomographer::Tools::conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").substr_e(2, 10) == Tomographer::Tools::conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").substr_e(2, std::string::npos) == Tomographer::Tools::conststr("23456789"));
// find(s,pos,not_found)
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").find("234") == 2);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").find("ab") == std::string::npos);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").find("ab",2,999u) == 999u);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").find("0123xyz",0) == std::string::npos);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").find("9",3) == 9);
// rfind(s,pos,not_found)
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").rfind("9") == 9);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").rfind("4") == 4);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").rfind("4",4) == 4);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").rfind("4",std::string::npos) == 4);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").rfind("4",3) == std::string::npos);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::conststr("0123456789").rfind("4",3,999u) == 999u);
