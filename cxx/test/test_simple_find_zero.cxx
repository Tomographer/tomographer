
#include <cmath>
#include <iostream>

#include <tomographer/tools/loggers.h>
#include <tomographer/qit/simple_find_zero.h>


Tomographer::SimpleFoutLogger flog(stdout, Tomographer::Logger::LONGDEBUG);


double f1(double x)
{
  // zero at x = 1.2 (by construction)
  return std::exp(0.5*x) - std::exp(0.5*1.2);
}
double f2(double x)
{
  // zero found at x = 0.511578
  return std::exp(0.5*x) + 3 - 6*std::sqrt(std::fabs(x));
}
double f3(double x)
{
  // zero at 0, but over a large interval is *really* small: our algorithm finds a
  // suitable zero at  x = 0.264675
  return exp(-1/(x*x));
}


void test1()
{
  const double x1 = -1;
  const double x2 = 10;
  const double tol = 1e-15;

  double pt = Tomographer::Tools::simpleFindZero<double, double>(f1, x1, x2, 50, tol, NULL, NULL, flog);

  flog.info("test1", [&](std::ostream& str) {
      str << "Point is = " << pt << " [tol="<<tol<<"]    (known to be = 1.2)";
    });
}

void test2()
{
  const double x1 = 0.1;
  const double x2 = 0.2;
  const double tol = 1e-10;

  double final_value = nan("");
  int final_iters = -1;

  double pt = Tomographer::Tools::simpleFindZero<double, double>(f2, x1, x2, 50, tol,
								 &final_value, &final_iters,
								 flog);

  flog.info("test2", [&](std::ostream& str) {
      str << "Point is = " << pt << ", final_value = " << final_value << " [tol="<<tol<<"]"
	  << "  final_iters="<<final_iters;
    });
}

void test3()
{
  const double x1 = 1;
  const double x2 = .5;
  const double tol = 1e-10;

  double final_value = nan("");

  double pt = Tomographer::Tools::simpleFindZero<double, double>(f3, x1, x2, 50, tol, &final_value);

  flog.info("test3", [&](std::ostream& str) {
      str << "Point is = " << pt << ", final_value = " << final_value << "\n";
    });
}


int main()
{
  test1();
  test2();
  test3();
  return 0;
}
