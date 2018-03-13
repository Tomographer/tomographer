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

#include <cmath>

#include <string>
#include <iostream>
#include <random>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/valuecalculator.h>
#include <tomographer/densedm/tspacefigofmerit.h>


// -----------------------------------------------------------------------------
// fixture(s)

class const_value_calculator {
private:
  const int _constval;
public:
  const_value_calculator(int constval_) : _constval(constval_) { }

  typedef int ValueType;
  inline ValueType getValue(const int = 0) const { return _constval; }
};

class const_value_calculator_noconstcall {
private:
  const int _constval;
public:
  const_value_calculator_noconstcall(int constval_) : _constval(constval_) { }

  typedef int ValueType;
  inline ValueType getValue(const int = 0) { return _constval; }
};


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_valuecalculator)

BOOST_AUTO_TEST_SUITE(multiplexorValueCalculator)

BOOST_AUTO_TEST_CASE(essential)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;

  DMTypes dmt;
  DMTypes::MatrixType rho_ref(dmt.initMatrixType());
  rho_ref <<
    1, 0,
    0, 0;
  DMTypes::MatrixType T_ref(dmt.initMatrixType());
  T_ref <<
    1, 0,
    0, 0;
  DMTypes::MatrixType A(dmt.initMatrixType());
  A <<
    2, 0,
    0, 1;
  DMTypes::MatrixType rho(dmt.initMatrixType());
  rho <<
    0.5, 0,
    0, 0.5;
  DMTypes::MatrixType T(dmt.initMatrixType());
  T <<
    boost::math::constants::half_root_two<double>(), 0,
    0, boost::math::constants::half_root_two<double>();
  
  const double correct_values[] = {
    // using rho_ref as ref state
    boost::math::constants::half_root_two<double>(),
    boost::math::constants::half_root_two<double>(),
    0.5,
    1.5,
    // using T*T' as ref state
    1,
    0,
    0,
    1.5
  };
  
  Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes> obs(dmt, A);

  for (int val = 0; val < 8; ++val) {
    Tomographer::MultiplexorValueCalculator<
      double,
      Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes>,
      Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes>,
      Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes>,
      Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>,
      Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes>,
      Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes>,
      Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes>,
      Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>
      >  multiplexor(
	  val,
	  [&](){ return new Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes>(T_ref); },
	  [&](){ return new Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes>(T_ref); },
	  [&](){ return new Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes>(rho_ref); },
	  [&](){ return new Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>(obs); },
	  [&](){ return new Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes>(T); },
	  [&](){ return new Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes>(T); },
	  [&](){ return new Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes>(rho); },
	  [&](){ return new Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>(obs); }
	  );
    MY_BOOST_CHECK_FLOATS_EQUAL(multiplexor.getValue(T), correct_values[val], tol);
  }
}


BOOST_AUTO_TEST_CASE(constnoconst)
{
  for (int val = 0; val < 2; ++val) {
    Tomographer::MultiplexorValueCalculator<
      int,
      const_value_calculator,
      const_value_calculator_noconstcall
      > multiplexor(
	  val,
	  []() { return new const_value_calculator(0); },
	  []() { return new const_value_calculator_noconstcall(1); }
	  );
    BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
  }
}

BOOST_AUTO_TEST_CASE(fixed1)
{
  {
    Tomographer::MultiplexorValueCalculator<
      int,
      const_value_calculator
      > multiplexor(0, [](){ return new const_value_calculator(191); });
    BOOST_CHECK_EQUAL(multiplexor.getValue(-1), 191);
  }
  {
    const Tomographer::MultiplexorValueCalculator<
      int,
      const_value_calculator
      > cmultiplexor(0, [](){ return new const_value_calculator(191); });
    BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), 191);
  }
}
BOOST_AUTO_TEST_CASE(fixed2)
{
  for (int val = 0; val < 2; ++val) {
    {
      Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator
        > multiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); }
            );
      BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
    }
    {
      const Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator
        > cmultiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); }
            );
      BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), val);
    }
  }
}
BOOST_AUTO_TEST_CASE(fixed3)
{
  for (int val = 0; val < 3; ++val) {
    {
      Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > multiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); }
            );
      BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
    }
    {
      const Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > cmultiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); }
            );
      BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), val);
    }
  }
}
BOOST_AUTO_TEST_CASE(fixed4)
{
  for (int val = 0; val < 4; ++val) {
    {
      Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > multiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); }
            );
      BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
    }
    {
      const Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > cmultiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); }
            );
      BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), val);    }
  }
}
BOOST_AUTO_TEST_CASE(fixed5)
{
  for (int val = 0; val < 5; ++val) {
    {
      Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > multiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); }
            );
      BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
    }
    {
      const Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > cmultiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); }
            );
      BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), val);
    }
  }
}
BOOST_AUTO_TEST_CASE(fixed6)
{
  for (int val = 0; val < 6; ++val) {
    {
      Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > multiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); }
            );
      BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
    }
    {
      const Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > cmultiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); }
            );
      BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), val);
    }
  }
}
BOOST_AUTO_TEST_CASE(fixed7)
{
  for (int val = 0; val < 7; ++val) {
    {
      Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > multiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); },
            []() { return new const_value_calculator(6); }
            );
      BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
    }
    {
      const Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > cmultiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); },
            []() { return new const_value_calculator(6); }
            );
      BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), val);    }
  }
}
BOOST_AUTO_TEST_CASE(fixed8)
{
  for (int val = 0; val < 8; ++val) {
    {
      Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > multiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); },
            []() { return new const_value_calculator(6); },
            []() { return new const_value_calculator(7); }
            );
      BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
    }
    {
      const Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > cmultiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); },
            []() { return new const_value_calculator(6); },
            []() { return new const_value_calculator(7); }
            );
      BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), val);
    }
  }
}
BOOST_AUTO_TEST_CASE(fixed9)
{
  for (int val = 0; val < 9; ++val) {
    {
      Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > multiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); },
            []() { return new const_value_calculator(6); },
            []() { return new const_value_calculator(7); },
            []() { return new const_value_calculator(8); }
            );
      BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
    }
    {
      const Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > cmultiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); },
            []() { return new const_value_calculator(6); },
            []() { return new const_value_calculator(7); },
            []() { return new const_value_calculator(8); }
            );
      BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), val);
    }
  }
}
BOOST_AUTO_TEST_CASE(fixed10)
{
  for (int val = 0; val < 10; ++val) {
    {
      Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > multiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); },
            []() { return new const_value_calculator(6); },
            []() { return new const_value_calculator(7); },
            []() { return new const_value_calculator(8); },
            []() { return new const_value_calculator(9); }
            );
      BOOST_CHECK_EQUAL(multiplexor.getValue(-1), val);
    }
    {
      const Tomographer::MultiplexorValueCalculator<
        int,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator,
        const_value_calculator
        > cmultiplexor(
            val,
            []() { return new const_value_calculator(0); },
            []() { return new const_value_calculator(1); },
            []() { return new const_value_calculator(2); },
            []() { return new const_value_calculator(3); },
            []() { return new const_value_calculator(4); },
            []() { return new const_value_calculator(5); },
            []() { return new const_value_calculator(6); },
            []() { return new const_value_calculator(7); },
            []() { return new const_value_calculator(8); },
            []() { return new const_value_calculator(9); }
            );
      BOOST_CHECK_EQUAL(cmultiplexor.getValue(-1), val);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

