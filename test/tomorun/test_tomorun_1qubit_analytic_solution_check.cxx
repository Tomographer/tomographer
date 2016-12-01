/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#include <cmath>
#include <cstdlib>

#include <limits>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

/* This program checks that the numerical histogram obtained for a 1-qubit system with all
 * measurement outcomes in one direction corresponds to the known analytical solution.
 *
 * If a qubit is measured in a single basis, and all outcomes are in the same direction,
 * then the fidelity to the state in the measured direction is known analytically.
 *
 * This program takes as argument the CSV output of `tomorun`, and checks that it
 * corresponds to the known analytical solution.
 */


struct AnalyticalSolutionFn
{
  const int n;
  const double c;
  const double lnc;

  AnalyticalSolutionFn(int n_)
    : n(n_),
      c(2.0 / (15.0 + 16.0*n_ + 4.0*n_*n_)),
      lnc(std::log(2.0) - std::log(15.0 + 16.0*n_ + 4.0*n_*n_))
  {
  }

  double lnvalue(double f) const
  {
    return 2*std::log(f) + std::log(1+f) + std::log(1-f) + 2 * n * std::log(f) - lnc;
  }
};

struct DataPoint {
  double fval;
  double val;
  double err;
  double valln;
  double errln;

  DataPoint(double fval_, double val_, double err_)
    : fval(fval_), val(val_), err(err_),
      valln(std::log(val_)),
      errln(err_ / val_)
  {
  }
};


int main(int argc, char **argv)
{
  // usage: ./test_tomorun_1qubit_analytic_solution_check <tomorun-output-file-histogram.csv> <N-meas>
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <tomorun-output-file-histogram.csv> <N-meas>\n";
    return 127;
  }

  if (argc == 4) {
    // also have a tomorun command to run
    int exitcode = std::system(argv[3]);
    std::cout << "Tomorun completed: exitcode = " << exitcode << "\n";
    if (exitcode != 0) {
      return exitcode;
    }
  }
  
  const int N = std::atoi(argv[2]);

  std::cout << "N = " << N << "\n";

  std::string fn(argv[1]);

  std::ifstream fnstr(fn);

  std::string line;

  // skip first line, the header:
  std::getline(fnstr, line);
  
  const AnalyticalSolutionFn solution(N);

  std::cout << std::setprecision(6);
  std::cout << "lnc = " << solution.lnc << "\nlnval(0.99)=" << solution.lnvalue(0.99) << "\n";


  double sumvals = 0.0;

  std::vector<DataPoint> pts;
  pts.reserve(200);
  // collect all data points
  int count_lines = 0;
  double first_fval = std::numeric_limits<double>::quiet_NaN();
  double bin_delta = std::numeric_limits<double>::quiet_NaN();
  while (true) {
    std::getline(fnstr, line);
    if (fnstr.eof()) {
      break;
    }

    std::istringstream linestr(line);
    double fval, val, err;
    linestr >> fval >> val >> err;

    if (count_lines == 0) {
      first_fval = fval;
    } else if (count_lines == 1) {
      bin_delta = fval - first_fval;
    }
    ++count_lines;

    // ignore zero data points
    if (val < 1e-12) {
      continue;
    }

    pts.push_back(DataPoint(fval, val, err));
    sumvals += val;
  }

  std::cout << "Read " << count_lines << " lines.\n"
            << "bin_delta = " << bin_delta << "\n"
            << "sumvals = " << sumvals << "\n";

  const double ln_bin_delta = std::log(bin_delta);

  double sumwsqdiff = 0.0;

  // now, analyze data points
  
  for (int j = 0; j < (int)pts.size(); ++j) {
    
    const DataPoint pt(pts[j].fval+bin_delta/2.0, pts[j].val, pts[j].err);

    // calculate squared difference to expected theoretical value, and weigh by 1/err

    const double theo_valln = solution.lnvalue(pt.fval) + ln_bin_delta;

    std::cout << "fval="<<pt.fval<<" val="<<pt.val<<" err="<<pt.err<<" valln="<<pt.valln<<" errln="<<pt.errln
              <<" theo_valln="<<theo_valln<<"\n";

    sumwsqdiff += std::pow( (pt.valln - theo_valln)/pt.errln , 2 );
  }

  double chi2_red = sumwsqdiff / (pts.size() - 1);

  std::cout << "analysis on ln(val):\n"
            << "pts.size() = " << pts.size() << "\n"
            << "chi2 = " << sumwsqdiff << "\n"
            << "chi2_red = " << chi2_red << "\n\n";

  if (chi2_red > 1.5) {
    std::cerr << "Error: !!! Fit doesn't seem good... !!!\n";
    return 1;
  }

  return 0;
}
